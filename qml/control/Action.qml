import QtQuick
import QtQuick.Templates as T

T.Action {
    default property list<QtObject> data
    property bool visible: true
    property int displayHint: 0
    property int busy: 0
    property real progress: 0
    property bool closeMenu: true
    property bool separator: false
    property Component displayComponent
    icon.width: 24
    icon.height: 24
}
