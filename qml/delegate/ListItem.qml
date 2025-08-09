pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.ItemDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

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

    required property int index
    required property var model

    property int leftMargin: 0
    property int rightMargin: 0

    property int count: (ListView.view?.count ?? GridView.view?.count) ?? 0

    readonly property bool prevSameSection: ListView.section == ListView.previousSection
    readonly property bool nextSameSection: ListView.section == ListView.nextSection
    property bool showDivider: nextSameSection && index + 1 !== count

    property MD.StateListItem mdState: MD.StateListItem {
        item: control
    }

    property string supportText
    property int maximumLineCount: 1
    property alias leader: m_holder_leader.item
    property alias trailing: m_holder_trailing.item
    property alias below: m_holder_below.item
    property alias divider: m_holder_divider.item
    property int radius: 0
    property MD.corners corners: MD.Util.corners(radius)

    property int heightMode: {
        if (supportText)
            return MD.Enum.ListItemTwoLine;
        else
            return MD.Enum.ListItemOneLine;
    }

    font.capitalization: Font.MixedCase

    contentItem: Item {
        implicitHeight: m_content.implicitHeight
        implicitWidth: m_content.implicitWidth
        opacity: control.mdState.contentOpacity
        Column {
            id: m_content
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 16

                MD.ItemHolder {
                    id: m_holder_leader
                    visible: {
                        let ok = false;
                        if (item instanceof MD.Icon) {
                            ok = (item as MD.Icon).name;
                        } else if (item instanceof MD.Loader) {
                            ok = (item as MD.Loader).item?.name;
                        } else {
                            ok = item;
                        }
                        return ok;
                    }
                    item: MD.Loader {
                        active: control.leader == this
                        sourceComponent: MD.Icon {
                            name: control.action ? control.action.icon.name : control.icon.name
                            size: control.action ? control.action.icon.width : control.icon.width
                        }
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

                MD.ItemHolder {
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
            MD.ItemHolder {
                id: m_holder_below
                x: m_holder_leader.visible ? m_holder_leader.width : 0
                width: parent.width - x
                visible: item
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
            id: m_ripple
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
            anchors.bottom: parent.bottom
            width: parent.width
            height: implicitHeight
            visible: control.showDivider
        }
    }
}
