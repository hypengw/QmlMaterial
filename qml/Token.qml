pragma Singleton
import QtQml
import QtQuick
import Qcm.Material as MD

MD.TokenImpl {
    id: root
    readonly property MD.MdColorMgr color: root.MD.MatProp.color
    property int theme: root.color.colorScheme
    readonly property bool is_dark_theme: Number(theme) == MD.MdColorMgr.Dark

    component FontComp: QtObject {
        property font default_font
        property font icon_round
        property font icon_outline
    }

    readonly property FontComp font: FontComp {
        default_font: Qt.application.font
        icon_round: m_fontload_material_round.font
    }

    FontLoader {
        id: m_fontload_material_round
        source: 'qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.ttf'
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
