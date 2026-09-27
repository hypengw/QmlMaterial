#include "qml_material/item/item_holder.hpp"
#include "qml_material/util/qt.hpp"
#include <QQmlEngine>
#include <QQmlProperty>
#include <QtQml/private/qqmlanybinding_p.h>
#include <QtQml/private/qqmlpropertytopropertybinding_p.h>

namespace qml_material
{
ItemHolder::~ItemHolder() {
    utils::disconnectAll(m_connections);
    if (m_releaseBinding) m_releaseBinding();
}
void ItemHolder::classBegin() {
    QQuickItem::classBegin();
    for (const auto& names : { std::pair { "itemImplicitWidth", "implicitWidth" },
                               std::pair { "itemImplicitHeight", "implicitHeight" } }) {
        const QQmlProperty source(this, QString::fromLatin1(names.first));
        const QQmlProperty target(this, QString::fromLatin1(names.second));
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        auto binding = QQmlPropertyToPropertyBinding::create(qmlEngine(this), source, target);
#else
        QQmlAnyBinding binding;
        binding = new QQmlPropertyToPropertyBinding(qmlEngine(this),
                                                    source.object(),
#    if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                                                    QQmlPropertyIndex(source.index()),
#    else
                                                    source.index(),
#    endif
                                                    target.object(),
                                                    target.index());
#endif
        binding.installOn(target);
    }
}
qreal ItemHolder::itemImplicitWidth() const { return m_item ? m_item->implicitWidth() : 0; }
qreal ItemHolder::itemImplicitHeight() const { return m_item ? m_item->implicitHeight() : 0; }
void  ItemHolder::setItem(QQuickItem* value) {
    if (m_identity == value) return;
    if (value && (value == this || value->isAncestorOf(this))) return;
    QPointer<ItemHolder> guard(this);
    QPointer<QQuickItem> next(value);
    const auto           revision = ++m_revision;
    utils::disconnectAll(m_connections);
    if (auto release = std::exchange(m_releaseBinding, {})) release();
    auto old   = m_item;
    m_item     = value;
    m_identity = value;
    if (old && old->parentItem() == this) old->setParentItem(nullptr);
    if (! guard || revision != m_revision) return;
    if (next) {
        m_connections.append(connect(next, &QObject::destroyed, this, [this] {
            setItem(nullptr);
        }));
        m_connections.append(connect(
            next, &QQuickItem::implicitWidthChanged, this, &ItemHolder::itemImplicitSizeChanged));
        m_connections.append(connect(
            next, &QQuickItem::implicitHeightChanged, this, &ItemHolder::itemImplicitSizeChanged));
        next->setParentItem(this);
        if (! guard || revision != m_revision || ! next) return;
        QQmlAnyBinding widthBinding, heightBinding;
        widthBinding  = bindableWidth().makeBinding();
        heightBinding = bindableHeight().makeBinding();
        auto release  = [next, widthBinding, heightBinding] {
            if (! next) return;
            const QQmlProperty width(next, QStringLiteral("width"));
            const QQmlProperty height(next, QStringLiteral("height"));
            if (QQmlAnyBinding::ofProperty(width) == widthBinding) QQmlAnyBinding::takeFrom(width);
            if (QQmlAnyBinding::ofProperty(height) == heightBinding)
                QQmlAnyBinding::takeFrom(height);
        };
        m_releaseBinding = release;
        if (widthBinding) widthBinding.installOn(QQmlProperty(next, QStringLiteral("width")));
        if (! guard || revision != m_revision || ! next) {
            // A binding may finish installing after its first evaluation replaces the item.
            release();
            return;
        }
        if (heightBinding) heightBinding.installOn(QQmlProperty(next, QStringLiteral("height")));
        if (! guard || revision != m_revision || ! next) {
            release();
            return;
        }
    }
    Q_EMIT itemChanged();
    if (guard && revision == m_revision) Q_EMIT itemImplicitSizeChanged();
}
} // namespace qml_material
