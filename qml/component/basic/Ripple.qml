import QtQuick

// Active ripple implementation. All Material controls go through this so
// switching the look (Skia "realistic" sparkle ↔ Shape/RadialGradient
// "classic") is a single-file edit. Public API matches both backings:
//   stateOpacity, pressed, pressX, pressY, color, corners, radius
//
// Currently routed to: RippleSkia (Material You "realistic" ripple)
// To use the classic variant instead, replace `RippleSkia` with `RippleShape`.
RippleSkia {
}
