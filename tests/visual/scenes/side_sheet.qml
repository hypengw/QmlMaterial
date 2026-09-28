import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 800
    height: 560
    color: MD.Token.color.surface
    Column {
        x: 16
        y: 16
        spacing: 16
        Repeater {
            model: 3
            MD.SideSheet {
                id: sheet
                required property int index
                width: 768
                height: 160
                expanded: true
                coplanar: true
                detached: index > 0
                edge: index === 1 ? MD.SideSheetBase.Left : MD.SideSheetBase.Right
                animationsEnabled: false
                color: MD.Token.color.surface_container_low
                elevation: MD.Token.elevation.level1
                mainContent: Rectangle {
                    color: MD.Token.color.secondary_container
                    MD.Text {
                        anchors.centerIn: parent
                        text: 'Main content'
                    }
                }
                MD.Text {
                    anchors.fill: parent
                    padding: 16
                    text: sheet.index === 0 ? 'Docked' : 'Detached'
                }
                Component.onCompleted: {
                    if (index === 2)
                        position = 0.5;
                }
            }
        }
    }
}
