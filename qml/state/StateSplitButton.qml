import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.StateButton {
    id: root
    
    readonly property real innerCorner: 4
    // Override default corners for the main button part of the SplitButton
    corners: MD.Util.corners(root.corner, innerCorner, root.corner, innerCorner)
    
    // Provided for reference if needed, though indicator uses its own state
    property MD.corners trailingCorners: MD.Util.corners(innerCorner, root.corner, innerCorner, root.corner)
}
