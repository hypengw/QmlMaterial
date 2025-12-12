#pragma once

#include <QSGMaterial>
#include <QVector2D>
#include <QVector4D>

namespace qml_material::sg
{

class RectangleMaterial : public QSGMaterial {
public:
    RectangleMaterial();

    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
    QSGMaterialType*   type() const override;
    int                compare(const QSGMaterial* other) const override;

    void setSize(const QVector2D& s) { m_size = s; }
    void setRadius(const QVector4D& r) { m_radius = r; }

    auto size() const noexcept -> const QVector2D& { return m_size; }
    auto radius() const noexcept -> const QVector4D& { return m_radius; }

private:
    QVector2D m_size { 0, 0 };
    QVector4D m_radius { 0, 0, 0, 0 }; // tl,tr,bl,br
};

class RectangleShader : public QSGMaterialShader {
public:
    RectangleShader();

    bool updateUniformData(QSGMaterialShader::RenderState& state, QSGMaterial* newMaterial,
                           QSGMaterial* oldMaterial) override;

};

} // namespace qml_material::sg
