pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 900
    height: 760
    color: MD.MProp.color.surface
    MD.ClockSource {
        id: clockSource
        mode: MD.ClockSource.Fixed
        fixedDateTime: new Date(Date.UTC(2026, 9, 16, 10, 9, 30))
    }
    Grid {
        anchors.fill: parent
        anchors.margins: 24
        columns: root.width < 600 ? 1 : 2
        spacing: 24
        Repeater {
            model: [
                {
                    preset: MD.ClockBase.ExpressiveAnalog,
                    shape: MD.MaterialShape.Flower,
                    locale: 'zh_CN',
                    family: 'Noto Serif CJK SC'
                },
                {
                    preset: MD.ClockBase.ExpressiveAnalog,
                    shape: MD.MaterialShape.Square,
                    locale: 'en_US',
                    family: 'Noto Sans'
                },
                {
                    preset: MD.ClockBase.Digital,
                    shape: MD.MaterialShape.Circle,
                    locale: 'ar_EG',
                    family: 'Noto Kufi Arabic'
                },
                {
                    preset: MD.ClockBase.StackedDigital,
                    shape: MD.MaterialShape.Circle,
                    locale: 'zh_CN',
                    family: 'Noto Serif CJK SC'
                }
            ]
            delegate: MD.Clock {
                required property var modelData
                width: (root.width - 48 - (parent.columns - 1) * parent.spacing) / parent.columns
                height: 332
                source: clockSource
                preset: modelData.preset
                shape: modelData.shape
                locale: Qt.locale(modelData.locale)
                layoutDirection: modelData.locale === 'ar_EG' ? Qt.RightToLeft : Qt.LeftToRight
                font.family: modelData.family
                timeZoneId: 'UTC'
                showSeconds: true
                showDate: true
            }
        }
    }
}
