#include "qml_material/token.h"

namespace qml_material::token
{
Token::Token(QObject* parent): QObject(parent), m_typescale(new TypeScale(this)) {}
Token::~Token() {}
auto Token::typescale() const -> TypeScale* { return m_typescale; }
auto Token::elevation() const -> const Elevation& { return m_elevation; }
auto Token::state() const -> const State& { return m_state; }
auto Token::shape() const -> const Shape& { return m_shape; }

auto Token::datas() -> QQmlListProperty<QObject> { return { this, &m_datas }; }
} // namespace qml_material::token