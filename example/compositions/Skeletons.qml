pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Column {
    id: root
    width: 640
    spacing: 16
    property bool loading: true
    property alias shimmer: sharedShimmer

    MD.Shimmer {
        id: sharedShimmer
        objectName: "skeletonShimmer"
        boundsMode: MD.Shimmer.Item
        boundsItem: loadingArea
    }

    Flow {
        width: parent.width
        spacing: 8
        MD.SegmentedButtonGroup {
            size: MD.Enum.XS
            MD.SegmentedButton {
                width: 88
                text: "View"
                checked: sharedShimmer.boundsMode === MD.Shimmer.View
                onCheckedChangeRequested: value => {
                    if (value)
                        sharedShimmer.boundsMode = MD.Shimmer.View;
                }
            }
            MD.SegmentedButton {
                width: 112
                text: "Window"
                checked: sharedShimmer.boundsMode === MD.Shimmer.Window
                onCheckedChangeRequested: value => {
                    if (value)
                        sharedShimmer.boundsMode = MD.Shimmer.Window;
                }
            }
            MD.SegmentedButton {
                width: 88
                text: "Item"
                checked: sharedShimmer.boundsMode === MD.Shimmer.Item
                onCheckedChangeRequested: value => {
                    if (value)
                        sharedShimmer.boundsMode = MD.Shimmer.Item;
                }
            }
        }
        MD.IconButton {
            objectName: "shimmerPlay"
            icon.name: sharedShimmer.running ? MD.Token.icon.pause : MD.Token.icon.play_arrow
            Accessible.name: sharedShimmer.running ? "Pause" : "Play"
            MD.ToolTip.text: Accessible.name
            MD.ToolTip.visible: mdState.interactions.hovered
            onClicked: sharedShimmer.running = !sharedShimmer.running
        }
        MD.IconButton {
            icon.name: MD.Token.icon.restart_alt
            Accessible.name: "Restart"
            MD.ToolTip.text: Accessible.name
            MD.ToolTip.visible: mdState.interactions.hovered
            onClicked: sharedShimmer.restart()
        }
        MD.ToggleButton {
            objectName: "skeletonLoading"
            text: "Loading"
            checked: root.loading
            onCheckedChangeRequested: value => root.loading = value
        }
    }

    Flow {
        width: parent.width
        spacing: 24
        Column {
            width: Math.min(260, root.width)
            MD.Label {
                text: "Angle"
            }
            MD.Slider {
                width: parent.width
                from: 0
                to: 90
                value: sharedShimmer.angle
                onValueChangeRequested: value => sharedShimmer.angle = value
            }
        }
        Column {
            width: Math.min(260, root.width)
            MD.Label {
                text: "Band width"
            }
            MD.Slider {
                width: parent.width
                from: 40
                to: 480
                value: sharedShimmer.bandWidth
                onValueChangeRequested: value => sharedShimmer.bandWidth = value
            }
        }
    }
    MD.Slider {
        objectName: "shimmerPhase"
        width: parent.width
        enabled: !sharedShimmer.running
        from: 0
        to: 1
        value: sharedShimmer.phase
        onValueChangeRequested: value => sharedShimmer.seek(value)
        Accessible.name: "Phase"
    }

    Item {
        id: loadingArea
        width: parent.width
        height: preview.height + 16 + view.height

        Item {
            id: preview
            width: parent.width
            height: 104
            PlaceholderRow {
                width: Math.max(0, parent.width - 104)
                wave: sharedShimmer
                loading: root.loading
                title: "Studio mix"
                subtitle: "New releases"
            }
            MD.Shape {
                x: parent.width - width
                width: 88
                height: 88
                layer.enabled: root.loading
                layer.effect: MD.ShimmerEffect {
                    shimmer: sharedShimmer
                }
                MD.MaterialShapePath {
                    shape: MD.MaterialShape.Cookie6Sided
                    size: Qt.size(88, 88)
                    fillColor: MD.MProp.color.primary
                    strokeWidth: 0
                }
            }
        }

        MD.LazyGrid {
            id: view
            objectName: "skeletonGrid"
            y: preview.height + 16
            width: parent.width
            height: 280
            clip: true
            minimumCellExtent: 220
            crossSpacing: 16
            mainSpacing: 12
            itemExtent: 88
            source: MD.ItemModelSource {
                model: entries
                keyRole: "key"
            }
            MD.ScrollBarBase.vertical: MD.ScrollBar {}
            delegate: PlaceholderRow {
                required property MD.LazyRow row
                wave: sharedShimmer
                loading: root.loading
                title: row.value.title
                subtitle: row.value.subtitle
                implicitHeight: 88
            }
        }
    }
    MD.IconButton {
        objectName: "skeletonAppend"
        icon.name: MD.Token.icon.add
        Accessible.name: "Add items"
        MD.ToolTip.text: Accessible.name
        MD.ToolTip.visible: mdState.interactions.hovered
        onClicked: root.appendItems()
    }
    ListModel {
        id: entries
    }
    function appendItems() {
        const start = entries.count;
        for (let i = 0; i < 12; ++i) {
            const index = start + i;
            entries.append({
                key: "station-" + index,
                title: "Station " + (index + 1),
                subtitle: index % 2 ? "Live session" : "Daily selection"
            });
        }
    }
    Component.onCompleted: appendItems()

    component PlaceholderRow: Item {
        id: rowItem
        required property MD.Shimmer wave
        property bool loading: true
        property string title: ""
        property string subtitle: ""
        implicitHeight: 88

        MD.Skeleton {
            x: 0
            y: 8
            width: 56
            height: 56
            radius: 28
            visible: rowItem.loading
            shimmer: rowItem.wave
        }
        MD.Skeleton {
            x: 72
            y: 12
            width: Math.max(0, rowItem.width - x)
            height: 18
            visible: rowItem.loading
            shimmer: rowItem.wave
        }
        MD.Skeleton {
            x: 72
            y: 42
            width: Math.max(0, (rowItem.width - x) * .7)
            height: 14
            visible: rowItem.loading
            shimmer: rowItem.wave
        }
        Rectangle {
            x: 0
            y: 8
            width: 56
            height: 56
            radius: 28
            visible: !rowItem.loading
            color: MD.MProp.color.primary_container
            MD.Icon {
                anchors.centerIn: parent
                name: MD.Token.icon.music_note
                color: MD.MProp.color.on_primary_container
            }
        }
        MD.Label {
            x: 72
            y: 8
            width: Math.max(0, rowItem.width - x)
            visible: !rowItem.loading
            text: rowItem.title
            elide: Text.ElideRight
        }
        MD.Label {
            x: 72
            y: 38
            width: Math.max(0, rowItem.width - x)
            visible: !rowItem.loading
            text: rowItem.subtitle
            color: MD.MProp.color.on_surface_variant
            elide: Text.ElideRight
        }
    }
}
