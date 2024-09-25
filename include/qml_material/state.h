#pragma once

#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include "qml_material/theme.h"

namespace qml_material
{

class State : public QQuickItem  {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* item READ item WRITE set_item NOTIFY itemChanged)
    Q_PROPERTY(Theme* ctx READ ctx NOTIFY ctxChanged FINAL)
    Q_PROPERTY(qint32 elevation READ elevation WRITE set_elevation NOTIFY elevationChanged FINAL)

    Q_PROPERTY(QColor textColor READ text_color WRITE set_text_color NOTIFY textColorChanged FINAL)
    Q_PROPERTY(QColor backgroundColor READ background_color WRITE set_background_color NOTIFY
                   backgroundColorChanged FINAL)
    Q_PROPERTY(QColor supportTextColor READ support_text_color WRITE set_support_text_color NOTIFY
                   supportTextColorChanged FINAL)
    Q_PROPERTY(QColor stateLayerColor READ state_layer_color WRITE set_state_layer_color NOTIFY
                   stateLayerColorChanged FINAL)

    Q_PROPERTY(double stateLayerOpacity READ state_layer_opacity WRITE set_state_layer_opacity
                   NOTIFY stateLayerOpacityChanged FINAL)
    Q_PROPERTY(double contentOpacity READ content_opacity WRITE set_content_opacity NOTIFY
                   contentOpacityChanged FINAL)
    Q_PROPERTY(double backgroundOpacity READ background_opacity WRITE set_background_opacity NOTIFY
                   backgroundOpacityChanged FINAL)

public:
    State(QQuickItem* parent = nullptr);
    ~State();
    void classBegin() override;
    void componentComplete() override;
    auto ctx() const -> Theme*;

    auto item() const -> QObject*;
    auto elevation() const -> qint32;
    auto text_color() const -> QColor;
    auto background_color() const -> QColor;
    auto support_text_color() const -> QColor;
    auto state_layer_color() const -> QColor;
    auto state_layer_opacity() const -> double;
    auto content_opacity() const -> double;
    auto background_opacity() const -> double;

    void set_item(QObject*);
    void set_elevation(qint32);
    void set_text_color(QColor);
    void set_background_color(QColor);
    void set_support_text_color(QColor);
    void set_state_layer_color(QColor);
    void set_state_layer_opacity(double);
    void set_content_opacity(double);
    void set_background_opacity(double);

Q_SIGNALS:
    void itemChanged();
    void ctxChanged();
    void elevationChanged();
    void textColorChanged();
    void backgroundColorChanged();
    void supportTextColorChanged();
    void stateLayerColorChanged();
    void stateLayerOpacityChanged();
    void contentOpacityChanged();
    void backgroundOpacityChanged();

private Q_SLOTS:
    void on_item_changed();

private:
    QObject* m_item;
    qint32   m_elevation;
    QColor   m_text_color;
    QColor   m_background_color;
    QColor   m_support_text_color;
    QColor   m_state_layer_color;
    double   m_state_layer_opacity;
    double   m_content_opacity;
    double   m_background_opacity;
};

class StateHolder : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(State* state READ state WRITE setState NOTIFY stateChanged FINAL)
public:
    StateHolder(QObject* parent = nullptr);
    ~StateHolder();

    auto state() const -> State*;

public Q_SLOTS:
    void setState(State*);

Q_SIGNALS:
    void stateChanged();

private:
    State* m_state;
};

} // namespace qml_material