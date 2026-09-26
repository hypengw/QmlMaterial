import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 640
    height: 240
    color: "#ffffff"

    MD.Action {
        id: shared
        icon.name: "favorite"
    }
    Row {
        x: 24
        y: 24
        spacing: 16
        MD.Button {
            action: shared
            text: "XS"
            mdState.size: MD.Enum.XS
        }
        MD.IconButton {
            action: shared
            mdState.size: MD.Enum.M
            icon.fill: true
        }
        MD.IconButton {
            action: shared
            icon.color: "#d02040"
        }
        MD.IconButton {
            icon.source: "../assets/icon-colors.svg"
            icon.width: 64
            icon.height: 32
            icon.color: "transparent"
        }
        MD.IconButton {
            icon.source: "../assets/icon-colors.svg"
            icon.width: 64
            icon.height: 32
            icon.color: "#00aa44"
        }
    }
    Row {
        x: 24
        y: 140
        spacing: 16
        MD.Button {
            icon.source: "../assets/icon-colors.svg"
            text: "Image"
        }
        MD.Button {
            icon.name: "favorite"
            icon.color: "transparent"
            text: "Transparent"
        }
        MD.Button {
            text: "No icon"
        }
        MD.Button {
            action: shared
            enabled: false
            text: "Disabled"
        }
    }
}
