pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.StateListItem {
    readonly property var control: item
    Component {
        id: m_default_supporting

        MD.Text {
            text: control.supportText
            color: control.mdState.supportTextColor
            typescale: MD.Token.typescale.body_medium
            verticalAlignment: Qt.AlignVCenter
        }
    }

    readonly property Item contentItem: Item {
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
                        if (item instanceof MD.IconView) {
                            ok = !(item as MD.IconView).icon.empty;
                        } else if (item instanceof MD.Icon) {
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

                    MD.Loader {
                        id: m_supporting

                        width: parent.width
                        active: control.hasSupporting
                        sourceComponent: control.supporting ?? m_default_supporting
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
                x: m_leader.visible ? m_leader.width + 16 : 0
                width: parent.width - x
            }
        }
    }
    readonly property Item background: MD.ElevationRectangle {
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
                interactionSource: control.interactionSource
                indication: control.mdState.indication
                color: control.mdState.stateLayerColor
            }
        }

        MD.Loader {
            active: control.showDivider
            sourceComponent: control.divider
        }
    }
}
