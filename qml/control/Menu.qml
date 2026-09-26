pragma ComponentBehavior: Bound
import QtQuick
import QtQml.Models

import Qcm.Material as MD

/** @ingroup control */
MD.MenuBase {
    id: control

    property alias mdState: item_state
    property bool autoClose: false
    property real maximumWidth: 280
    readonly property bool cascade: true
    default property list<QtObject> entries
    property list<Item> __entryItems
    property list<Item> __ownedItems
    property list<Item> __entryOwnedItems

    function insertAction(index: int, action: MD.Action) {
        const item = delegate.createObject(contentItem, {
            action
        });
        insertItem(index, item);
        __ownedItems.push(item);
    }
    function addAction(action: MD.Action) {
        insertAction(count, action);
    }
    function insertMenu(index: int, menu: MD.MenuBase) {
        const item = delegate.createObject(contentItem, {
            text: Qt.binding(() => menu.title),
            subMenu: menu
        });
        insertItem(index, item);
        __ownedItems.push(item);
    }
    function addMenu(menu: MD.MenuBase) {
        insertMenu(count, menu);
    }
    function menuAt(index: int): MD.MenuBase {
        return (itemAt(index) as MD.MenuItem)?.subMenu ?? null;
    }
    function removeMenu(menu: MD.MenuBase) {
        for (let index = count - 1; index >= 0; --index) {
            if (menuAt(index) === menu)
                __removeEntry(itemAt(index));
        }
    }
    function __removeEntry(item: Item) {
        takeItem(indexOf(item));
        if (item instanceof MD.MenuItem)
            item.menu = null;
        const entryIndex = __entryItems.indexOf(item);
        if (entryIndex >= 0)
            __entryItems.splice(entryIndex, 1);
        for (const owned of [__ownedItems, __entryOwnedItems]) {
            const index = owned.indexOf(item);
            if (index >= 0) {
                owned.splice(index, 1);
                item.destroy();
                break;
            }
        }
    }
    function actionAt(index: int): MD.Action {
        const item = itemAt(index);
        return item instanceof MD.AbstractButtonBase ? item.action : null;
    }
    function removeAction(action: MD.Action) {
        for (let index = count - 1; index >= 0; --index) {
            if (actionAt(index) === action) {
                __removeEntry(itemAt(index));
            }
        }
    }
    function __syncEntries() {
        for (const item of __entryItems)
            takeItem(indexOf(item));
        for (const item of __entryOwnedItems)
            item.destroy();
        __entryOwnedItems = [];
        __entryItems = [];
        for (const entry of entries) {
            let item = null;
            if (entry instanceof MD.Action)
                item = delegate.createObject(contentItem, {
                    action: entry
                });
            else if (entry instanceof MD.MenuBase)
                item = delegate.createObject(contentItem, {
                    text: Qt.binding(() => entry.title),
                    subMenu: entry
                });
            else if (entry instanceof Item)
                item = entry;
            if (!item)
                continue;
            if (item !== entry)
                __entryOwnedItems.push(item);
            __entryItems.push(item);
            addItem(item);
        }
    }
    onEntriesChanged: Qt.callLater(__syncEntries)
    function __dismissChain() {
        dismiss();
    }
    function __moveCurrent(delta: int) {
        for (let offset = 1; offset <= count; ++offset) {
            const index = (currentIndex + delta * offset + count * 2) % count;
            const item = itemAt(index);
            if (item instanceof MD.AbstractButtonBase && item.enabled && item.visible) {
                currentIndex = index;
                item.forceActiveFocus(Qt.TabFocusReason);
                return;
            }
        }
    }

    implicitWidth: {
        const preferredWidth = Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding);
        return maximumWidth > 0 ? Math.min(preferredWidth, maximumWidth) : preferredWidth;
    }
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    margins: 0
    verticalPadding: 8

    transformOrigin: !cascade ? Item.Top : (mirrored ? Item.TopRight : Item.TopLeft)

    delegate: MD.MenuItem {}

    property var model: null
    property alias contentDelegate: m_instantiator.delegate

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1.0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
        NumberAnimation {
            property: "scale"
            from: 0.8
            to: 1.0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            to: 0
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
        NumberAnimation {
            property: "scale"
            to: 0.8
            easing: MD.Token.easing.emphasized_decelerate
            duration: MD.Token.duration.medium2
        }
    }

    contentItem: MD.ListView {
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Down || event.key === Qt.Key_Up) {
                control.__moveCurrent(event.key === Qt.Key_Down ? 1 : -1);
                event.accepted = true;
            } else if (event.key === (control.mirrored ? Qt.Key_Left : Qt.Key_Right)) {
                const item = control.itemAt(control.currentIndex) as MD.MenuItem;
                if (item?.subMenu) {
                    item.click();
                    event.accepted = true;
                }
            } else if (event.key === (control.mirrored ? Qt.Key_Right : Qt.Key_Left)) {
                const parentItem = control.parent as MD.MenuItem;
                if (parentItem) {
                    control.close();
                    parentItem.forceActiveFocus(Qt.TabFocusReason);
                    event.accepted = true;
                }
            }
        }
        implicitWidth: {
            let preferredWidth = 0;
            for (let index = 0; index < control.count; ++index) {
                const item = control.itemAt(index);
                if (item && (!control.visible || item.visible))
                    preferredWidth = Math.max(preferredWidth, item.implicitWidth);
            }
            return preferredWidth;
        }
        implicitHeight: contentHeight
        model: {
            if (!control.contentDelegate && control.model instanceof DelegateModel) {
                return control.model;
            }
            return control.contentModel;
        }
        interactive: contentHeight + control.topPadding + control.bottomPadding > control.height
        keyNavigationEnabled: false
        MD.ScrollIndicatorBase.vertical: MD.ScrollIndicator {}
    }

    Instantiator {
        id: m_instantiator
        active: control.contentDelegate !== null
        model: control.model

        onObjectAdded: (index, object) => control.insertItem(index, object)
        onObjectRemoved: (index, object) => control.takeItem(control.indexOf(object))
    }

    Instantiator {
        model: control.count
        delegate: QtObject {
            required property int index
            readonly property Item item: {
                const count = control.count;
                return index < count ? control.itemAt(index) : null;
            }
            readonly property MD.MenuItem menuItem: item as MD.MenuItem
            property Connections events: Connections {
                target: menuItem
                function onTriggered() {
                    if (!menuItem.action || (menuItem.action as MD.Action)?.closeMenu || control.autoClose)
                        control.__dismissChain();
                }
                function onHoveredChanged() {
                    if (menuItem.hovered && menuItem.enabled)
                        control.currentIndex = index;
                }
            }
            property Binding itemWidth: Binding {
                target: item
                property: "width"
                value: control.availableWidth
                when: item !== null
            }
            property Binding itemMenu: Binding {
                target: menuItem
                property: "menu"
                value: control
                when: menuItem !== null
            }
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 200
        implicitHeight: 48
        radius: MD.Token.shape.corner.extra_small
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
    }

    MD.Overlay.modal: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    MD.Overlay.modeless: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    MD.MState {
        id: item_state

        elevation: MD.Token.elevation.level2
        textColor: MD.Token.color.on_surface
        backgroundColor: MD.Token.color.surface_container
        supportTextColor: MD.Token.color.on_surface_variant
    }
}
