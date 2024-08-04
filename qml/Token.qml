pragma Singleton
import QtCore
import QtQml
import QtQuick
import Qcm.Material as MD

MD.TokenImpl {
    id: root
    readonly property QtObject color: root.MD.MatProp.color
    property int theme: root.color.colorScheme
    readonly property var icon: MD.IconToken.codeMap
    readonly property bool is_dark_theme: Number(theme) == MD.MdColorMgr.Dark

    readonly property QtObject font: QtObject {
        readonly property var default_font: Qt.application.font
        readonly property FontLoader fontload_material_outline: FontLoader {
            source: 'qrc:/Qcm/Material/assets/MaterialIconsOutlined-Regular.otf'
        }
        readonly property FontLoader fontload_material_round: FontLoader {
            source: 'qrc:/Qcm/Material/assets/MaterialIconsRound-Regular.otf'
        }
        readonly property var icon_outline: this.fontload_material_outline.font
        readonly property var icon_round: this.fontload_material_round.font
    }

    // seems icon font size need map
    function ic_size(s) {
        switch (Math.floor(s)) {
        case 24:
            return 18;
        default:
            return s;
        }
    }
    function toMatTheme(th, inverse = false) {
        function fn_inverse(v, iv) {
            return iv ? !v : v;
        }
        return fn_inverse(th === MdColorMgr.Dark, inverse) ? Material.Dark : Material.Light;
    }
}
