#pragma once

#include <QQuickItem>
#include <QQmlEngine>
#include <memory>

#include "qml_material/export.hpp"

namespace qml_material
{

class LayoutContainerPrivate;

/**
 * Shared owner for lightweight layout geometry and preferred size propagation.
 *
 * Child implicit sizes determine this item's implicit size. Child actual sizes determine placement
 * and content size. The container owns x/y for participating children and owns width/height only
 * on axes explicitly selected by Layout.fillWidth or Layout.fillHeight.
 *
 * @ingroup component
 */
class QML_MATERIAL_API LayoutContainer : public QQuickItem {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qreal implicitWidth READ implicitWidth NOTIFY implicitWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight NOTIFY implicitHeightChanged FINAL)
    Q_PROPERTY(
        qreal leftPadding READ leftPadding WRITE setLeftPadding NOTIFY leftPaddingChanged FINAL)
    Q_PROPERTY(
        qreal rightPadding READ rightPadding WRITE setRightPadding NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding NOTIFY topPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY
                   bottomPaddingChanged FINAL)
    Q_PROPERTY(
        Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection
                   NOTIFY layoutDirectionChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection effectiveLayoutDirection READ effectiveLayoutDirection NOTIFY
                   effectiveLayoutDirectionChanged FINAL)
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentWidthChanged FINAL)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentHeightChanged FINAL)

public:
    ~LayoutContainer() override;

    qreal leftPadding() const;
    void  setLeftPadding(qreal padding);
    qreal rightPadding() const;
    void  setRightPadding(qreal padding);
    qreal topPadding() const;
    void  setTopPadding(qreal padding);
    qreal bottomPadding() const;
    void  setBottomPadding(qreal padding);

    Qt::Alignment alignment() const;
    void          setAlignment(Qt::Alignment alignment);

    Qt::LayoutDirection layoutDirection() const;
    void                setLayoutDirection(Qt::LayoutDirection direction);
    Qt::LayoutDirection effectiveLayoutDirection() const;

    qreal contentWidth() const;
    qreal contentHeight() const;

    Q_SIGNAL void leftPaddingChanged();
    Q_SIGNAL void rightPaddingChanged();
    Q_SIGNAL void topPaddingChanged();
    Q_SIGNAL void bottomPaddingChanged();
    Q_SIGNAL void alignmentChanged();
    Q_SIGNAL void layoutDirectionChanged();
    Q_SIGNAL void effectiveLayoutDirectionChanged();
    Q_SIGNAL void contentWidthChanged();
    Q_SIGNAL void contentHeightChanged();

protected:
    enum class Mode
    {
        Box,
        Row,
        Column,
    };

    explicit LayoutContainer(Mode mode, QQuickItem* parent = nullptr);

    qreal linearSpacing() const;
    bool  setLinearSpacing(qreal spacing);

    void componentComplete() override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData& data) override;
    void updatePolish() override;

private:
    friend class LayoutContainerPrivate;
    const std::unique_ptr<LayoutContainerPrivate> d;
};

class QML_MATERIAL_API LinearLayoutContainer : public LayoutContainer {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)

public:
    qreal spacing() const;
    void  setSpacing(qreal spacing);

    Q_SIGNAL void spacingChanged();

protected:
    explicit LinearLayoutContainer(Mode mode, QQuickItem* parent = nullptr);
};

/**
 * Overlays children in its padded content rectangle.
 *
 * @ingroup component
 */
class QML_MATERIAL_API Box : public LayoutContainer {
    Q_OBJECT
    QML_NAMED_ELEMENT(Box)

public:
    explicit Box(QQuickItem* parent = nullptr);
};

/**
 * Places children in one horizontal line and supports one Layout.fillWidth child.
 *
 * @ingroup component
 */
class QML_MATERIAL_API Row : public LinearLayoutContainer {
    Q_OBJECT
    QML_NAMED_ELEMENT(Row)

public:
    explicit Row(QQuickItem* parent = nullptr);
};

/**
 * Places children in one vertical line and supports one Layout.fillHeight child.
 *
 * @ingroup component
 */
class QML_MATERIAL_API Column : public LinearLayoutContainer {
    Q_OBJECT
    QML_NAMED_ELEMENT(Column)

public:
    explicit Column(QQuickItem* parent = nullptr);
};

} // namespace qml_material
