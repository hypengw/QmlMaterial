#include "qml_material/model/header_model.hpp"
#include <QJSValue>

namespace qml_material
{
HeaderModel::HeaderModel(QObject* parent): QObject(parent), m_header(this), m_list(this) {
    for (auto model : { static_cast<QAbstractItemModel*>(&m_header),
                        static_cast<QAbstractItemModel*>(&m_list) }) {
        const auto changed = [this, model] {
            if (! m_updating && m_effective == model) Q_EMIT contentChanged();
        };
        connect(model, &QAbstractItemModel::modelReset, this, changed);
        connect(model, &QAbstractItemModel::dataChanged, this, changed);
    }
}

QVariant HeaderModel::model() const {
    if (m_model.metaType().flags().testFlag(QMetaType::PointerToQObject))
        return QVariant::fromValue(m_object.data());
    return m_model;
}

void HeaderModel::setModel(const QVariant& source) {
    const auto value = source.metaType() == QMetaType::fromType<QJSValue>()
                           ? source.value<QJSValue>().toVariant()
                           : source;
    if (! value.isValid()) {
        resetModel();
        return;
    }
    if (m_explicit && m_model == value) return;
    disconnect(m_modelDestroyed);
    m_explicit = true;
    m_model    = value;
    m_object   = value.value<QObject*>();
    if (m_object)
        m_modelDestroyed = connect(m_object, &QObject::destroyed, this, [this] {
            m_model  = QVariant::fromValue(static_cast<QObject*>(nullptr));
            m_object = nullptr;
            update();
            Q_EMIT modelChanged();
        });
    update();
    Q_EMIT modelChanged();
}

void HeaderModel::resetModel() {
    if (! m_explicit) return;
    disconnect(m_modelDestroyed);
    m_explicit = false;
    m_model    = {};
    m_object   = nullptr;
    update();
    Q_EMIT modelChanged();
}

void HeaderModel::setSyncModel(QAbstractItemModel* model) {
    if (m_sync == model) return;
    disconnect(m_syncDestroyed);
    m_sync = model;
    if (model)
        m_syncDestroyed = connect(model, &QObject::destroyed, this, [this] {
            m_sync = nullptr;
            update();
            Q_EMIT syncModelChanged();
        });
    update();
    Q_EMIT syncModelChanged();
}

void HeaderModel::setSyncSource(const QVariant& source) {
    const auto value = source.metaType() == QMetaType::fromType<QJSValue>()
                           ? source.value<QJSValue>().toVariant()
                           : source;
    setSyncModel(qobject_cast<QAbstractItemModel*>(value.value<QObject*>()));
}

void HeaderModel::setOrientation(Qt::Orientation value) {
    if (value == m_orientation || (value != Qt::Horizontal && value != Qt::Vertical)) return;
    m_orientation = value;
    m_header.setOrientation(value);
    m_list.setOrientation(value);
    Q_EMIT orientationChanged();
    Q_EMIT contentChanged();
}

void HeaderModel::setSourceMode(SourceMode value) {
    if (value == m_mode || value < Auto || value > List) return;
    m_mode = value;
    update();
    Q_EMIT sourceModeChanged();
}

void HeaderModel::setTextRole(const QString& value) {
    if (value == m_textRole) return;
    m_textRole = value;
    Q_EMIT textRoleChanged();
    Q_EMIT contentChanged();
}

void HeaderModel::update() {
    m_updating  = true;
    auto source = m_explicit ? qobject_cast<QAbstractItemModel*>(m_object.data()) : m_sync.data();
    const bool header = ! m_explicit || m_mode == HeaderData ||
                        (m_mode == Auto && qobject_cast<QAbstractTableModel*>(source));
    auto       next   = header ? static_cast<QAbstractItemModel*>(&m_header)
                               : static_cast<QAbstractItemModel*>(&m_list);
    if (header) {
        m_header.setSourceModel(source);
        m_list.setSource({});
    } else {
        if (! m_list.trySetSource(model())) m_list.setSource({});
        m_header.setSourceModel(nullptr);
    }
    const bool changed = m_effective != next;
    m_effective        = next;
    m_updating         = false;
    if (changed) Q_EMIT effectiveModelChanged();
    Q_EMIT contentChanged();
}

int HeaderModel::sectionCount() const {
    return m_orientation == Qt::Horizontal ? m_effective->columnCount() : m_effective->rowCount();
}

QStringList HeaderModel::texts() const {
    const auto roles    = m_effective->roleNames();
    auto       roleName = m_textRole.toUtf8();
    if (roleName.isEmpty())
        roleName = m_effective == &m_list && ! m_object ? "modelData" : "display";
    const int   role = roles.key(roleName, -1);
    QStringList result;
    result.reserve(sectionCount());
    for (int section = 0; section < sectionCount(); ++section) {
        const auto index = m_orientation == Qt::Horizontal ? m_effective->index(0, section)
                                                           : m_effective->index(section, 0);
        result.append(role < 0 ? QString {} : m_effective->data(index, role).toString());
    }
    return result;
}
} // namespace qml_material
