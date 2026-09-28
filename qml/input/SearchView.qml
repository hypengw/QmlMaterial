pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.SearchViewBase {
    id: control
    property alias placeholderText: editor.placeholderText
    property real maximumDockedHeight: overlayHeight * 2 / 3
    readonly property real radius: presentation === MD.SearchView.FullScreen ? 28 * (1 - expansion) : 28

    parent: searchBar
    positioningItem: presentation === MD.SearchView.FullScreen ? overlayItem : null
    x: 0
    y: 0
    implicitWidth: searchBar ? searchBar.width : 360
    implicitHeight: Math.max(240, implicitContentHeight + 56)
    width: presentation === MD.SearchView.FullScreen ? overlayWidth : Math.min(implicitWidth, overlayWidth)
    height: presentation === MD.SearchView.FullScreen ? overlayHeight : Math.max(0, Math.min(implicitHeight, maximumDockedHeight, overlayHeight))
    padding: 0
    modal: true
    dim: presentation === MD.SearchView.FullScreen
    focus: true
    inputField: editor
    font.pixelSize: MD.Token.typescale.body_large.size
    font.weight: MD.Token.typescale.body_large.weight
    font.letterSpacing: MD.Token.typescale.body_large.tracking

    header: Item {
        height: Math.min(56, control.surfaceItem.height)
        clip: true
        MD.IconButton {
            id: backButton
            objectName: "searchBack"
            x: control.mirrored ? parent.width - width - 4 : 4
            anchors.verticalCenter: parent.verticalCenter
            icon.name: control.mirrored ? "arrow_forward" : "arrow_back"
            onClicked: control.close()
        }
        MD.TextFieldEmbed {
            id: editor
            objectName: "searchInput"
            x: control.mirrored ? clearButton.width + 8 : backButton.width + 8
            width: Math.max(0, parent.width - backButton.width - clearButton.width - 16)
            height: parent.height
            color: MD.Token.color.on_surface
            text: control.searchText
            EnterKey.type: Qt.EnterKeySearch
            onTextEdited: control.searchText = text
            onAccepted: control.submit()
        }
        MD.IconButton {
            id: clearButton
            objectName: "searchClear"
            x: control.mirrored ? 4 : parent.width - width - 4
            anchors.verticalCenter: parent.verticalCenter
            icon.name: "close"
            visible: control.searchText.length > 0
            onClicked: control.clear()
        }
        MD.Divider {
            anchors.bottom: parent.bottom
            width: parent.width
            opacity: control.expansion
        }
    }
    contentItem: Item {
        LayoutMirroring.enabled: control.mirrored
        LayoutMirroring.childrenInherit: true
        clip: true
        opacity: control.expansion
        layer.enabled: control.radius > 0
        layer.effect: MD.RoundClip {
            corners: MD.Util.corners(0, 0, control.radius, control.radius)
            size: Qt.vector2d(control.contentItem.width, control.contentItem.height)
        }
    }
    background: MD.ElevationRectangle {
        radius: control.radius
        color: MD.Token.color.surface_container_high
        elevation: control.presentation === MD.SearchView.FullScreen ? 0 : MD.Token.elevation.level3
    }
    enter: Transition {
        NumberAnimation {
            property: "expansion"
            duration: MD.Token.duration.medium3
            easing: MD.Token.easing.emphasized_decelerate
        }
    }
    exit: Transition {
        NumberAnimation {
            property: "expansion"
            duration: MD.Token.duration.short3
            easing: MD.Token.easing.emphasized_accelerate
        }
    }
    MD.Overlay.modal: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: MD.Token.duration.short3
                easing: MD.Token.easing.linear
            }
        }
    }
}
