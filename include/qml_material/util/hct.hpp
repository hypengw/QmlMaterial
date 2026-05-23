#pragma once

#include <QtQml/QQmlEngine>
#include <QtGui/QColor>

#include "qml_material/export.hpp"

namespace qml_material
{

class QML_MATERIAL_API Hct : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(double hue READ hue WRITE setHue NOTIFY hctChanged FINAL)
    Q_PROPERTY(double chroma READ chroma WRITE setChroma NOTIFY hctChanged FINAL)
    Q_PROPERTY(double tone READ tone WRITE setTone NOTIFY hctChanged FINAL)

public:
    explicit Hct(QObject* parent = nullptr);
    ~Hct() override;

    QColor color() const noexcept;
    double hue() const noexcept;
    double chroma() const noexcept;
    double tone() const noexcept;

    void setColor(QColor);
    void setHue(double);
    void setChroma(double);
    void setTone(double);

    Q_SIGNAL void colorChanged();
    Q_SIGNAL void hctChanged();

private:
    void rebuildFromHct();
    void rebuildFromColor(QColor);

    QColor m_color;
    double m_hue { 0.0 };
    double m_chroma { 0.0 };
    double m_tone { 0.0 };
    bool   m_blocked { false };
};

class QML_MATERIAL_API HctUtil : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit HctUtil(QObject* parent = nullptr);

    Q_INVOKABLE QColor fromHct(double hue, double chroma, double tone) const;
    Q_INVOKABLE QColor paletteTone(QColor seed, double tone) const;
    Q_INVOKABLE double hueOf(QColor c) const;
    Q_INVOKABLE double chromaOf(QColor c) const;
    Q_INVOKABLE double toneOf(QColor c) const;
};

} // namespace qml_material
