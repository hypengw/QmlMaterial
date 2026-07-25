#pragma once

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QQmlEngine>

#include "qml_material/export.hpp"

namespace qml_material
{

/**
 * Attached fill and participation policy for the lightweight layout containers.
 *
 * @ingroup component
 */
class QML_MATERIAL_API LayoutAttached final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool fillWidth READ fillWidth WRITE setFillWidth NOTIFY fillWidthChanged FINAL)
    Q_PROPERTY(bool fillHeight READ fillHeight WRITE setFillHeight NOTIFY fillHeightChanged FINAL)
    Q_PROPERTY(QQuickItem* visibilitySource READ visibilitySource WRITE setVisibilitySource RESET
                   resetVisibilitySource NOTIFY visibilitySourceChanged FINAL)

public:
    explicit LayoutAttached(QObject* parent = nullptr);

    bool fillWidth() const;
    void setFillWidth(bool fill);

    bool fillHeight() const;
    void setFillHeight(bool fill);

    QQuickItem* visibilitySource() const;
    void        setVisibilitySource(QQuickItem* source);
    void        resetVisibilitySource();

    // A configured null source excludes the item; an unset source preserves legacy behavior.
    bool isVisibilitySourceSet() const;

    Q_SIGNAL void fillWidthChanged();
    Q_SIGNAL void fillHeightChanged();
    Q_SIGNAL void visibilitySourceChanged();

private:
    QPointer<QQuickItem>    m_visibility_source;
    QMetaObject::Connection m_visibility_source_destroyed;
    bool                    m_fill_width            = false;
    bool                    m_fill_height           = false;
    bool                    m_visibility_source_set = false;
};

/**
 * Provides attached properties for Box, Row, and Column children.
 *
 * @ingroup component
 */
class QML_MATERIAL_API Layout : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Layout)
    QML_UNCREATABLE("Layout is only available via attached properties.")
    QML_ATTACHED(LayoutAttached)

public:
    explicit Layout(QObject* parent = nullptr);

    static LayoutAttached* qmlAttachedProperties(QObject* object);
};

} // namespace qml_material
