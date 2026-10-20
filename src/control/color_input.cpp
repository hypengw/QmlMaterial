#include "qml_material/control/color_input.hpp"
#include "qml_material/util/qt.hpp"
#include "input/control_accessibility_p.hpp"
#include <QKeyEvent>
#include <numbers>
#include <cmath>
#include <algorithm>

namespace qml_material
{
namespace
{
constexpr qreal tau = 2 * std::numbers::pi;
}
ColorInput::ColorInput(QQuickItem* parent): InteractiveItem(parent) {
    inputHost().click().keyboardEnabled = false;
    auto& drag                          = inputHost().drag();
    drag.startImmediately               = true;
    drag.started                        = [this](QPointF) {
        m_lastPoint.reset();
    };
    drag.moved = [this](QPointF point) {
        m_lastPoint = point;
        requestAt(point);
    };
    drag.finished = [this](QPointF point, bool cancelled) {
        const bool changed = m_lastPoint != point;
        m_lastPoint.reset();
        if (! cancelled && changed) requestAt(point);
    };
    connect(this, &ColorInput::hueChanged, this, &ColorInput::geometryChanged);
}
void ColorInput::setHue(qreal value) {
    if (! std::isfinite(value)) return;
    value = std::clamp(value, qreal(0), qreal(1));
    if (m_hue == value) return;
    m_hue = value;
    Q_EMIT hueChanged();
}
#define CHANNEL(Name, Upper)                           \
    void ColorInput::set##Upper(qreal value) {         \
        if (! std::isfinite(value)) return;            \
        value = std::clamp(value, qreal(0), qreal(1)); \
        if (m_##Name == value) return;                 \
        m_##Name = value;                              \
        Q_EMIT Name##Changed();                        \
    }
CHANNEL(saturation, Saturation)
CHANNEL(value, Value)
#undef CHANNEL
void ColorInput::geometryChange(const QRectF& now, const QRectF& old) {
    InteractiveItem::geometryChange(now, old);
    if (now.size() == old.size()) return;
    const QPointer<ColorInput> guard(this);
    inputHost().cancelActivation();
    if (guard) Q_EMIT geometryChanged();
}
ColorTriangle::ColorTriangle(QQuickItem* parent): ColorInput(parent) {
    connect(this, &ColorInput::geometryChanged, this, &ColorTriangle::handlePositionChanged);
    connect(this, &ColorInput::saturationChanged, this, &ColorTriangle::handlePositionChanged);
    connect(this, &ColorInput::valueChanged, this, &ColorTriangle::handlePositionChanged);
}
QPointF ColorTriangle::vertex(qreal angle) const {
    const auto radius = std::max(qreal(0), std::min(width(), height()) / 2);
    return QPointF(width() / 2, height() / 2) + radius * QPointF(std::cos(angle), std::sin(angle));
}
QPointF ColorTriangle::hueVertex() const { return vertex(-hue() * tau); }
QPointF ColorTriangle::whiteVertex() const { return vertex(-hue() * tau + tau / 3); }
QPointF ColorTriangle::blackVertex() const { return vertex(-hue() * tau + 2 * tau / 3); }
QPointF ColorTriangle::handlePosition() const {
    return saturation() * value() * hueVertex() + (1 - saturation()) * value() * whiteVertex() +
           (1 - value()) * blackVertex();
}
std::optional<std::array<qreal, 3>> ColorTriangle::coordinates(QPointF point) const {
    const auto a = hueVertex(), b = whiteVertex() - a, c = blackVertex() - a, p = point - a;
    const auto d00 = QPointF::dotProduct(b, b), d01 = QPointF::dotProduct(b, c),
               d11 = QPointF::dotProduct(c, c);
    const auto d20 = QPointF::dotProduct(p, b), d21 = QPointF::dotProduct(p, c);
    const auto denominator = d00 * d11 - d01 * d01;
    if (std::abs(denominator) < 1e-9) return {};
    const auto beta  = (d11 * d20 - d01 * d21) / denominator;
    const auto gamma = (d00 * d21 - d01 * d20) / denominator;
    return std::array<qreal, 3> { 1 - beta - gamma, beta, gamma };
}
bool ColorTriangle::contains(const QPointF& point) const {
    const auto weights = coordinates(point);
    return weights && std::all_of(weights->begin(), weights->end(), [](qreal v) {
               return v >= -1e-9;
           });
}
void ColorTriangle::requestAt(QPointF point) {
    auto weights = coordinates(point);
    if (! weights) return;
    qreal sum = 0;
    for (auto& v : *weights) {
        v = std::clamp(v, qreal(0), qreal(1));
        sum += v;
    }
    if (sum < 1e-9) return;
    const auto value = std::clamp(1 - (*weights)[2] / sum, qreal(0), qreal(1));
    const auto saturation =
        value < 1e-6 ? 0 : std::clamp((*weights)[0] / sum / value, qreal(0), qreal(1));
    Q_EMIT svChangeRequested(saturation, value);
}
void ColorTriangle::keyPressEvent(QKeyEvent* event) {
    auto s = saturation(), v = value();
    switch (event->key()) {
    case Qt::Key_Left: s -= .01; break;
    case Qt::Key_Right: s += .01; break;
    case Qt::Key_Up: v += .01; break;
    case Qt::Key_Down: v -= .01; break;
    default: InteractiveItem::keyPressEvent(event); return;
    }
    event->accept();
    if (isEnabled())
        Q_EMIT svChangeRequested(std::clamp(s, qreal(0), qreal(1)),
                                 std::clamp(v, qreal(0), qreal(1)));
}
ColorWheel::ColorWheel(QQuickItem* parent): ColorInput(parent) {
    connect(this, &ColorInput::hueChanged, this, [this] {
        detail::notifyAccessibleValue(this, hue());
    });
}
ColorWheel::~ColorWheel() { utils::disconnectAll(m_triangleConnections); }
qreal ColorWheel::outerRadius() const {
    return std::max(qreal(0), std::min(width(), height()) / 2 - 1);
}
qreal   ColorWheel::innerRadius() const { return std::max(qreal(0), outerRadius() - thickness()); }
QPointF ColorWheel::handlePosition() const {
    const auto radius = (outerRadius() + innerRadius()) / 2;
    return QPointF(width() / 2, height() / 2) +
           radius * QPointF(std::cos(hue() * tau), -std::sin(hue() * tau));
}
void ColorWheel::setThickness(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_thickness) return;
    m_thickness = value;
    const QPointer<ColorWheel> guard(this);
    inputHost().cancelActivation();
    if (guard) Q_EMIT geometryChanged();
}
bool ColorWheel::contains(const QPointF& point) const {
    const auto delta  = point - QPointF(width() / 2, height() / 2);
    const auto radius = std::hypot(delta.x(), delta.y());
    return outerRadius() > 0 && thickness() > 0 && radius >= innerRadius() &&
           radius <= outerRadius() + 2;
}
void ColorWheel::requestAt(QPointF point) {
    const auto delta = point - QPointF(width() / 2, height() / 2);
    if (delta.isNull()) return;
    auto angle = std::atan2(-delta.y(), delta.x()) / tau;
    if (angle < 0) angle += 1;
    requestHue(angle);
}
void ColorWheel::requestHue(qreal value) {
    if (isEnabled() && isVisible() && std::isfinite(value))
        Q_EMIT hueChangeRequested(std::clamp(value, qreal(0), qreal(1)));
}
void ColorWheel::keyPressEvent(QKeyEvent* event) {
    qreal delta;
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Down: delta = -1.0 / 360; break;
    case Qt::Key_Right:
    case Qt::Key_Up: delta = 1.0 / 360; break;
    default: InteractiveItem::keyPressEvent(event); return;
    }
    event->accept();
    if (isEnabled()) Q_EMIT hueChangeRequested(std::fmod(hue() + delta + 1, 1));
}
void ColorWheel::setTriangle(ColorTriangle* triangle) {
    if (m_triangle == triangle) return;
    const QPointer<ColorWheel>    guard(this);
    const QPointer<ColorTriangle> next(triangle);
    utils::disconnectAll(m_triangleConnections);
    if (m_triangle) m_triangle->cancelActivation();
    if (! guard) return;
    m_triangle = next;
    if (next) {
        m_triangleConnections.append(
            connect(next, &ColorInput::svChangeRequested, this, &ColorInput::svChangeRequested));
        m_triangleConnections.append(
            connect(next, &QObject::destroyed, this, &ColorWheel::triangleChanged));
    }
    Q_EMIT triangleChanged();
}
} // namespace qml_material
