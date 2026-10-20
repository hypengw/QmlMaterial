#include "qml_material/control/split_button.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
SplitButton::~SplitButton() { utils::disconnectAll(m_connections); }
void SplitButton::setLeadingControl(CommandButton* value) {
    if (m_leading == value && (value || m_connections.isEmpty())) return;
    utils::disconnectAll(m_connections);
    m_leading = value;
    if (value) {
        m_connections.append(connect(
            value, &CommandButton::clicked, this, [this, target = QPointer<CommandButton>(value)] {
                if (target && m_leading == target && isEnabled() && isVisible() &&
                    target->isEnabled() && target->isVisible())
                    Q_EMIT clicked();
            }));
        m_connections.append(connect(value, &QObject::destroyed, this, [this] {
            setLeadingControl(nullptr);
        }));
    }
    Q_EMIT leadingControlChanged();
}
} // namespace qml_material
