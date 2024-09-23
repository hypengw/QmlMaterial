import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.ItemDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    verticalPadding: 8
    leftPadding: 16
    rightPadding: trailing ? 16 : 24
    spacing: 0

    icon.width: 24
    icon.height: 24

    property int count: (ListView.view?.count ?? GridView.view?.count) ?? 0
    property int dgIndex: index ?? 0

    property alias mdState: m_sh.state

    property string supportText
    property int maximumLineCount: 1
    property alias leader: item_holder_leader.contentItem
    property alias trailing: item_holder_trailing.contentItem
    property alias below: item_holder_below.contentItem
    property alias divider: holder_divider.item
    property var radius: 0

    property int heightMode: {
        if (supportText)
            return MD.Enum.ListItemTwoLine;
        else
            return MD.Enum.ListItemOneLine;
    }

    font.capitalization: Font.MixedCase

    function indexRadius(index, count, size) {
        return [index === 0 ? size : 0, index + 1 === count ? size : 0];
    }

    contentItem: ColumnLayout {
        id: m_test
        opacity: control.mdState.contentOpacity
        RowLayout {
            id: m_test2
            spacing: 16

            MD.Control {
                id: item_holder_leader
                visible: contentItem
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

            MD.Control {
                id: item_holder_trailing
                Layout.alignment: Qt.AlignVCenter
                visible: contentItem
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
            visible: item_holder_below.contentItem
            Item {
                implicitWidth: item_holder_leader.height
                visible: item_holder_leader.visible
            }

            MD.Control {
                id: item_holder_below
                Layout.fillWidth: true
            }
        }
    }

    background: MD.Rectangle {
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

        radius: control.radius
        color: control.mdState.backgroundColor

        layer.enabled: control.enabled && color.a > 0
        layer.effect: MD.RoundedElevationEffect {
            elevation: control.mdState.elevation
        }

        MD.Ripple2 {
            radius: control.radius
            width: parent.width
            height: parent.height
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }

        MD.ItemHolder {
            id: holder_divider
            visible: control.dgIndex + 1 !== control.count
        }
    }

    MD.StateHolder {
        id: m_sh
        state: MD.StateListItem {
            item: control
        }
    }
}
