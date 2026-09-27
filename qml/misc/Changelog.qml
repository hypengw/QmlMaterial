pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.VerticalListView {
    id: control

    property alias source: releaseModel.source
    readonly property alias status: releaseModel.status
    readonly property alias errorString: releaseModel.errorString
    readonly property alias releaseCount: releaseModel.count

    implicitWidth: 360
    implicitHeight: expand ? contentHeight + topMargin + bottomMargin : 360
    interactive: !expand
    leftMargin: 20
    rightMargin: 20
    spacing: 0
    model: MD.AppStreamReleaseModel {
        id: releaseModel
    }

    header: MD.Text {
        width: ListView.view?.contentWidth ?? 0
        height: visible ? implicitHeight + 24 : 0
        visible: releaseModel.status === MD.AppStreamReleaseModel.Error
        text: releaseModel.errorString
        typescale: MD.Token.typescale.body_medium
        color: MD.Token.color.error
        wrapMode: Text.Wrap
    }

    delegate: Item {
        id: releaseItem

        required property int index
        required property string version
        required property string date
        required property var sections

        width: ListView.view?.contentWidth ?? 0
        implicitHeight: releaseRow.implicitHeight + (index + 1 < control.releaseCount ? 24 : 0)

        RowLayout {
            id: releaseRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            spacing: 12

            Item {
                Layout.preferredWidth: 20
                Layout.fillHeight: true

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 10
                    width: 2
                    height: Math.max(0, parent.height + 24 - y)
                    visible: releaseItem.index + 1 < control.releaseCount
                    color: MD.Token.color.outline_variant
                }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 6
                    width: 12
                    height: 12
                    radius: 6
                    color: releaseItem.index === 0 ? MD.Token.color.primary : MD.Token.color.surface
                    border.width: releaseItem.index === 0 ? 0 : 2
                    border.color: MD.Token.color.outline
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    MD.Text {
                        Layout.fillWidth: true
                        text: releaseItem.version
                        typescale: MD.Token.typescale.title_medium
                        color: releaseItem.index === 0 ? MD.Token.color.primary : MD.Token.color.on_surface
                    }

                    MD.Text {
                        visible: releaseItem.date.length > 0
                        text: releaseItem.date
                        typescale: MD.Token.typescale.label_medium
                        color: MD.Token.color.on_surface_variant
                    }
                }

                Repeater {
                    model: releaseItem.sections

                    delegate: ColumnLayout {
                        id: section
                        required property var modelData

                        Layout.fillWidth: true
                        spacing: 6

                        MD.Text {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: section.modelData.title ?? ""
                            typescale: MD.Token.typescale.label_large
                            color: MD.Token.color.on_surface_variant
                        }

                        Repeater {
                            model: section.modelData.items ?? []

                            delegate: RowLayout {
                                id: noteRow
                                required property string modelData

                                Layout.fillWidth: true
                                spacing: 8

                                Item {
                                    Layout.preferredWidth: 8
                                    Layout.preferredHeight: noteText.implicitHeight

                                    Rectangle {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        y: Math.max(0, noteText.font.pixelSize * 0.5 - 2)
                                        width: 4
                                        height: 4
                                        radius: 2
                                        color: MD.Token.color.on_surface_variant
                                    }
                                }

                                MD.Text {
                                    id: noteText
                                    Layout.fillWidth: true
                                    text: noteRow.modelData
                                    typescale: MD.Token.typescale.body_medium
                                    color: MD.Token.color.on_surface
                                    wrapMode: Text.Wrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
