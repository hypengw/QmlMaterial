#pragma once

#include "qml_material/control/page_stack.hpp"
#include <QtCore/QHash>
#include <QtQml/QJSValue>

Q_MOC_INCLUDE("qml_material/util/pool.hpp")

namespace qml_material
{
class Pool;
class PoolRequest;

class QML_MATERIAL_API PageContainer : public MaterialPageStack {
    Q_OBJECT
    QML_NAMED_ELEMENT(PageContainer)
    QML_ATTACHED(PageStackAttached)
    Q_PROPERTY(PoolRequest* pendingRequest READ pendingRequest NOTIFY pendingRequestChanged FINAL)
    Q_PROPERTY(PoolRequest* currentRequest READ currentRequest NOTIFY currentRequestChanged FINAL)
    Q_PROPERTY(QString currentKey READ currentKey NOTIFY currentKeyChanged FINAL)
    Q_PROPERTY(bool canBack READ canBack NOTIFY canBackChanged FINAL)
public:
    explicit PageContainer(QQuickItem* parent = nullptr);
    ~PageContainer() override;
    PoolRequest*     pendingRequest() const;
    PoolRequest*     currentRequest() const;
    QString          currentKey() const;
    bool             canBack() const;
    Q_INVOKABLE void switchTo(const QJSValue& source, const QJSValue& properties,
                              bool cached = true);
    Q_INVOKABLE void back();
    Q_SIGNAL void    pendingRequestChanged();
    Q_SIGNAL void    currentRequestChanged();
    Q_SIGNAL void    currentKeyChanged();
    Q_SIGNAL void    canBackChanged();
    Q_SIGNAL void    pageLoadFailed(const QString& error);

protected:
    void classBegin() override;

private:
    Q_SLOT void                           updateCanBack();
    void                                  handlePendingRequest();
    void                                  setPendingRequest(PoolRequest*);
    void                                  setCurrentRequest(PoolRequest*);
    Pool*                                 m_pool;
    QPointer<PoolRequest>                 m_pending;
    QPointer<PoolRequest>                 m_current;
    QPointer<PoolRequest>                 m_accepting;
    QHash<quint64, QPointer<PoolRequest>> m_leases;
    QMetaObject::Connection               m_statusConnection;
    QMetaObject::Connection               m_backConnection;
    QString                               m_currentKey;
    bool                                  m_changing = false;
    bool                                  m_canBack  = false;
};
} // namespace qml_material

QML_DECLARE_TYPEINFO(qml_material::PageContainer, QML_HAS_ATTACHED_PROPERTIES)
