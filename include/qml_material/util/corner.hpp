#pragma once
#include <QQmlEngine>
#include <QVector4D>
#include "qml_material/export.hpp"

namespace qml_material
{
class CornersGroup {
    Q_GADGET_EXPORT(QML_MATERIAL_API)
    QML_VALUE_TYPE(corners)
    QML_CONSTRUCTIBLE_VALUE

    Q_PROPERTY(qreal topLeft READ topLeft WRITE setTopLeft FINAL)
    Q_PROPERTY(qreal topRight READ topRight WRITE setTopRight FINAL)
    Q_PROPERTY(qreal bottomLeft READ bottomLeft WRITE setBottomLeft FINAL)
    Q_PROPERTY(qreal bottomRight READ bottomRight WRITE setBottomRight FINAL)

public:
    QML_MATERIAL_API Q_INVOKABLE CornersGroup() noexcept;
    QML_MATERIAL_API ~CornersGroup();

    Q_INVOKABLE CornersGroup(qreal) noexcept;
    Q_INVOKABLE CornersGroup(qreal bottomRight, qreal topRight, qreal bottomLeft,
                             qreal topLeft) noexcept;
    Q_INVOKABLE CornersGroup(QVector4D vec4) noexcept;

    CornersGroup(const CornersGroup&)            = default;
    CornersGroup(CornersGroup&&)                 = default;
    CornersGroup& operator=(const CornersGroup&) = default;
    CornersGroup& operator=(CornersGroup&&)      = default;

    Q_INVOKABLE qreal radius() const noexcept;
    void              setRadius(qreal) noexcept;

    qreal& operator[](int index) noexcept;
    qreal  operator[](int index) const noexcept;

    CornersGroup operator+(const CornersGroup& other) const noexcept;
    CornersGroup operator-(const CornersGroup& other) const noexcept;
    CornersGroup operator*(qreal scalar) const noexcept;
    CornersGroup operator/(qreal scalar) const noexcept;

    CornersGroup& operator+=(const CornersGroup& other) noexcept;
    CornersGroup& operator-=(const CornersGroup& other) noexcept;
    CornersGroup& operator*=(qreal scalar) noexcept;
    CornersGroup& operator/=(qreal scalar) noexcept;

    qreal topLeft() const noexcept;
    void  setTopLeft(qreal newTopLeft) noexcept;

    qreal topRight() const noexcept;
    void  setTopRight(qreal newTopRight) noexcept;

    qreal bottomLeft() const noexcept;
    void  setBottomLeft(qreal newBottomLeft) noexcept;

    qreal bottomRight() const noexcept;
    void  setBottomRight(qreal newBottomRight) noexcept;

    Q_INVOKABLE QVector4D toVector4D() const noexcept;
                          operator QVector4D() const noexcept;

    QML_MATERIAL_API static auto interpolated(const CornersGroup& from, const CornersGroup& to,
                                              qreal progress) -> QVariant;

private:
    qreal m_bottomRight;
    qreal m_topRight;
    qreal m_bottomLeft;
    qreal m_topLeft;
};

} // namespace qml_material