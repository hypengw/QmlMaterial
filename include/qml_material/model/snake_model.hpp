#pragma once

#include <QAbstractListModel>
#include <QTimer>
#include <vector>
#include <unordered_map>
#include "qml_material/control/action.hpp"
#include "qml_material/core/enum.hpp"

namespace qml_material
{
struct QML_MATERIAL_API SnakeModelItem {
    Q_GADGET

    Q_PROPERTY(qint32 sid MEMBER id FINAL)
    Q_PROPERTY(QString text MEMBER text FINAL)
    Q_PROPERTY(qint32 duration MEMBER duration FINAL)
    Q_PROPERTY(Enum::ToastFlags flag MEMBER flag FINAL)
    Q_PROPERTY(Action* action READ action WRITE setAction FINAL)
    Q_PROPERTY(qreal maximumWidth MEMBER maximum_width FINAL)
    Q_PROPERTY(bool actionOnNewLine MEMBER action_on_new_line FINAL)
public:
    qint32           id { 0 };
    QString          text {};
    qint32           duration { 0 };
    Enum::ToastFlags flag {};
    qreal            maximum_width { 0 };
    bool             action_on_new_line { false };
    Action*          action() const { return m_action; }
    void             setAction(Action* value) { m_action = value; }

private:
    QPointer<Action> m_action;
};

class QML_MATERIAL_API SnakeModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int currentId READ currentId NOTIFY currentIdChanged FINAL)
public:
    explicit SnakeModel(QObject* parent = nullptr);
    ~SnakeModel() override;

    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int                    currentId() const { return m_items.empty() ? -1 : m_items.front().id; }
    bool                   isCurrent(int id) const { return id >= 0 && currentId() == id; }
    quint64                presentationId(int id) const { return isCurrent(id) ? m_revision : 0; }
    bool                   isPresented(int id, quint64 presentation) const {
        return ! m_mutating && presentation && presentationId(id) == presentation;
    }
    Q_INVOKABLE SnakeModelItem createSnake();
    Q_INVOKABLE void show(const QString& text, int duration = 4000,
                          int flag = int(Enum::ToastFlag::TFCloseable), Action* action = nullptr,
                          qreal maximumWidth = 0, bool actionOnNewLine = false);

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    Q_SLOT void   showSnake(const SnakeModelItem&);
    Q_SLOT void   removeById(qint32 id);
    bool          performAction(int id, quint64 presentation, Action* expected, QObject* source);
    void          dismiss(int id);
    void          dismissPresentation(int id, quint64 presentation);
    Q_SIGNAL void currentIdChanged();

private:
    void                        insertCurrent(const SnakeModelItem&);
    void                        removeCurrent(bool preserve);
    void                        removeExtra(int id);
    int                         stopTimer(int id);
    void                        scheduleRestore();
    qint32                      m_nextId     = 0;
    quint64                     m_generation = 0, m_revision = 0;
    bool                        m_mutating = false, m_restorePending = false;
    std::vector<SnakeModelItem> m_items;
    std::vector<SnakeModelItem> m_savedItems;

    struct Extra {
        QPointer<QTimer>        timer;
        QMetaObject::Connection actionDestroyed;
        quint64                 generation = 0;
    };
    std::unordered_map<qint32, Extra> m_extra;
};
} // namespace qml_material
