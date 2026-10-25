#include "qml_material/control/split_button.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
SplitButton::~SplitButton() {
    utils::disconnectAll(m_leadingConnections);
    utils::disconnectAll(m_trailingConnections);
}
void SplitButton::setLeadingControl(CommandButton* value) { setControl(value, false); }
void SplitButton::setTrailingControl(CommandButton* value) { setControl(value, true); }
void SplitButton::setControl(CommandButton* value, bool trailing) {
    auto& current     = trailing ? m_trailing : m_leading;
    auto& connections = trailing ? m_trailingConnections : m_leadingConnections;
    if (current == value && (value || connections.isEmpty())) return;
    utils::disconnectAll(connections);
    current = value;
    if (value) {
        connections.append(connect(value,
                                   &CommandButton::clicked,
                                   this,
                                   [this, trailing, target = QPointer<CommandButton>(value)] {
                                       if (target &&
                                           (trailing ? m_trailing : m_leading) == target &&
                                           isEnabled() && isVisible() && target->isEnabled() &&
                                           target->isVisible()) {
                                           if (trailing)
                                               Q_EMIT trailingClicked();
                                           else
                                               Q_EMIT clicked();
                                       }
                                   }));
        connections.append(connect(value, &QObject::destroyed, this, [this, trailing] {
            setControl(nullptr, trailing);
        }));
    }
    if (trailing)
        Q_EMIT trailingControlChanged();
    else
        Q_EMIT leadingControlChanged();
}
} // namespace qml_material
