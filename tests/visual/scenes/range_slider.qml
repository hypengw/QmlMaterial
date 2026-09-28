import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 640
    height: 480
    color: "#fff8ff"
    Column {
        x: 32
        y: 40
        spacing: 28
        MD.RangeSlider {
            width: 450
            first.value: 0.2
            second.value: 0.8
        }
        MD.RangeSlider {
            width: 450
            from: 0
            to: 100
            stepSize: 10
            first.value: 20
            second.value: 70
            labelBehavior: MD.Enum.SliderLabelVisible
        }
        MD.RangeSlider {
            width: 450
            layoutDirection: Qt.RightToLeft
            first.value: 0.1
            second.value: 0.6
        }
        MD.RangeSlider {
            width: 450
            enabled: false
            first.value: 0.3
            second.value: 0.7
        }
        MD.RangeSlider {
            width: 450
            first.value: 0.5
            second.value: 0.5
        }
        MD.Slider {
            width: 450
            value: 0.4
        }
    }
    MD.RangeSlider {
        x: 540
        y: 40
        height: 360
        orientation: Qt.Vertical
        first.value: 0.2
        second.value: 0.8
    }
}
