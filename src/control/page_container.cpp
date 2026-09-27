#include "qml_material/control/page_container.hpp"
#include "qml_material/util/pool.hpp"
#include "qml_material/util/loggingcategory.hpp"
#include <QtQml/QQmlContext>
#include <QtQml/QQmlProperty>

namespace qml_material
{
PageContainer::PageContainer(QQuickItem* parent)
    : MaterialPageStack(parent), m_pool(new Pool(this)) {
    connect(this, &PageStack::entryAdded, this, [this](quint64 id, QQuickItem* item) {
        if (! m_accepting || m_accepting->object() != item) return;
        m_leases.insert(id, m_accepting);
        setCurrentRequest(m_accepting);
    });
    connect(this, &PageStack::entryRemoved, this, [this](quint64 id) {
        const auto request = m_leases.take(id);
        if (! request) return;
        QPointer<PageContainer> guard(this);
        const bool              changing = m_changing;
        m_changing                       = true;
        if (m_current == request) setCurrentRequest(nullptr);
        if (! guard) return;
        if (request) request->release();
        if (guard) m_changing = changing;
    });
    connect(this, &PageStack::currentItemChanged, this, [this] {
        disconnect(m_backConnection);
        if (auto* item = currentItem()) {
            QQmlProperty property(item, QStringLiteral("canBack"));
            const auto   signal = property.property().notifySignal();
            if (signal.isValid())
                m_backConnection =
                    connect(item,
                            signal,
                            this,
                            metaObject()->method(metaObject()->indexOfSlot("updateCanBack()")));
        }
        updateCanBack();
    });
}

PageContainer::~PageContainer() {
    // Base teardown still emits entry signals after derived members are destroyed.
    disconnect(this, nullptr, this, nullptr);
    disconnect(m_statusConnection);
    disconnect(m_backConnection);
    m_changing = true;
    clear(Immediate);
    delete m_pool;
}

void PageContainer::classBegin() {
    MaterialPageStack::classBegin();
    QQmlEngine::setContextForObject(m_pool, qmlContext(this));
}

PoolRequest* PageContainer::pendingRequest() const { return m_pending; }
PoolRequest* PageContainer::currentRequest() const { return m_current; }
QString      PageContainer::currentKey() const { return m_currentKey; }
bool         PageContainer::canBack() const { return m_canBack; }

void PageContainer::updateCanBack() {
    const bool value = currentItem() && currentItem()->property("canBack").toBool();
    if (m_canBack == value) return;
    m_canBack = value;
    Q_EMIT canBackChanged();
}

void PageContainer::back() {
    if (auto* item = currentItem()) QMetaObject::invokeMethod(item, "back");
}

void PageContainer::setPendingRequest(PoolRequest* request) {
    if (m_pending == request) return;
    disconnect(m_statusConnection);
    m_pending = request;
    if (request)
        m_statusConnection = connect(
            request, &PoolRequest::statusChanged, this, &PageContainer::handlePendingRequest);
    Q_EMIT pendingRequestChanged();
}

void PageContainer::setCurrentRequest(PoolRequest* request) {
    if (m_current == request) return;
    m_current                = request;
    const QString key        = request && request->cached() ? request->key().toString() : QString();
    const bool    keyChanged = m_currentKey != key;
    m_currentKey             = key;
    QPointer<PageContainer> guard(this);
    Q_EMIT currentRequestChanged();
    if (guard && keyChanged) Q_EMIT currentKeyChanged();
}

void PageContainer::switchTo(const QJSValue& source, const QJSValue& properties, bool cached) {
    if (m_changing) {
        QMetaObject::invokeMethod(
            this,
            [this, source, properties, cached] {
                switchTo(source, properties, cached);
            },
            Qt::QueuedConnection);
        return;
    }
    auto* engine = qmlEngine(this);
    if (! engine) return;
    QString key;
    if (cached) {
        auto object = engine->newObject();
        object.setProperty(QStringLiteral("url"), source);
        object.setProperty(QStringLiteral("props"), properties);
        const auto json = engine->globalObject().property(QStringLiteral("JSON"));
        const auto result =
            json.property(QStringLiteral("stringify")).callWithInstance(json, { object });
        if (result.isError()) {
            Q_EMIT pageLoadFailed(result.toString());
            return;
        }
        key = result.toString();
    }
    QPointer<PageContainer> guard(this);
    m_changing          = true;
    const auto previous = m_pending;
    setPendingRequest(nullptr);
    if (! guard) return;
    if (previous) previous->cancel();
    if (! guard) return;
    if (! cached || key != m_currentKey) {
        completeTransition();
        if (! guard) return;
        auto* request = m_pool->request(source.toVariant(),
                                        properties.toVariant().toMap(),
                                        cached ? QVariant(key) : QVariant(),
                                        Pool::AsynchronousIfNested);
        if (! guard) return;
        setPendingRequest(request);
        if (! guard) return;
    }
    m_changing = false;
    handlePendingRequest();
}

void PageContainer::handlePendingRequest() {
    if (m_changing) {
        QMetaObject::invokeMethod(this, &PageContainer::handlePendingRequest, Qt::QueuedConnection);
        return;
    }
    const auto request = m_pending;
    if (! request || request->status() == PoolRequest::Loading) return;
    if (request->status() != PoolRequest::Ready && request->status() != PoolRequest::Error &&
        request->status() != PoolRequest::Cancelled)
        return;
    QPointer<PageContainer> guard(this);
    m_changing = true;
    setPendingRequest(nullptr);
    if (! guard) return;
    QString error;
    if (request && request->status() == PoolRequest::Ready) {
        m_accepting         = request;
        auto*      item     = qobject_cast<QQuickItem*>(request->object());
        const bool accepted = item && replaceCurrentItem(item);
        if (! guard) return;
        m_accepting = nullptr;
        if (! accepted || m_current != request)
            error = QStringLiteral("failed to replace current page");
    } else if (request && request->status() == PoolRequest::Error) {
        error = request->errorString();
    }
    if (! error.isEmpty()) {
        qCWarning(qml_material_logcat) << "failed to load page:" << error;
        Q_EMIT pageLoadFailed(error);
        if (! guard) return;
        if (request) request->release();
        if (! guard) return;
    }
    m_changing = false;
}
} // namespace qml_material
