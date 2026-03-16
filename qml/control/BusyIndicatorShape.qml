import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

// Renders a filled polygon defined by `points` (a QPolygonF in normalised
// [-1, 1] space).  The item's transform is expected to be set by the parent
// (see BusyIndicator.qml) to map that space to pixel coordinates.
MD.Shape {
    id: root

    property color     color:  MD.MProp.color.primary
    property alias     points: polyline.path

    // Prefer the GPU rasteriser; fall back to software only when unavailable.
    preferredRendererType: Shape.CurveRenderer

    // The item lives in normalised space; its visual size is determined by the
    // Scale transform applied by the parent, not by width/height in pixels.
    width:  0
    height: 0

    ShapePath {
        id: shapePath
        fillColor:   root.color
        strokeColor: "transparent"
        strokeWidth: 0

        PathPolyline {
            id: polyline
        }
    }
}
