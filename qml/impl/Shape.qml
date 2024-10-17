import QtQuick.Shapes

Shape {
    asynchronous: true
    preferredRendererType: Shape.CurveRenderer

    layer.enabled: false
    layer.mipmap: true
    layer.smooth: true
    layer.samples: 8
}