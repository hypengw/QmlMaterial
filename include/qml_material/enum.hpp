#pragma once
#include <QObject>
#include <QQmlEngine>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API Enum : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    using QObject::QObject;

    enum class ColorScheme
    {
        Light,
        Dark
    };
    Q_ENUM(ColorScheme)

    enum class IconLabelStyle
    {
        IconAndText = 0,
        IconOnly,
        TextOnly
    };
    Q_ENUM(IconLabelStyle)

    enum class ButtonType
    {
        BtElevated = 0,
        BtFilled,
        BtFilledTonal,
        BtOutlined,
        BtText
    };
    Q_ENUM(ButtonType)

    enum class IconButtonType
    {
        IBtFilled = 0,
        IBtFilledTonal,
        IBtOutlined,
        IBtStandard
    };
    Q_ENUM(IconButtonType)

    enum class FABType
    {
        FABSmall = 0,
        FABNormal,
        FABLarge
    };
    Q_ENUM(FABType)

    enum class FABColor
    {
        FABColorPrimary = 0,
        FABColorSurfaec,
        FABColorSecondary,
        FABColorTertiary
    };
    Q_ENUM(FABColor)

    enum class CardType
    {
        CardElevated = 0,
        CardFilled,
        CardOutlined
    };
    Q_ENUM(CardType)

    enum class TextFieldType
    {
        TextFieldFilled = 0,
        TextFieldOutlined
    };
    Q_ENUM(TextFieldType)

    enum class ListItemHeightMode
    {
        ListItemOneLine = 0,
        ListItemTwoLine,
        ListItemThreeLine
    };
    Q_ENUM(ListItemHeightMode)

    enum class TabType
    {
        PrimaryTab,
        SecondaryTab
    };
    Q_ENUM(TabType)

    enum class AppBarType
    {
        AppBarCenterAligned,
        AppBarSmall,
        AppBarMedium,
        AppBarLarge
    };
    Q_ENUM(AppBarType)

    enum class ToastFlag
    {
        TFCloseable = 1,
        TFSave      = 1 << 1,
    };
    Q_ENUM(ToastFlag)
    Q_DECLARE_FLAGS(ToastFlags, ToastFlag)

    enum class WindowClassType
    {
        WindowClassCompact,
        WindowClassMedium,
        WindowClassExpanded,
        WindowClassLarge,
        WindowClassExtraLarge,
    };
    Q_ENUM(WindowClassType)
};
Q_DECLARE_OPERATORS_FOR_FLAGS(Enum::ToastFlags);
} // namespace qml_material