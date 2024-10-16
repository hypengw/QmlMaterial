import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QC

import Qcm.Material as MD

MD.Page {
    title: 'Components'
    padding: 0

    MD.Flickable {
        anchors.fill: parent
        topMargin: 16
        bottomMargin: 16

        ColumnLayout {
            width: parent.width

            MD.Pane {
                Layout.alignment: Qt.AlignHCenter

                ColumnLayout {
                    spacing: 16
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: 'Elevation'
                        typescale: MD.Token.typescale.title_large
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Shadow'
                            typescale: MD.Token.typescale.title_medium
                        }
                        MD.Pane {
                            Layout.alignment: Qt.AlignHCenter
                            padding: 36
                            radius: MD.Token.shape.corner.medium

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 24

                                MD.Slider {
                                    id: m_slider_radius
                                    value: 8
                                    from: 0
                                    to: 80
                                }

                                RowLayout {
                                    Layout.alignment: Qt.AlignHCenter
                                    spacing: 36
                                    Elevation {
                                        level: MD.Token.elevation.level0
                                    }
                                    Elevation {
                                        level: MD.Token.elevation.level1
                                    }
                                    Elevation {
                                        level: MD.Token.elevation.level2
                                    }
                                }
                                RowLayout {
                                    spacing: 36
                                    Elevation {
                                        level: MD.Token.elevation.level3
                                    }
                                    Elevation {
                                        level: MD.Token.elevation.level4
                                    }
                                    Elevation {
                                        level: MD.Token.elevation.level5
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    component Elevation: MD.Elevation {
        radius: m_slider_radius.value
        implicitWidth: 160
        implicitHeight: 160

        MD.Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: MD.MatProp.color.surface_container
        }
    }
}
