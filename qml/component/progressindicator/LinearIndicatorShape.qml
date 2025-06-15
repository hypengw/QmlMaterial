pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import QtQml.Models
import Qcm.Material as MD

MD.Shape {
    id: root
    property int strokeWidth: 4

    property real x1: 0
    property real x2: 0

    property color c1: root.MD.MProp.color.primary

    property int count
    property list<MD.LinearActiveIndicatorData> indicators

    Instantiator {
        model: root.indicators
        delegate: ShapePath {
            required property MD.LinearActiveIndicatorData modelData
            capStyle: ShapePath.RoundCap
            fillColor: "transparent"
            strokeColor: modelData.color
            strokeWidth: root.strokeWidth
            startX: root.width * modelData.startFraction
            startY: 0
            PathLine {
                x: root.width * modelData.endFraction
                y: 0
            }
        }
        onObjectAdded: (idx, obj) => root.data.push(obj)
        onObjectRemoved: (idx, obj) => MD.Util.removeObj(root.data, obj)
    }
}
