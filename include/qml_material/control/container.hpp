#pragma once

#include <QQmlListProperty>
#include <QHash>
#include "qml_material/control/control.hpp"

namespace qml_material
{
class ContainerAttached;

/** @ingroup control
 * Owns an ordered collection; visual layout belongs to the concrete control.
 */
class QML_MATERIAL_API Container : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(ContainerBase)
    QML_ATTACHED(ContainerAttached)
    Q_CLASSINFO("DefaultProperty", "contentData")
    Q_PROPERTY(QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickItem> contentChildren READ contentChildren NOTIFY
                   contentChildrenChanged FINAL)
    Q_PROPERTY(QQuickItem* contentHost READ contentHost CONSTANT FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(
        int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QQuickItem* currentItem READ currentItem NOTIFY currentItemChanged)
    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth RESET resetContentWidth
                   NOTIFY contentWidthChanged FINAL)
    Q_PROPERTY(qreal contentHeight READ contentHeight WRITE setContentHeight RESET
                   resetContentHeight NOTIFY contentHeightChanged FINAL)
public:
    explicit Container(QQuickItem* parent = nullptr);
    ~Container() override;
    static ContainerAttached*    qmlAttachedProperties(QObject*);
    QQmlListProperty<QObject>    contentData();
    QQmlListProperty<QQuickItem> contentChildren();
    QQuickItem*                  contentHost() const { return m_host; }
    int                          count() const { return m_items.size(); }
    int                          currentIndex() const { return m_index; }
    QQuickItem*                  currentItem() const { return m_current; }
    Q_INVOKABLE void             setCurrentIndex(int);
    Q_INVOKABLE QQuickItem*      itemAt(int) const;
    Q_INVOKABLE int              indexOf(QQuickItem*) const;
    Q_INVOKABLE void             addItem(QQuickItem*);
    Q_INVOKABLE void             insertItem(int, QQuickItem*);
    Q_INVOKABLE void             moveItem(int, int);
    // removeItem schedules deletion; takeItem detaches without destroying the item.
    Q_INVOKABLE void         removeItem(QQuickItem*);
    Q_INVOKABLE QQuickItem*  takeItem(int);
    Q_INVOKABLE virtual void incrementCurrentIndex();
    Q_INVOKABLE virtual void decrementCurrentIndex();
    qreal                    contentWidth() const;
    qreal                    contentHeight() const;
    void                     setContentWidth(qreal);
    void                     setContentHeight(qreal);
    void                     resetContentWidth();
    void                     resetContentHeight();
    Q_SIGNAL void            countChanged();
    Q_SIGNAL void            currentIndexChanged();
    Q_SIGNAL void            currentItemChanged();
    Q_SIGNAL void            contentChildrenChanged();
    Q_SIGNAL void            contentWidthChanged();
    Q_SIGNAL void            contentHeightChanged();

protected:
    virtual bool                       isContent(QQuickItem*) const;
    virtual void                       itemAdded(QQuickItem*) {}
    virtual void                       itemRemoved(QQuickItem*) {}
    virtual void                       itemsChanged() {}
    virtual void                       currentItemChange() {}
    virtual int                        initialIndex() const;
    void                               componentComplete() override;
    void                               updatePolish() override;
    QSizeF                             measureImplicitContent() const override;
    const QList<QPointer<QQuickItem>>& items() const { return m_items; }
    quint64                            revision() const { return m_revision; }
    void                               refreshItems();
    void                               setImplicitContentSize(const QSizeF&);
    void                               beginTeardown();
    void                               setPresentationHost(QQuickItem*, QQuickItem*);
    QQuickItem*                        presentationItem(QQuickItem*) const;

private:
    void                                     updateContentSize();
    QQuickItem*                              m_host;
    QHash<QQuickItem*, QPointer<QQuickItem>> m_presentation_hosts;
    QList<QPointer<QQuickItem>>              m_items;
    QList<QPointer<QQuickItem>>              m_observed;
    QList<QMetaObject::Connection>           m_connections;
    QList<QMetaObject::Connection>           m_measure_connections;
    QPointer<QQuickItem>                     m_current;
    QQuickItem*                              m_current_identity = nullptr;
    int                                      m_index            = -1;
    bool                                     m_explicit_index   = false;
    bool                                     m_refreshing       = false;
    bool                                     m_refresh_again    = false;
    bool                                     m_destroying       = false;
    quint64                                  m_revision         = 0;
    std::optional<QSizeF>                    m_implicit_content;
    QSizeF                                   m_content_size { 0, 0 };
    std::optional<qreal>                     m_content_width;
    std::optional<qreal>                     m_content_height;
};

class QML_MATERIAL_API ContainerAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(Container* container READ container NOTIFY containerChanged FINAL)
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)
    Q_PROPERTY(bool managed READ isManaged WRITE setManaged NOTIFY managedChanged FINAL)
public:
    explicit ContainerAttached(QObject* parent): QObject(parent) {}
    Container*    container() const { return m_container; }
    int           index() const { return m_index; }
    bool          isManaged() const { return m_managed; }
    void          setManaged(bool);
    Q_SIGNAL void containerChanged();
    Q_SIGNAL void indexChanged();
    Q_SIGNAL void managedChanged();

private:
    friend class Container;
    void                update(Container*, int);
    QPointer<Container> m_container;
    int                 m_index   = -1;
    bool                m_managed = true;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::Container, QML_HAS_ATTACHED_PROPERTIES)
