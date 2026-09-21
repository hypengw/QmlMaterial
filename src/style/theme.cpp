#include "qml_material/style/theme.hpp"
#include "qml_material/core/enum.hpp"
#include "qml_material/token/token.hpp"

#include <type_traits>

using namespace qml_material;

namespace
{
struct GlobalTheme {
    ~GlobalTheme() {}
    QColor      textColor;
    QColor      supportTextColor;
    QColor      backgroundColor;
    QColor      stateLayerColor;
    int         elevation { 0 };
    MdColorMgr  color_;
    MdColorMgr* color { &color_ };

    ThemeSize  size_;
    ThemeSize* size { &size_ };

    PageContext  page_;
    PageContext* page { &page_ };
};
// The default objects below are handed to QML and bindings may still read them
// while static destruction is running, which would turn every color into null.
// Create them once and keep them for the process lifetime.
GlobalTheme* theGlobalTheme() {
    static GlobalTheme* const the = new GlobalTheme();
    return the;
}

} // namespace

Theme::Theme(QObject* parent): AttachedPropertyNode(parent, &Theme::staticMetaObject) {
    initializeAttachedProperty();
}
Theme::~Theme() {}

Theme* Theme::qmlAttachedProperties(QObject* object) { return new Theme(object); }

#define IMPL_ATTACH_PROP(_type_, _name_, _prop_, ...)                                      \
    Theme::AttachProp<_type_>& Theme::get_##_name_() { return _prop_; }                    \
    _type_ Theme::_name_() const { return _prop_.value.value_or(theGlobalTheme()->_name_); } \
    void   Theme::set_##_name_(_type_ v) { setProp(_prop_, v); }                           \
    void   Theme::reset_##_name_() {                                                       \
        auto* attached = qobject_cast<Self*>(attachedParent());                            \
        resetProp(_prop_, attached ? attached->_name_() : theGlobalTheme()->_name_);         \
    }

IMPL_ATTACH_PROP(QColor, textColor, m_textColor)
IMPL_ATTACH_PROP(QColor, backgroundColor, m_backgroundColor)
IMPL_ATTACH_PROP(int, elevation, m_elevation)
IMPL_ATTACH_PROP(MdColorMgr*, color, m_color)
IMPL_ATTACH_PROP(ThemeSize*, size, m_size)
IMPL_ATTACH_PROP(PageContext*, page, m_page)

template<typename V>
void Theme::setProp(AttachProp<V>& property, const V& value) {
    property.explicited = true;
    if (property.value == value) return;

    property.value = value;
    if constexpr (std::is_pointer_v<V> && std::is_base_of_v<QObject, std::remove_pointer_t<V>>) {
        if (value) value->setParent(this);
    }
    propagateAttachedValues();
    std::invoke(property.sig_func, this);
}

template<typename V>
void Theme::resetProp(AttachProp<V>& property, const V& inheritedValue) {
    if (! property.explicited) return;

    property.explicited = false;
    if (! inheritProp(property, inheritedValue)) return;
    propagateAttachedValues();
    std::invoke(property.sig_func, this);
}

template<typename V>
bool Theme::inheritProp(AttachProp<V>& property, const V& value) {
    if (property.explicited || property.value == value) return false;
    property.value = value;
    return true;
}

void Theme::updateInheritedValues() {
    auto* attached = qobject_cast<Theme*>(attachedParent());

    const auto textColorChanged =
        inheritProp(m_textColor, attached ? attached->textColor() : theGlobalTheme()->textColor);
    const auto backgroundColorChanged =
        inheritProp(m_backgroundColor,
                    attached ? attached->backgroundColor() : theGlobalTheme()->backgroundColor);
    const auto elevationChanged =
        inheritProp(m_elevation, attached ? attached->elevation() : theGlobalTheme()->elevation);
    const auto colorChanged =
        inheritProp(m_color, attached ? attached->color() : theGlobalTheme()->color);
    const auto sizeChanged =
        inheritProp(m_size, attached ? attached->size() : theGlobalTheme()->size);
    const auto pageChanged =
        inheritProp(m_page, attached ? attached->page() : theGlobalTheme()->page);

    if (! textColorChanged && ! backgroundColorChanged && ! elevationChanged && ! colorChanged &&
        ! sizeChanged && ! pageChanged)
        return;

    propagateAttachedValues();
    if (textColorChanged) std::invoke(m_textColor.sig_func, this);
    if (backgroundColorChanged) std::invoke(m_backgroundColor.sig_func, this);
    if (elevationChanged) std::invoke(m_elevation.sig_func, this);
    if (colorChanged) std::invoke(m_color.sig_func, this);
    if (sizeChanged) std::invoke(m_size.sig_func, this);
    if (pageChanged) std::invoke(m_page.sig_func, this);
}

ThemeSize::ThemeSize(QObject* parent)
    : QObject(parent),
      m_window_class((qint32)Enum::WindowClassType::WindowClassMedium),
      m_width(0),
      m_duration(200) {
    m_width_timer.setSingleShot(true);
    connect(this, &ThemeSize::widthChanged, this, [this]() {
        if (m_duration > 0) {
            if (! m_width_timer.isActive()) m_width_timer.start(m_duration);
        } else {
            static auto the_wclass = token::WindowClass {};
            setWindowClass((qint32)the_wclass.select_type(m_width));
        }
    });
    connect(&m_width_timer, &QTimer::timeout, this, [this]() {
        static auto the_wclass = token::WindowClass {};
        setWindowClass((qint32)the_wclass.select_type(m_width));
    });
}
ThemeSize::~ThemeSize() {}

auto ThemeSize::isCompact() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassCompact;
}
auto ThemeSize::isMedium() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassMedium;
}
auto ThemeSize::isExpanded() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassExpanded;
}
auto ThemeSize::isLarge() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassLarge;
}
auto ThemeSize::isExtraLarge() const -> bool {
    return m_window_class == (qint32)Enum::WindowClassType::WindowClassExtraLarge;
}

auto ThemeSize::width() const -> qint32 { return m_width; }

void ThemeSize::setWidth(qint32 v) {
    if (m_width != v) {
        m_width = v;
        widthChanged(m_width);
    }
}
auto ThemeSize::duration() const -> qint32 { return m_duration; }

void ThemeSize::setDuration(qint32 v) {
    if (m_duration != v) {
        m_duration = v;
        durationChanged(m_width);
    }
}

auto ThemeSize::windowClass() const -> qint32 { return m_window_class; }
void ThemeSize::setWindowClass(qint32 v) {
    if (v != m_window_class) {
        m_window_class = v;
        windowClassChanged(m_window_class);

        verticalPaddingChanged();
    }
}
auto ThemeSize::verticalPadding() const -> qint32 {
    using WT = Enum::WindowClassType;
    switch ((WT)windowClass()) {
    case WT::WindowClassCompact: return 8;
    default: return 16;
    }
}

#include <qml_material/style/moc_theme.cpp>
