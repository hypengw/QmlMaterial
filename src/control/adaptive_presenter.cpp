#include "qml_material/control/adaptive_presenter.hpp"
#include <QQuickWindow>

namespace qml_material
{
PresentationSite::PresentationSite(QQuickItem* parent)
    : QQuickItem(parent), m_proxy(new ItemProxy(this)) {
    connect(m_proxy, &QQuickItem::implicitWidthChanged, this, [this] {
        setImplicitWidth(m_proxy->implicitWidth());
    });
    connect(m_proxy, &QQuickItem::implicitHeightChanged, this, [this] {
        setImplicitHeight(m_proxy->implicitHeight());
    });
    connect(m_proxy, &ItemProxy::controllingChanged, this, [this] {
        if (m_presenter && m_presenter->m_current == this && ! current() &&
            ! m_presenter->m_relocating) {
            m_presenter->m_lostControl = true;
            changed();
        }
        if (! m_destroying) Q_EMIT currentChanged();
    });
    connect(this, &QQuickItem::windowChanged, this, &PresentationSite::changed);
    connect(this, &QQuickItem::parentChanged, this, &PresentationSite::changed);
}
PresentationSite::~PresentationSite() {
    m_destroying = true;
    disconnect(this, nullptr, this, nullptr);
    disconnect(m_proxy, nullptr, this, nullptr);
    for (auto connection : m_popupConnections) disconnect(connection);
    disconnect(m_windowConnection);
    if (m_presenter) {
        m_presenter->siteChanging(this);
        if (m_presenter) m_presenter->m_sites.removeAll(this);
    }
    if (m_popup) m_popup->releasePresentation(this);
}
void PresentationSite::changed() {
    if (m_presenter) m_presenter->schedule();
}
void PresentationSite::setPresenter(AdaptivePresenter* value) {
    if (m_presenter == value) return;
    QPointer<PresentationSite>  guard(this);
    QPointer<AdaptivePresenter> next(value);
    if (m_presenter) {
        m_presenter->siteChanging(this);
        if (! guard) return;
        if (m_presenter) m_presenter->m_sites.removeAll(this);
    }
    if (m_popup) m_popup->releasePresentation(this);
    if (! guard) return;
    m_presenter = next;
    if (next) next->m_sites.append(this);
    acquirePopup();
    if (! guard) return;
    changed();
    Q_EMIT presenterChanged();
}
bool PresentationSite::acquirePopup() {
    if (! m_popup) return true;
    QPointer<PresentationSite> guard(this);
    if (! m_presenter || ! m_popup->acquirePresentation(this) || ! guard || ! m_popup ||
        ! m_presenter)
        return false;
    m_popup->setPresentationRequestEnabled(this, m_activationEnabled && m_presenter->enabled());
    return true;
}
void PresentationSite::observePopupParent() {
    disconnect(m_windowConnection);
    if (m_popup && m_popup->parentItem())
        m_windowConnection = connect(
            m_popup->parentItem(), &QQuickItem::windowChanged, this, &PresentationSite::changed);
    changed();
}
void PresentationSite::setPopup(Popup* value) {
    if (m_popup == value && m_popupRequired == bool(value)) return;
    QPointer<PresentationSite> guard(this);
    QPointer<Popup>            next(value);
    if (m_presenter) m_presenter->siteChanging(this);
    if (! guard) return;
    for (auto connection : m_popupConnections) disconnect(connection);
    m_popupConnections.clear();
    if (m_popup) m_popup->releasePresentation(this);
    if (! guard) return;
    m_popup         = next;
    m_popupRequired = bool(next);
    if (next) {
        m_popupConnections << connect(next, &Popup::presentationRequested, this, [this] {
            if (! m_activationEnabled || ! m_presenter || ! m_presenter->enabled() ||
                m_popup->presentationOwner() != this)
                return;
            QPointer<PresentationSite> guard(this);
            Q_EMIT activationRequested();
            if (! guard || ! m_presenter || m_presenter->destination() != this) return;
            m_presenter->reconcile(true);
        });
        m_popupConnections << connect(next, &QObject::destroyed, this, [this] {
            QPointer<PresentationSite> guard(this);
            if (m_presenter) m_presenter->siteChanging(this);
            if (! guard) return;
            m_popup = nullptr;
            changed();
            Q_EMIT popupChanged();
        });
        m_popupConnections << connect(
            next, &Popup::presentationOwnerChanged, this, &PresentationSite::changed);
        m_popupConnections << connect(
            next, &Popup::parentChanged, this, &PresentationSite::observePopupParent);
    }
    acquirePopup();
    if (! guard) return;
    observePopupParent();
    Q_EMIT popupChanged();
}
void PresentationSite::setAutoOpen(bool value) {
    if (m_autoOpen == value) return;
    m_autoOpen = value;
    Q_EMIT autoOpenChanged();
}
void PresentationSite::setActivationEnabled(bool value) {
    if (m_activationEnabled == value) return;
    m_activationEnabled = value;
    if (m_popup)
        m_popup->setPresentationRequestEnabled(this,
                                               value && m_presenter && m_presenter->enabled());
    Q_EMIT activationEnabledChanged();
}
void PresentationSite::geometryChange(const QRectF& current, const QRectF& previous) {
    QQuickItem::geometryChange(current, previous);
    m_proxy->setSize(current.size());
}
void PresentationSite::componentComplete() {
    QQuickItem::componentComplete();
    changed();
}

AdaptivePresenter::AdaptivePresenter(QObject* parent): QObject(parent) {}
AdaptivePresenter::~AdaptivePresenter() {
    m_destroying = true;
    detach();
    const auto sites = m_sites;
    for (auto site : sites) {
        if (! site) continue;
        site->m_presenter = nullptr;
        site->m_proxy->setActive(false);
        if (site && site->m_popup) site->m_popup->releasePresentation(site);
    }
}
PresentationSite* AdaptivePresenter::destination() const { return m_destination; }
PresentationSite* AdaptivePresenter::currentSite() const { return m_current; }
void              AdaptivePresenter::setContent(QQuickItem* value) {
    if (m_content == value) return;
    for (auto connection : m_contentConnections) disconnect(connection);
    m_contentConnections.clear();
    m_content = value;
    ++m_generation;
    m_lostControl = m_suppressed = false;
    if (value) {
        m_contentConnections << connect(value, &QObject::destroyed, this, [this] {
            m_content = nullptr;
            ++m_generation;
            schedule();
            Q_EMIT contentChanged();
        });
        m_contentConnections << connect(
            value, &QQuickItem::windowChanged, this, &AdaptivePresenter::schedule);
    }
    schedule();
    Q_EMIT contentChanged();
}
void AdaptivePresenter::setDestination(PresentationSite* value) {
    if (m_destination == value) return;
    disconnect(m_destinationConnection);
    m_destination = value;
    ++m_generation;
    m_lostControl = m_suppressed = false;
    if (value)
        m_destinationConnection = connect(value, &QObject::destroyed, this, [this] {
            m_destination = nullptr;
            ++m_generation;
            schedule();
            Q_EMIT destinationChanged();
        });
    schedule();
    Q_EMIT destinationChanged();
}
void AdaptivePresenter::setEnabled(bool value) {
    if (m_enabled == value) return;
    m_enabled = value;
    ++m_generation;
    m_lostControl = m_suppressed = false;
    if (value && m_destination && m_destination->autoOpen()) m_openRequested = true;
    schedule();
    QPointer<AdaptivePresenter> guard(this);
    for (auto site : m_sites) {
        if (site && site->m_popup)
            site->m_popup->setPresentationRequestEnabled(site, value && site->activationEnabled());
    }
    if (! value && m_popup && m_current) m_popup->setPresentationAllowed(m_current, false);
    if (! guard) return;
    Q_EMIT enabledChanged();
}
void AdaptivePresenter::present() {
    ++m_generation;
    m_suppressed = m_lostControl = m_dismissRequested = false;
    m_openRequested = m_explicit = m_session = true;
    schedule();
}
void AdaptivePresenter::dismiss() {
    if (m_dismissRequested || (! m_session && ! m_openRequested)) return;
    m_openRequested    = false;
    m_dismissRequested = true;
    m_explicit = m_session = true;
    schedule();
}
void AdaptivePresenter::schedule() {
    if (m_queued || m_destroying) return;
    m_queued = true;
    QMetaObject::invokeMethod(
        this,
        [this] {
            m_queued = false;
            reconcile();
        },
        Qt::QueuedConnection);
}
void AdaptivePresenter::setStatus(Status value) {
    if (m_status == value) return;
    m_status = value;
    if (! m_destroying) Q_EMIT statusChanged();
}
void AdaptivePresenter::setError(const QString& value) {
    if (m_error == value) return;
    m_error = value;
    Q_EMIT errorStringChanged();
}
void AdaptivePresenter::updateStatus() {
    setStatus(! m_current           ? Inactive
              : ! m_popup           ? Mounted
              : m_popup->closing()  ? Closing
              : m_popup->isOpened() ? Open
              : m_popup->entering() ? Opening
                                    : Mounted);
}
void AdaptivePresenter::detach() {
    const bool relocating = m_relocating;
    m_relocating          = true;
    QPointer<AdaptivePresenter> guard(this);
    auto                        old   = m_current;
    auto                        popup = m_popup;
    for (auto connection : m_popupConnections) disconnect(connection);
    m_popupConnections.clear();
    m_popup = nullptr;
    if (popup && old) popup->setPresentationAllowed(old, false);
    if (! guard) return;
    if (old) old->m_proxy->setActive(false);
    if (! guard) return;
    m_current     = nullptr;
    m_reachedOpen = false;
    m_relocating  = relocating;
    if (old && ! m_destroying) Q_EMIT currentSiteChanged();
}
void AdaptivePresenter::siteChanging(PresentationSite* site) {
    ++m_generation;
    QPointer<AdaptivePresenter> guard(this);
    if (m_current == site) detach();
    if (guard) schedule();
}
void AdaptivePresenter::completeDismiss(quint64 generation) {
    if (generation != m_generation || ! m_session || m_relocating || ! m_enabled) return;
    m_session = m_explicit = m_reachedOpen = m_dismissRequested = false;
    Q_EMIT dismissed();
}
void AdaptivePresenter::observePopup() {
    if (! m_popup) return;
    m_popupConnections << connect(m_popup, &Popup::aboutToShow, this, [this] {
        ++m_generation;
        if (! m_session) m_explicit = false;
        m_session     = true;
        m_reachedOpen = false;
        updateStatus();
    });
    m_popupConnections << connect(m_popup, &Popup::opened, this, [this] {
        m_reachedOpen = m_session = true;
        updateStatus();
    });
    m_popupConnections << connect(m_popup, &Popup::closingChanged, this, [this] {
        if (m_popup && m_popup->closing()) m_closeGeneration = m_generation;
        updateStatus();
    });
    m_popupConnections << connect(m_popup, &Popup::closed, this, [this] {
        const auto generation = m_closeGeneration;
        const auto popup      = m_popup;
        const bool completed  = m_explicit || m_reachedOpen;
        QMetaObject::invokeMethod(
            this,
            [this, generation, popup, completed] {
                if (! popup || popup != m_popup || generation != m_generation || m_openRequested ||
                    ! m_enabled || m_destination != m_current || popup->isVisible())
                    return;
                QPointer<AdaptivePresenter> guard(this);
                updateStatus();
                if (! guard) return;
                if (completed)
                    completeDismiss(generation);
                else
                    m_session = false;
            },
            Qt::QueuedConnection);
    });
}
void AdaptivePresenter::reconcile(bool activationRequest) {
    if (m_relocating) {
        schedule();
        return;
    }
    QPointer<AdaptivePresenter> guard(this);
    const auto                  generation  = m_generation;
    auto                        next        = m_destination;
    auto                        syncTargets = [&] {
        const auto sites = m_sites;
        for (auto site : sites) {
            if (site) site->acquirePopup();
            if (! guard || generation != m_generation) return false;
            if (site && ! site->current()) site->m_proxy->setTarget(m_content);
            if (! guard || generation != m_generation) return false;
        }
        return true;
    };
    if (! syncTargets()) return;
    if (! m_enabled || ! m_content || ! next || m_suppressed) {
        if (m_current) {
            m_relocating = true;
            Q_EMIT aboutToRelocate(m_current, nullptr);
            if (! guard) return;
            m_relocating = false;
            if (generation != m_generation) {
                schedule();
                return;
            }
        }
        detach();
        if (! guard) return;
        if (! syncTargets()) return;
        if (! m_enabled || ! m_content || ! next) {
            m_session = m_explicit = m_dismissRequested = false;
        }
        setError({});
        if (guard) updateStatus();
        return;
    }
    if (next->m_presenter == this) next->acquirePopup();
    if (! guard || ! next || generation != m_generation) {
        if (guard) schedule();
        return;
    }
    const auto error = siteError(next);
    if (! error.isEmpty()) {
        if (m_lostControl) detach();
        if (! guard) return;
        setError(error);
        if (guard) setStatus(Error);
        return;
    }
    setError({});
    if (! guard || generation != m_generation) return;
    if (next->m_popup &&
        (! next->m_popup->parentItem() || ! next->m_popup->parentItem()->window())) {
        if (m_current == next) detach();
        if (guard) updateStatus();
        return;
    }
    bool mounted = false;
    if (m_current != next || ! next->current() || next->m_proxy->target() != m_content) {
        m_relocating = true;
        setStatus(Switching);
        if (! guard) return;
        if (generation != m_generation) {
            m_relocating = false;
            schedule();
            return;
        }
        Q_EMIT aboutToRelocate(m_current, next);
        if (! guard) return;
        if (! next || generation != m_generation) {
            m_relocating = false;
            schedule();
            return;
        }
        const auto error = siteError(next);
        if (! error.isEmpty()) {
            m_relocating = false;
            setError(error);
            if (guard) setStatus(Error);
            return;
        }
        detach();
        if (! guard) return;
        if (! next || generation != m_generation) {
            m_relocating = false;
            schedule();
            return;
        }
        next->m_proxy->setTarget(m_content);
        if (! guard) return;
        if (! next || generation != m_generation) {
            m_relocating = false;
            schedule();
            return;
        }
        next->m_proxy->setActive(true);
        if (! guard) return;
        m_relocating = false;
        if (generation != m_generation) {
            if (next) next->m_proxy->setActive(false);
            if (guard) schedule();
            return;
        }
        if (! next || ! next->current()) {
            setError(QStringLiteral("destination failed to acquire content"));
            if (guard) setStatus(Error);
            return;
        }
        m_current = next;
        m_popup   = next->m_popup;
        observePopup();
        if (m_popup) m_popup->setPresentationAllowed(next, true);
        m_session  = ! m_popup || m_openRequested;
        m_explicit = m_openRequested;
        mounted    = true;
        Q_EMIT currentSiteChanged();
        if (! guard || generation != m_generation) {
            if (guard) schedule();
            return;
        }
    }
    if (m_popup) m_popup->setPresentationAllowed(m_current, true);
    if (m_dismissRequested) {
        m_dismissRequested = false;
        if (m_popup && m_popup->isVisible()) {
            m_popup->close();
        } else {
            if (! m_popup) {
                m_suppressed = true;
                detach();
                if (! guard) return;
            }
            updateStatus();
            if (guard) completeDismiss(generation);
            return;
        }
    } else if (! activationRequest && (m_openRequested || (mounted && next->autoOpen()))) {
        m_openRequested = false;
        m_explicit = m_session = true;
        if (m_popup) m_popup->open();
    }
    if (guard) updateStatus();
}
QString AdaptivePresenter::siteError(PresentationSite* next) const {
    if (! next || next->m_destroying || next->m_presenter != this)
        return QStringLiteral("destination is not registered with this presenter");
    if (m_lostControl)
        return QStringLiteral("content was reparented outside its presentation site");
    if (next->m_popupRequired && ! next->m_popup)
        return QStringLiteral("destination popup was destroyed");
    if (next->m_popup && next->m_popup->presentationOwner() != next)
        return QStringLiteral("destination popup already has a presentation owner");
    const auto error =
        next->m_proxy->acquisitionError(m_content, m_current ? m_current->m_proxy : nullptr);
    if (! error.isEmpty()) return error;
    auto* sourceWindow = m_content->window();
    if (! sourceWindow && m_popup && m_popup->parentItem())
        sourceWindow = m_popup->parentItem()->window();
    auto* targetWindow = next->window();
    if (next->m_popup && next->m_popup->parentItem())
        targetWindow = next->m_popup->parentItem()->window();
    if (sourceWindow && targetWindow && sourceWindow != targetWindow)
        return QStringLiteral("destination belongs to a different window");
    if (next->m_popup) {
        auto* host = next->m_popup->parentItem();
        if (host && host->window() && m_content->window() && m_content->window() != host->window())
            return QStringLiteral("destination popup belongs to a different window");
        if (! next->m_popup->surfaceItem()->isAncestorOf(next))
            return QStringLiteral("popup destination must be inside its content tree");
    }
    return {};
}
} // namespace qml_material
