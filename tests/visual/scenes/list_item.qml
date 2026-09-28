import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 400
    height: 280
    color: MD.Token.color.surface
    MD.MProp.backgroundColor: color
    MD.MProp.textColor: MD.Token.color.on_surface

    Column {
        x: 16
        y: 16
        width: parent.width - 32

        MD.ListItem {
            width: parent.width
            text: "Notifications"
            icon.name: "notifications"
            trailing: MD.Icon {
                name: "chevron_right"
            }
        }
        MD.ListItem {
            width: parent.width
            text: "Downloads"
            supportText: "Files available offline"
            icon.name: "download"
        }
        MD.ListItem {
            width: parent.width
            text: "Storage"
            supportText: "Local files and cached content"
            icon.name: "folder"
            heightMode: MD.Enum.ListItemThreeLine
            enabled: false
        }
    }
}
