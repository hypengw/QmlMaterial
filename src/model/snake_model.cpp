#include "qml_material/model/snake_model.hpp"
#include <algorithm>
#include <limits>

namespace qml_material
{
SnakeModel::SnakeModel(QObject* parent): QAbstractListModel(parent) {}
SnakeModel::~SnakeModel() {
    while (! m_extra.empty()) removeExtra(m_extra.begin()->first);
}
int SnakeModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : int(m_items.size());
}
QHash<int, QByteArray> SnakeModel::roleNames() const {
    return { { Qt::UserRole, "sid" },
             { Qt::UserRole + 1, "text" },
             { Qt::UserRole + 2, "duration" },
             { Qt::UserRole + 3, "flag" },
             { Qt::UserRole + 4, "action" },
             { Qt::UserRole + 5, "maximumWidth" },
             { Qt::UserRole + 6, "actionOnNewLine" },
             { Qt::UserRole + 7, "presentationId" } };
}
QVariant SnakeModel::data(const QModelIndex& index, int role) const {
    if (! index.isValid() || index.model() != this || index.column() != 0 || index.row() < 0 ||
        index.row() >= rowCount())
        return {};
    const auto& item = m_items[index.row()];
    switch (role) {
    case Qt::UserRole: return item.id;
    case Qt::UserRole + 1: return item.text;
    case Qt::UserRole + 2: return item.duration;
    case Qt::UserRole + 3: return QVariant::fromValue(item.flag);
    case Qt::UserRole + 4: return QVariant::fromValue(item.action());
    case Qt::UserRole + 5: return item.maximum_width;
    case Qt::UserRole + 6: return item.action_on_new_line;
    case Qt::UserRole + 7: return QVariant::fromValue(m_revision);
    default: return {};
    }
}
SnakeModelItem SnakeModel::createSnake() {
    while (m_extra.contains(m_nextId))
        m_nextId = m_nextId == std::numeric_limits<qint32>::max() ? 0 : m_nextId + 1;
    SnakeModelItem item;
    item.id  = m_nextId;
    m_nextId = m_nextId == std::numeric_limits<qint32>::max() ? 0 : m_nextId + 1;
    return item;
}
void SnakeModel::show(const QString& text, int duration, int flag, Action* action,
                      qreal maximumWidth, bool actionOnNewLine) {
    auto item     = createSnake();
    item.text     = text;
    item.duration = duration;
    item.flag     = Enum::ToastFlags::fromInt(flag);
    item.setAction(action);
    item.maximum_width      = maximumWidth;
    item.action_on_new_line = actionOnNewLine;
    showSnake(item);
}
void SnakeModel::showSnake(const SnakeModelItem& value) {
    if (m_mutating) {
        QMetaObject::invokeMethod(
            this,
            [this, value] {
                showSnake(value);
            },
            Qt::QueuedConnection);
        return;
    }
    auto item = value;
    if (item.id < 0 || m_extra.contains(item.id)) return;
    const QPointer<SnakeModel> guard(this);
    if (! m_items.empty()) {
        const auto revision = m_revision + 1;
        removeCurrent(m_items.front().flag.testFlag(Enum::ToastFlag::TFSave));
        if (! guard || m_revision != revision || ! m_items.empty()) return;
    }
    if (item.duration == 0) item.flag |= Enum::ToastFlag::TFSave;
    if (item.flag.testFlag(Enum::ToastFlag::TFSave)) item.flag |= Enum::ToastFlag::TFCloseable;
    auto& extra      = m_extra[item.id];
    extra.generation = ++m_generation;
    if (auto action = item.action()) {
        extra.actionDestroyed = connect(
            action, &QObject::destroyed, this, [this, id = item.id, generation = extra.generation] {
                const auto found = m_extra.find(id);
                if (found != m_extra.end() && found->second.generation == generation &&
                    isCurrent(id))
                    Q_EMIT dataChanged(index(0), index(0), { Qt::UserRole + 4 });
            });
    }
    insertCurrent(item);
}
void SnakeModel::insertCurrent(const SnakeModelItem& item) {
    const QPointer<SnakeModel> guard(this);
    m_mutating = true;
    ++m_revision;
    beginInsertRows({}, 0, 0);
    m_items.push_back(item);
    endInsertRows();
    if (! guard) return;
    m_mutating = false;
    if (item.duration > 0) {
        auto timer = new QTimer(this);
        timer->setSingleShot(true);
        const auto generation     = m_extra.at(item.id).generation;
        m_extra.at(item.id).timer = timer;
        connect(timer, &QTimer::timeout, this, [this, id = item.id, generation] {
            const auto found = m_extra.find(id);
            if (found != m_extra.end() && found->second.generation == generation && isCurrent(id))
                removeById(id);
        });
        timer->start(item.duration);
    }
    Q_EMIT currentIdChanged();
}
void SnakeModel::removeCurrent(bool preserve) {
    if (m_items.empty()) return;
    auto item = m_items.front();
    if (preserve) {
        const auto remaining = stopTimer(item.id);
        if (item.duration > 0) item.duration = std::max(remaining, 1000);
        m_savedItems.push_back(item);
    } else
        removeExtra(item.id);
    const QPointer<SnakeModel> guard(this);
    m_mutating = true;
    ++m_revision;
    beginRemoveRows({}, 0, 0);
    m_items.clear();
    endRemoveRows();
    if (! guard) return;
    m_mutating = false;
    scheduleRestore();
    Q_EMIT currentIdChanged();
}
bool SnakeModel::removeRows(int row, int count, const QModelIndex& parent) {
    if (m_mutating || parent.isValid() || row != 0 || count != 1 || m_items.empty()) return false;
    removeCurrent(false);
    return true;
}
void SnakeModel::removeById(qint32 id) {
    if (m_mutating) {
        QMetaObject::invokeMethod(
            this,
            [this, id] {
                removeById(id);
            },
            Qt::QueuedConnection);
        return;
    }
    if (isCurrent(id)) {
        removeCurrent(false);
        return;
    }
    const auto found =
        std::find_if(m_savedItems.begin(), m_savedItems.end(), [id](const auto& item) {
            return item.id == id;
        });
    if (found != m_savedItems.end()) {
        removeExtra(id);
        m_savedItems.erase(found);
    }
}
int SnakeModel::stopTimer(int id) {
    const auto found = m_extra.find(id);
    if (found == m_extra.end() || ! found->second.timer) return 0;
    auto timer          = found->second.timer;
    found->second.timer = nullptr;
    // Coarse timer deadlines can extend beyond the nominal interval.
    const auto remaining = std::clamp(timer->remainingTime(), 0, timer->interval());
    timer->stop();
    disconnect(timer, nullptr, this, nullptr);
    timer->deleteLater();
    return remaining;
}
void SnakeModel::removeExtra(int id) {
    const auto found = m_extra.find(id);
    if (found == m_extra.end()) return;
    stopTimer(id);
    disconnect(found->second.actionDestroyed);
    m_extra.erase(found);
}
void SnakeModel::scheduleRestore() {
    if (m_restorePending) return;
    m_restorePending = true;
    QMetaObject::invokeMethod(
        this,
        [this] {
            m_restorePending = false;
            if (m_mutating || ! m_items.empty() || m_savedItems.empty()) return;
            auto item = m_savedItems.back();
            m_savedItems.pop_back();
            insertCurrent(item);
        },
        Qt::QueuedConnection);
}
bool SnakeModel::performAction(int id, quint64 presentation, Action* expected, QObject* source) {
    if (m_mutating || ! isPresented(id, presentation) || ! expected ||
        m_items.front().action() != expected)
        return false;
    const auto                 generation = m_extra.at(id).generation;
    const QPointer<SnakeModel> guard(this);
    const bool                 triggered = expected->trigger(source);
    if (guard && triggered) {
        const auto found = m_extra.find(id);
        if (found != m_extra.end() && found->second.generation == generation) removeById(id);
    }
    return triggered;
}
void SnakeModel::dismiss(int id) {
    if (isCurrent(id)) removeById(id);
}
void SnakeModel::dismissPresentation(int id, quint64 presentation) {
    if (isPresented(id, presentation)) removeById(id);
}

} // namespace qml_material
