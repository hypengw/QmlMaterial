#pragma once

#include "qml_material/control/selection_button.hpp"

namespace qml_material
{
class NavigationItem;
class QML_MATERIAL_API NavigationControl : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(NavigationControlBase)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged
                   BINDABLE bindableCurrentIndex FINAL)
public:
    explicit NavigationControl(QQuickItem* parent = nullptr): Control(parent) {}
    int            currentIndex() const { return m_currentIndex; }
    void           setCurrentIndex(int value) { m_currentIndex = value; }
    QBindable<int> bindableCurrentIndex() { return &m_currentIndex; }
    void           requestSelection(NavigationItem*);
    Q_SIGNAL void  currentIndexChanged();
    Q_SIGNAL void  currentIndexChangeRequested(int index);

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NavigationControl, int, m_currentIndex, 0,
                                         &NavigationControl::currentIndexChanged)
};

class QML_MATERIAL_API NavigationItem : public SelectionButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(NavigationItemBase)
    Q_PROPERTY(qml_material::NavigationControl* navigation READ navigation WRITE setNavigation
                   NOTIFY navigationChanged FINAL)
    Q_PROPERTY(int navigationIndex READ navigationIndex WRITE setNavigationIndex NOTIFY
                   navigationIndexChanged FINAL)
public:
    explicit NavigationItem(QQuickItem* parent = nullptr);
    ~NavigationItem() override;
    NavigationControl* navigation() const { return m_navigation; }
    void               setNavigation(NavigationControl*);
    int                navigationIndex() const { return m_navigationIndex; }
    void               setNavigationIndex(int);
    Q_SIGNAL void      navigationChanged();
    Q_SIGNAL void      navigationIndexChanged();

protected:
    bool canActivate() const override;

private:
    QPointer<NavigationControl>    m_navigation;
    QList<QMetaObject::Connection> m_navigationConnections;
    int                            m_navigationIndex = -1;
};
} // namespace qml_material
