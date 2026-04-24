import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 400
    height: 300
    color: "#F5F5F5"

    MD.Button {
        anchors.centerIn: parent
        text: "Click Me"
    }
}
