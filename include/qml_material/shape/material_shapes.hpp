#pragma once

#include <QtCore/QObject>
#include <QtQml/qqmlregistration.h>
#include <array>
#include <memory>
#include "qml_material/shape/rounded_polygon.hpp"

namespace qml_material
{

class QML_MATERIAL_API MaterialShape : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("MaterialShape provides named shape values")
public:
    enum Type
    {
        None = -1,
        Circle,
        Square,
        Slanted,
        Arch,
        Fan,
        Arrow,
        SemiCircle,
        Oval,
        Pill,
        Triangle,
        Diamond,
        ClamShell,
        Pentagon,
        Gem,
        Sunny,
        VerySunny,
        Cookie4Sided,
        Cookie6Sided,
        Cookie7Sided,
        Cookie9Sided,
        Cookie12Sided,
        Ghostish,
        Clover4Leaf,
        Clover8Leaf,
        Burst,
        SoftBurst,
        Boom,
        SoftBoom,
        Flower,
        Puffy,
        PuffyDiamond,
        PixelCircle,
        PixelTriangle,
        Bun,
        Heart
    };
    Q_ENUM(Type)
};

class QML_MATERIAL_API MaterialShapes {
public:
    static constexpr int                count = 35;
    static bool                         isValid(MaterialShape::Type shape);
    static const RoundedPolygon&        polygon(MaterialShape::Type shape, bool radial = false);
    static std::shared_ptr<const Morph> morph(MaterialShape::Type from, MaterialShape::Type to,
                                              bool radial = false);
    static const std::array<MaterialShape::Type, 7>& loadingSequence();
};

} // namespace qml_material
