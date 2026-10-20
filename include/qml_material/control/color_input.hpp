#pragma once

#include "qml_material/input/interactive_item.hpp"
#include <array>

namespace qml_material
{
/** @ingroup control */
class QML_MATERIAL_API ColorInput : public InteractiveItem {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged FINAL)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged FINAL)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
public:
    explicit ColorInput(QQuickItem* parent = nullptr);
    qreal         hue() const { return m_hue; }
    qreal         saturation() const { return m_saturation; }
    qreal         value() const { return m_value; }
    void          setHue(qreal);
    void          setSaturation(qreal);
    void          setValue(qreal);
    Q_SIGNAL void hueChanged();
    Q_SIGNAL void saturationChanged();
    Q_SIGNAL void valueChanged();
    Q_SIGNAL void svChangeRequested(qreal saturation, qreal value);
    Q_SIGNAL void geometryChanged();

protected:
    void         activate() override {}
    virtual void requestAt(QPointF) = 0;
    void         geometryChange(const QRectF&, const QRectF&) override;

private:
    qreal                  m_hue = 0, m_saturation = 1, m_value = 1;
    std::optional<QPointF> m_lastPoint;
};

/** @ingroup control */
class QML_MATERIAL_API ColorTriangle : public ColorInput {
    Q_OBJECT
    QML_NAMED_ELEMENT(ColorTriangleBase)
    Q_PROPERTY(QPointF hueVertex READ hueVertex NOTIFY geometryChanged FINAL)
    Q_PROPERTY(QPointF whiteVertex READ whiteVertex NOTIFY geometryChanged FINAL)
    Q_PROPERTY(QPointF blackVertex READ blackVertex NOTIFY geometryChanged FINAL)
    Q_PROPERTY(QPointF handlePosition READ handlePosition NOTIFY handlePositionChanged FINAL)
public:
    explicit ColorTriangle(QQuickItem* parent = nullptr);
    QPointF       hueVertex() const;
    QPointF       whiteVertex() const;
    QPointF       blackVertex() const;
    QPointF       handlePosition() const;
    bool          contains(const QPointF&) const override;
    Q_SIGNAL void handlePositionChanged();

protected:
    void requestAt(QPointF) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    QPointF                             vertex(qreal angle) const;
    std::optional<std::array<qreal, 3>> coordinates(QPointF) const;
};

/** @ingroup control */
class QML_MATERIAL_API ColorWheel : public ColorInput {
    Q_OBJECT
    QML_NAMED_ELEMENT(ColorWheelBase)
    Q_PROPERTY(qreal thickness READ thickness WRITE setThickness NOTIFY geometryChanged FINAL)
    Q_PROPERTY(qreal outerRadius READ outerRadius NOTIFY geometryChanged FINAL)
    Q_PROPERTY(qreal innerRadius READ innerRadius NOTIFY geometryChanged FINAL)
    Q_PROPERTY(QPointF handlePosition READ handlePosition NOTIFY geometryChanged FINAL)
    Q_PROPERTY(ColorTriangle* triangle READ triangle WRITE setTriangle NOTIFY triangleChanged FINAL)
public:
    explicit ColorWheel(QQuickItem* parent = nullptr);
    ~ColorWheel() override;
    qreal            thickness() const { return m_thickness; }
    void             setThickness(qreal);
    qreal            outerRadius() const;
    qreal            innerRadius() const;
    QPointF          handlePosition() const;
    bool             contains(const QPointF&) const override;
    ColorTriangle*   triangle() const { return m_triangle; }
    void             setTriangle(ColorTriangle*);
    Q_INVOKABLE void requestHue(qreal);
    Q_SIGNAL void    triangleChanged();
    Q_SIGNAL void    hueChangeRequested(qreal hue);

protected:
    void requestAt(QPointF) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    qreal                          m_thickness = 22;
    QPointer<ColorTriangle>        m_triangle;
    QList<QMetaObject::Connection> m_triangleConnections;
};
} // namespace qml_material
