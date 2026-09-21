#include "qml_material/model/header_list_model.hpp"
#include <QJSValue>
#include <QSet>
#include <algorithm>

namespace qml_material
{
namespace
{
constexpr int ModelDataRole = Qt::UserRole;
}

HeaderListModel::HeaderListModel(QObject* parent): QAbstractTableModel(parent) {}

QVariant HeaderListModel::source() const {
    if (m_source.metaType().flags().testFlag(QMetaType::PointerToQObject))
        return QVariant::fromValue(m_model.data());
    return m_source;
}

void HeaderListModel::setSource(const QVariant& source) { trySetSource(source); }

bool HeaderListModel::trySetSource(const QVariant& source) {
    const auto value = source.metaType() == QMetaType::fromType<QJSValue>()
                           ? source.value<QJSValue>().toVariant()
                           : source;
    if (m_source == value) return true;
    auto model = qobject_cast<QAbstractItemModel*>(value.value<QObject*>());
    if (model == this) return false;
    QVariantList values;
    int          integerCount = 0;
    if (model) {
        if (model->columnCount() > 1) {
            qWarning("HeaderListModel requires a single-column source model");
            return false;
        }
    } else if (value.metaType() == QMetaType::fromType<QStringList>()) {
        for (const auto& text : value.toStringList()) values.append(text);
    } else if (value.metaType() == QMetaType::fromType<QVariantList>()) {
        values = value.toList();
    } else if (value.metaType() == QMetaType::fromType<int>() && value.toInt() >= 0) {
        integerCount = value.toInt();
    } else if (value.isValid() && ! value.isNull()) {
        qWarning("HeaderListModel source must be an array, string list, count or item model");
        return false;
    }
    QSet<QByteArray> names;
    for (const auto& entry : values) {
        if (entry.metaType() != QMetaType::fromType<QVariantMap>()) continue;
        const auto fields = entry.toMap();
        for (auto it = fields.cbegin(); it != fields.cend(); ++it)
            if (it.key() != QStringLiteral("modelData")) names.insert(it.key().toUtf8());
    }
    auto sortedNames = names.values();
    std::sort(sortedNames.begin(), sortedNames.end());
    beginResetModel();
    if (m_model) disconnect(m_model, nullptr, this, nullptr);
    m_source       = value;
    m_model        = model;
    m_values       = std::move(values);
    m_integerCount = integerCount;
    m_roles        = { { ModelDataRole, "modelData" } };
    int role       = ModelDataRole + 1;
    for (const auto& name : sortedNames) m_roles.insert(role++, name);
    connectSource();
    endResetModel();
    Q_EMIT sourceChanged();
    return true;
}

void HeaderListModel::setOrientation(Qt::Orientation orientation) {
    if (orientation == m_orientation ||
        (orientation != Qt::Horizontal && orientation != Qt::Vertical))
        return;
    beginResetModel();
    m_orientation = orientation;
    endResetModel();
    Q_EMIT orientationChanged();
}

int HeaderListModel::count() const {
    return m_model ? (m_model->columnCount() == 1 ? m_model->rowCount() : 0)
                   : qMax(m_integerCount, int(m_values.size()));
}

int HeaderListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid() || ! count()) return 0;
    return m_orientation == Qt::Horizontal ? 1 : count();
}

int HeaderListModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid() || ! count()) return 0;
    return m_orientation == Qt::Horizontal ? count() : 1;
}

QModelIndex HeaderListModel::sectionIndex(int section) const {
    return m_orientation == Qt::Horizontal ? index(0, section) : index(section, 0);
}

QVariant HeaderListModel::data(const QModelIndex& index, int role) const {
    if (! index.isValid() || index.model() != this || ! hasIndex(index.row(), index.column()))
        return {};
    const int section = m_orientation == Qt::Horizontal ? index.column() : index.row();
    // QQmlListModel does not safely handle arbitrary unknown role IDs.
    if (m_model)
        return m_model->roleNames().contains(role) ? m_model->data(m_model->index(section, 0), role)
                                                   : QVariant {};
    if (role == ModelDataRole || role == Qt::DisplayRole) {
        const auto value = m_integerCount ? QVariant(section) : m_values.at(section);
        return role == ModelDataRole || value.canConvert<QString>() ? value : QVariant {};
    }
    const auto name = m_roles.value(role);
    if (name.isEmpty() || m_integerCount) return {};
    return m_values.at(section).toMap().value(QString::fromUtf8(name));
}

QHash<int, QByteArray> HeaderListModel::roleNames() const {
    return m_model ? m_model->roleNames() : m_roles;
}

void HeaderListModel::connectSource() {
    if (! m_model) return;
    connect(m_model, &QObject::destroyed, this, [this] {
        setSource({});
    });
    connect(m_model,
            &QAbstractItemModel::dataChanged,
            this,
            [this](const QModelIndex& first, const QModelIndex& last, const QList<int>& roles) {
                if (first.parent().isValid() || last.parent().isValid() || first.column() > 0 ||
                    ! count())
                    return;
                Q_EMIT dataChanged(sectionIndex(first.row()), sectionIndex(last.row()), roles);
            });
    const auto begin = [this] {
        beginResetModel();
    };
    const auto end = [this] {
        endResetModel();
    };
    connect(m_model, &QAbstractItemModel::modelAboutToBeReset, this, begin);
    connect(m_model, &QAbstractItemModel::modelReset, this, end);
    connect(m_model, &QAbstractItemModel::layoutAboutToBeChanged, this, begin);
    connect(m_model, &QAbstractItemModel::layoutChanged, this, end);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeInserted, this, begin);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, end);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, begin);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, end);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeMoved, this, begin);
    connect(m_model, &QAbstractItemModel::rowsMoved, this, end);
    connect(m_model, &QAbstractItemModel::columnsAboutToBeInserted, this, begin);
    connect(m_model, &QAbstractItemModel::columnsInserted, this, end);
    connect(m_model, &QAbstractItemModel::columnsAboutToBeRemoved, this, begin);
    connect(m_model, &QAbstractItemModel::columnsRemoved, this, end);
    connect(m_model, &QAbstractItemModel::columnsAboutToBeMoved, this, begin);
    connect(m_model, &QAbstractItemModel::columnsMoved, this, end);
}
} // namespace qml_material
