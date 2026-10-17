pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: 400
    implicitHeight: content.implicitHeight
    property string lastResult: ''
    MD.FileDialog {
        id: openOne
        objectName: 'exampleOpenFile'
        title: 'Open file'
        fileMode: MD.FileDialog.OpenFile
        nameFilters: ['Images (*.png *.jpg *.jpeg)', 'Text (*.txt *.md)', 'All files (*)']
        onAccepted: root.lastResult = 'OpenFile: ' + selectedFile
        onRejected: root.lastResult = 'OpenFile: cancelled'
    }
    MD.FileDialog {
        id: openMany
        objectName: 'exampleOpenFiles'
        title: 'Open files'
        fileMode: MD.FileDialog.OpenFiles
        nameFilters: ['All files (*)']
        onAccepted: root.lastResult = 'OpenFiles: ' + JSON.stringify(selectedFiles.map(url => url.toString()))
        onRejected: root.lastResult = 'OpenFiles: cancelled'
    }
    MD.FileDialog {
        id: save
        objectName: 'exampleSaveFile'
        title: 'Save file'
        fileMode: MD.FileDialog.SaveFile
        nameFilters: ['Text (*.txt)', 'All files (*)']
        defaultSuffix: 'txt'
        onAccepted: root.lastResult = 'SaveFile: ' + selectedFile
        onRejected: root.lastResult = 'SaveFile: cancelled'
    }
    MD.FolderDialog {
        id: folder
        objectName: 'exampleFolder'
        title: 'Pick folder'
        onAccepted: root.lastResult = 'FolderDialog: ' + selectedFolder
        onRejected: root.lastResult = 'FolderDialog: cancelled'
    }
    Column {
        id: content
        width: parent.width
        spacing: 12
        Flow {
            width: parent.width
            spacing: 8
            MD.Button {
                text: 'Open file'
                icon.name: MD.Token.icon.file_open
                onClicked: openOne.open()
            }
            MD.Button {
                text: 'Open multiple'
                icon.name: MD.Token.icon.file_open
                mdState.type: MD.Enum.BtFilledTonal
                onClicked: openMany.open()
            }
            MD.Button {
                text: 'Save file'
                icon.name: MD.Token.icon.file_save
                mdState.type: MD.Enum.BtFilledTonal
                onClicked: save.open()
            }
            MD.Button {
                text: 'Pick folder'
                icon.name: MD.Token.icon.folder
                mdState.type: MD.Enum.BtFilledTonal
                onClicked: folder.open()
            }
        }
        MD.Label {
            width: parent.width
            visible: root.lastResult.length > 0
            text: root.lastResult
            typescale: MD.Token.typescale.body_small
            wrapMode: Text.Wrap
        }
    }
}
