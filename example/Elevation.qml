pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

import Qcm.Material as MD

MD.Page {
    id: root
    title: 'Elevation'
    padding: 0

    MD.VerticalFlickable {
        anchors.fill: parent
        topMargin: 16
        bottomMargin: 16

        ColumnLayout {
            width: parent.width
            spacing: 24

            MD.Pane {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(parent.width - 32, 800)

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 16
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: 'Elevation'
                        typescale: MD.Token.typescale.title_large
                    }

                    MD.Slider {
                        id: m_slider_radius
                        Layout.fillWidth: true
                        Layout.margins: 20
                        value: 20
                        from: 0
                        to: 80
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 24
                        Layout.margins: 20
                        
                        ElevationBox { elevation: MD.Token.elevation.level0; text: "L0" }
                        ElevationBox { elevation: MD.Token.elevation.level1; text: "L1" }
                        ElevationBox { elevation: MD.Token.elevation.level2; text: "L2" }
                        ElevationBox { elevation: MD.Token.elevation.level3; text: "L3" }
                        ElevationBox { elevation: MD.Token.elevation.level4; text: "L4" }
                        ElevationBox { elevation: MD.Token.elevation.level5; text: "L5" }
                    }
                }
            }

            MD.Pane {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(parent.width - 32, 800)

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 16
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: 'Custom Shapes'
                        typescale: MD.Token.typescale.title_large
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 32
                        Layout.margins: 20

                        // Segmented (First)
                        ShapeBox {
                            title: "Seg First"
                            elevation: 1
                            corners: MD.Util.corners(m_slider_radius.value, 0, m_slider_radius.value, 0)
                        }

                        // Segmented (Middle)
                        ShapeBox {
                            title: "Seg Middle"
                            elevation: 1
                            corners: MD.Util.corners(0)
                        }

                        // Segmented (Last)
                        ShapeBox {
                            title: "Seg Last"
                            elevation: 1
                            corners: MD.Util.corners(0, m_slider_radius.value, 0, m_slider_radius.value)
                        }

                        // Mixed
                        ShapeBox {
                            title: "Mixed"
                            elevation: 3
                            corners: MD.Util.corners(m_slider_radius.value, 0, 0, m_slider_radius.value)
                        }
                    }
                }
            }
        }
    }

    component ElevationBox: MD.Rectangle {
        id: eb_root
        property alias elevation: m_elev.elevation
        property string text: ""
        
        implicitWidth: 100
        implicitHeight: 100
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

    component ShapeBox: ColumnLayout {
        id: sb_root
        property string title: ""
        property alias elevation: m_elev.elevation
        property alias corners: m_elev.corners
        spacing: 8

        Item {
            id: m_rect
            implicitWidth: 120
            implicitHeight: 60
            // color: MD.MProp.color.surface_container_high
            
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
