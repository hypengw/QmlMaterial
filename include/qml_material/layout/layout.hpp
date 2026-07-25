#pragma once

#include <QObject>
#include <QQmlEngine>

#include "qml_material/export.hpp"

namespace qml_material
{

/**
 * Attached fill policy for the lightweight layout containers.
 *
 * @ingroup component
 */
class QML_MATERIAL_API LayoutAttached final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool fillWidth READ fillWidth WRITE setFillWidth NOTIFY fillWidthChanged FINAL)
    Q_PROPERTY(bool fillHeight READ fillHeight WRITE setFillHeight NOTIFY fillHeightChanged FINAL)

public:
    explicit LayoutAttached(QObject* parent = nullptr);

    bool fillWidth() const;
    void setFillWidth(bool fill);

    bool fillHeight() const;
    void setFillHeight(bool fill);

    Q_SIGNAL void fillWidthChanged();
    Q_SIGNAL void fillHeightChanged();

private:
    bool m_fill_width  = false;
    bool m_fill_height = false;
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
