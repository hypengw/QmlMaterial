#pragma once

#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include "qml_material/theme.hpp"

namespace qml_material
{

class State : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(MStateImpl)
    Q_PROPERTY(QObject* target READ target WRITE set_target NOTIFY targetChanged FINAL)
    Q_PROPERTY(qml_material::Theme* ctx READ ctx WRITE setCtx NOTIFY ctxChanged FINAL)
    Q_PROPERTY(qint32 elevation READ elevation WRITE set_elevation NOTIFY elevationChanged FINAL)

    Q_PROPERTY(QColor textColor READ text_color WRITE set_text_color NOTIFY textColorChanged FINAL)
    Q_PROPERTY(QColor backgroundColor READ background_color WRITE set_background_color NOTIFY
                   backgroundColorChanged FINAL)
    Q_PROPERTY(QColor outlineColor READ outline_color WRITE set_outline_color NOTIFY
                   outlineColorChanged FINAL)
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
    void        classBegin() override;
    void        componentComplete() override;
    auto        ctx() const -> Theme*;
    Q_SLOT void setCtx(Theme*);

    auto target() const noexcept -> QObject*;
    auto elevation() const noexcept -> qint32;
    auto text_color() const noexcept -> QColor;
    auto outline_color() const noexcept -> QColor;
    auto background_color() const noexcept -> QColor;
    auto support_text_color() const noexcept -> QColor;
    auto state_layer_color() const noexcept -> QColor;
    auto state_layer_opacity() const noexcept -> double;
    auto content_opacity() const noexcept -> double;
    auto background_opacity() const noexcept -> double;

    void set_target(QObject*);
    void set_elevation(qint32);
    void set_text_color(QColor);
    void set_outline_color(QColor);
    void set_background_color(QColor);
    void set_support_text_color(QColor);
    void set_state_layer_color(QColor);
    void set_state_layer_opacity(double);
    void set_content_opacity(double);
    void set_background_opacity(double);

    Q_SIGNAL void targetChanged();
    Q_SIGNAL void ctxChanged();
    Q_SIGNAL void elevationChanged();
    Q_SIGNAL void textColorChanged();
    Q_SIGNAL void outlineColorChanged();
    Q_SIGNAL void backgroundColorChanged();
    Q_SIGNAL void supportTextColorChanged();
    Q_SIGNAL void stateLayerColorChanged();
    Q_SIGNAL void stateLayerOpacityChanged();
    Q_SIGNAL void contentOpacityChanged();
    Q_SIGNAL void backgroundOpacityChanged();

private:
    Q_SLOT void updateCtx();

private:
    QObject* m_target;
    Theme*   m_ctx;
    qint32   m_elevation;
    QColor   m_text_color;
    QColor   m_outline_color;
    QColor   m_background_color;
    QColor   m_support_text_color;
    QColor   m_state_layer_color;
    double   m_state_layer_opacity;
    double   m_content_opacity;
    double   m_background_opacity;
};

} // namespace qml_material