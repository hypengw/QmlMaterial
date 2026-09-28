#pragma once

#include <QObject>
#include <QProperty>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API SearchState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        QString query READ query WRITE setQuery NOTIFY queryChanged BINDABLE bindableQuery FINAL)
public:
    explicit SearchState(QObject* parent = nullptr): QObject(parent) {}
    QString            query() const { return m_query; }
    void               setQuery(const QString& value) { m_query = value; }
    QBindable<QString> bindableQuery() { return &m_query; }
    Q_SIGNAL void      queryChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(SearchState, QString, m_query, &SearchState::queryChanged)
};
} // namespace qml_material
