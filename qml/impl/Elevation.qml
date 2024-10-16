import QtQuick
import Qcm.Material as MD

MD.ElevationImpl {
    id: root
    color: MD.MatProp.color.shadow
    visible: elevation != MD.Token.elevation.level0
}
