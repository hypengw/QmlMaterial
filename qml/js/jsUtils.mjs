

export function epsilonEqual(x, y) {
  return Math.abs(x - y) < Number.EPSILON;
}

export function findLast(arr, fun) {
  let i = arr.length;
  for (; i >= 0; i--) {
    const el = arr[i];
    if (fun(el)) return el;
  }
  return undefined;
}