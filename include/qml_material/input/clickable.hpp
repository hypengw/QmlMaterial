#pragma once

#include "qml_material/input/interactive_item.hpp"

namespace qml_material
{
class QML_MATERIAL_API Clickable : public InteractiveItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool longClickEnabled READ longClickEnabled WRITE setLongClickEnabled NOTIFY
                   longClickEnabledChanged FINAL)
    Q_PROPERTY(bool doubleClickEnabled READ doubleClickEnabled WRITE setDoubleClickEnabled NOTIFY
                   doubleClickEnabledChanged FINAL)
    Q_PROPERTY(int longClickInterval READ longClickInterval WRITE setLongClickInterval NOTIFY
                   longClickIntervalChanged FINAL)
public:
    explicit Clickable(QQuickItem* parent = nullptr);
    bool          longClickEnabled() const { return m_longClickEnabled; }
    void          setLongClickEnabled(bool);
    bool          doubleClickEnabled() const { return m_doubleClickEnabled; }
    void          setDoubleClickEnabled(bool);
    int           longClickInterval() const { return m_longClickInterval; }
    void          setLongClickInterval(int);
    Q_SIGNAL void clicked();
    Q_SIGNAL void longClicked();
    Q_SIGNAL void doubleClicked();
    Q_SIGNAL void longClickEnabledChanged();
    Q_SIGNAL void doubleClickEnabledChanged();
    Q_SIGNAL void longClickIntervalChanged();

protected:
    void activate() override { Q_EMIT clicked(); }

private:
    void updateCombined();
    bool m_longClickEnabled = false, m_doubleClickEnabled = false;
    int  m_longClickInterval;
};
} // namespace qml_material
