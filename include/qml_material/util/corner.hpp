#pragma once
#include <QQmlEngine>
#include <QVector4D>

namespace qml_material
{
class CornersGroup {
    Q_GADGET
    QML_VALUE_TYPE(corners)
    QML_CONSTRUCTIBLE_VALUE

    Q_PROPERTY(qreal topLeft READ topLeft WRITE setTopLeft FINAL)
    Q_PROPERTY(qreal topRight READ topRight WRITE setTopRight FINAL)
    Q_PROPERTY(qreal bottomLeft READ bottomLeft WRITE setBottomLeft FINAL)
    Q_PROPERTY(qreal bottomRight READ bottomRight WRITE setBottomRight FINAL)

public:
    Q_INVOKABLE CornersGroup() noexcept;
    Q_INVOKABLE CornersGroup(qreal) noexcept;
    Q_INVOKABLE CornersGroup(qreal bottomRight, qreal topRight, qreal bottomLeft, qreal topLeft) noexcept;
    ~CornersGroup();

    Q_INVOKABLE CornersGroup& operator=(qreal) { return *this; }

    Q_INVOKABLE qreal radius() const;
    void setRadius(qreal);

    qreal topLeft() const;
    void  setTopLeft(qreal newTopLeft);

    qreal topRight() const;
    void  setTopRight(qreal newTopRight);

    qreal bottomLeft() const;
    void  setBottomLeft(qreal newBottomLeft);

    qreal bottomRight() const;
    void  setBottomRight(qreal newBottomRight);

    Q_INVOKABLE QVector4D toVector4D() const noexcept;
    operator QVector4D() const noexcept;

private:
    double m_bottomRight;
    double m_topRight;
    double m_bottomLeft;
    double m_topLeft;
};

} // namespace qml_material