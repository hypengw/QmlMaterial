pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 640
    height: 700
    color: MD.Token.color.surface

    MD.ColorPickerButton {
        id: picker
        x: 32
        y: 24
        color: "#6750A4"
        onDialogChanged: {
            if (dialog) {
                dialog.enter = null;
                dialog.exit = null;
            }
        }
    }
    function showDialog() {
        if (Window.window && !picker.dialog)
            picker.click();
    }
    Connections {
        target: root.Window.window
        function onWidthChanged() {
            root.showDialog();
        }
        function onHeightChanged() {
            root.showDialog();
        }
    }
    Component.onCompleted: Qt.callLater(showDialog)
}
