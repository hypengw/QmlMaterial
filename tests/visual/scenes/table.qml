import QtQuick
import QtQuick.Layouts
import Qt.labs.qmlmodels
import Qcm.Material as MD

Rectangle {
    id: root
    width: 720
    height: 420
    color: MD.Token.color.surface

    TableModel {
        id: tableModel

        TableModelColumn { display: "component" }
        TableModelColumn { display: "usage" }

        rows: [
            { component: "Buttons", usage: "Primary and secondary actions" },
            { component: "Bottom sheets", usage: "Modal and persistent surfaces" },
            { component: "Navigation", usage: "Destination switching" },
            { component: "Text inputs", usage: "Form controls" },
            { component: "Tables", usage: "Structured data" }
        ]
    }

    GridLayout {
        anchors.centerIn: parent
        columns: 1
        columnSpacing: 0
        rowSpacing: 0

        MD.HorizontalHeaderView {
            id: horizontalHeader
            Layout.row: 0
            Layout.column: 0
            Layout.preferredWidth: 560
            Layout.preferredHeight: 48
            clip: true
            syncView: tableView
            model: ["Component", "Usage"]
        }

        MD.TableView {
            id: tableView
            Layout.row: 1
            Layout.column: 0
            Layout.preferredWidth: 560
            Layout.preferredHeight: 220
            radius: MD.Token.shape.corner.extra_large
            hasHeader: true
            model: tableModel
            selectionBehavior: TableView.SelectionDisabled
            columnWidthProvider: function(column) {
                return [200, 360][column] ?? 120;
            }
            rowHeightProvider: function() {
                return 44;
            }
        }
    }
}
