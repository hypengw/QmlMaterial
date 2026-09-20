#include "qml_material/control/tool_tip.hpp"
#include "qml_material/control/tool_tip_popup.hpp"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QScopeGuard>
#include <QQuickItem>
#include <QUrl>
#include <QVariant>
#include <cstring>

namespace qml_material
{

namespace
{
constexpr auto managerPropertyName = "_qcm_material_tool_tip_manager";
constexpr auto plainToolTipUrl     = "qrc:/Qcm/Material/qml/control/PlainToolTip.qml";

} // namespace

class ToolTipManager final : public QObject {
    Q_OBJECT

public:
    explicit ToolTipManager(QQmlEngine* engine): QObject(engine), m_engine(engine) {}

    static ToolTipManager* get(QQmlEngine* engine, bool create) {
        if (! engine) return nullptr;

        const auto value = engine->property(managerPropertyName);
        if (auto* manager = qobject_cast<ToolTipManager*>(value.value<QObject*>())) {
            return manager;
        }
        if (! create) return nullptr;

        auto* manager = new ToolTipManager(engine);
        engine->setProperty(managerPropertyName,
                            QVariant::fromValue(static_cast<QObject*>(manager)));
        return manager;
    }

    ToolTipPopup* toolTip(bool create) {
        if (m_toolTip || ! create) return m_toolTip;

        QQmlComponent component(m_engine, QUrl(QString::fromLatin1(plainToolTipUrl)));
        if (! component.isReady()) {
            reportErrors(component);
            return nullptr;
        }

        auto* object  = component.create();
        auto* toolTip = qobject_cast<ToolTipPopup*>(object);
        if (! toolTip) {
            delete object;
            reportErrors(component);
            return nullptr;
        }

        toolTip->setParent(this);
        m_toolTip = toolTip;
        connect(toolTip, &Popup::visibleChanged, this, &ToolTipManager::toolTipVisibleChanged);
        return toolTip;
    }

    bool isVisible(const ToolTipAttached* attached) const {
        return attached && attached == m_current && m_toolTip && m_toolTip->isVisible();
    }

    void show(ToolTipAttached* attached, const QString& text, int timeout) {
        QPointer<ToolTipManager>  guard(this);
        QPointer<ToolTipAttached> source(attached);
        QPointer<QQuickItem>      item = attached ? attached->target() : nullptr;
        if (! item) return;
        if (m_showing) {
            const auto revision = ++m_requestRevision;
            QMetaObject::invokeMethod(
                this,
                [this, source, text, timeout, revision] {
                    if (source && revision == m_requestRevision) show(source, text, timeout);
                },
                Qt::QueuedConnection);
            return;
        }
        m_showing          = true;
        const auto cleanup = qScopeGuard([guard] {
            if (guard) guard->m_showing = false;
        });

        QPointer<ToolTipPopup> toolTip = this->toolTip(true);
        if (! toolTip) return;
        const auto revision = ++m_requestRevision;
        const auto current  = [&] {
            return guard && source && item && toolTip && revision == m_requestRevision;
        };

        if (m_current && m_current != attached) {
            m_showRequested = false;
            toolTip->dismissImmediately();
            if (! current()) return;
        }

        m_current = attached;
        setOwner(item);

        toolTip->resetWidth();
        if (! current()) return;
        toolTip->resetHeight();
        if (! current()) return;
        toolTip->setParentItem(item);
        if (! current()) return;
        toolTip->setText(text);
        if (! current()) return;
        toolTip->setDelay(attached->delay());
        if (! current()) return;
        toolTip->setTimeout(attached->timeout());
        if (! current()) return;

        m_requestText    = text;
        m_requestTimeout = timeout;
        m_showRequested  = true;
        if (item->window()) invokeRequestedShow();
    }

    void hide(ToolTipAttached* attached) {
        if (! attached || attached != m_current || ! m_toolTip) return;
        ++m_requestRevision;
        m_showRequested = false;
        m_toolTip->hide();
    }

    void release(ToolTipAttached* attached) {
        if (! attached || attached != m_current) return;
        ++m_requestRevision;
        m_showRequested = false;
        m_current.clear();
        setOwner(nullptr);
        if (m_toolTip) m_toolTip->dismissImmediately();
    }

