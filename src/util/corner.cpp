#include "qml_material/util/corner.hpp"

using namespace qml_material;

CornersGroup::CornersGroup() noexcept: CornersGroup(0) {}
CornersGroup::CornersGroup(qreal r) noexcept: CornersGroup(r, r, r, r) {}
CornersGroup::CornersGroup(qreal bottomRight, qreal topRight, qreal bottomLeft,
                           qreal topLeft) noexcept
    : m_bottomRight(bottomRight),
      m_topRight(topRight),
      m_bottomLeft(bottomLeft),
      m_topLeft(topLeft) {}
CornersGroup::CornersGroup(QVector4D vec4) noexcept
    : m_bottomRight(vec4[0]), m_topRight(vec4[1]), m_bottomLeft(vec4[2]), m_topLeft(vec4[3]) {}
CornersGroup::~CornersGroup() {}

qreal CornersGroup::topLeft() const noexcept { return m_topLeft; }

auto CornersGroup::radius() const noexcept -> qreal {
    // return max
    return std::max({ m_topLeft, m_topRight, m_bottomLeft, m_bottomRight });
}
void CornersGroup::setRadius(qreal v) noexcept {
    m_topLeft     = v;
    m_topRight    = v;
    m_bottomLeft  = v;
    m_bottomRight = v;
}

qreal& CornersGroup::operator[](int index) noexcept {
    switch (index) {
    case 0: return m_topLeft;
    case 1: return m_topRight;
    case 2: return m_bottomLeft;
    case 3: return m_bottomRight;
    default: return m_topLeft;
    }
}
qreal CornersGroup::operator[](int index) const noexcept {
    switch (index) {
    case 0: return m_topLeft;
    case 1: return m_topRight;
    case 2: return m_bottomLeft;
    case 3: return m_bottomRight;
    default: return m_topLeft;
    }
}

CornersGroup CornersGroup::operator+(const CornersGroup& other) const noexcept {
    return CornersGroup(m_bottomRight + other.m_bottomRight,
                        m_topRight + other.m_topRight,
                        m_bottomLeft + other.m_bottomLeft,
                        m_topLeft + other.m_topLeft);
}

CornersGroup CornersGroup::operator-(const CornersGroup& other) const noexcept {
    return CornersGroup(m_bottomRight - other.m_bottomRight,
                        m_topRight - other.m_topRight,
                        m_bottomLeft - other.m_bottomLeft,
                        m_topLeft - other.m_topLeft);
}

CornersGroup CornersGroup::operator*(qreal scalar) const noexcept {
    return CornersGroup(
        m_bottomRight * scalar, m_topRight * scalar, m_bottomLeft * scalar, m_topLeft * scalar);
}

CornersGroup CornersGroup::operator/(qreal scalar) const noexcept {
    if (scalar == 0) return *this;
    return CornersGroup(
        m_bottomRight / scalar, m_topRight / scalar, m_bottomLeft / scalar, m_topLeft / scalar);
}

CornersGroup& CornersGroup::operator+=(const CornersGroup& other) noexcept {
    m_topLeft += other.m_topLeft;
    m_topRight += other.m_topRight;
    m_bottomLeft += other.m_bottomLeft;
    m_bottomRight += other.m_bottomRight;
    return *this;
}

CornersGroup& CornersGroup::operator-=(const CornersGroup& other) noexcept {
    m_topLeft -= other.m_topLeft;
    m_topRight -= other.m_topRight;
    m_bottomLeft -= other.m_bottomLeft;
    m_bottomRight -= other.m_bottomRight;
    return *this;
}

CornersGroup& CornersGroup::operator*=(qreal scalar) noexcept {
    m_topLeft *= scalar;
    m_topRight *= scalar;
    m_bottomLeft *= scalar;
    m_bottomRight *= scalar;
    return *this;
}

CornersGroup& CornersGroup::operator/=(qreal scalar) noexcept {
    if (scalar != 0) {
        m_topLeft /= scalar;
        m_topRight /= scalar;
        m_bottomLeft /= scalar;
        m_bottomRight /= scalar;
    }
    return *this;
}

void CornersGroup::setTopLeft(qreal newTopLeft) noexcept { m_topLeft = newTopLeft; }

qreal CornersGroup::topRight() const noexcept { return m_topRight; }

void CornersGroup::setTopRight(qreal newTopRight) noexcept { m_topRight = newTopRight; }

qreal CornersGroup::bottomLeft() const noexcept { return m_bottomLeft; }

void CornersGroup::setBottomLeft(qreal newBottomLeft) noexcept { m_bottomLeft = newBottomLeft; }

qreal CornersGroup::bottomRight() const noexcept { return m_bottomRight; }

void CornersGroup::setBottomRight(qreal newBottomRight) noexcept { m_bottomRight = newBottomRight; }

QVector4D CornersGroup::toVector4D() const noexcept {
    return QVector4D {
        (float)m_bottomRight, (float)m_topRight, (float)m_bottomLeft, (float)m_topLeft
    };
}
bool CornersGroup::isUniform() const noexcept {
    return qFuzzyCompare(m_bottomLeft, m_bottomRight) && qFuzzyCompare(m_bottomRight, m_topLeft) &&
           qFuzzyCompare(m_topLeft, m_topRight);
}
CornersGroup::operator QVector4D() const noexcept { return toVector4D(); }

auto CornersGroup::interpolated(const CornersGroup& from, const CornersGroup& to, qreal progress)
    -> QVariant {
    return QVariant::fromValue(from + (to - from) * progress);
}

#include <qml_material/util/moc_corner.cpp>