#pragma once

#include "qml_material/control/text_field.hpp"
#include "qml_material/input/time_state.hpp"

namespace qml_material
{
/** @ingroup control */
class QML_MATERIAL_API TimeField : public TextField {
    Q_OBJECT
    QML_NAMED_ELEMENT(TimeFieldBase)
    Q_PROPERTY(TimeState* time READ time WRITE setTime NOTIFY configurationChanged FINAL)
    Q_PROPERTY(TimeState::Selection selection READ selection WRITE setSelection NOTIFY
                   configurationChanged FINAL)
public:
    explicit TimeField(QQuickItem* parent = nullptr);
    ~TimeField() override;
    TimeState*           time() const { return m_time; }
    void                 setTime(TimeState*);
    TimeState::Selection selection() const { return m_selection; }
    void                 setSelection(TimeState::Selection);
    Q_SIGNAL void        configurationChanged();
    Q_SIGNAL void        inputRequested();

protected:
    bool usesEditorInput() const override { return ! isReadOnly(); }
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void touchEvent(QTouchEvent*) override;
    void touchUngrabEvent() override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

private:
    void                    selectField();
    QPointer<TimeState>     m_time;
    QMetaObject::Connection m_destroyed;
    TimeState::Selection    m_selection = TimeState::Hours;
    quint64                 m_revision  = 0;
};
} // namespace qml_material
