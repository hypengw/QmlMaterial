import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.StackView {
    id: root

    property var pendingRequest: null
    property var currentRequest: null
    property string currentKey: ""

    signal pageLoadFailed(string error)

    readonly property bool canBack: currentItem?.canBack ?? false

    function back() {
        currentItem.back();
    }

    function handlePendingRequest() {
        const request = root.pendingRequest;
        if (!request)
            return;

        if (request.status === MD.PoolRequest.Ready) {
            const object = request.object;
            root.pendingRequest = null;
            const item = root.replaceCurrentItem(object) as Item;
            if (item !== object) {
                request.release();
                const error = "failed to replace current page";
                root.pageLoadFailed(error);
                console.error(error);
                return;
            }

            root.currentRequest = request;
            root.currentKey = request.cached ? request.key.toString() : "";
            item.T.StackView.removed.connect(request, function() {
                if (root.currentRequest === request) {
                    root.currentRequest = null;
                    root.currentKey = "";
                }
                request.release();
            });
        } else if (request.status === MD.PoolRequest.Error) {
            root.pendingRequest = null;
            root.pageLoadFailed(request.errorString);
            console.error(`failed to load page: ${request.errorString}`);
            request.release();
        } else if (request.status === MD.PoolRequest.Cancelled) {
            root.pendingRequest = null;
        }
    }

    function switchTo(page_url, props, is_cache = true) {
        const key = is_cache ? JSON.stringify({
            "url": page_url,
            "props": props
        }) : "";
        if (is_cache && key === root.currentKey) {
            if (root.pendingRequest)
                root.pendingRequest.cancel();
            return;
        }

        if (root.pendingRequest)
            root.pendingRequest.cancel();

        const request = m_pool.request(page_url, props, is_cache ? key : null,
                                       MD.Pool.AsynchronousIfNested);
        root.pendingRequest = request;
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
