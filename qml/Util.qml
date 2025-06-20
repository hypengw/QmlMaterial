pragma Singleton
import QtQuick
import Qcm.Material as MD

MD.UtilCpp {
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
}
