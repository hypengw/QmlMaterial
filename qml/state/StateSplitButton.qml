import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.StateButton {
    id: root

    readonly property MD.SplitButtonSizeItem sizeToken: {
        switch (root.size) {
        case MD.Enum.XS:
            return MD.Token.split_button.xsmall;
        case MD.Enum.S:
            return MD.Token.split_button.small;
        case MD.Enum.M:
            return MD.Token.split_button.medium;
        case MD.Enum.L:
            return MD.Token.split_button.large;
        case MD.Enum.XL:
            return MD.Token.split_button.xlarge;
        default:
            return MD.Token.split_button.medium;
        }
    }

    readonly property real innerCorner: {
        if (root.item.down)
            return sizeToken.inner_corner_pressed_size;
        if (root.item.hovered)
            return sizeToken.inner_corner_hovered_size;
        return sizeToken.inner_corner_size;
    }

    readonly property real outerCorner: sizeToken.outer_corner_size

    // Override default corners for the main button part of the SplitButton
    corners: MD.Util.corners(outerCorner, innerCorner, outerCorner, innerCorner)
    property MD.corners trailingCorners: MD.Util.corners(innerCorner, outerCorner, innerCorner, outerCorner)

    readonly property real containerHeight: sizeToken.container_height
    readonly property real betweenSpace: sizeToken.between_space
    readonly property real leadingSpace: sizeToken.leading_button_leading_space
    readonly property real trailingSpace: sizeToken.leading_button_trailing_space
}
