#pragma once

#include <QSGMaterial>

namespace qml_material::sg
{

class RectangleMaterial : public QSGMaterial {
public:
    RectangleMaterial();

    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
    QSGMaterialType*   type() const override;
    int                compare(const QSGMaterial* other) const override;

    QVector2D aspect      = QVector2D { 1.0, 1.0 };
    float     size        = 0.0;
    QVector4D radius      = QVector4D { 0.0, 0.0, 0.0, 0.0 };
    QColor    color       = Qt::white;
    QColor    shadowColor = Qt::black;
    QVector2D offset;
};

class RectangleShader : public QSGMaterialShader {
public:
    RectangleShader();

    bool updateUniformData(QSGMaterialShader::RenderState& state, QSGMaterial* newMaterial,
                           QSGMaterial* oldMaterial) override;
};

} // namespace qml_material::sg