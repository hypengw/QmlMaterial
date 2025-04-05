pragma Singleton
pragma ComponentBehavior: Bound
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
        property string icon_round_family
        property string icon_round_fill_family
        // property font icon_outline
    }

    property FontComp font: FontComp {
        default_font: Qt.application.font
        icon_round_family: ''
        icon_round_fill_family: ''
    }

    Loader {
        sourceComponent: root.iconFontUrl ? m_material_round_vf : m_material_round_static
    }

    Component {
        id: m_material_round_vf
        FontLoader {
            source: root.iconFontUrl
            onStatusChanged: if (status == FontLoader.Ready) {
                root.font.icon_round = font;
                root.font.icon_round_fill = font;
            }
        }
    }

    Component {
        id: m_material_round_static
        Item {
            FontLoader {
                source: 'qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2'
                onStatusChanged: if (status == FontLoader.Ready) {
                    root.font.icon_round = font.family;
                }
            }
            FontLoader {
                source: 'qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_1.ttf'
                onStatusChanged: if (status == FontLoader.Ready) {
                    root.font.icon_round_fill = font.family;
                }
            }
        }
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
