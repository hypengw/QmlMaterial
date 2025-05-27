pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.ItemDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    property int leftMargin: 0
    property int rightMargin: 0

    topInset: 0
    bottomInset: 0
    leftInset: leftMargin
    rightInset: rightMargin

    verticalPadding: 8
    leftPadding: 16 + leftMargin
    rightPadding: (trailing ? 16 : 24) + rightMargin
    spacing: 0
    clip: false

    icon.width: 24
    icon.height: 24

    property int count: (ListView.view?.count ?? GridView.view?.count) ?? 0
    property int dgIndex: index ?? 0

    property MD.StateListItem mdState: MD.StateListItem {
        item: control
    }

    property string supportText
    property int maximumLineCount: 1
    property alias leader: m_holder_leader.sourceComponent
    property alias trailing: m_holder_trailing.sourceComponent
    property alias below: m_holder_below.sourceComponent
    property alias divider: m_holder_divider.item
    property int radius: 0
    property MD.t_corner corners: MD.Util.corner(radius)

    property int heightMode: {
        if (supportText)
            return MD.Enum.ListItemTwoLine;
        else
            return MD.Enum.ListItemOneLine;
    }

    font.capitalization: Font.MixedCase

    function indexCorners(index, count, size) {
        return MD.Util.corner(index === 0 ? size : 0, index + 1 === count ? size : 0);
    }

    contentItem: ColumnLayout {
        opacity: control.mdState.contentOpacity
        RowLayout {
            spacing: 16

            MD.Loader {
                id: m_holder_leader
                visible: item instanceof MD.Icon ? (item as MD.Icon).name : item
                sourceComponent: MD.Icon {
                    name: control.action ? control.action.icon.name : control.icon.name
                    size: control.action ? control.action.icon.width : control.icon.width
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 0
                MD.Text {
                    Layout.fillWidth: true
                    text: control.text
                    font: control.font
                    typescale: MD.Token.typescale.body_large
                    maximumLineCount: control.maximumLineCount
                    verticalAlignment: Qt.AlignVCenter
                }

                MD.Text {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: control.supportText
                    color: control.mdState.supportTextColor
                    typescale: MD.Token.typescale.body_medium
                    verticalAlignment: Qt.AlignVCenter
                }
            }
            MD.Text {
                id: item_text_trailing_support
                Layout.alignment: Qt.AlignVCenter
                visible: text.length > 0
                typescale: MD.Token.typescale.label_small
                verticalAlignment: Qt.AlignVCenter
            }

            MD.Loader {
                id: m_holder_trailing
                Layout.alignment: Qt.AlignVCenter
                visible: item
            }

            MD.Icon {
                id: item_text_trailing_icon
                Layout.alignment: Qt.AlignVCenter
                visible: name.length
                size: 24
            }
        }

        RowLayout {
            spacing: 16
            visible: m_holder_below.item
            Item {
                implicitWidth: m_holder_leader.height
                visible: m_holder_leader.visible
            }

            MD.Loader {
                id: m_holder_below
                Layout.fillWidth: true
            }
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 64
        implicitHeight: {
            switch (control.heightMode) {
            case MD.Enum.ListItemThreeLine:
                return 96;
            case MD.Enum.ListItemTwoLine:
                return 72;
            case MD.Enum.ListItemOneLine:
            default:
                return 56;
            }
        }

        opacity: control.mdState.backgroundOpacity

        corners: control.corners
        color: control.mdState.backgroundColor

        elevationVisible: elevation && color.a > 0
        elevation: control.mdState.elevation

        MD.Ripple2 {
            corners: control.corners
            width: parent.width
            height: parent.height
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }

        MD.ItemHolder {
            id: m_holder_divider
            visible: control.dgIndex + 1 !== control.count
        }
    }

    MD.StateHolder {
        state: control.mdState
    }
}
