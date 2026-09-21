import QtQuick
import Qcm.Material as MD

/** @ingroup control */
MD.PageStackBase {
    popEnter: MD.FadeInThroughMotion {}
    popExit: MD.FadeOutThroughMotion {}
    pushEnter: MD.FadeInThroughMotion {}
    pushExit: MD.FadeOutThroughMotion {}
    replaceEnter: MD.FadeInThroughMotion {}
    replaceExit: MD.FadeOutThroughMotion {}
}
