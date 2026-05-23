#include "qml_material/util/hct.hpp"

#include "cpp/cam/hct.h"
#include "cpp/palettes/tones.h"

namespace
{
auto qcolor_to_argb(QColor c) -> uint32_t {
    return (uint32_t(c.alpha()) << 24) | (uint32_t(c.red()) << 16) | (uint32_t(c.green()) << 8) |
           uint32_t(c.blue());
}
auto argb_to_qcolor(uint32_t argb) -> QColor {
    return QColor::fromRgba(argb);
}
} // namespace

namespace qml_material
{

Hct::Hct(QObject* parent): QObject(parent), m_color(Qt::black) {
    rebuildFromColor(m_color);
}
Hct::~Hct() = default;

QColor Hct::color() const noexcept { return m_color; }
double Hct::hue() const noexcept { return m_hue; }
double Hct::chroma() const noexcept { return m_chroma; }
double Hct::tone() const noexcept { return m_tone; }

void Hct::setColor(QColor c) {
    if (m_blocked || c == m_color) return;
    rebuildFromColor(c);
}
void Hct::setHue(double v) {
    if (m_blocked) return;
    if (qFuzzyCompare(m_hue, v)) return;
    m_hue = v;
    rebuildFromHct();
}
void Hct::setChroma(double v) {
    if (m_blocked) return;
    if (qFuzzyCompare(m_chroma, v)) return;
    m_chroma = v;
    rebuildFromHct();
}
void Hct::setTone(double v) {
    if (m_blocked) return;
    if (qFuzzyCompare(m_tone, v)) return;
    m_tone = v;
    rebuildFromHct();
}

void Hct::rebuildFromHct() {
    m_blocked = true;
    material_color_utilities::Hct hct(m_hue, m_chroma, m_tone);
    // chroma may have been clamped
    m_hue    = hct.get_hue();
    m_chroma = hct.get_chroma();
    m_tone   = hct.get_tone();
    QColor c = argb_to_qcolor(hct.ToInt());
    bool   color_changed = c != m_color;
    m_color              = c;
    m_blocked            = false;
    Q_EMIT hctChanged();
    if (color_changed) Q_EMIT colorChanged();
}

void Hct::rebuildFromColor(QColor c) {
    m_blocked = true;
    m_color   = c;
    material_color_utilities::Hct hct(qcolor_to_argb(c));
    m_hue     = hct.get_hue();
    m_chroma  = hct.get_chroma();
    m_tone    = hct.get_tone();
    m_blocked = false;
    Q_EMIT colorChanged();
    Q_EMIT hctChanged();
}

HctUtil::HctUtil(QObject* parent): QObject(parent) {}

QColor HctUtil::fromHct(double hue, double chroma, double tone) const {
    material_color_utilities::Hct hct(hue, chroma, tone);
    return argb_to_qcolor(hct.ToInt());
}
QColor HctUtil::paletteTone(QColor seed, double tone) const {
    material_color_utilities::Hct          hct(qcolor_to_argb(seed));
    material_color_utilities::TonalPalette palette(hct);
    return argb_to_qcolor(palette.get(tone));
}
double HctUtil::hueOf(QColor c) const {
    material_color_utilities::Hct hct(qcolor_to_argb(c));
    return hct.get_hue();
}
double HctUtil::chromaOf(QColor c) const {
    material_color_utilities::Hct hct(qcolor_to_argb(c));
    return hct.get_chroma();
}
double HctUtil::toneOf(QColor c) const {
    material_color_utilities::Hct hct(qcolor_to_argb(c));
    return hct.get_tone();
}

} // namespace qml_material

#include "qml_material/util/moc_hct.cpp"
