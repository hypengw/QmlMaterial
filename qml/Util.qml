pragma Singleton
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
}
