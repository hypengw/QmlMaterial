pragma Singleton
import QtQuick
import QtQuick.Templates as T
import QtQuick.Window
import Qcm.Material as MD

MD.UtilCpp {
    id: root

    function overlayChildOf(item) {
        if (!item || !item.children)
            return null;
        for (let i = 0; i < item.children.length; ++i) {
            const ch = item.children[i];
            if (ch instanceof T.Overlay)
                return ch;
        }
        return null;
    }

    function overlayFor(anchorItem) {
        if (!anchorItem)
            return null;
        let p = anchorItem;
        let top = null;
        while (p) {
            if (p.overlay)
                return p.overlay;
            top = p;
            p = p.parent;
        }
        const fromTree = overlayChildOf(top);
        if (fromTree)
            return fromTree;
        if (anchorItem instanceof Item) {
            const win = anchorItem.Window.window;
            if (win && win.contentItem) {
                let root = win.contentItem;
                while (root.parent)
                    root = root.parent;
                return overlayChildOf(root);
            }
        }
        return null;
    }

    function attachOverlay(popup, anchorItem) {
        if (!popup || !anchorItem)
            return null;
        const overlay = overlayFor(anchorItem);
        if (!overlay)
            return null;
        if (popup.parent !== overlay)
            popup.parent = overlay;
        return overlay;
    }

    function epsilonEqual(x: real, y: real): real {
        return Math.abs(x - y) < Number.EPSILON;
    }

    function findLast(arr, fun) {
        let i = arr.length;
        for (; i >= 0; i--) {
            const el = arr[i];
            if (fun(el))
                return el;
        }
        return undefined;
    }

    function removeObj(arr: list<QtObject>, obj: QtObject) {
        const idx = arr.indexOf(obj);
        if (idx >= 0)
            arr.splice(idx, 1);
    }

    function clamp(number: real, lower: real, upper: real): real {
        number = number < lower ? lower : number;
        number = number > upper ? upper : number;

        return number;
    }

    function printJsObj(obj: var) {
        const visited = new Set();
        let current = obj;

        while (current !== null) {
            Object.getOwnPropertyNames(current).forEach(prop => {
                if (!visited.has(prop)) {
                    visited.add(prop);
                    try {
                        const value = obj[prop];
                        console.log(`${prop}:`, value);
                    } catch (e) {
                        console.log(`${prop}: <unable to access>`);
                    }
                }
            });
            current = Object.getPrototypeOf(current);
        }
    }

    function closeMenuOn(obj: QtObject) {
        if (obj instanceof MD.Action) {
            const parent = root.getParent(obj);
            (parent as T.Menu)?.close();
        } else if (obj instanceof T.Menu) {
            (obj as T.Menu)?.close();
        }
    }

    function applyTypescale(item: Item, typescale: MD.typescale, prominent: bool) {
        if (item instanceof T.Control) {
            const c = item as T.Control;
            c.font.pixelSize = typescale.size;
            c.font.weight = prominent ? typescale.weight_prominent : typescale.weight;
            c.font.letterSpacing = typescale.tracking;
        } else if (item instanceof Text) {
            const l = item as Text;
            l.font.pixelSize = typescale.size;
            l.font.weight = prominent ? typescale.weight_prominent : typescale.weight;
            l.font.letterSpacing = typescale.tracking;
        }
    }
}
