pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQml.Models

import Qcm.Material as MD

T.Menu {
    id: control

    property alias mdState: item_state
    property bool autoClose: false
    property real maximumWidth: 280

    implicitWidth: {
        const preferredWidth = Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                        implicitContentWidth + leftPadding + rightPadding);
        return maximumWidth > 0 ? Math.min(preferredWidth, maximumWidth) : preferredWidth;
    }
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    margins: 0
    verticalPadding: 8

    transformOrigin: !cascade ? Item.Top : (mirrored ? Item.TopRight : Item.TopLeft)

    delegate: MD.MenuItem {
        id: m_item

        function clickedCB() {
            if ((action as MD.Action)?.closeMenu || control.autoClose) {
                triggered();
            }
        }

        Component.onCompleted: {
            MD.Util.disconnectAll(m_item, "clicked()");
            m_item.clicked.connect(clickedCB);
        }
    }

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
        // Qt 6.8-6.11 do not propagate menu item implicit widths to the content item.
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
        T.ScrollIndicator.vertical: MD.ScrollIndicator {}
    }

    Instantiator {
        id: m_instantiator
        active: control.contentDelegate !== null
        model: control.model

        onObjectAdded: (index, object) => control.insertItem(index, object)
        onObjectRemoved: (index, object) => control.removeItem(object)
    }

    background: MD.ElevationRectangle {
        implicitWidth: 200
        implicitHeight: 48
        radius: MD.Token.shape.corner.extra_small
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
    }

    T.Overlay.modal: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    T.Overlay.modeless: Rectangle {
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
