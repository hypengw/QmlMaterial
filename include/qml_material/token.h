#pragma once

#include <QQmlEngine>
#include <QFont>
#include "core/core.h"
#include "qml_material/type_scale.h"

namespace qml_material::token
{
struct Elevation {
    Q_GADGET
    QML_VALUE_TYPE(t_token_elevation)
    Q_PROPERTY(qint32 level0 MEMBER level0 CONSTANT FINAL)
    Q_PROPERTY(qint32 level1 MEMBER level1 CONSTANT FINAL)
    Q_PROPERTY(qint32 level2 MEMBER level2 CONSTANT FINAL)
    Q_PROPERTY(qint32 level3 MEMBER level3 CONSTANT FINAL)
    Q_PROPERTY(qint32 level4 MEMBER level4 CONSTANT FINAL)
    Q_PROPERTY(qint32 level5 MEMBER level5 CONSTANT FINAL)
public:
    i32 level0 { 0 };
    i32 level1 { 1 };
    i32 level2 { 3 };
    i32 level3 { 6 };
    i32 level4 { 8 };
    i32 level5 { 12 };
};

struct ShapeCorner {
    Q_GADGET
    QML_VALUE_TYPE(t_token_shape_corner)
    Q_PROPERTY(qint32 none MEMBER none CONSTANT FINAL)
    Q_PROPERTY(qint32 extra_small MEMBER extra_small CONSTANT FINAL)
    Q_PROPERTY(qint32 small MEMBER small CONSTANT FINAL)
    Q_PROPERTY(qint32 medium MEMBER medium CONSTANT FINAL)
    Q_PROPERTY(qint32 large MEMBER large CONSTANT FINAL)
    Q_PROPERTY(qint32 extra_large MEMBER extra_large CONSTANT FINAL)
    Q_PROPERTY(qint32 full MEMBER full CONSTANT FINAL)
public:
    // dp
    i32 none { 0 };
    i32 extra_small { 4 };
    i32 small { 8 };
    i32 medium { 12 };
    i32 large { 16 };
    i32 extra_large { 28 };
    i32 full { 100 };
};

struct Shape {
    Q_GADGET
    QML_VALUE_TYPE(t_token_shape)
    Q_PROPERTY(ShapeCorner corner MEMBER corner CONSTANT FINAL)
public:
    ShapeCorner corner;
};

struct StateItem {
    Q_GADGET
    Q_PROPERTY(double state_layer_opacity MEMBER state_layer_opacity CONSTANT FINAL)
public:
    double state_layer_opacity;
};

struct State {
    Q_GADGET
    QML_VALUE_TYPE(t_token_state)
    Q_PROPERTY(StateItem hover MEMBER hover CONSTANT FINAL)
    Q_PROPERTY(StateItem pressed MEMBER pressed CONSTANT FINAL)
    Q_PROPERTY(StateItem focus MEMBER focus CONSTANT FINAL)

public:
    StateItem hover { 0.08 };
    StateItem pressed { 0.1 };
    StateItem focus { 0.1 };
};

class Token : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> datas READ datas FINAL)
    Q_CLASSINFO("DefaultProperty", "datas")
    QML_NAMED_ELEMENT(TokenImpl)

    Q_PROPERTY(TypeScale* typescale READ typescale CONSTANT FINAL)
    Q_PROPERTY(Elevation elevation READ elevation CONSTANT FINAL)
    Q_PROPERTY(State state READ state CONSTANT FINAL)
    Q_PROPERTY(Shape shape READ shape CONSTANT FINAL)
public:
    Token(QObject* = nullptr);
    ~Token();

    auto typescale() const -> TypeScale*;
    auto elevation() const -> const Elevation&;
    auto state() const -> const State&;
    auto shape() const -> const Shape&;

    auto datas() -> QQmlListProperty<QObject>;

private:
    TypeScale* m_typescale;
    Elevation  m_elevation;
    State      m_state;
    Shape      m_shape;

    QList<QObject*> m_datas;
};
} // namespace qml_material::token