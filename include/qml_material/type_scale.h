#pragma once

#include <QQmlEngine>
#include <QFont>
#include "qml_material/core.h"

namespace qml_material
{
//  Font.Thin	0
//  Font.ExtraLight	12
//  Font.Light	25
//  Font.Normal	50
//  Font.Medium	57
//  Font.DemiBold	63
//  Font.Bold	75
//  Font.ExtraBold	81
//  Font.Black	87

// Value 	Common weight name
// 100 	Thin (Hairline)
// 200 	Extra Light (Ultra Light)
// 300 	Light
// 400 	Normal (Regular)
// 500 	Medium
// 600 	Semi Bold (Demi Bold)
// 700 	Bold
// 800 	Extra Bold (Ultra Bold)
// 900 	Black (Heavy)
// 950 	Extra Black (Ultra Black)
struct TypeScaleItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(t_typescale)

    Q_PROPERTY(qint32 size MEMBER size CONSTANT FINAL)
    Q_PROPERTY(qint32 line_height MEMBER line_height CONSTANT FINAL)
    Q_PROPERTY(qint32 weight MEMBER weight CONSTANT FINAL)
    Q_PROPERTY(qint32 weight_prominent MEMBER weight_prominent CONSTANT FINAL)
    Q_PROPERTY(qreal tracking MEMBER tracking CONSTANT FINAL)
public:
    Q_INVOKABLE TypeScaleItem fresh() const { return *this; }

    i32           size;
    i32           line_height;
    QFont::Weight weight;
    QFont::Weight weight_prominent;
    qreal         tracking;
};

class TypeScale : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    using QObject::QObject;

#define X(NAME, ...)                                                       \
    Q_PROPERTY(qml_material::TypeScaleItem NAME READ NAME NOTIFY typescaleChanged FINAL) \
public:                                                                    \
    TypeScaleItem NAME() const { return m_##NAME; }                        \
                                                                           \
private:                                                                   \
    TypeScaleItem m_##NAME { __VA_ARGS__ };
    // clang-format off
    X(display_large  , 57, 64, QFont::Normal, QFont::Normal, -0.25)
    X(display_medium , 45, 52, QFont::Normal, QFont::Normal, 0.0  )
    X(display_small  , 36, 44, QFont::Normal, QFont::Normal, 0.0  )
    X(headline_large , 32, 40, QFont::Medium, QFont::Medium, 0.0  )
    X(headline_medium, 28, 36, QFont::Medium, QFont::Medium, 0.0  )
    X(headline_small , 24, 32, QFont::Medium, QFont::Medium, 0.0  )
    X(title_large    , 22, 28, QFont::Normal, QFont::Normal, 0.0  )
    X(title_medium   , 16, 24, QFont::Medium, QFont::Medium, 0.15 )
    X(title_small    , 14, 20, QFont::Medium, QFont::Medium, 0.1  )
    X(body_large     , 16, 24, QFont::Normal, QFont::Normal, 0.5  )
    X(body_medium    , 14, 20, QFont::Normal, QFont::Normal, 0.25 )
    X(body_small     , 12, 16, QFont::Normal, QFont::Normal, 0.4  )
    X(label_large    , 14, 20, QFont::Medium, QFont::Bold  , 0.1  )
    X(label_medium   , 12, 16, QFont::Medium, QFont::Bold  , 0.5  )
    X(label_small    , 11, 16, QFont::Medium, QFont::Bold  , 0.5  )
    // clang-format on

#undef X

Q_SIGNALS:
    void typescaleChanged();
};

} // namespace qml_material