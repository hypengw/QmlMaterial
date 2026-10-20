#include "qml_material/control/check_box.hpp"

namespace qml_material
{
void CheckBox::setCheckState(Qt::CheckState value) {
    if (value >= Qt::Unchecked && value <= Qt::Checked) m_state = value;
}
void CheckBox::requestCheckState(Qt::CheckState value) {
    if (! canActivate() || value < Qt::Unchecked || value > Qt::Checked || value == checkState())
        return;
    Q_EMIT checkStateChangeRequested(value);
}
void CheckBox::activate() {
    requestCheckState(isTristate()                  ? Qt::CheckState((checkState() + 1) % 3)
                      : checkState() == Qt::Checked ? Qt::Unchecked
                                                    : Qt::Checked);
}
} // namespace qml_material
