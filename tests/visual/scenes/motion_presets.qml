import QtQuick
import "../../../example" as Example

Rectangle {
    width: 900
    height: 700
    color: "white"

    Example.Motion {
        anchors.fill: parent
        showHeader: false
        animationsEnabled: false
        Component.onCompleted: {
            atEnd = true;
            synchronize();
            progress = 1.1;
        }
    }
}
