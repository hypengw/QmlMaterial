#pragma once

#include "qml_material/model/header_data_model.hpp"
#include "qml_material/model/header_list_model.hpp"

namespace qml_material
{
class QML_MATERIAL_API HeaderModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariant model READ model WRITE setModel RESET resetModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QAbstractItemModel* syncModel READ syncModel WRITE setSyncModel NOTIFY
                   syncModelChanged FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged FINAL)
    Q_PROPERTY(
        SourceMode sourceMode READ sourceMode WRITE setSourceMode NOTIFY sourceModeChanged FINAL)
    Q_PROPERTY(QString textRole READ textRole WRITE setTextRole NOTIFY textRoleChanged FINAL)
    Q_PROPERTY(
        QAbstractItemModel* effectiveModel READ effectiveModel NOTIFY effectiveModelChanged FINAL)
    Q_PROPERTY(int sectionCount READ sectionCount NOTIFY contentChanged FINAL)
    Q_PROPERTY(QStringList texts READ texts NOTIFY contentChanged FINAL)
public:
    enum SourceMode
    {
        Auto,
        HeaderData,
        List
    };
    Q_ENUM(SourceMode)
    explicit HeaderModel(QObject* parent = nullptr);
    QVariant            model() const;
    void                setModel(const QVariant&);
    Q_INVOKABLE void    resetModel();
    QAbstractItemModel* syncModel() const { return m_sync; }
    void                setSyncModel(QAbstractItemModel*);
    void                setSyncSource(const QVariant&);
    Qt::Orientation     orientation() const { return m_orientation; }
    void                setOrientation(Qt::Orientation);
    SourceMode          sourceMode() const { return m_mode; }
    void                setSourceMode(SourceMode);
    QString             textRole() const { return m_textRole; }
    void                setTextRole(const QString&);
    QAbstractItemModel* effectiveModel() const { return m_effective; }
    int                 sectionCount() const;
    QStringList         texts() const;
    Q_SIGNAL void       modelChanged();
    Q_SIGNAL void       syncModelChanged();
    Q_SIGNAL void       orientationChanged();
    Q_SIGNAL void       sourceModeChanged();
    Q_SIGNAL void       textRoleChanged();
    Q_SIGNAL void       effectiveModelChanged();
    Q_SIGNAL void       contentChanged();

private:
    void                         update();
    HeaderDataModel              m_header;
    HeaderListModel              m_list;
    QAbstractItemModel*          m_effective = &m_header;
    QPointer<QAbstractItemModel> m_sync;
    QPointer<QObject>            m_object;
    QMetaObject::Connection      m_syncDestroyed, m_modelDestroyed;
    QVariant                     m_model;
    QString                      m_textRole;
    Qt::Orientation              m_orientation = Qt::Horizontal;
    SourceMode                   m_mode        = Auto;
    bool                         m_explicit    = false;
    bool                         m_updating    = false;
};
} // namespace qml_material
