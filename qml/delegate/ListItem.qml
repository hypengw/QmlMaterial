pragma ComponentBehavior: Bound
import QtQuick
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
    property int elide: Text.ElideNone
    property int wrapMode: Text.NoWrap
    property int maximumLineCount: 1

    property Component leader: MD.Icon {
        name: control.action ? control.action.icon.name : control.icon.name
        size: control.action ? control.action.icon.width : control.icon.width
    }

    property Component trailing: null
    property Component below: null
    property Component divider: null

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

            Row {
                spacing: 16
                width: parent.width
                Loader {
                    id: m_leader
                    anchors.verticalCenter: parent.verticalCenter
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

                    sourceComponent: control.leader
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - m_leader.width * m_leader.visible - m_trailing_row.width * m_trailing_row.visible - (parent.spacing * Math.max(parent.visibleChildren.length - 1, 0))
                    spacing: 0
                    MD.Text {
                        width: parent.width
                        text: control.text
                        font: control.font
                        elide: control.elide
                        wrapMode: control.wrapMode
                        typescale: MD.Token.typescale.body_large
                        maximumLineCount: control.maximumLineCount
                        verticalAlignment: Qt.AlignVCenter
                    }

                    MD.Text {
                        width: parent.width
                        visible: text.length > 0
                        text: control.supportText
                        color: control.mdState.supportTextColor
                        typescale: MD.Token.typescale.body_medium
                        verticalAlignment: Qt.AlignVCenter
                    }
                }

                Row {
                    id: m_trailing_row
                    spacing: parent.spacing

                    anchors.verticalCenter: parent.verticalCenter
                    MD.Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: text.length > 0
                        typescale: MD.Token.typescale.label_small
                    }

                    Loader {
                        id: m_trailing
                        anchors.verticalCenter: parent.verticalCenter
                        sourceComponent: control.trailing
                    }

                    MD.Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: name.length
                        size: 24
                    }
                }
            }

            Loader {
                id: m_proxy_below
                sourceComponent: control.below
                width: parent.width - x
                x: m_leader.visible ? m_trailing.width : 0
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

        MD.Loader {
            active: parent.parent
            sourceComponent: MD.Ripple {
                corners: control.corners
                width: control.background.width
                height: control.background.height
                pressX: control.pressX
                pressY: control.pressY
                pressed: control.pressed
                stateOpacity: control.mdState.stateLayerOpacity
                color: control.mdState.stateLayerColor
            }
        }

        MD.Loader {
            active: control.showDivider
            sourceComponent: control.divider
        }
    }
}
