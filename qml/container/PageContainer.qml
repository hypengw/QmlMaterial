import QtQuick
import Qcm.Material as MD

MD.PageStack {
    id: root

    property var pendingRequest: null
    property var currentRequest: null
    property string currentKey: ""

    signal pageLoadFailed(string error)

    readonly property bool canBack: currentItem?.canBack ?? false

    QtObject {
        id: d
        property var leases: ({})
        property var accepting: null
        property bool changing: false
    }

    onEntryAdded: (id, item) => {
        const request = d.accepting;
        if (!request || request.object !== item)
            return;
        d.leases[id] = request;
        root.currentRequest = request;
        root.currentKey = request.cached ? request.key.toString() : "";
    }

    onEntryRemoved: id => {
        const request = d.leases[id];
        delete d.leases[id];
        if (!request)
            return;
        const changing = d.changing;
        d.changing = true;
        try {
            if (root.currentRequest === request) {
                root.currentRequest = null;
                root.currentKey = "";
            }
            request.release();
        } finally {
            d.changing = changing;
        }
    }

    function back() {
        currentItem.back();
    }

    function handlePendingRequest() {
        if (d.changing) {
            Qt.callLater(root.handlePendingRequest);
            return;
        }
        const request = root.pendingRequest;
        if (!request)
            return;

        if (request.status === MD.PoolRequest.Ready) {
            d.changing = true;
            try {
                const object = request.object as Item;
                root.pendingRequest = null;
                d.accepting = request;
                const accepted = object && root.replaceCurrentItem(object);
                if (!accepted || root.currentRequest !== request) {
                    request.release();
                    const error = "failed to replace current page";
                    root.pageLoadFailed(error);
                    console.error(error);
                    return;
                }
            } finally {
                d.accepting = null;
                d.changing = false;
            }
        } else if (request.status === MD.PoolRequest.Error) {
            d.changing = true;
            try {
                root.pendingRequest = null;
                root.pageLoadFailed(request.errorString);
                console.error(`failed to load page: ${request.errorString}`);
                request.release();
            } finally {
                d.changing = false;
            }
        } else if (request.status === MD.PoolRequest.Cancelled) {
            root.pendingRequest = null;
        }
    }

    function switchTo(page_url, props, is_cache = true) {
        if (d.changing) {
            Qt.callLater(root.switchTo, page_url, props, is_cache);
            return;
        }
        d.changing = true;
        try {
            const key = is_cache ? JSON.stringify({
                "url": page_url,
                "props": props
            }) : "";
            if (is_cache && key === root.currentKey) {
                if (root.pendingRequest) {
                    const previous = root.pendingRequest;
                    root.pendingRequest = null;
                    previous.cancel();
                }
                return;
            }

            if (root.pendingRequest) {
                const previous = root.pendingRequest;
                root.pendingRequest = null;
                previous.cancel();
            }

            root.completeTransition();
            const request = m_pool.request(page_url, props, is_cache ? key : null,
                                           MD.Pool.AsynchronousIfNested);
            root.pendingRequest = request;
        } finally {
            d.changing = false;
        }
        root.handlePendingRequest();
    }

    Connections {
        target: root.pendingRequest

        function onStatusChanged() {
            root.handlePendingRequest();
        }
    }

    MD.Pool {
        id: m_pool
    }
}
