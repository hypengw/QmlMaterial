#pragma once

#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QPointer>

namespace qml_material
{
class StateHolder : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickItem* state READ state WRITE setState NOTIFY stateChanged FINAL)
public:
    StateHolder(QObject* parent = nullptr);
    ~StateHolder();

    auto state() const -> QQuickItem*;

public Q_SLOTS:
    void setState(QQuickItem*);

Q_SIGNALS:
    void stateChanged();

private:
    QQuickItem* m_state;
};
} // namespace qml_material