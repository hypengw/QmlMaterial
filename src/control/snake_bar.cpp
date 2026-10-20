#include "qml_material/control/snake_bar.hpp"
#include "qml_material/model/snake_model.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
SnakeBar::~SnakeBar() {
    utils::disconnectAll(m_actionConnections);
    utils::disconnectAll(m_actionControlConnections);
    utils::disconnectAll(m_iconControlConnections);
    utils::disconnectAll(m_modelConnections);
}
bool SnakeBar::isAvailable() const {
    return ! m_modelBound || (m_model && m_model->isPresented(m_snakeId, m_presentation));
}
SnakeModel* SnakeBar::snakeModel() const { return m_model; }
void        SnakeBar::cancelCommands() {
    const QPointer<SnakeBar> guard(this);
    if (m_actionControl) m_actionControl->cancelActivation();
    if (guard && m_iconControl) m_iconControl->cancelActivation();
}
void SnakeBar::setAction(Action* value) {
    if (m_action == value && (value || m_actionConnections.isEmpty())) return;
    utils::disconnectAll(m_actionConnections);
    m_action            = value;
    const auto revision = ++m_revision;
    if (value) {
        m_actionConnections.append(connect(value, &Action::enabledChanged, this, [this] {
            if ((! m_action || ! m_action->isEnabled()) && m_actionControl)
                m_actionControl->cancelActivation();
        }));
        m_actionConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setAction(nullptr);
        }));
    }
    const QPointer<SnakeBar> guard(this);
    if (m_actionControl) m_actionControl->cancelActivation();
    if (guard && revision == m_revision) Q_EMIT actionChanged();
}
void SnakeBar::setActionControl(CommandButton* value) {
    if (m_actionControl == value && (value || m_actionControlConnections.isEmpty())) return;
    const auto old = m_actionControl;
    utils::disconnectAll(m_actionControlConnections);
    m_actionControl     = value;
    const auto revision = ++m_revision;
    if (value) {
        m_actionControlConnections.append(
            connect(value, &CommandButton::clicked, this, &SnakeBar::performAction));
        m_actionControlConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setActionControl(nullptr);
        }));
    }
    const QPointer<SnakeBar> guard(this);
    if (old) old->cancelActivation();
    if (! guard || revision != m_revision) return;
    if (m_actionControl) m_actionControl->cancelActivation();
    if (guard && revision == m_revision) Q_EMIT actionControlChanged();
}
void SnakeBar::setIconControl(CommandButton* value) {
    if (m_iconControl == value && (value || m_iconControlConnections.isEmpty())) return;
    const auto old = m_iconControl;
    utils::disconnectAll(m_iconControlConnections);
    m_iconControl       = value;
    const auto revision = ++m_revision;
    if (value) {
        m_iconControlConnections.append(
            connect(value, &CommandButton::clicked, this, &SnakeBar::dismiss));
        m_iconControlConnections.append(connect(value, &QObject::destroyed, this, [this] {
            setIconControl(nullptr);
        }));
    }
    const QPointer<SnakeBar> guard(this);
    if (old) old->cancelActivation();
    if (! guard || revision != m_revision) return;
    if (m_iconControl) m_iconControl->cancelActivation();
    if (guard && revision == m_revision) Q_EMIT iconControlChanged();
}
void SnakeBar::setSnakeModel(SnakeModel* value) {
    if (m_model == value && m_modelBound == bool(value)) return;
    utils::disconnectAll(m_modelConnections);
    m_model             = value;
    m_modelBound        = value;
    const auto revision = ++m_revision;
    if (value) {
        m_modelConnections.append(connect(value, &SnakeModel::currentIdChanged, this, [this] {
            ++m_revision;
            const QPointer<SnakeBar> guard(this);
            cancelCommands();
            if (guard) Q_EMIT availableChanged();
        }));
        m_modelConnections.append(connect(value, &QObject::destroyed, this, [this] {
            utils::disconnectAll(m_modelConnections);
            const auto               revision = ++m_revision;
            const QPointer<SnakeBar> guard(this);
            cancelCommands();
            if (! guard || revision != m_revision) return;
            Q_EMIT snakeModelChanged();
            if (guard && revision == m_revision) Q_EMIT availableChanged();
        }));
    }
    const QPointer<SnakeBar> guard(this);
    cancelCommands();
    if (! guard || revision != m_revision) return;
    Q_EMIT snakeModelChanged();
    if (guard && revision == m_revision) Q_EMIT availableChanged();
}
void SnakeBar::setSnakeId(int value) {
    if (m_snakeId == value) return;
    m_snakeId                         = value;
    const auto               revision = ++m_revision;
    const QPointer<SnakeBar> guard(this);
    cancelCommands();
    if (! guard || revision != m_revision) return;
    Q_EMIT snakeIdChanged();
    if (guard && revision == m_revision) Q_EMIT availableChanged();
}
void SnakeBar::setPresentationId(quint64 value) {
    if (m_presentation == value) return;
    m_presentation                    = value;
    const auto               revision = ++m_revision;
    const QPointer<SnakeBar> guard(this);
    cancelCommands();
    if (! guard || revision != m_revision) return;
    Q_EMIT presentationIdChanged();
    if (guard && revision == m_revision) Q_EMIT availableChanged();
}
void SnakeBar::performAction() {
    if (! isEnabled() || ! isVisible() || ! isAvailable() || ! m_actionControl ||
        ! m_actionControl->canActivate() || ! m_action)
        return;
    if (m_model)
        m_model->performAction(m_snakeId, m_presentation, m_action, m_actionControl);
    else
        m_action->trigger(m_actionControl);
}
void SnakeBar::dismiss() {
    if (! isEnabled() || ! isVisible() || ! isAvailable() || ! m_iconControl ||
        ! m_iconControl->canActivate())
        return;
    const QPointer<SnakeBar> guard(this);
    const auto               model        = m_model;
    const auto               icon         = m_iconControl;
    const auto               id           = m_snakeId;
    const auto               presentation = m_presentation;
    const bool               bound        = m_modelBound;
    if (model) model->dismissPresentation(id, presentation);
    if (guard && m_modelBound == bound && (! bound || model) && m_model == model &&
        m_iconControl == icon && m_snakeId == id && m_presentation == presentation)
        Q_EMIT dismissRequested();
}
} // namespace qml_material
