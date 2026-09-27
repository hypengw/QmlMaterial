#pragma once
#include "qml_material/control/container.hpp"
#include "qml_material/control/button_group.hpp"
#include "qml_material/core/enum.hpp"
#include <QProperty>

namespace qml_material
{
class QML_MATERIAL_API SegmentedButtonGroup : public MaterialContainer {
    Q_OBJECT
    QML_NAMED_ELEMENT(SegmentedButtonGroup)
    Q_PROPERTY(ButtonGroup* group READ group CONSTANT FINAL)
    Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive NOTIFY exclusiveChanged FINAL)
    Q_PROPERTY(int size READ buttonSize WRITE setButtonSize NOTIFY sizeChanged BINDABLE
                   bindableButtonSize FINAL)
public:
    explicit SegmentedButtonGroup(QQuickItem* parent = nullptr);
    ButtonGroup*   group() const { return m_group; }
    bool           isExclusive() const { return m_group->isExclusive(); }
    void           setExclusive(bool value) { m_group->setExclusive(value); }
    Q_SIGNAL void  exclusiveChanged();
    int            buttonSize() const { return m_size.value(); }
    void           setButtonSize(int value) { m_size = value; }
    QBindable<int> bindableButtonSize() { return QBindable<int>(&m_size); }
    Q_SIGNAL void  sizeChanged();

protected:
    bool isContent(QQuickItem*) const override;
    void itemAdded(QQuickItem*) override;
    void itemRemoved(QQuickItem*) override;
    void updatePolish() override;

private:
    ButtonGroup* m_group;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SegmentedButtonGroup, int, m_size,
                                         int(Enum::ButtonSize::S),
                                         &SegmentedButtonGroup::sizeChanged)
};
} // namespace qml_material
