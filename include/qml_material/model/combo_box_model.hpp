#pragma once

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <memory>
#include "qml_material/export.hpp"

class QQmlContext;
class QQmlComponent;
class QQmlInstanceModel;
class QQmlDelegateModel;

namespace qml_material
{

// Owned models need a delegate component before exposing rows, as in QQmlDelegateModel.
// Role queries never instantiate delegates; external instance models remain borrowed.
class QML_MATERIAL_API ComboBoxModel : public QObject {
    Q_OBJECT
public:
    explicit ComboBoxModel(QQmlContext*, QObject* parent = nullptr);
    ~ComboBoxModel() override;
    QVariant           model() const;
    void               setModel(const QVariant&);
    void               setDelegate(QQmlComponent*);
    QQmlComponent*     delegate() const;
    QQmlInstanceModel* delegateModel() const;
    void               complete();
    int                count() const;
    QString            textRole() const { return m_text_role; }
    void               setTextRole(const QString&);
    QString            valueRole() const { return m_value_role; }
    void               setValueRole(const QString&);
    QString            textAt(int) const;
    QVariant           valueAt(int) const;
    int                indexOfValue(const QVariant&) const;
    int                indexOf(QObject*) const;

    Q_SIGNAL void modelChanged();
    Q_SIGNAL void delegateModelChanged();
    Q_SIGNAL void countChanged();
    Q_SIGNAL void changed();
    Q_SIGNAL void itemCreated(QObject* object);

private:
    QVariant                           roleAt(int, const QString&) const;
    void                               observe();
    void                               observeData();
    void                               updateCount();
    QPointer<QQmlContext>              m_context;
    QVariant                           m_model;
    QPointer<QObject>                  m_source;
    QPointer<QQmlComponent>            m_delegate;
    QPointer<QQmlInstanceModel>        m_instance;
    std::unique_ptr<QQmlDelegateModel> m_owned;
    QList<QMetaObject::Connection>     m_connections;
    QMetaObject::Connection            m_data_connection;
    QString                            m_text_role, m_value_role;
    int                                m_count    = 0;
    bool                               m_complete = false;
};

} // namespace qml_material
