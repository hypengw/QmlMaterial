#include "qml_material/util/corner.hpp"

using namespace qml_material;

CornersGroup::CornersGroup(): CornersGroup(0) {}
CornersGroup::CornersGroup(qreal r): CornersGroup(r, r, r, r) {}
CornersGroup::CornersGroup(qreal bottomRight, qreal topRight, qreal bottomLeft, qreal topLeft)
    : m_bottomRight(bottomRight),
      m_topRight(topRight),
      m_bottomLeft(bottomLeft),
      m_topLeft(topLeft) {}
CornersGroup::~CornersGroup() {}

qreal CornersGroup::topLeft() const { return m_topLeft; }

auto CornersGroup::radius() const -> qreal {
    // return max
    return std::max({ m_topLeft, m_topRight, m_bottomLeft, m_bottomRight });
}
void CornersGroup::setRadius(qreal v) {
    m_topLeft     = v;
    m_topRight    = v;
    m_bottomLeft  = v;
    m_bottomRight = v;
}
void CornersGroup::setTopLeft(qreal newTopLeft) { m_topLeft = newTopLeft; }

qreal CornersGroup::topRight() const { return m_topRight; }

void CornersGroup::setTopRight(qreal newTopRight) { m_topRight = newTopRight; }

qreal CornersGroup::bottomLeft() const { return m_bottomLeft; }

void CornersGroup::setBottomLeft(qreal newBottomLeft) { m_bottomLeft = newBottomLeft; }

qreal CornersGroup::bottomRight() const { return m_bottomRight; }

void CornersGroup::setBottomRight(qreal newBottomRight) { m_bottomRight = newBottomRight; }

QVector4D CornersGroup::toVector4D() const noexcept {
    return QVector4D {
        (float)m_topLeft, (float)m_topRight, (float)m_bottomLeft, (float)m_bottomRight
    };
}
CornersGroup::operator QVector4D() const noexcept { return toVector4D(); }


#include <qml_material/util/moc_corner.cpp>