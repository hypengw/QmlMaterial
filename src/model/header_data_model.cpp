#include "qml_material/model/header_data_model.hpp"

namespace qml_material
{
HeaderDataModel::HeaderDataModel(QObject* parent): QAbstractTableModel(parent) {}

void HeaderDataModel::setSourceModel(QAbstractItemModel* source) {
    if (source == m_source || source == this) return;
    beginResetModel();
    if (m_source) disconnect(m_source, nullptr, this, nullptr);
    m_source = source;
    connectSource();
    endResetModel();
    Q_EMIT sourceModelChanged();
}

void HeaderDataModel::setOrientation(Qt::Orientation orientation) {
    if (orientation == m_orientation ||
        (orientation != Qt::Horizontal && orientation != Qt::Vertical))
        return;
    beginResetModel();
    m_orientation = orientation;
    endResetModel();
    Q_EMIT orientationChanged();
}

int HeaderDataModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid() || ! m_source) return 0;
    return m_orientation == Qt::Horizontal ? 1 : m_source->rowCount();
}

int HeaderDataModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid() || ! m_source) return 0;
    return m_orientation == Qt::Vertical ? 1 : m_source->columnCount();
}

bool HeaderDataModel::validIndex(const QModelIndex& index) const {
    return index.isValid() && index.model() == this && hasIndex(index.row(), index.column());
}

QVariant HeaderDataModel::data(const QModelIndex& index, int role) const {
    if (! validIndex(index)) return {};
    const int section = m_orientation == Qt::Horizontal ? index.column() : index.row();
    return m_source->headerData(section, m_orientation, role);
}

bool HeaderDataModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (! validIndex(index)) return false;
    const int section = m_orientation == Qt::Horizontal ? index.column() : index.row();
    return m_source->setHeaderData(section, m_orientation, value, role);
}

QHash<int, QByteArray> HeaderDataModel::roleNames() const {
    return m_source ? m_source->roleNames() : QAbstractTableModel::roleNames();
}

void HeaderDataModel::connectSource() {
    if (! m_source) return;
    connect(m_source, &QObject::destroyed, this, [this] {
        beginResetModel();
        m_source = nullptr;
        endResetModel();
        Q_EMIT sourceModelChanged();
    });
    connect(m_source,
            &QAbstractItemModel::headerDataChanged,
            this,
            [this](Qt::Orientation orientation, int first, int last) {
                if (orientation != m_orientation) return;
                const int count = orientation == Qt::Horizontal ? columnCount() : rowCount();
                first           = qMax(0, first);
                last            = qMin(count - 1, last);
                if (first > last) return;
                if (orientation == Qt::Horizontal)
                    Q_EMIT dataChanged(index(0, first), index(0, last));
                else
                    Q_EMIT dataChanged(index(first, 0), index(last, 0));
            });
    // Source indexes and persistent selections cannot be forwarded to a one-dimensional header.
    const auto begin = [this] {
        beginResetModel();
    };
    const auto end = [this] {
        endResetModel();
    };
    connect(m_source, &QAbstractItemModel::modelAboutToBeReset, this, begin);
    connect(m_source, &QAbstractItemModel::modelReset, this, end);
    connect(m_source, &QAbstractItemModel::layoutAboutToBeChanged, this, begin);
    connect(m_source, &QAbstractItemModel::layoutChanged, this, end);
    connect(m_source, &QAbstractItemModel::rowsAboutToBeInserted, this, begin);
    connect(m_source, &QAbstractItemModel::rowsInserted, this, end);
    connect(m_source, &QAbstractItemModel::rowsAboutToBeRemoved, this, begin);
    connect(m_source, &QAbstractItemModel::rowsRemoved, this, end);
    connect(m_source, &QAbstractItemModel::rowsAboutToBeMoved, this, begin);
    connect(m_source, &QAbstractItemModel::rowsMoved, this, end);
    connect(m_source, &QAbstractItemModel::columnsAboutToBeInserted, this, begin);
    connect(m_source, &QAbstractItemModel::columnsInserted, this, end);
    connect(m_source, &QAbstractItemModel::columnsAboutToBeRemoved, this, begin);
    connect(m_source, &QAbstractItemModel::columnsRemoved, this, end);
    connect(m_source, &QAbstractItemModel::columnsAboutToBeMoved, this, begin);
    connect(m_source, &QAbstractItemModel::columnsMoved, this, end);
}
} // namespace qml_material
