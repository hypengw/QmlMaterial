#pragma once

#include <QQuickItem>
#include <QPointer>
#include <array>
#include <optional>
#include "qml_material/export.hpp"

namespace qml_material
{

class QML_MATERIAL_API TextControlBackground : public QObject {
    Q_OBJECT
public:
    explicit TextControlBackground(QQuickItem* owner);
    ~TextControlBackground() override;
    QQuickItem* item() const { return m_item; }
    void        setItem(QQuickItem*);
    qreal       implicitWidth() const;
    qreal       implicitHeight() const;
    qreal       inset(Qt::Edge) const;
    void        setInset(Qt::Edge, qreal);
    void        resetInset(Qt::Edge);
    void        complete();
    void        layout();

    Q_SIGNAL void itemChanged();
    Q_SIGNAL void implicitWidthChanged();
    Q_SIGNAL void implicitHeightChanged();
    Q_SIGNAL void insetChanged(Qt::Edge);

private:
    std::optional<qreal>&               insetValue(Qt::Edge);
    const std::optional<qreal>&         insetValue(Qt::Edge) const;
    void                                sizeChanged();
    QQuickItem*                         m_owner;
    QPointer<QQuickItem>                m_item;
    QList<QMetaObject::Connection>      m_connections;
    std::array<std::optional<qreal>, 4> m_insets;
    bool                                m_width_explicit  = false;
    bool                                m_height_explicit = false;
    bool                                m_laying_out      = false;
    bool                                m_complete        = false;
};

} // namespace qml_material
