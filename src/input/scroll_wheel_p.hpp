#pragma once

#include <QByteArray>
#include <QPointF>
#include <QWheelEvent>
#include <optional>

namespace qml_material
{
enum class WheelDistanceMode
{
    Platform,
    Steps
};

inline std::optional<WheelDistanceMode> parseWheelDistanceMode(QByteArray value) {
    value = value.trimmed().toLower();
    if (value.isEmpty() || value == "steps") return WheelDistanceMode::Steps;
    if (value == "platform") return WheelDistanceMode::Platform;
    return {};
}

struct WheelInput {
    QPointF delta;
    bool    smooth;
};

inline WheelInput wheelInput(const QWheelEvent& event, WheelDistanceMode mode, int scrollLines) {
    // Ordinary wheels can report pixels too; phases select direct input.
    const bool    smooth = event.phase() == Qt::NoScrollPhase;
    const QPointF pixels = -QPointF(event.pixelDelta());
    if (! pixels.isNull() && (! smooth || mode == WheelDistanceMode::Platform))
        return { pixels, smooth };
    return { -QPointF(event.angleDelta()) / 120 * (24.0 * scrollLines), smooth };
}
} // namespace qml_material