    void sync(ToolTipAttached* attached, const char* property, const QVariant& value) {
        if (! attached || attached != m_current || ! m_toolTip ||
            (! m_showRequested && ! isVisible(attached))) {
            return;
        }

        if (std::strcmp(property, "text") == 0) {
            m_requestText = value.toString();
            m_toolTip->setText(m_requestText);
        } else if (std::strcmp(property, "timeout") == 0) {
            m_requestTimeout = -1;
            m_toolTip->setTimeout(value.toInt());
        } else if (std::strcmp(property, "delay") == 0) {
            QPointer<ToolTipManager> guard(this);
            m_toolTip->setDelay(value.toInt());
            if (! guard || ! m_toolTip) return;
            if (m_showRequested && ! m_toolTip->isVisible() && m_owner && m_owner->window())
                invokeRequestedShow();
        }
    }

private Q_SLOTS:
    void toolTipVisibleChanged() {
        if (m_toolTip && ! m_toolTip->isVisible()) m_showRequested = false;
        if (m_current) m_current->notifyVisibleChanged();
    }

private:
    void setOwner(QQuickItem* owner) {
        if (m_owner == owner) return;
        if (m_owner) disconnect(m_owner, nullptr, this, nullptr);
        m_owner = owner;
        if (m_owner) {
            connect(m_owner, &QQuickItem::windowChanged, this, [this](QQuickWindow* window) {
                if (window && m_showRequested) invokeRequestedShow();
            });
        }
    }

    void invokeRequestedShow() {
        if (! m_toolTip || ! m_owner || ! m_showRequested) return;
        m_toolTip->show(m_requestText, m_requestTimeout);
    }

    void reportErrors(const QQmlComponent& component) const {
        const auto errors = component.errors();
        if (errors.isEmpty()) {
            qmlWarning(m_engine) << "Failed to create PlainToolTip";
            return;
        }
        for (const auto& error : errors) {
            qmlWarning(m_engine) << error.toString();
        }
    }

    QPointer<QQmlEngine>      m_engine;
    QPointer<ToolTipPopup>    m_toolTip;
    QPointer<ToolTipAttached> m_current;
    QPointer<QQuickItem>      m_owner;
    QString                   m_requestText;
    int                       m_requestTimeout { -1 };
    bool                      m_showRequested { false };
    quint64                   m_requestRevision = 0;
    bool                      m_showing         = false;
};

ToolTipAttached::ToolTipAttached(QObject* parent): QObject(parent) {
    if (parent && ! qobject_cast<QQuickItem*>(parent)) {
        qmlWarning(parent) << "ToolTip attached properties require a QQuickItem target";
    }
}

ToolTipAttached::~ToolTipAttached() {
    if (auto* tooltipManager = manager(false)) tooltipManager->release(this);
}

QString ToolTipAttached::text() const { return m_text; }
void    ToolTipAttached::setText(const QString& text) {
    if (m_text == text) return;
    m_text = text;
    if (auto* tooltipManager = manager(false)) {
        tooltipManager->sync(this, "text", text);
    }
    emit textChanged();
}

int  ToolTipAttached::delay() const { return m_delay; }
void ToolTipAttached::setDelay(int delay) {
    if (m_delay == delay) return;
    m_delay = delay;
    if (auto* tooltipManager = manager(false)) {
        tooltipManager->sync(this, "delay", delay);
    }
    emit delayChanged();
}

int  ToolTipAttached::timeout() const { return m_timeout; }
void ToolTipAttached::setTimeout(int timeout) {
    if (m_timeout == timeout) return;
    m_timeout = timeout;
    if (auto* tooltipManager = manager(false)) {
        tooltipManager->sync(this, "timeout", timeout);
    }
    emit timeoutChanged();
}

bool ToolTipAttached::visible() const {
    auto* tooltipManager = manager(false);
    return tooltipManager && tooltipManager->isVisible(this);
}

void ToolTipAttached::setVisible(bool visible) {
    if (visible) {
        show(m_text);
    } else {
        hide();
    }
}

QObject* ToolTipAttached::toolTip() const {
    auto* tooltipManager = manager(true);
    return tooltipManager ? tooltipManager->toolTip(true) : nullptr;
}

void ToolTipAttached::show(const QString& text, int timeout) {
    auto* tooltipManager = manager(true);
    if (tooltipManager) tooltipManager->show(this, text, timeout);
}

void ToolTipAttached::hide() {
    auto* tooltipManager = manager(false);
    if (tooltipManager) tooltipManager->hide(this);
}

ToolTipManager* ToolTipAttached::manager(bool create) const {
    return ToolTipManager::get(qmlEngine(parent()), create);
}

QQuickItem* ToolTipAttached::target() const { return qobject_cast<QQuickItem*>(parent()); }

void ToolTipAttached::notifyVisibleChanged() { emit visibleChanged(); }

ToolTip::ToolTip(QObject* parent): QObject(parent) {}

ToolTipAttached* ToolTip::qmlAttachedProperties(QObject* object) {
    return new ToolTipAttached(object);
}

} // namespace qml_material

#include "tool_tip.moc"
