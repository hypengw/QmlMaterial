#include "qml_material/control/popup_presenter.hpp"

#include <QMetaMethod>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQml/qqml.h>
#include <algorithm>
#include <ranges>
#include <utility>

namespace qml_material
{

PopupPresentation::PopupPresentation(PopupPresenter* presenter)
    : QObject(presenter), m_presenter(presenter) {}

PopupPresentation::~PopupPresentation() {
    if (m_presenter) m_presenter->presentationDestroyed(this);
}

auto PopupPresentation::status() const -> Status { return m_status; }

auto PopupPresentation::active() const -> bool {
    switch (m_status) {
    case Loading:
    case WaitingForReady:
    case Opening:
    case Open:
    case Closing: return true;
    case Closed:
    case Error:
    case Cancelled: return false;
    }
    return false;
}

auto PopupPresentation::errorString() const -> QString { return m_errorString; }
auto PopupPresentation::result() const -> QVariant { return m_result; }

void PopupPresentation::close() {
    if (m_presenter) m_presenter->requestClose(this, Finish::Closed);
}

void PopupPresentation::cancel() {
    if (m_presenter) m_presenter->requestClose(this, Finish::Cancelled);
}

void PopupPresentation::onPopupOpened() {
    if (m_presenter) m_presenter->popupOpened(this);
}

void PopupPresentation::onPopupClosed() {
    if (m_presenter) m_presenter->popupClosed(this);
}

void PopupPresentation::setStatus(Status value) {
    if (m_status == value) return;
    const auto wasActive = active();
    m_status             = value;
    emit statusChanged();
    if (wasActive != active()) emit activeChanged();
}

void PopupPresentation::setErrorString(QString value) {
    if (m_errorString == value) return;
    m_errorString = std::move(value);
    emit errorStringChanged();
}

void PopupPresentation::setResult(QVariant value) {
    if (m_result == value) return;
    m_result = std::move(value);
    emit resultChanged();
}

PresentationAttached::PresentationAttached(QObject* parent): QObject(parent) {}

auto PresentationAttached::ready() const -> bool { return m_ready; }

void PresentationAttached::setReady(bool value) {
    if (m_ready == value) return;
    m_ready = value;
    emit readyChanged();
}

void PresentationAttached::fail(const QString& error) {
    if (m_failed) return;
    m_failed      = true;
    m_errorString = error.isEmpty() ? QStringLiteral("popup preparation failed") : error;
    emit failureReported(m_errorString);
}

void PresentationAttached::complete(const QVariant& result) {
    if (! m_presentation) {
        qmlWarning(parent()) << "Presentation.complete() requires PopupPresenter.present()";
        return;
    }
    emit completionRequested(result);
}

void PresentationAttached::bind(PopupPresentation* presentation) { m_presentation = presentation; }

void PresentationAttached::unbind(PopupPresentation* presentation) {
    if (m_presentation == presentation) m_presentation = nullptr;
}

Presentation::Presentation(QObject* parent): QObject(parent) {}

PresentationAttached* Presentation::qmlAttachedProperties(QObject* object) {
    return new PresentationAttached(object);
}

PopupPresenter::PopupPresenter(QObject* parent): QObject(parent), m_pool(new Pool(this)) {}

PopupPresenter::~PopupPresenter() {
    m_destroying             = true;
    const auto presentations = m_presentations;
    for (auto presentation : presentations) {
        if (! presentation || presentation->m_terminal) continue;
        presentation->m_terminal = true;
        if (presentation->m_request) presentation->m_request->cancel();
    }
    m_presentations.clear();
}

auto PopupPresenter::host() const -> QQuickItem* { return m_host; }

void PopupPresenter::setHost(QQuickItem* value) {
    if (m_host == value) return;
    if (std::ranges::any_of(m_presentations, [](const auto& presentation) {
            return presentation && presentation->active();
        })) {
        qmlWarning(this) << "PopupPresenter.host cannot change while a presentation is active";
        return;
    }

    if (m_host) disconnect(m_host, nullptr, this, nullptr);
    m_host = value;
    if (m_host) {
        connect(m_host, &QObject::destroyed, this, [this]() {
            m_host = nullptr;
            emit       hostChanged();
            const auto presentations = m_presentations;
            for (auto presentation : presentations) {
                if (presentation && presentation->active())
                    requestClose(presentation, PopupPresentation::Finish::Cancelled);
            }
        });
    }
    emit hostChanged();
}

auto PopupPresenter::incubationMode() const -> Pool::IncubationMode { return m_incubationMode; }

void PopupPresenter::setIncubationMode(Pool::IncubationMode value) {
    if (m_incubationMode == value) return;
    m_incubationMode = value;
    emit incubationModeChanged();
}

PopupPresentation* PopupPresenter::present(const QVariant&    source,
                                           const QVariantMap& initialProperties) {
    auto* presentation = new PopupPresentation(this);
    QQmlEngine::setObjectOwnership(presentation, QQmlEngine::JavaScriptOwnership);
    m_presentations.push_back(presentation);

    if (! m_host || ! m_host->window()) {
        failPresentation(presentation,
                         QStringLiteral("PopupPresenter requires a host in a window"));
        return presentation;
    }
    if (initialProperties.contains(QStringLiteral("parent"))) {
        failPresentation(presentation,
                         QStringLiteral("PopupPresenter owns the popup parent property"));
        return presentation;
    }

    ensurePoolContext();
    auto properties = initialProperties;
    properties.insert(QStringLiteral("parent"), QVariant::fromValue(m_host.data()));

    auto* request           = m_pool->request(source, properties, {}, m_incubationMode);
    presentation->m_request = request;
    connect(request, &PoolRequest::statusChanged, presentation, [this, presentation]() {
        handleRequest(presentation);
    });
    connect(request, &QObject::destroyed, presentation, [this, presentation]() {
        if (! presentation->m_terminal)
            failPresentation(presentation,
                             QStringLiteral("popup creation request was destroyed unexpectedly"));
    });
    handleRequest(presentation);
    return presentation;
}

void PopupPresenter::closeAll() {
    const auto presentations = m_presentations;
    for (auto presentation : presentations) {
        if (presentation && presentation->active())
            requestClose(presentation, PopupPresentation::Finish::Closed);
    }
}

void PopupPresenter::requestClose(PopupPresentation*        presentation,
                                  PopupPresentation::Finish finish) {
    if (! presentation || presentation->m_terminal) return;
    if (presentation->m_finish != PopupPresentation::Finish::None) return;

    if (presentation->m_status == PopupPresentation::Loading ||
        presentation->m_status == PopupPresentation::WaitingForReady) {
        finishPresentation(presentation, finish);
        return;
    }

    presentation->m_finish = finish;
    if (presentation->m_status != PopupPresentation::Closing)
        presentation->setStatus(PopupPresentation::Closing);
    if (! presentation->m_popup || ! QMetaObject::invokeMethod(presentation->m_popup, "close")) {
        if (finish == PopupPresentation::Finish::Error)
            finishPresentation(presentation, finish);
        else
            failPresentation(presentation, QStringLiteral("failed to invoke popup close()"));
    }
}

void PopupPresenter::handleRequest(PopupPresentation* presentation) {
    if (! presentation || presentation->m_terminal || ! presentation->m_request) return;

    switch (presentation->m_request->status()) {
    case PoolRequest::Null:
    case PoolRequest::Loading: return;
    case PoolRequest::Ready:
        if (! presentation->m_popup) attachPopup(presentation, presentation->m_request->object());
        return;
    case PoolRequest::Error:
        failPresentation(presentation,
                         presentation->m_request->errorString().isEmpty()
                             ? QStringLiteral("popup creation failed")
                             : presentation->m_request->errorString());
        return;
    case PoolRequest::Cancelled:
        finishPresentation(presentation, PopupPresentation::Finish::Cancelled);
        return;
    case PoolRequest::Released:
        failPresentation(presentation,
                         QStringLiteral("popup creation request was released unexpectedly"));
        return;
    }
}

void PopupPresenter::attachPopup(PopupPresentation* presentation, QObject* popup) {
    if (! presentation || presentation->m_terminal) return;
    if (! popup) {
        failPresentation(presentation,
                         QStringLiteral("popup creation completed without an object"));
        return;
    }

    const auto* meta = popup->metaObject();
    QStringList missing;
    if (meta->indexOfMethod("open()") < 0) missing.push_back(QStringLiteral("open()"));
    if (meta->indexOfMethod("close()") < 0) missing.push_back(QStringLiteral("close()"));
    const auto openedIndex = meta->indexOfSignal("opened()");
    const auto closedIndex = meta->indexOfSignal("closed()");
    if (openedIndex < 0) missing.push_back(QStringLiteral("opened()"));
    if (closedIndex < 0) missing.push_back(QStringLiteral("closed()"));
    if (! missing.isEmpty()) {
        failPresentation(
            presentation,
            QStringLiteral("dynamic object does not satisfy popup contract: missing %1")
                .arg(missing.join(QStringLiteral(", "))));
        return;
    }

    const auto* presentationMeta = presentation->metaObject();
    const auto  openedSlot       = presentationMeta->indexOfSlot("onPopupOpened()");
    const auto  closedSlot       = presentationMeta->indexOfSlot("onPopupClosed()");
    const auto  openedConnection = QObject::connect(
        popup, meta->method(openedIndex), presentation, presentationMeta->method(openedSlot));
    const auto closedConnection = QObject::connect(
        popup, meta->method(closedIndex), presentation, presentationMeta->method(closedSlot));
    if (! openedConnection || ! closedConnection) {
        failPresentation(presentation, QStringLiteral("failed to connect popup lifecycle signals"));
        return;
    }

    presentation->m_popup = popup;
    connect(popup, &QObject::destroyed, presentation, [this, presentation]() {
        popupDestroyed(presentation);
    });

    auto* attached =
        static_cast<PresentationAttached*>(qmlAttachedPropertiesObject<Presentation>(popup, true));
    if (! attached) {
        failPresentation(presentation,
                         QStringLiteral("failed to create Presentation attached object"));
        return;
    }

    presentation->m_attached = attached;
    attached->bind(presentation);
    connect(attached, &PresentationAttached::readyChanged, presentation, [this, presentation]() {
        if (presentation->m_terminal || ! presentation->m_attached) return;
        if (presentation->m_status == PopupPresentation::WaitingForReady &&
            presentation->m_attached->ready())
            openPopup(presentation);
    });
    connect(attached,
            &PresentationAttached::failureReported,
            presentation,
            [this, presentation](const QString& error) {
                failPresentation(presentation, error);
            });
    connect(attached,
            &PresentationAttached::completionRequested,
            presentation,
            [this, presentation](const QVariant& result) {
                completePresentation(presentation, result);
            });

    if (attached->m_failed) {
        failPresentation(presentation, attached->m_errorString);
        return;
    }

    presentation->setStatus(PopupPresentation::WaitingForReady);
    if (attached->ready()) openPopup(presentation);
}

void PopupPresenter::openPopup(PopupPresentation* presentation) {
    if (! presentation || presentation->m_terminal || ! presentation->m_popup ||
        presentation->m_status != PopupPresentation::WaitingForReady)
        return;

    presentation->setStatus(PopupPresentation::Opening);
    if (! QMetaObject::invokeMethod(presentation->m_popup, "open"))
        failPresentation(presentation, QStringLiteral("failed to invoke popup open()"));
}

void PopupPresenter::failPresentation(PopupPresentation* presentation, QString error) {
    if (! presentation || presentation->m_terminal) return;
    presentation->setErrorString(error.isEmpty() ? QStringLiteral("popup presentation failed")
                                                 : std::move(error));

    if (presentation->m_status == PopupPresentation::Opening ||
        presentation->m_status == PopupPresentation::Open ||
        presentation->m_status == PopupPresentation::Closing) {
        if (presentation->m_finish == PopupPresentation::Finish::None) {
            requestClose(presentation, PopupPresentation::Finish::Error);
        }
        return;
    }

    finishPresentation(presentation, PopupPresentation::Finish::Error);
}

void PopupPresenter::finishPresentation(PopupPresentation*        presentation,
                                        PopupPresentation::Finish finish) {
    if (! presentation || presentation->m_terminal) return;
    presentation->m_terminal = true;
    presentation->m_finish   = finish;

    if (presentation->m_request)
        disconnect(presentation->m_request, nullptr, presentation, nullptr);
    if (presentation->m_popup) disconnect(presentation->m_popup, nullptr, presentation, nullptr);
    if (presentation->m_attached) {
        disconnect(presentation->m_attached, nullptr, presentation, nullptr);
        presentation->m_attached->unbind(presentation);
    }

    auto request             = presentation->m_request;
    presentation->m_request  = nullptr;
    presentation->m_popup    = nullptr;
    presentation->m_attached = nullptr;
    if (request) {
        if (request->status() == PoolRequest::Loading)
            request->cancel();
        else
            request->release();
    }

    switch (finish) {
    case PopupPresentation::Finish::Closed:
        presentation->setStatus(PopupPresentation::Closed);
        emit presentation->closed();
        break;
    case PopupPresentation::Finish::Error:
        presentation->setStatus(PopupPresentation::Error);
        emit presentation->failed(presentation->errorString());
        break;
    case PopupPresentation::Finish::Cancelled:
        presentation->setStatus(PopupPresentation::Cancelled);
        emit presentation->cancelled();
        break;
    case PopupPresentation::Finish::None: return;
    }
    retire(presentation);
}

void PopupPresenter::completePresentation(PopupPresentation* presentation, QVariant result) {
    if (! presentation || presentation->m_terminal || presentation->m_completed) return;
    if (presentation->m_status != PopupPresentation::Opening &&
        presentation->m_status != PopupPresentation::Open) {
        qmlWarning(presentation->m_attached)
            << "Presentation.complete() requires an opening or open popup";
        return;
    }

    presentation->m_completed = true;
    presentation->setResult(std::move(result));
    emit presentation->completed(presentation->result());
    requestClose(presentation, PopupPresentation::Finish::Closed);
}

void PopupPresenter::popupOpened(PopupPresentation* presentation) {
    if (! presentation || presentation->m_terminal) return;
    if (presentation->m_status != PopupPresentation::Opening) return;
    presentation->setStatus(PopupPresentation::Open);
    emit presentation->opened();
}

void PopupPresenter::popupClosed(PopupPresentation* presentation) {
    if (! presentation || presentation->m_terminal) return;
    const auto finish = presentation->m_finish == PopupPresentation::Finish::None
                            ? PopupPresentation::Finish::Closed
                            : presentation->m_finish;
    finishPresentation(presentation, finish);
}

void PopupPresenter::popupDestroyed(PopupPresentation* presentation) {
    if (! presentation || presentation->m_terminal) return;
    if (presentation->m_status == PopupPresentation::Closing &&
        presentation->m_finish != PopupPresentation::Finish::None) {
        finishPresentation(presentation, presentation->m_finish);
        return;
    }
    failPresentation(presentation, QStringLiteral("popup was destroyed before it closed"));
}

void PopupPresenter::presentationDestroyed(PopupPresentation* presentation) {
    if (m_destroying) return;
    if (presentation && ! presentation->m_terminal) {
        presentation->m_terminal = true;
        if (presentation->m_request) presentation->m_request->cancel();
    }
    std::erase_if(m_presentations, [presentation](const auto& current) {
        return ! current || current == presentation;
    });
}

void PopupPresenter::retire(PopupPresentation* presentation) {
    std::erase_if(m_presentations, [presentation](const auto& current) {
        return ! current || current == presentation;
    });
    presentation->m_presenter = nullptr;
    presentation->setParent(nullptr);
}

void PopupPresenter::ensurePoolContext() {
    if (qmlContext(m_pool)) return;
    if (auto* context = qmlContext(this)) QQmlEngine::setContextForObject(m_pool, context);
}

} // namespace qml_material
