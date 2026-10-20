pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import "compositions"

MD.Page {
    id: root
    title: 'Compositions'
    padding: 0

    contentItem: MD.VerticalFlickable {
        MD.NestedScroll.pressDelay: 0
        topMargin: 24
        bottomMargin: 24
        Column {
            width: Math.max(0, Math.min(900, parent.width - 32))
            x: (parent.width - width) / 2
            spacing: 20
            MD.Text {
                text: 'Quick settings'
                typescale: MD.Token.typescale.title_large
            }
            QuickControls {
                objectName: 'quickControls'
                width: parent.width
                columns: width >= 680 ? 3 : 2
                rows: columns === 3 ? 3 : 4
                onAlarmRequested: timePickers.openDialog()
            }
            MD.AutoDivider {}
            Item {
                width: parent.width
                height: 48
                MD.Text {
                    width: Math.max(0, parent.width - restore.width - 8)
                    height: parent.height
                    text: 'Notifications'
                    typescale: MD.Token.typescale.title_large
                }
                MD.IconButton {
                    id: restore
                    objectName: 'restoreNotifications'
                    anchors.right: parent.right
                    icon.name: MD.Token.icon.restart_alt
                    Accessible.name: 'Restore notifications'
                    MD.ToolTip.text: 'Restore notifications'
                    MD.ToolTip.visible: mdState.interactions.hovered
                    onClicked: notifications.feed.reset()
                }
            }
            NotificationList {
                id: notifications
                objectName: 'notifications'
                width: parent.width
            }
            MD.AutoDivider {}
            Flow {
                width: parent.width
                spacing: 16
                MD.Text {
                    width: parent.width < 440 ? parent.width : Math.max(0, parent.width - layoutChoice.width - parent.spacing)
                    height: layoutChoice.height
                    text: 'Media'
                    typescale: MD.Token.typescale.title_large
                }
                MD.SegmentedButtonGroup {
                    id: layoutChoice
                    objectName: 'mediaLayoutChoice'
                    size: MD.Enum.XS
                    MD.SegmentedButton {
                        objectName: 'mediaCompactChoice'
                        width: 128
                        text: 'Compact'
                        checked: !media.expanded
                        onCheckedChangeRequested: value => {
                            if (value)
                                media.expanded = false;
                        }
                    }
                    MD.SegmentedButton {
                        objectName: 'mediaExpandedChoice'
                        width: 128
                        text: 'Expanded'
                        checked: media.expanded
                        onCheckedChangeRequested: value => {
                            if (value)
                                media.expanded = true;
                        }
                    }
                }
            }
            MediaCard {
                id: media
                objectName: 'standaloneMedia'
                width: parent.width
            }
            MD.AutoDivider {}
            MD.Text {
                text: 'Motion'
                typescale: MD.Token.typescale.title_large
            }
            Motion {
                objectName: 'compositionMotion'
                x: (parent.width - width) / 2
                width: Math.min(800, parent.width)
            }
            MD.AutoDivider {}
            MD.Text {
                text: 'Clocks'
                typescale: MD.Token.typescale.title_large
            }
            Clocks {
                objectName: 'compositionClocks'
                alarmTime: timePickers.time
                x: (parent.width - width) / 2
                width: Math.min(520, parent.width)
            }
            MD.AutoDivider {}
            MD.Text {
                text: 'Time picker'
                typescale: MD.Token.typescale.title_large
            }
            TimePickers {
                id: timePickers
                objectName: 'compositionTimePickers'
                x: (parent.width - width) / 2
                width: Math.min(400, parent.width)
            }
            MD.AutoDivider {}
            MD.Text {
                text: 'Date picker'
                typescale: MD.Token.typescale.title_large
            }
            DatePickers {
                objectName: 'compositionDatePickers'
                x: (parent.width - width) / 2
                width: Math.min(400, parent.width)
            }
            MD.AutoDivider {}
            MD.Text {
                text: 'Color picker'
                typescale: MD.Token.typescale.title_large
            }
            ColorPickers {
                objectName: 'compositionColorPickers'
                x: (parent.width - width) / 2
                width: Math.min(400, parent.width)
            }
            MD.AutoDivider {}
            MD.Text {
                text: 'File pickers'
                typescale: MD.Token.typescale.title_large
            }
            FilePickers {
                objectName: 'compositionFilePickers'
                width: parent.width
            }
        }
    }
}
