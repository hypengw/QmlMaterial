#include "qml_material/control/page_context.hpp"
#include "qml_material/core/enum.hpp"

namespace qml_material
{
PageContext::PageContext(QObject* parent): QObject(parent) {}

PageContext::~PageContext() { disconnectInherit(); }

auto PageContext::inherit() const -> PageContext* { return m_inherit.data(); }
void PageContext::setInherit(PageContext* v) {
    if (v == m_inherit.data() || v == this) {
        return;
    }
    for (auto* ancestor = v; ancestor; ancestor = ancestor->inherit()) {
        if (ancestor == this) return;
    }

    disconnectInherit();
    m_inherit = v;

    if (v) {
#define X(Func, Mem)                                                                      \
    m_inherit_connections.append(connect(v, &PageContext::Func##Changed, this, [this]() { \
        if (! Mem.has_value()) Func##Changed();                                           \
    }))
        X(leadingAction, m_leading_action);
        X(headerType, m_header_type);
        X(backgroundRadius, m_background_radius);
        X(headerBackgroundOpacity, m_header_background_opacity);
        X(radius, m_radius);
        X(showHeader, m_show_header);
        X(showBackground, m_show_background);
        X(leftMargin, m_left_margin);
        X(rightMargin, m_right_margin);
        X(topMargin, m_top_margin);
        X(bottomMargin, m_bottom_margin);
#undef X
        m_inherit_connections.append(connect(v, &QObject::destroyed, this, [this]() {
            m_inherit = nullptr;
            m_inherit_connections.clear();
            inheritChanged();
            notifyInheritedProperties();
        }));
    }

    inheritChanged();
    notifyInheritedProperties();
}

#define X(Func, Mem, Def) (Mem.has_value() ? *Mem : (m_inherit ? m_inherit->Func() : Def))

auto PageContext::leadingAction() const -> QObject* {
    return X(leadingAction, m_leading_action, nullptr);
}
void PageContext::setLeadingAction(QObject* v) {
    if (v != m_leading_action) {
        m_leading_action = v;
        if (v && v->parent() == nullptr) {
            v->setParent(this);
        }
        leadingActionChanged();
    }
}

auto PageContext::headerType() const -> qint32 {
    return X(headerType, m_header_type, (qint32)Enum::AppBarType::AppBarSmall);
}
void PageContext::setHeaderType(qint32 v) {
    if (v != m_header_type) {
        m_header_type = v;
        headerTypeChanged();
    }
}

auto PageContext::backgroundRadius() const -> qint32 {
    return X(backgroundRadius, m_background_radius, 0);
}
void PageContext::setBackgroundRadius(qint32 v) {
    if (v != m_background_radius) {
        m_background_radius = v;
        backgroundRadiusChanged();
    }
}

auto PageContext::headerBackgroundOpacity() const -> double {
    return X(headerBackgroundOpacity, m_header_background_opacity, 1.0);
}
void PageContext::setHeaderBackgroundOpacity(double v) {
    if (! m_header_background_opacity || ! qFuzzyCompare(m_header_background_opacity.value(), v)) {
        m_header_background_opacity = v;
        headerBackgroundOpacityChanged();
    }
}

auto PageContext::radius() const -> qint32 { return X(radius, m_radius, 0); }
void PageContext::setRadius(qint32 v) {
    if (v != m_radius) {
        m_radius = v;
        radiusChanged();
    }
}

auto PageContext::showHeader() const -> bool { return X(showHeader, m_show_header, false); }
void PageContext::setShowHeader(bool v) {
    if (v != m_show_header) {
        m_show_header = v;
        showHeaderChanged();
    }
}

auto PageContext::showBackground() const -> bool {
    return X(showBackground, m_show_background, false);
}
void PageContext::setShowBackground(bool v) {
    if (v != m_show_background) {
        m_show_background = v;
        showBackgroundChanged();
    }
}

auto PageContext::leftMargin() const -> qint32 { return X(leftMargin, m_left_margin, 0); }
void PageContext::setLeftMargin(qint32 v) {
    if (v != m_left_margin) {
        m_left_margin = v;
        leftMarginChanged();
    }
}

auto PageContext::rightMargin() const -> qint32 { return X(rightMargin, m_right_margin, 0); }
void PageContext::setRightMargin(qint32 v) {
    if (v != m_right_margin) {
        m_right_margin = v;
        rightMarginChanged();
    }
}

auto PageContext::topMargin() const -> qint32 { return X(topMargin, m_top_margin, 0); }
void PageContext::setTopMargin(qint32 v) {
    if (v != m_top_margin) {
        m_top_margin = v;
        topMarginChanged();
    }
}

auto PageContext::bottomMargin() const -> qint32 { return X(bottomMargin, m_bottom_margin, 0); }
void PageContext::setBottomMargin(qint32 v) {
    if (v != m_bottom_margin) {
        m_bottom_margin = v;
        bottomMarginChanged();
    }
}
#undef X

#define X(Func, Signal, Mem)          \
    void PageContext::reset##Func() { \
        if (Mem.has_value()) {        \
            Mem.reset();              \
            Signal##Changed();        \
        }                             \
    }
X(LeadingAction, leadingAction, m_leading_action)
X(HeaderType, headerType, m_header_type)
X(BackgroundRadius, backgroundRadius, m_background_radius)
X(HeaderBackgroundOpacity, headerBackgroundOpacity, m_header_background_opacity)
X(Radius, radius, m_radius)
X(ShowHeader, showHeader, m_show_header)
X(ShowBackground, showBackground, m_show_background)
X(LeftMargin, leftMargin, m_left_margin)
X(RightMargin, rightMargin, m_right_margin)
X(TopMargin, topMargin, m_top_margin)
X(BottomMargin, bottomMargin, m_bottom_margin)
#undef X

void PageContext::disconnectInherit() {
    for (const auto& connection : m_inherit_connections) {
        QObject::disconnect(connection);
    }
    m_inherit_connections.clear();
}

void PageContext::notifyInheritedProperties() {
#define X(Func, Mem)         \
    if (! Mem.has_value()) { \
        Func##Changed();     \
    }
    X(leadingAction, m_leading_action)
    X(headerType, m_header_type)
    X(backgroundRadius, m_background_radius)
    X(headerBackgroundOpacity, m_header_background_opacity)
    X(radius, m_radius)
    X(showHeader, m_show_header)
    X(showBackground, m_show_background)
    X(leftMargin, m_left_margin)
    X(rightMargin, m_right_margin)
    X(topMargin, m_top_margin)
    X(bottomMargin, m_bottom_margin)
#undef X
}
} // namespace qml_material

#include <qml_material/control/moc_page_context.cpp>
