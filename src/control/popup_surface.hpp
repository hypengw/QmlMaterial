#pragma once

#include "qml_material/control/popup.hpp"
#include <QWheelEvent>

namespace qml_material
{
template<typename Base>
class PopupSurface final : public Base {
public:
    explicit PopupSurface(Popup* popup): m_popup(popup) {}

    bool contains(const QPointF& point) const override {
        // The layout surface can be larger than the interactive popup (e.g. a bottom sheet).
        if (m_popup->popupItem() == this) return Base::contains(point);
        return m_popup->containsScenePoint(this->mapToScene(point));
    }

protected:
    void wheelEvent(QWheelEvent* event) override { event->accept(); }

private:
    Popup* m_popup;
};
} // namespace qml_material
