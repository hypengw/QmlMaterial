#include "qml_material/control/check_box.hpp"
#include <QQmlInfo>

namespace qml_material
{
CheckBox::CheckBox(QQuickItem* parent): AbstractButton(parent) {
    setCheckable(true);
    connect(this, &AbstractButton::checkedChanged, this, [this] {
        if (! m_syncing) setCheckState(isChecked() ? Qt::Checked : Qt::Unchecked);
    });
}
void CheckBox::setTristate(bool value) {
    if (m_tristate == value) return;
    m_tristate = value;
    Q_EMIT tristateChanged();
}
void CheckBox::setCheckState(Qt::CheckState value) {
    if (value < Qt::Unchecked || value > Qt::Checked || m_state == value) return;
    m_state = value;
    QPointer<CheckBox> guard(this);
    m_syncing = true;
    setChecked(value == Qt::Checked);
    if (! guard) return;
    m_syncing = false;
    if (isChecked() != (m_state == Qt::Checked))
        m_state = isChecked() ? Qt::Checked : Qt::Unchecked;
    Q_EMIT checkStateChanged();
}
void CheckBox::setNextCheckStateCallback(const QJSValue& value) {
    if (m_callback.strictlyEquals(value)) return;
    m_callback = value;
    Q_EMIT nextCheckStateChanged();
}
void CheckBox::nextCheckState() {
    if (! isCheckable() || (isChecked() && ! canUncheck())) return;
    if (! m_callback.isCallable() && ! m_tristate) {
        AbstractButton::nextCheckState();
        return;
    }
    QPointer<CheckBox> guard(this);
    const bool         previous = isChecked();
    auto               next     = Qt::CheckState((m_state + 1) % 3);
    if (m_callback.isCallable()) {
        const auto result = m_callback.call();
        if (! guard) return;
        if (result.isError()) {
            qmlWarning(this) << result.toString();
            return;
        }
        next = Qt::CheckState(result.toInt());
    }
    setCheckState(next);
    if (guard && previous != isChecked()) Q_EMIT toggled();
}
} // namespace qml_material
