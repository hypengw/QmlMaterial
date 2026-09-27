pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Page {
    id: root
    title: 'Shape'
    padding: 0

    property int fromIndex: 0
    property int toIndex: 24
    property real morphProgress: 0

    readonly property var shapes: [
        {
            name: 'Circle',
            shape: MD.MaterialShape.Circle
        },
        {
            name: 'Square',
            shape: MD.MaterialShape.Square
        },
        {
            name: 'Slanted',
            shape: MD.MaterialShape.Slanted
        },
        {
            name: 'Arch',
            shape: MD.MaterialShape.Arch
        },
        {
            name: 'Semicircle',
            shape: MD.MaterialShape.SemiCircle
        },
        {
            name: 'Oval',
            shape: MD.MaterialShape.Oval
        },
        {
            name: 'Pill',
            shape: MD.MaterialShape.Pill
        },
        {
            name: 'Triangle',
            shape: MD.MaterialShape.Triangle
        },
        {
            name: 'Arrow',
            shape: MD.MaterialShape.Arrow
        },
        {
            name: 'Fan',
            shape: MD.MaterialShape.Fan
        },
        {
            name: 'Diamond',
            shape: MD.MaterialShape.Diamond
        },
        {
            name: 'Clamshell',
            shape: MD.MaterialShape.ClamShell
        },
        {
            name: 'Pentagon',
            shape: MD.MaterialShape.Pentagon
        },
        {
            name: 'Gem',
            shape: MD.MaterialShape.Gem
        },
        {
            name: 'Very sunny',
            shape: MD.MaterialShape.VerySunny
        },
        {
            name: 'Sunny',
            shape: MD.MaterialShape.Sunny
        },
        {
            name: '4-sided cookie',
            shape: MD.MaterialShape.Cookie4Sided
        },
        {
            name: '6-sided cookie',
            shape: MD.MaterialShape.Cookie6Sided
        },
        {
            name: '7-sided cookie',
            shape: MD.MaterialShape.Cookie7Sided
        },
        {
            name: '9-sided cookie',
            shape: MD.MaterialShape.Cookie9Sided
        },
        {
            name: '12-sided cookie',
            shape: MD.MaterialShape.Cookie12Sided
        },
        {
            name: '4-leaf clover',
            shape: MD.MaterialShape.Clover4Leaf
        },
        {
            name: '8-leaf clover',
            shape: MD.MaterialShape.Clover8Leaf
        },
        {
            name: 'Burst',
            shape: MD.MaterialShape.Burst
        },
        {
            name: 'Soft burst',
            shape: MD.MaterialShape.SoftBurst
        },
        {
            name: 'Boom',
            shape: MD.MaterialShape.Boom
        },
        {
            name: 'Soft boom',
            shape: MD.MaterialShape.SoftBoom
        },
        {
            name: 'Flower',
            shape: MD.MaterialShape.Flower
        },
        {
            name: 'Puffy',
            shape: MD.MaterialShape.Puffy
        },
        {
            name: 'Puffy diamond',
            shape: MD.MaterialShape.PuffyDiamond
        },
        {
            name: 'Ghost-ish',
            shape: MD.MaterialShape.Ghostish
        },
        {
            name: 'Pixel circle',
            shape: MD.MaterialShape.PixelCircle
        },
        {
            name: 'Pixel triangle',
            shape: MD.MaterialShape.PixelTriangle
        },
        {
            name: 'Bun',
            shape: MD.MaterialShape.Bun
        },
        {
            name: 'Heart',
            shape: MD.MaterialShape.Heart
        }
    ]

    contentItem: Item {
        MD.TabBar {
            id: tabs
            objectName: 'shapeTabs'
            anchors.top: parent.top
            width: parent.width
            MD.TabButton {
                text: 'Shapes'
            }
            MD.TabButton {
                text: 'Morph'
            }
            MD.TabButton {
                text: 'Effects'
            }
        }

        Loader {
            anchors.top: tabs.bottom
            anchors.bottom: parent.bottom
            width: parent.width
            sourceComponent: [shapeGrid, morphView, effectsView][tabs.currentIndex]
        }
    }

    Component {
        id: shapeGrid
        MD.VerticalFlickable {
            id: shapesFlick
            clip: true
            topMargin: 16
            bottomMargin: 16
            contentHeight: grid.implicitHeight
            Grid {
                id: grid
                x: 16
                width: Math.max(0, shapesFlick.width - 32)
                columns: Math.max(1, Math.min(7, Math.floor(width / 132)))
                Repeater {
                    model: root.shapes
                    Item {
                        id: cell
                        required property var modelData
                        width: grid.width / grid.columns
                        height: 148
                        MD.Shape {
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: 16
                            width: 88
                            height: 88
                            MD.MaterialShapePath {
                                shape: cell.modelData.shape
                                size: Qt.size(88, 88)
                                fillColor: MD.MProp.color.on_surface
                                strokeWidth: 0
                            }
                        }
                        MD.Text {
                            y: 116
                            width: parent.width
                            leftPadding: 4
                            rightPadding: 4
                            text: cell.modelData.name
                            typescale: MD.Token.typescale.label_medium
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }

    Component {
        id: morphView
        MD.VerticalFlickable {
            id: morphFlick
            clip: true
            topMargin: 24
            bottomMargin: 24
            contentHeight: morphContent.implicitHeight
            Column {
                id: morphContent
                x: (morphFlick.width - width) / 2
                width: Math.max(0, Math.min(morphFlick.width - 32, 600))
                spacing: 24

                Grid {
                    width: parent.width
                    columns: width < 420 ? 1 : 2
                    spacing: 16
                    MD.ComboBox {
                        objectName: 'fromShape'
                        width: (parent.width - parent.spacing * (parent.columns - 1)) / parent.columns
                        popupMaximumHeight: Math.max(1, Math.min(320, morphFlick.height - 24))
                        label: 'From'
                        model: root.shapes
                        textRole: 'name'
                        currentIndex: root.fromIndex
                        onActivated: root.fromIndex = currentIndex
                    }
                    MD.ComboBox {
                        objectName: 'toShape'
                        width: (parent.width - parent.spacing * (parent.columns - 1)) / parent.columns
                        popupMaximumHeight: Math.max(1, Math.min(320, morphFlick.height - 24))
                        label: 'To'
                        model: root.shapes
                        textRole: 'name'
                        currentIndex: root.toIndex
                        onActivated: root.toIndex = currentIndex
                    }
                }

                Item {
                    width: parent.width
                    height: 256
                    MD.Shape {
                        id: morphShape
                        anchors.centerIn: parent
                        width: Math.min(parent.width, 224)
                        height: width
                        MD.MaterialShapePath {
                            objectName: 'morphPath'
                            shape: root.shapes[root.fromIndex].shape
                            toShape: root.shapes[root.toIndex].shape
                            progress: root.morphProgress
                            size: Qt.size(morphShape.width, morphShape.height)
                            fillColor: MD.MProp.color.primary
                            strokeWidth: 0
                        }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 12
                    MD.IconButton {
                        objectName: 'morphPlayback'
                        icon.name: motion.running && !motion.paused ? MD.Token.icon.pause : MD.Token.icon.play_arrow
                        MD.ToolTip.text: motion.running && !motion.paused ? 'Pause' : 'Play'
                        MD.ToolTip.visible: hovered
                        onClicked: {
                            if (!motion.running)
                                motion.start();
                            else if (motion.paused)
                                motion.resume();
                            else
                                motion.pause();
                        }
                    }
                    MD.Slider {
                        id: progressSlider
                        objectName: 'morphSlider'
                        width: parent.width - 116
                        anchors.verticalCenter: parent.verticalCenter
                        from: 0
                        to: 1
                        value: root.morphProgress
                        onMoved: {
                            motion.stop();
                            root.morphProgress = value;
                        }
                    }
                    MD.Text {
                        width: 44
                        anchors.verticalCenter: parent.verticalCenter
                        horizontalAlignment: Text.AlignRight
                        text: Math.round(root.morphProgress * 100) + '%'
                        typescale: MD.Token.typescale.label_large
                    }
                }

                SequentialAnimation {
                    id: motion
                    loops: Animation.Infinite
                    NumberAnimation {
                        target: root
                        property: 'morphProgress'
                        to: 1
                        duration: 1800
                    }
                    PauseAnimation {
                        duration: 300
                    }
                    NumberAnimation {
                        target: root
                        property: 'morphProgress'
                        to: 0
                        duration: 1800
                    }
                    PauseAnimation {
                        duration: 300
                    }
                }
            }
        }
    }

    Component {
        id: effectsView
        ShapeEffects {}
    }
}
