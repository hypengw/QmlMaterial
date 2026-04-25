pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

// Merged showcase of shape (rounded corners) + elevation (shadow). Both
// effects are SDF-driven and share a corners model, so it's natural to
// demonstrate them together. A single radius slider drives the dynamic
// boxes; the token-shape-scale row is informational and uses fixed M3
// tokens, not the slider.
MD.Page {
    id: root
    title: 'Effect'
    padding: 0

    // Responsive: key off this page's own width rather than the global
    // window class so the layout reflows correctly regardless of where the
    // page is mounted (full window, drawer-shrunken content area, etc).
    // ~720px is roughly two 320px panes plus margins/spacing.
    readonly property int gridColumns: (width < 720 || MD.MProp.size.isCompact) ? 1 : 2

    MD.VerticalFlickable {
        anchors.fill: parent
        topMargin: 16
        bottomMargin: 16

        ColumnLayout {
            width: parent.width
            spacing: 16

            // ---- Shared radius slider (drives the dynamic panes below) ----
            MD.Pane {
                id: m_slider_pane
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(parent.width - 32, 800)

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: 'Corner radius — ' + Math.round(m_slider_radius.value) + 'px'
                        typescale: MD.Token.typescale.title_medium
                    }

                    MD.Slider {
                        id: m_slider_radius
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        value: 20
                        from: 0
                        to: 80
                    }
                }
            }

            // ---- Responsive 2-up / 1-up grid of demo panes ----
            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(parent.width - 32, 1200)
                Layout.fillWidth: true
                columns: root.gridColumns
                columnSpacing: 16
                rowSpacing: 16

                // ------ Pane 1: M3 shape scale tokens ------
                MD.Pane {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 320

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 16

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Shape scale'
                            typescale: MD.Token.typescale.title_large
                        }

                        Flow {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            leftPadding: 16
                            spacing: 12

                            Repeater {
                                model: [
                                    { name: 'none',        r: MD.Token.shape.corner.none },
                                    { name: 'xs',          r: MD.Token.shape.corner.extra_small },
                                    { name: 'small',       r: MD.Token.shape.corner.small },
                                    { name: 'medium',      r: MD.Token.shape.corner.medium },
                                    { name: 'large',       r: MD.Token.shape.corner.large },
                                    { name: 'xl',          r: MD.Token.shape.corner.extra_large },
                                    { name: 'full',        r: -1 }
                                ]

                                ColumnLayout {
                                    id: scale_cell
                                    required property var modelData
                                    spacing: 4
                                    MD.Rectangle {
                                        Layout.alignment: Qt.AlignHCenter
                                        color: MD.MProp.color.primary
                                        implicitWidth: 80
                                        implicitHeight: 56
                                        radius: scale_cell.modelData.r < 0
                                                ? height / 2
                                                : scale_cell.modelData.r
                                    }
                                    MD.Text {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: scale_cell.modelData.name
                                        typescale: MD.Token.typescale.label_small
                                    }
                                }
                            }
                        }
                    }
                }

                // ------ Pane 2: Asymmetric corner combinations ------
                MD.Pane {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 320

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 16

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Asymmetric'
                            typescale: MD.Token.typescale.title_large
                        }

                        Flow {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            leftPadding: 16
                            spacing: 16

                            Repeater {
                                model: [
                                    { name: 'top',      c: MD.Util.corners(m_slider_radius.value, m_slider_radius.value, 0, 0) },
                                    { name: 'left',     c: MD.Util.corners(m_slider_radius.value, 0, m_slider_radius.value, 0) },
                                    { name: 'right',    c: MD.Util.corners(0, m_slider_radius.value, 0, m_slider_radius.value) },
                                    { name: 'diagonal', c: MD.Util.corners(m_slider_radius.value, 0, 0, m_slider_radius.value) }
                                ]

                                ColumnLayout {
                                    id: asym_cell
                                    required property var modelData
                                    spacing: 4
                                    MD.Rectangle {
                                        Layout.alignment: Qt.AlignHCenter
                                        color: MD.MProp.color.primary
                                        implicitWidth: 110
                                        implicitHeight: 64
                                        corners: asym_cell.modelData.c
                                    }
                                    MD.Text {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: asym_cell.modelData.name
                                        typescale: MD.Token.typescale.label_small
                                    }
                                }
                            }
                        }
                    }
                }

                // ------ Pane 3: Elevation levels L0..L5 ------
                MD.Pane {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 320

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 16

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Elevation'
                            typescale: MD.Token.typescale.title_large
                        }

                        Flow {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            leftPadding: 16
                            spacing: 24

                            Repeater {
                                model: 6
                                ElevationBox {
                                    required property int index
                                    elevation: MD.Token.elevation['level' + index]
                                    text: 'L' + index
                                }
                            }
                        }
                    }
                }

                // ------ Pane 4: Shape + elevation combinations ------
                MD.Pane {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 320

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 16

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Shape + Elevation'
                            typescale: MD.Token.typescale.title_large
                        }

                        Flow {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            leftPadding: 16
                            spacing: 24

                            ShapeBox {
                                title: 'Seg First'
                                elevation: 1
                                corners: MD.Util.corners(m_slider_radius.value, 0, m_slider_radius.value, 0)
                            }
                            ShapeBox {
                                title: 'Seg Middle'
                                elevation: 1
                                corners: MD.Util.corners(0)
                            }
                            ShapeBox {
                                title: 'Seg Last'
                                elevation: 1
                                corners: MD.Util.corners(0, m_slider_radius.value, 0, m_slider_radius.value)
                            }
                            ShapeBox {
                                title: 'Mixed'
                                elevation: 3
                                corners: MD.Util.corners(m_slider_radius.value, 0, 0, m_slider_radius.value)
                            }
                        }
                    }
                }

                // ------ Pane 5: Ripple2 (QShape + RadialGradient) ------
                MD.Pane {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 320

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 16

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Ripple — classic'
                            typescale: MD.Token.typescale.title_large
                        }

                        Flow {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            leftPadding: 16
                            spacing: 16

                            RippleCell {
                                label: 'Rounded'
                                useSkia: false
                                cellCorners: MD.Util.corners(m_slider_radius.value)
                            }
                            RippleCell {
                                label: 'Pill'
                                useSkia: false
                                cellCorners: MD.Util.corners(height / 2)
                            }
                            RippleCell {
                                label: 'Top'
                                useSkia: false
                                cellCorners: MD.Util.corners(m_slider_radius.value,
                                                             m_slider_radius.value, 0, 0)
                            }
                        }
                    }
                }

                // ------ Pane 6: RippleSkia (Material You "realistic" ripple) ------
                MD.Pane {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 320

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 16

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Ripple — realistic'
                            typescale: MD.Token.typescale.title_large
                        }

                        Flow {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            leftPadding: 16
                            spacing: 16

                            RippleCell {
                                label: 'Rounded'
                                useSkia: true
                                cellCorners: MD.Util.corners(m_slider_radius.value)
                            }
                            RippleCell {
                                label: 'Pill'
                                useSkia: true
                                cellCorners: MD.Util.corners(height / 2)
                            }
                            RippleCell {
                                label: 'Top'
                                useSkia: true
                                cellCorners: MD.Util.corners(m_slider_radius.value,
                                                             m_slider_radius.value, 0, 0)
                            }
                        }
                    }
                }
            }
        }
    }

    component ElevationBox: MD.Rectangle {
        id: eb_root
        property alias elevation: m_elev.elevation
        property string text: ''

        implicitWidth: 96
        implicitHeight: 96
        radius: m_slider_radius.value
        color: MD.MProp.color.surface_container

        MD.Elevation {
            id: m_elev
            anchors.fill: parent
            corners: eb_root.corners
            z: -1
        }

        MD.Text {
            anchors.centerIn: parent
            text: eb_root.text
        }
    }

    // Interactive cell — click anywhere inside to play the ripple. The
    // useSkia flag picks between RippleShape (Shape/RadialGradient, "classic")
    // and RippleSkia (sparkle wave shader, "realistic Material You").
    // Note: production code goes through MD.Ripple, which is itself just an
    // alias to whichever of the two is currently active.
    component RippleCell: Item {
        id: rc_root
        property string label: 'Tap'
        property var cellCorners: MD.Util.corners(20)
        property bool useSkia: false

        implicitWidth: 130
        implicitHeight: 80

        MD.Rectangle {
            id: rc_bg
            anchors.fill: parent
            color: MD.MProp.color.surface_container
            corners: rc_root.cellCorners
        }

        Loader {
            anchors.fill: parent
            sourceComponent: rc_root.useSkia ? rc_skia_cmp : rc_classic_cmp
        }

        Component {
            id: rc_classic_cmp
            MD.RippleShape {
                color: MD.MProp.color.primary
                corners: rc_root.cellCorners
                stateOpacity: rc_ma.containsMouse ? 0.08 : 0
                pressed: rc_ma.pressed
                pressX: rc_ma.mouseX
                pressY: rc_ma.mouseY
            }
        }
        Component {
            id: rc_skia_cmp
            MD.RippleSkia {
                color: MD.MProp.color.primary
                corners: rc_root.cellCorners
                stateOpacity: rc_ma.containsMouse ? 0.08 : 0
                pressed: rc_ma.pressed
                pressX: rc_ma.mouseX
                pressY: rc_ma.mouseY
            }
        }

        MD.Text {
            anchors.centerIn: parent
            text: rc_root.label
            typescale: MD.Token.typescale.label_medium
        }

        MouseArea {
            id: rc_ma
            anchors.fill: parent
            hoverEnabled: true
        }
    }

    component ShapeBox: ColumnLayout {
        id: sb_root
        property string title: ''
        property alias elevation: m_elev.elevation
        property alias corners: m_elev.corners
        spacing: 8

        MD.Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 120
            implicitHeight: 60
            color: MD.MProp.color.surface_container_high
            corners: m_elev.corners

            MD.Elevation {
                id: m_elev
                anchors.fill: parent
                z: -1
            }
        }

        MD.Text {
            Layout.alignment: Qt.AlignHCenter
            text: sb_root.title
            typescale: MD.Token.typescale.label_medium
        }
    }
}
