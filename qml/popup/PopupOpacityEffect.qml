import QtQuick

QtObject {
    readonly property Component effect: Component {
        Item {
            id: effect

            property ShaderEffectSource source

            Binding {
                target: effect.source
                property: "smooth"
                value: true
                when: effect.source !== null
            }

            ShaderEffect {
                property ShaderEffectSource source: effect.source

                x: source?.sourceRect.x ?? 0
                y: source?.sourceRect.y ?? 0
                width: source?.sourceRect.width ?? 0
                height: source?.sourceRect.height ?? 0
            }
        }
    }
}
