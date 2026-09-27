#include "qml_material/control/label.hpp"
#include "qml_material/util/qt.hpp"
#include <QGuiApplication>

namespace qml_material
{

Label::Label(QQuickItem* parent): QQuickText(parent), m_background(this) {
    connect(&m_background, &TextControlBackground::itemChanged, this, &Label::backgroundChanged);
    connect(&m_background,
            &TextControlBackground::implicitWidthChanged,
            this,
            &Label::implicitBackgroundWidthChanged);
    connect(&m_background,
            &TextControlBackground::implicitHeightChanged,
            this,
            &Label::implicitBackgroundHeightChanged);
    connect(&m_background, &TextControlBackground::insetChanged, this, [this](Qt::Edge edge) {
        switch (edge) {
        case Qt::TopEdge: Q_EMIT topInsetChanged(); break;
        case Qt::LeftEdge: Q_EMIT leftInsetChanged(); break;
        case Qt::RightEdge: Q_EMIT rightInsetChanged(); break;
        case Qt::BottomEdge: Q_EMIT bottomInsetChanged(); break;
        }
    });
    resolveFont();
    connect(qGuiApp, &QGuiApplication::fontChanged, this, &Label::resolveFont);
}

Label::~Label() = default;

QFont Label::font() const {
    auto result = m_effective_font;
    result.setResolveMask(m_requested_font.resolveMask());
    return result;
}

void Label::setFont(const QFont& font) {
    if (m_requested_font == font && m_requested_font.resolveMask() == font.resolveMask()) return;
    m_requested_font = font;
    resolveFont();
}

void Label::resetFont() { setFont(QFont()); }

void Label::inheritFont(const QFont& inherited) {
    if (! utils::canUpdateControlEnvironment()) return;
    const auto resolved = utils::resolveFont(m_requested_font, inherited);
    const bool changed  = m_effective_font != resolved;
    if (! changed && m_effective_font.resolveMask() == resolved.resolveMask()) return;
    m_effective_font = resolved;
    QPointer<Label> guard(this);
    QQuickText::setFont(resolved);
    if (! guard) return;
    utils::propagateFont(this, resolved);
    if (guard && changed) Q_EMIT fontChanged();
}

void Label::resolveFont() {
    if (! utils::canUpdateControlEnvironment()) return;
    inheritFont(utils::inheritedFont(this));
}

qreal Label::implicitBackgroundWidth() const { return m_background.implicitWidth(); }
qreal Label::implicitBackgroundHeight() const { return m_background.implicitHeight(); }
void  Label::setBackground(QQuickItem* item) { m_background.setItem(item); }

void Label::componentComplete() {
    QQuickText::componentComplete();
    resolveFont();
    m_background.complete();
}
void Label::itemChange(ItemChange change, const ItemChangeData& data) {
    QQuickText::itemChange(change, data);
    if ((change == ItemParentHasChanged && data.item) || (change == ItemSceneChange && data.window))
        resolveFont();
}
void Label::geometryChange(const QRectF& geometry, const QRectF& oldGeometry) {
    QQuickText::geometryChange(geometry, oldGeometry);
    m_background.layout();
}

void Label::setTopInset(qreal value) { m_background.setInset(Qt::TopEdge, value); }
void Label::resetTopInset() { m_background.resetInset(Qt::TopEdge); }

void Label::setLeftInset(qreal value) { m_background.setInset(Qt::LeftEdge, value); }
void Label::resetLeftInset() { m_background.resetInset(Qt::LeftEdge); }

void Label::setRightInset(qreal value) { m_background.setInset(Qt::RightEdge, value); }
void Label::resetRightInset() { m_background.resetInset(Qt::RightEdge); }

void Label::setBottomInset(qreal value) { m_background.setInset(Qt::BottomEdge, value); }
void Label::resetBottomInset() { m_background.resetInset(Qt::BottomEdge); }
} // namespace qml_material
