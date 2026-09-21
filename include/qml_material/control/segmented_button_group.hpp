#pragma once
#include "qml_material/control/container.hpp"
#include "qml_material/control/button_group.hpp"

namespace qml_material
{
class QML_MATERIAL_API SegmentedButtonGroup : public Container {
    Q_OBJECT
    QML_NAMED_ELEMENT(SegmentedButtonGroupBase)
    Q_PROPERTY(ButtonGroup* group READ group CONSTANT FINAL)
    Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive NOTIFY exclusiveChanged FINAL)
public:
    explicit SegmentedButtonGroup(QQuickItem* parent = nullptr);
    ButtonGroup*  group() const { return m_group; }
    bool          isExclusive() const { return m_group->isExclusive(); }
    void          setExclusive(bool value) { m_group->setExclusive(value); }
    Q_SIGNAL void exclusiveChanged();

protected:
    bool isContent(QQuickItem*) const override;
    void itemAdded(QQuickItem*) override;
    void itemRemoved(QQuickItem*) override;
    void updatePolish() override;

private:
    ButtonGroup* m_group;
};
} // namespace qml_material
