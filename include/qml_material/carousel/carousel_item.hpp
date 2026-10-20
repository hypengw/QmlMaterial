#pragma once

#include "qml_material/control/command_button.hpp"

namespace qml_material
{
class CarouselView;
class QML_MATERIAL_API CarouselItem : public CommandButton {
    Q_OBJECT
    QML_NAMED_ELEMENT(CarouselItemBase)
public:
    explicit CarouselItem(QQuickItem* parent = nullptr);
    ~CarouselItem() override;
    CarouselView* carousel() const;

protected:
    bool canActivate() const override;
    void keyPressEvent(QKeyEvent*) override;

private:
    friend class CarouselView;
    void                           setCarousel(CarouselView*);
    void                           updateFocus();
    QPointer<CarouselView>         m_carousel;
    QList<QMetaObject::Connection> m_focusConnections;
    bool                           m_retired = false;
};
} // namespace qml_material
