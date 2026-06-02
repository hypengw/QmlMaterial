import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)
    property MD.StateRailItem mdState: MD.StateRailItem {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: false
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 0
    spacing: 0

    icon.width: 24
    icon.height: 24

    property bool expand: false
    property Item trailing: null

    // -- metrics --
    // expanded indicator inset from rail edge
    readonly property real _indicatorMargin: 16
    // M3 expressive wide rail collapsed container = 96dp
    readonly property real _collapsedWidth: 96
    readonly property real _collapsedIndicatorW: 56
    readonly property real _collapsedIndicatorH: 32
    readonly property real _expandedIndicatorH: 56
    // leading space between the active indicator edge and the icon
    readonly property real _expandedLeadingPad: 16
    readonly property real _expandedTrailingPad: 24
    readonly property real _iconLabelSpacing: 12

    // expanded indicator wraps content
    readonly property real _expandedIndicatorW: _expandedLeadingPad + control.icon.width + _iconLabelSpacing + m_label.implicitWidth + _expandedTrailingPad
    readonly property real _expandedWidth: _expandedIndicatorW + _indicatorMargin * 2

    contentItem: Item {
        id: m_content
        implicitWidth: control.expand ? Math.max(control._expandedIndicatorW, 220) : control._collapsedWidth
        implicitHeight: control.expand ? control._expandedIndicatorH : (control._collapsedIndicatorH + 4 + m_label.implicitHeight)

        Behavior on implicitWidth {
            NumberAnimation {
                duration: MD.Token.duration.long2
                easing: MD.Token.easing.emphasized
            }
        }
        Behavior on implicitHeight {
            NumberAnimation {
                duration: MD.Token.duration.long2
                easing: MD.Token.easing.emphasized
            }
        }

        // -- icon --
        MD.Icon {
            id: m_icon
            name: control.icon.name
            size: control.icon.width
            color: control.mdState.supportTextColor
            fill: control.checked

            // positioned by states
        }

        // -- label --
        MD.Text {
            id: m_label
            text: control.text
            font.capitalization: Font.Capitalize
            typescale: control.expand ? MD.Token.typescale.label_large : MD.Token.typescale.label_medium
            prominent: control.checked
            color: control.expand ? control.mdState.expandedLabelColor : control.mdState.collapsedLabelColor
            // positioned by states
        }

        states: [
            State {
                name: "collapsed"
                when: !control.expand
                PropertyChanges {
                    m_icon {
                        // center within the fixed collapsed width so it never jumps to the
                        // expanded center while the rail width is still animating
                        x: (control._collapsedWidth - control.icon.width) / 2
                        y: (control._collapsedIndicatorH - control.icon.height) / 2
                    }
                    m_label {
                        x: (control._collapsedWidth - m_label.implicitWidth) / 2
                        y: control._collapsedIndicatorH + 4
                    }
                }
            },
            State {
                name: "expanded"
                when: control.expand
                PropertyChanges {
                    m_icon {
                        x: control._expandedLeadingPad + control._indicatorMargin
                        y: (control._expandedIndicatorH - control.icon.height) / 2
                    }
                    m_label {
                        x: control._expandedLeadingPad + control._indicatorMargin + control.icon.width + control._iconLabelSpacing
                        y: (control._expandedIndicatorH - m_label.implicitHeight) / 2
                    }
                }
            }
        ]

        transitions: [
            Transition {
                from: "collapsed"
                to: "expanded"
                NumberAnimation {
                    targets: [m_icon, m_label]
                    properties: "x,y"
                    duration: MD.Token.duration.long2
                    easing: MD.Token.easing.emphasized
                }
            },
            Transition {
                from: "expanded"
                to: "collapsed"
                NumberAnimation {
                    targets: [m_icon, m_label]
                    properties: "x,y"
                    duration: MD.Token.duration.long2
                    easing: MD.Token.easing.emphasized
                }
            }
        ]
    }

    background: Item {
        implicitWidth: control.expand ? Math.max(control._expandedIndicatorW, 220) : control._collapsedWidth
        implicitHeight: control.expand ? control._expandedIndicatorH : (control._collapsedIndicatorH + 4 + m_label.implicitHeight)

        Behavior on implicitWidth {
            NumberAnimation {
                duration: MD.Token.duration.long2
                easing: MD.Token.easing.emphasized
            }
        }
        Behavior on implicitHeight {
            NumberAnimation {
                duration: MD.Token.duration.long2
                easing: MD.Token.easing.emphasized
            }
        }

        MD.ElevationRectangle {
            id: m_indicator
            x: control.expand ? control._indicatorMargin : (control._collapsedWidth - control._collapsedIndicatorW) / 2
            y: 0
            width: control.expand ? control.width - control._indicatorMargin * 2 : control._collapsedIndicatorW
            height: control.expand ? control._expandedIndicatorH : control._collapsedIndicatorH

            radius: height / 2
            color: control.mdState.backgroundColor

            elevationVisible: control.enabled && color.a > 0
            elevation: control.mdState.elevation

            Behavior on width {
                enabled: !control.expand
                NumberAnimation {
                    duration: MD.Token.duration.long2
                    easing: MD.Token.easing.emphasized
                }
            }
            Behavior on height {
                NumberAnimation {
                    duration: MD.Token.duration.long2
                    easing: MD.Token.easing.emphasized
                }
            }
            Behavior on x {
                NumberAnimation {
                    duration: MD.Token.duration.long2
                    easing: MD.Token.easing.emphasized
                }
            }

            MD.Ripple {
                anchors.fill: parent
                readonly property point p: control.mapToItem(this, control.pressX, control.pressY)
                radius: parent.radius
                pressX: p.x
                pressY: p.y
                pressed: control.pressed
                stateOpacity: control.mdState.stateLayerOpacity
                color: control.mdState.stateLayerColor
            }
        }
    }
}
