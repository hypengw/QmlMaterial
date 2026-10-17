pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: 560
    implicitHeight: content.implicitHeight
    property bool wifiEnabled: true
    property bool bluetoothEnabled: true
    property bool focusEnabled: false
    property bool nightLightEnabled: false
    property bool expanded: true
    property bool flashlightEnabled: false
    property bool airplaneEnabled: false
    property bool rotationEnabled: false
    property bool batterySaverEnabled: false
    property bool hotspotEnabled: false
    property bool locationEnabled: true
    property int columns: 2
    property int rows: 4
    readonly property int pageCapacity: columns * rows
    readonly property var tileSpecs: ['wifi', 'bluetooth', 'flashlight', 'focus', 'alarm', 'airplane', 'controls', 'rotation', 'battery', 'night', 'hotspot', 'location']
    readonly property int pageCount: Math.max(1, Math.ceil(tileSpecs.length / pageCapacity))
    readonly property int currentPage: pager.currentIndex
    readonly property real pagePosition: pager.width > 0 && expanded ? Math.max(0, Math.min(pageCount - 1, pager.contentX / pager.width)) : 0
    function setPage(index) {
        pager.currentIndex = Math.max(0, Math.min(pageCount - 1, index));
        pager.positionViewAtIndex(pager.currentIndex, ListView.Beginning);
    }
    function __syncPage() {
        setPage(currentPage);
    }
    function __resetPage() {
        setPage(0);
    }
    function tileState(spec) {
        switch (spec) {
        case 'wifi':
            return {
                title: 'Wi-Fi',
                status: wifiEnabled ? network : '',
                symbol: wifiEnabled ? MD.Token.icon.wifi : MD.Token.icon.wifi_off,
                active: wifiEnabled,
                dual: true
            };
        case 'bluetooth':
            return {
                title: 'Bluetooth',
                status: bluetoothEnabled ? device : '',
                symbol: MD.Token.icon.bluetooth,
                active: bluetoothEnabled,
                dual: true
            };
        case 'flashlight':
            return {
                title: 'Flashlight',
                status: '',
                symbol: MD.Token.icon.flashlight_on,
                active: flashlightEnabled
            };
        case 'focus':
            return {
                title: 'Modes',
                status: focusEnabled ? 'Do Not Disturb' : '',
                symbol: MD.Token.icon.do_not_disturb_on,
                active: focusEnabled
            };
        case 'alarm':
            return {
                title: 'Alarm',
                status: 'No alarm set',
                symbol: MD.Token.icon.alarm,
                toggleable: false
            };
        case 'airplane':
            return {
                title: 'Airplane mode',
                status: '',
                symbol: MD.Token.icon.airplanemode_active,
                active: airplaneEnabled
            };
        case 'controls':
            return {
                title: 'Device controls',
                status: 'Unavailable',
                symbol: MD.Token.icon.home,
                available: false
            };
        case 'rotation':
            return {
                title: 'Auto-rotate',
                status: '',
                symbol: MD.Token.icon.screen_rotation,
                active: rotationEnabled
            };
        case 'battery':
            return {
                title: 'Battery Saver',
                status: '',
                symbol: MD.Token.icon.battery_full,
                active: batterySaverEnabled
            };
        case 'night':
            return {
                title: 'Night Light',
                status: '',
                symbol: MD.Token.icon.nightlight,
                active: nightLightEnabled
            };
        case 'hotspot':
            return {
                title: 'Hotspot',
                status: '',
                symbol: MD.Token.icon.wifi_tethering,
                active: hotspotEnabled
            };
        case 'location':
            return {
                title: 'Location',
                status: '',
                symbol: MD.Token.icon.location_on,
                active: locationEnabled
            };
        default:
            return {
                title: '',
                status: '',
                symbol: ''
            };
        }
    }
    function toggleTile(spec) {
        switch (spec) {
        case 'alarm':
            alarmRequested();
            break;
        case 'wifi':
            wifiEnabled = !wifiEnabled;
            break;
        case 'bluetooth':
            bluetoothEnabled = !bluetoothEnabled;
            break;
        case 'flashlight':
            flashlightEnabled = !flashlightEnabled;
            break;
        case 'focus':
            focusEnabled = !focusEnabled;
            break;
        case 'airplane':
            airplaneEnabled = !airplaneEnabled;
            break;
        case 'rotation':
            rotationEnabled = !rotationEnabled;
            break;
        case 'battery':
            batterySaverEnabled = !batterySaverEnabled;
            break;
        case 'night':
            nightLightEnabled = !nightLightEnabled;
            break;
        case 'hotspot':
            hotspotEnabled = !hotspotEnabled;
            break;
        case 'location':
            locationEnabled = !locationEnabled;
            break;
        }
    }
    onPageCountChanged: Qt.callLater(__syncPage)
    onColumnsChanged: Qt.callLater(__resetPage)
    onRowsChanged: Qt.callLater(__resetPage)
    onExpandedChanged: setPage(0)
    property real brightness: 64
    property string network: 'Studio'
    property string device: 'Headphones'
    readonly property var networks: ['Studio', 'Guest', 'Pocket hotspot']
    readonly property var devices: ['Headphones', 'Desk speaker', 'Keyboard']
    property int detailIndex: 0
    onVisibleChanged: if (!visible)
        closeDetails()
    signal alarmRequested

    function showDetails(index, anchor) {
        detailIndex = index;
        details.anchorItem = anchor;
        details.open();
    }
    function closeDetails() {
        details.close();
    }
    Column {
        id: content
        width: parent.width
        spacing: 16
        ShadeSlider {
            objectName: 'brightnessSlider'
            width: parent.width
            visible: root.expanded
            value: root.brightness
            onMoved: root.brightness = value
        }
        ListView {
            id: pager
            objectName: 'quickPager'
            width: parent.width
            height: root.expanded ? root.rows * 72 + (root.rows - 1) * 8 : 72
            model: root.expanded ? root.pageCount : 1
            orientation: ListView.Horizontal
            snapMode: ListView.SnapOneItem
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            interactive: root.expanded && root.pageCount > 1
            highlightRangeMode: ListView.StrictlyEnforceRange
            preferredHighlightBegin: 0
            preferredHighlightEnd: width
            highlightMoveDuration: 0
            onWidthChanged: Qt.callLater(root.__syncPage)
            delegate: Grid {
                id: page
                required property int index
                width: pager.width
                height: pager.height
                columns: root.columns
                spacing: 8
                Repeater {
                    model: Math.min(root.expanded ? root.pageCapacity : root.columns, Math.max(0, root.tileSpecs.length - page.index * root.pageCapacity))
                    delegate: QuickTile {
                        required property int index
                        readonly property string spec: page ? root.tileSpecs[page.index * root.pageCapacity + index] ?? '' : ''
                        readonly property var tileData: root.tileState(spec)
                        objectName: spec + 'Tile'
                        width: (pager.width - (root.columns - 1) * 8) / root.columns
                        title: tileData.title
                        status: tileData.status
                        symbol: tileData.symbol
                        active: tileData.active ?? false
                        available: tileData.available ?? true
                        dualTarget: tileData.dual ?? false
                        toggleable: tileData.toggleable ?? true
                        onToggleRequested: root.toggleTile(spec)
                        onDetailsRequested: root.showDetails(spec === 'wifi' ? 0 : 1, detailsAnchor)
                    }
                }
            }
        }
        Item {
            width: parent.width
            height: 48
            visible: root.expanded
            Row {
                anchors.centerIn: parent
                spacing: 8
                visible: root.pageCount > 1
                MD.IconButton {
                    objectName: 'previousQuickPage'
                    width: 24
                    height: 24
                    padding: 0
                    icon.width: 24
                    icon.height: 24
                    icon.name: MD.Token.icon.chevron_left
                    enabled: root.currentPage > 0
                    Accessible.name: 'Previous page'
                    onClicked: root.setPage(root.currentPage - 1)
                }
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    Repeater {
                        model: root.pageCount
                        Rectangle {
                            required property int index
                            readonly property real weight: Math.max(0, 1 - Math.abs(index - root.pagePosition))
                            width: 6 + 6 * weight
                            height: 6
                            radius: 3
                            opacity: 0.5 + 0.5 * weight
                            color: root.MD.MProp.color.on_surface_variant
                        }
                    }
                }
                MD.IconButton {
                    objectName: 'nextQuickPage'
                    width: 24
                    height: 24
                    padding: 0
                    icon.width: 24
                    icon.height: 24
                    icon.name: MD.Token.icon.chevron_right
                    enabled: root.currentPage < root.pageCount - 1
                    Accessible.name: 'Next page'
                    onClicked: root.setPage(root.currentPage + 1)
                }
            }
        }
    }

    MD.Popup {
        id: details
        objectName: 'quickDetails'
        MD.MProp.backgroundColor: mdState.backgroundColor
        MD.MProp.textColor: mdState.textColor
        width: Math.min(320, root.width)
        padding: 12
        margins: 12
        dim: false
        modal: true
        focus: true
        anchorGap: 8
        preferredPlacement: MD.PopupBase.Below
        contentItem: Column {
            spacing: 4
            MD.Text {
                width: parent.width
                padding: 12
                text: root.detailIndex === 0 ? 'Wi-Fi networks' : 'Bluetooth devices'
                typescale: MD.Token.typescale.title_medium
            }
            Repeater {
                id: choices
                objectName: 'detailChoices'
                model: root.detailIndex === 0 ? root.networks : root.devices
                delegate: MD.ListItem {
                    required property string modelData
                    required property int index
                    objectName: 'detailChoice' + index
                    width: parent.width
                    radius: 8
                    text: modelData
                    icon.name: root.detailIndex === 0 ? MD.Token.icon.wifi : MD.Token.icon.bluetooth
                    highlighted: root.detailIndex === 0 ? root.wifiEnabled && root.network === modelData : root.bluetoothEnabled && root.device === modelData
                    trailing: MD.Icon {
                        name: MD.Token.icon.check
                        visible: root.detailIndex === 0 ? root.wifiEnabled && root.network === modelData : root.bluetoothEnabled && root.device === modelData
                    }
                    onClicked: {
                        if (root.detailIndex === 0) {
                            root.network = modelData;
                            root.wifiEnabled = true;
                        } else {
                            root.device = modelData;
                            root.bluetoothEnabled = true;
                        }
                        details.close();
                    }
                }
            }
        }
    }
}
