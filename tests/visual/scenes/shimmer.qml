import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 640
    height: 480
    color: "#fafafa"
    MD.Shimmer {
        id: shared
        boundsMode: MD.Shimmer.Item
        boundsItem: root
        running: false
        angle: 15
        bandWidth: 160
        Component.onCompleted: seek(0.5)
    }
    Repeater {
        model: 4
        Item {
            required property int index
            x: 24 + (index % 2) * 312
            y: 24 + Math.floor(index / 2) * 120
            width: 280
            height: 96
            MD.Skeleton {
                width: 64
                height: 64
                radius: 32
                shimmer: shared
                color: "#536b65"
            }
            MD.Skeleton {
                x: 80
                y: 6
                width: 188
                height: 20
                shimmer: shared
                color: "#536b65"
            }
            MD.Skeleton {
                x: 80
                y: 40
                width: 128
                height: 16
                shimmer: shared
                color: "#536b65"
            }
        }
    }
    MD.Shape {
        x: 272
        y: 292
        width: 96
        height: 96
        layer.enabled: true
        layer.effect: MD.ShimmerEffect {
            shimmer: shared
        }
        MD.MaterialShapePath {
            shape: MD.MaterialShape.Cookie6Sided
            size: Qt.size(96, 96)
            fillColor: "#95495d"
            strokeWidth: 0
        }
    }
}
