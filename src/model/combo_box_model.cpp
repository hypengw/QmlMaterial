#include "qml_material/model/combo_box_model.hpp"
#include "qml_material/util/qt.hpp"
#include <QQmlContext>
#include <QQmlComponent>
#include <QJSValue>
#include <QAbstractItemModel>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>

namespace qml_material
{
namespace
{
class ComboDelegateModel : public QQmlDelegateModel {
public:
    using QQmlDelegateModel::QQmlDelegateModel;
    QVariant variantValue(int index, const QString& role) override {
        // Preserve Qt 6 ComboBox's single-field map compatibility.
        if (role == QStringLiteral("modelData") &&
            model().metaType() == QMetaType::fromType<QVariantList>()) {
            const auto object = model().toList().value(index);
            if (object.metaType() == QMetaType::fromType<QVariantMap>()) {
                const auto fields = object.toMap();
                if (fields.size() == 1) return fields.first();
            }
        }
        return QQmlDelegateModel::variantValue(index, role);
    }
};
} // namespace

ComboBoxModel::ComboBoxModel(QQmlContext* context, QObject* parent)
    : QObject(parent), m_context(context) {}
ComboBoxModel::~ComboBoxModel() {
    utils::disconnectAll(m_connections);
    disconnect(m_data_connection);
}
QVariant ComboBoxModel::model() const {
    if (m_model.metaType().flags().testFlag(QMetaType::PointerToQObject) && ! m_source) return {};
    return m_model;
}
QQmlComponent*     ComboBoxModel::delegate() const { return m_delegate; }
QQmlInstanceModel* ComboBoxModel::delegateModel() const { return m_instance; }
int                ComboBoxModel::count() const { return m_instance ? m_instance->count() : 0; }

void ComboBoxModel::setModel(const QVariant& value) {
    const auto normalized = value.metaType() == QMetaType::fromType<QJSValue>()
                                ? value.value<QJSValue>().toVariant()
                                : value;
    if (m_model == normalized) return;
    utils::disconnectAll(m_connections);
    disconnect(m_data_connection);
    auto old = std::move(m_owned);
    if (old) old->setParent(nullptr);
    m_instance = nullptr;
    m_model    = normalized;
    m_source   = normalized.value<QObject*>();
    m_instance = qobject_cast<QQmlInstanceModel*>(m_source);
    if (! m_instance && normalized.isValid() && ! normalized.isNull() && m_context) {
        m_owned = std::make_unique<ComboDelegateModel>(m_context, this);
        m_owned->setModel(normalized);
        m_owned->setDelegate(m_delegate);
        m_instance = m_owned.get();
        if (m_complete) m_owned->componentComplete();
    }
    observe();
    QPointer<ComboBoxModel> guard(this);
    Q_EMIT delegateModelChanged();
    if (! guard) return;
    updateCount();
    if (! guard) return;
    Q_EMIT modelChanged();
    if (guard) Q_EMIT changed();
}

void ComboBoxModel::observe() {
    if (m_instance) {
        m_connections.append(connect(
            m_instance, &QQmlInstanceModel::createdItem, this, [this](int, QObject* object) {
                Q_EMIT itemCreated(object);
            }));
        m_connections.append(connect(
            m_instance, &QQmlInstanceModel::countChanged, this, &ComboBoxModel::updateCount));
        m_connections.append(connect(m_instance, &QQmlInstanceModel::modelUpdated, this, [this]() {
            observeData();
            Q_EMIT changed();
        }));
    }
    if (m_source) {
        m_connections.append(connect(m_source, &QObject::destroyed, this, [this]() {
            setModel({});
        }));
    }
    observeData();
}

void ComboBoxModel::observeData() {
    disconnect(m_data_connection);
    // DelegateModel need not emit modelUpdated for data changes without live delegates.
    auto source = m_instance ? m_instance->abstractItemModel() : nullptr;
    if (source) {
        m_data_connection =
            connect(source, &QAbstractItemModel::dataChanged, this, &ComboBoxModel::changed);
    }
}

void ComboBoxModel::updateCount() {
    const auto value = count();
    if (m_count == value) return;
    m_count = value;
    Q_EMIT countChanged();
}

void ComboBoxModel::setDelegate(QQmlComponent* delegate) {
    if (m_delegate == delegate) return;
    m_delegate = delegate;
    if (m_owned) m_owned->setDelegate(delegate);
}

void ComboBoxModel::complete() {
    if (m_complete) return;
    m_complete = true;
    QPointer<ComboBoxModel> guard(this);
    if (m_owned) m_owned->componentComplete();
    if (! guard) return;
    updateCount();
    if (guard) Q_EMIT changed();
}

void ComboBoxModel::setTextRole(const QString& role) {
    if (m_text_role == role) return;
    m_text_role = role;
    Q_EMIT changed();
}
void ComboBoxModel::setValueRole(const QString& role) {
    if (m_value_role == role) return;
    m_value_role = role;
    Q_EMIT changed();
}
QVariant ComboBoxModel::roleAt(int index, const QString& role) const {
    if (! m_instance || index < 0 || index >= count()) return {};
    return m_instance->variantValue(index, role.isEmpty() ? QStringLiteral("modelData") : role);
}
QString  ComboBoxModel::textAt(int index) const { return roleAt(index, m_text_role).toString(); }
QVariant ComboBoxModel::valueAt(int index) const { return roleAt(index, m_value_role); }
int      ComboBoxModel::indexOfValue(const QVariant& value) const {
    for (int index = 0; index < count(); ++index)
        if (valueAt(index) == value) return index;
    return -1;
}
int ComboBoxModel::indexOf(QObject* object) const {
    return m_instance ? m_instance->indexOf(object, nullptr) : -1;
}

} // namespace qml_material
