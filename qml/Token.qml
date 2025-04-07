pragma Singleton
pragma ComponentBehavior: Bound
import QtQml
import QtQuick
import Qcm.Material as MD

/**
 * @brief Matieral Token singleton
 */
MD.TokenImpl {
    id: root

    /**
     * @brief global color token
     */
    readonly property MD.MdColorMgr color: root.MD.MatProp.color
    property MD.Enum.ThemeMode themeMode: root.color.mode
    readonly property bool isDarkTheme: themeMode == MD.Enum.Dark

    component FontComp: QtObject {
        property font default_font
        property string icon_family
        property string icon_fill_family
        property bool is_vf_icon
        // property font icon_outline
    }

    property FontComp font: FontComp {
        default_font: Qt.application.font
        is_vf_icon: false
        icon_family: ''
        icon_fill_family: ''
    }

    Loader {
        sourceComponent: root.iconFontUrl ? m_material_round_vf : m_material_round_static
    }

    Component {
        id: m_material_round_vf
        FontLoader {
            source: root.iconFontUrl
            function init() {
                if (status == FontLoader.Ready) {
                    root.font.icon_family = font.family;
                    root.font.icon_fill_family = font.family;
                    root.font.is_vf_icon = true;
                }
                console.error(root.font.icon_family);
            }
            onStatusChanged: init()
            Component.onCompleted: init()
        }
    }

    Component {
        id: m_material_round_static
        Item {
            FontLoader {
                source: 'qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.ttf'
                Component.onCompleted: {
                    root.font.icon_family = font.family;
                }
            }
            FontLoader {
                source: 'qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_1.ttf'
                Component.onCompleted: {
                    root.font.icon_fill_family = font.family;
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
