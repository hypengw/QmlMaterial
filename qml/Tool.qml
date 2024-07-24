pragma Singleton
import QtQuick
import Qcm.Material as MD

Item {
    id: root

    function create_item(url_or_comp, props, parent) {
        const com = (url_or_comp instanceof Component) ? url_or_comp : Qt.createComponent(url_or_comp);
        if (com.status === Component.Ready) {
            try {
                return com.createObject(parent, props);
            } catch (e) {
                console.error(e);
            }
        } else if (com.status == Component.Error) {
            console.error(com.errorString());
        }
    }

    function show_popup(url, props, parent, open_and_destry = true) {
        const popup = create_item(url, props, parent);
        if (open_and_destry) {
            popup.closed.connect(() => {
                if (popup.destroy)
                    popup.destroy(1000);
            });
            popup.open();
        }
        return popup;
    }
}
