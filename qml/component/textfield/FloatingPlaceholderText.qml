import QtQuick

Text {
    id: root
    property bool controlFocus: false
    property bool controlHasText: false
    property int verticalPadding: 8
    property int controlHeight: height
    property color cutoutColor: "transparent"
    property font sourceFont

    property bool filled: false
    property real targetScale: 0.8
    readonly property bool floated: controlFocus || controlHasText
    readonly property int restPixelSize: sourceFont.pixelSize
    readonly property int floatedPixelSize: Math.round(restPixelSize * targetScale)
    readonly property int largestHeight: restMetrics.height
    readonly property real restImplicitWidth: restMetrics.width

    font.family: sourceFont.family
    font.styleName: sourceFont.styleName
    font.weight: sourceFont.weight
    font.letterSpacing: sourceFont.letterSpacing
    font.capitalization: sourceFont.capitalization
    font.hintingPreference: sourceFont.hintingPreference
    font.pixelSize: floated ? floatedPixelSize : restPixelSize
    y: Math.round((controlHeight - height) / 2.0)
    verticalAlignment: Text.AlignVCenter

    TextMetrics {
        id: restMetrics
        text: root.text
        font.family: root.sourceFont.family
        font.weight: root.sourceFont.weight
        font.letterSpacing: root.sourceFont.letterSpacing
        font.capitalization: root.sourceFont.capitalization
        font.pixelSize: root.restPixelSize
    }

    Rectangle {
        z: -1
        visible: root.floated && root.cutoutColor.a > 0
        x: -6
        anchors.verticalCenter: parent.verticalCenter
        width: parent.implicitWidth + 12
        height: Math.max(2, parent.height)
        color: root.cutoutColor
    }

    states: [
        State {
            name: 'float'
            when: root.controlFocus || root.controlHasText
            PropertyChanges {
                root.y: root.filled ? root.verticalPadding : -Math.round(root.largestHeight
                                                                         * root.targetScale / 2.0)
            }
        }
    ]

    transitions: [
        Transition {
            to: ''
            YAnimator {
                duration: 300
                easing.type: Easing.OutSine
            }
        },
        Transition {
            to: 'float'
            YAnimator {
                duration: 300
                easing.type: Easing.OutSine
            }
        }
    ]
}
