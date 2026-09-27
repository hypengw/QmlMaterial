#include "qml_material/shape/material_shapes.hpp"
#include <cmath>
#include <map>
#include <mutex>
#include <numbers>
#include <stdexcept>
#include <tuple>

namespace qml_material
{
namespace
{
using Vertex      = RoundedPolygon::Vertex;
constexpr auto pi = std::numbers::pi;
QPointF        polar(qreal radius, qreal angle) {
    return { radius * std::cos(angle), radius * std::sin(angle) };
}
RoundedPolygon regular(int n, qreal radius, CornerRounding rounding, qreal innerRadius = -1) {
    std::vector<Vertex> vertices;
    for (int i = 0; i < n; ++i)
        vertices.push_back(
            { polar(innerRadius > 0 && i % 2 ? innerRadius : radius, 2 * pi * i / n), rounding });
    return RoundedPolygon(vertices);
}
RoundedPolygon circle(int n = 8) { return regular(n, 1 / std::cos(pi / n), { 1, 0 }); }
RoundedPolygon rotated(const RoundedPolygon& polygon, qreal angle) {
    return polygon.transformed(QTransform().rotate(angle));
}
RoundedPolygon custom(std::initializer_list<Vertex> points, int repeat, bool mirror = false) {
    const std::vector<Vertex> input(points);
    const QPointF             center(.5, .5);
    const auto                repeats    = repeat * (mirror ? 2 : 1);
    const auto                span       = 2 * pi / repeats;
    const auto                start      = input.front().position - center;
    const auto                startAngle = std::atan2(start.y(), start.x());
    std::vector<Vertex>       vertices;
    for (int i = 0; i < repeats; ++i) {
        const bool reverse = mirror && i % 2;
        for (size_t j = 0; j < input.size(); ++j) {
            const auto index = reverse ? input.size() - j - 1 : j;
            if (reverse && index == 0) continue;
            const auto p     = input[index].position - center;
            const auto angle = std::atan2(p.y(), p.x());
            vertices.push_back(
                { center + polar(std::hypot(p.x(), p.y()),
                                 span * i + (reverse ? span - angle + 2 * startAngle : angle)),
                  input[index].rounding });
        }
    }
    return RoundedPolygon(vertices, center);
}
RoundedPolygon makeShape(MaterialShape::Type shape) {
    using S = MaterialShape;
    switch (shape) {
    case S::Circle: return circle(10);
    case S::Square:
        return RoundedPolygon({ { { .5, .5 }, { .3 } },
                                { { -.5, .5 }, { .3 } },
                                { { -.5, -.5 }, { .3 } },
                                { { .5, -.5 }, { .3 } } });
    case S::Slanted:
        return custom({ { { .926, .970 }, { .189, .811 } }, { { -.021, .967 }, { .187, .057 } } },
                      2);
    case S::Arch:
        return rotated(RoundedPolygon({ { { 1, 0 }, { 1 } },
                                        { { 0, 1 }, { 1 } },
                                        { { -1, 0 }, { .2 } },
                                        { { 0, -1 }, { .2 } } }),
                       -135);
    case S::Fan:
        return custom({ { { 1.004, 1 }, { .148, .417 } },
                        { { 0, 1 }, { .151 } },
                        { { 0, -.003 }, { .148 } },
                        { { .978, .020 }, { .803 } } },
                      1);
    case S::Arrow:
        return custom({ { { .500, .892 }, { .313 } },
                        { { -.216, 1.050 }, { .207 } },
                        { { .499, -.160 }, { .215, 1 } },
                        { { 1.225, 1.060 }, { .211 } } },
                      1);
    case S::SemiCircle:
        return RoundedPolygon({ { { .8, .5 }, { .2 } },
                                { { -.8, .5 }, { .2 } },
                                { { -.8, -.5 }, { 1 } },
                                { { .8, -.5 }, { 1 } } });
    case S::Oval: return rotated(circle().transformed(QTransform().scale(1, .64)), -45);
    case S::Pill:
        return custom(
            { { { .961, .039 }, { .426 } }, { { 1.001, .428 }, {} }, { { 1, .609 }, { 1 } } },
            2,
            true);
    case S::Triangle: return rotated(regular(3, 1, { .2 }), -90);
    case S::Diamond:
        return custom({ { { .500, 1.096 }, { .151, .524 } }, { { .040, .500 }, { .159 } } }, 2);
    case S::ClamShell:
        return custom({ { { .171, .841 }, { .159 } },
                        { { -.020, .500 }, { .140 } },
                        { { .170, .159 }, { .159 } } },
                      2);
    case S::Pentagon:
        return custom({ { { .500, -.009 }, { .172 } },
                        { { 1.030, .365 }, { .164 } },
                        { { .828, .970 }, { .169 } } },
                      1,
                      true);
    case S::Gem:
        return custom({ { { .499, 1.023 }, { .241, .778 } },
                        { { -.005, .792 }, { .208 } },
                        { { .073, .258 }, { .228 } },
                        { { .433, 0 }, { .491 } } },
                      1,
                      true);
    case S::Sunny: return regular(16, 1, { .15 }, .8);
    case S::VerySunny:
        return custom({ { { .500, 1.080 }, { .085 } }, { { .358, .843 }, { .085 } } }, 8);
    case S::Cookie4Sided:
        return custom({ { { 1.237, 1.236 }, { .258 } }, { { .500, .918 }, { .233 } } }, 4);
    case S::Cookie6Sided:
        return custom({ { { .723, .884 }, { .394 } }, { { .500, 1.099 }, { .398 } } }, 6);
    case S::Cookie7Sided: return rotated(regular(14, 1, { .5 }, .75), -90);
    case S::Cookie9Sided: return rotated(regular(18, 1, { .5 }, .8), -90);
    case S::Cookie12Sided: return rotated(regular(24, 1, { .5 }, .8), -90);
    case S::Ghostish:
        return custom({ { { .5, 0 }, { 1 } },
                        { { 1, 0 }, { 1 } },
                        { { 1, 1.140 }, { .254, .106 } },
                        { { .575, .906 }, { .253 } } },
                      1,
                      true);
    case S::Clover4Leaf:
        return custom({ { { .500, .074 }, {} }, { { .725, -.099 }, { .476 } } }, 4, true);
    case S::Clover8Leaf:
        return custom({ { { .500, .036 }, {} }, { { .758, -.101 }, { .209 } } }, 8);
    case S::Burst:
        return custom({ { { .500, -.006 }, { .006 } }, { { .592, .158 }, { .006 } } }, 12);
    case S::SoftBurst:
        return custom({ { { .193, .277 }, { .053 } }, { { .176, .055 }, { .053 } } }, 10);
    case S::Boom:
        return custom({ { { .457, .296 }, { .007 } }, { { .500, -.051 }, { .007 } } }, 15);
    case S::SoftBoom:
        return custom({ { { .733, .454 }, {} },
                        { { .839, .437 }, { .532 } },
                        { { .949, .449 }, { .439, 1 } },
                        { { .998, .478 }, { .174 } } },
                      16,
                      true);
    case S::Flower:
        return custom(
            { { { .370, .187 }, {} }, { { .416, .049 }, { .381 } }, { { .479, .001 }, { .095 } } },
            8,
            true);
    case S::Puffy:
        return custom({ { { .500, .053 }, {} },
                        { { .545, -.040 }, { .405 } },
                        { { .670, -.035 }, { .426 } },
                        { { .717, .066 }, { .574 } },
                        { { .722, .128 }, {} },
                        { { .777, .002 }, { .360 } },
                        { { .914, .149 }, { .660 } },
                        { { .926, .289 }, { .660 } },
                        { { .881, .346 }, {} },
                        { { .940, .344 }, { .126 } },
                        { { 1.003, .437 }, { .255 } } },
                      2,
                      true)
            .transformed(QTransform().scale(1, .742));
    case S::PuffyDiamond:
        return custom(
            { { { .870, .130 }, { .146 } }, { { .818, .357 }, {} }, { { 1, .332 }, { .853 } } },
            4,
            true);
    case S::PixelCircle:
        return custom({ { { .5, 0 }, {} },
                        { { .704, 0 }, {} },
                        { { .704, .065 }, {} },
                        { { .843, .065 }, {} },
                        { { .843, .148 }, {} },
                        { { .926, .148 }, {} },
                        { { .926, .296 }, {} },
                        { { 1, .296 }, {} } },
                      2,
                      true);
    case S::PixelTriangle:
        return custom({ { { .110, .500 }, {} },
                        { { .113, 0 }, {} },
                        { { .287, 0 }, {} },
                        { { .287, .087 }, {} },
                        { { .421, .087 }, {} },
                        { { .421, .170 }, {} },
                        { { .560, .170 }, {} },
                        { { .560, .265 }, {} },
                        { { .674, .265 }, {} },
                        { { .675, .344 }, {} },
                        { { .789, .344 }, {} },
                        { { .789, .439 }, {} },
                        { { .888, .439 }, {} } },
                      1,
                      true);
    case S::Bun:
        return custom({ { { .796, .500 }, {} },
                        { { .853, .518 }, { 1 } },
                        { { .992, .631 }, { 1 } },
                        { { .968, 1 }, { 1 } } },
                      2,
                      true);
    case S::Heart:
        return custom({ { { .500, .268 }, { .016 } },
                        { { .792, -.066 }, { .958 } },
                        { { 1.064, .276 }, { 1 } },
                        { { .501, .946 }, { .129 } } },
                      1,
                      true);
    case S::None: break;
    }
    throw std::invalid_argument("Unknown Material shape");
}
} // namespace

bool MaterialShapes::isValid(MaterialShape::Type shape) {
    return shape >= MaterialShape::Circle && shape <= MaterialShape::Heart;
}
const RoundedPolygon& MaterialShapes::polygon(MaterialShape::Type shape, bool radial) {
    if (! isValid(shape)) throw std::invalid_argument("Unknown Material shape");
    static const auto shapes = [] {
        std::array<std::array<RoundedPolygon, count>, 2> result;
        for (int i = 0; i < count; ++i) {
            const auto polygon = makeShape(static_cast<MaterialShape::Type>(i));
            result[0][i]       = polygon.normalized();
            result[1][i]       = polygon.normalized(true);
        }
        return result;
    }();
    return shapes[radial ? 1 : 0][shape];
}
std::shared_ptr<const Morph> MaterialShapes::morph(MaterialShape::Type from, MaterialShape::Type to,
                                                   bool radial) {
    if (! isValid(from) || ! isValid(to)) throw std::invalid_argument("Unknown Material shape");
    static std::mutex                                                         mutex;
    static std::map<std::tuple<int, int, bool>, std::shared_ptr<const Morph>> cache;
    const std::lock_guard                                                     lock(mutex);
    const auto key   = std::tuple(int(from), int(to), radial);
    auto&      morph = cache[key];
    if (! morph) morph = std::make_shared<Morph>(polygon(from, radial), polygon(to, radial));
    return morph;
}
const std::array<MaterialShape::Type, 7>& MaterialShapes::loadingSequence() {
    static constexpr std::array sequence { MaterialShape::SoftBurst, MaterialShape::Cookie9Sided,
                                           MaterialShape::Pentagon,  MaterialShape::Pill,
                                           MaterialShape::Sunny,     MaterialShape::Cookie4Sided,
                                           MaterialShape::Oval };
    return sequence;
}
} // namespace qml_material
