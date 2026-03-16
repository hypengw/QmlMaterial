#pragma once

#include <QtCore/QList>
#include <QtGui/QColor>
#include <QtGui/QPolygonF>
#include <QtQml/QQmlEngine>

#include "qml_material/core.hpp"

namespace qml_material
{

class LoadingIndicatorUpdator : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // ── Animated outputs ────────────────────────────────────────────────────
    Q_PROPERTY(QPolygonF currentShape  READ currentShape  NOTIFY updated      FINAL)
    Q_PROPERTY(double    rotation      READ rotation      NOTIFY updated      FINAL)
    Q_PROPERTY(QColor    color         READ color         NOTIFY updated      FINAL)
    Q_PROPERTY(double    morphFraction READ morphFraction NOTIFY updated      FINAL)

    // ── Driver input ────────────────────────────────────────────────────────
    Q_PROPERTY(double    progress      READ progress  WRITE setProgress NOTIFY updated FINAL)

    // ── Configuration ───────────────────────────────────────────────────────
    Q_PROPERTY(QList<QColor> colors READ colors WRITE setColors NOTIFY colorsChanged FINAL)

    // ── Constants exposed to QML (avoids hard-coding magic numbers) ─────────
    Q_PROPERTY(int shapeCount READ shapeCount CONSTANT FINAL)
    Q_PROPERTY(int msPerShape READ msPerShape CONSTANT FINAL)

public:
    explicit LoadingIndicatorUpdator(QObject* parent = nullptr);

    // Animated outputs
    auto currentShape()  const noexcept -> QPolygonF;
    auto rotation()      const noexcept -> double;
    auto color()         const noexcept -> QColor;
    auto morphFraction() const noexcept -> double;

    // Driver
    auto progress() const noexcept -> double;
    void setProgress(double p);

    // Configuration
    auto colors() const -> QList<QColor>;
    void setColors(const QList<QColor>& colors);

    // Constants
    static int shapeCount() { return SHAPE_COUNT; }
    static int msPerShape()  { return MS_PER_SHAPE; }

    Q_SIGNAL void updated();
    Q_SIGNAL void colorsChanged();

private:
    void updateInternal() noexcept;

    auto interpolateShapes(const QPolygonF& s1, const QPolygonF& s2,
                           double t) const -> QPolygonF;

    // State
    double        m_progress;
    double        m_morph_fraction;
    double        m_rotation;
    QColor        m_color;
    QList<QColor> m_colors;

    // Pre-built shape data
    QList<QPolygonF> m_predefined_shapes; // original shapes
    QList<QPolygonF> m_aligned_shapes;    // s[i] rotated to best match s[i-1]

    // Constants
    static constexpr int SHAPE_COUNT       = 7;
    static constexpr int SAMPLE_COUNT      = 64; // points per shape (power-of-2)
    static constexpr int MS_PER_SHAPE      = 650;
};

} // namespace qml_material
