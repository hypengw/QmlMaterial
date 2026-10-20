#pragma once

#include "qml_material/control/action.hpp"
#include "qml_material/control/command_button.hpp"
#include "qml_material/control/panel.hpp"

Q_MOC_INCLUDE("qml_material/model/snake_model.hpp")

namespace qml_material
{
class SnakeModel;
class QML_MATERIAL_API SnakeBar : public Panel {
    Q_OBJECT
    QML_NAMED_ELEMENT(SnakeBarBase)
    Q_PROPERTY(Action* action READ action WRITE setAction NOTIFY actionChanged FINAL)
    Q_PROPERTY(CommandButton* actionControl READ actionControl WRITE setActionControl NOTIFY
                   actionControlChanged FINAL)
    Q_PROPERTY(CommandButton* iconControl READ iconControl WRITE setIconControl NOTIFY
                   iconControlChanged FINAL)
    Q_PROPERTY(
        SnakeModel* snakeModel READ snakeModel WRITE setSnakeModel NOTIFY snakeModelChanged FINAL)
    Q_PROPERTY(int snakeId READ snakeId WRITE setSnakeId NOTIFY snakeIdChanged FINAL)
    Q_PROPERTY(quint64 presentationId READ presentationId WRITE setPresentationId NOTIFY
                   presentationIdChanged FINAL)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged FINAL)
public:
    explicit SnakeBar(QQuickItem* parent = nullptr): Panel(parent) {}
    ~SnakeBar() override;
    Action*        action() const { return m_action; }
    void           setAction(Action*);
    CommandButton* actionControl() const { return m_actionControl; }
    void           setActionControl(CommandButton*);
    CommandButton* iconControl() const { return m_iconControl; }
    void           setIconControl(CommandButton*);
    SnakeModel*    snakeModel() const;
    void           setSnakeModel(SnakeModel*);
    int            snakeId() const { return m_snakeId; }
    void           setSnakeId(int);
    quint64        presentationId() const { return m_presentation; }
    void           setPresentationId(quint64);
    bool           isAvailable() const;
    Q_SIGNAL void  actionChanged();
    Q_SIGNAL void  actionControlChanged();
    Q_SIGNAL void  iconControlChanged();
    Q_SIGNAL void  snakeModelChanged();
    Q_SIGNAL void  snakeIdChanged();
    Q_SIGNAL void  presentationIdChanged();
    Q_SIGNAL void  availableChanged();
    Q_SIGNAL void  dismissRequested();

private:
    void                           cancelCommands();
    void                           performAction();
    void                           dismiss();
    QPointer<Action>               m_action;
    QPointer<CommandButton>        m_actionControl, m_iconControl;
    QPointer<SnakeModel>           m_model;
    QList<QMetaObject::Connection> m_actionConnections, m_actionControlConnections,
        m_iconControlConnections, m_modelConnections;
    quint64 m_revision     = 0;
    quint64 m_presentation = 0;
    int     m_snakeId      = -1;
    bool    m_modelBound   = false;
};
} // namespace qml_material
