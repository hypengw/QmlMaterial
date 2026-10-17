import QtQuick
import "../../../example/compositions" as Example

Rectangle {
    width: 900
    height: 700
    color: "white"

    Example.Motion {
        anchors.fill: parent
        animationsEnabled: false
        Component.onCompleted: {
            atEnd = true;
            synchronize();
            progress = 1.1;
        }
    }
}
