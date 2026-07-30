#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <qqmlregistration.h>

#include "qml_material/export.hpp"

class QQuickItem;

namespace qml_material
{

class ToolTipManager;

class QML_MATERIAL_API ToolTipAttached final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged FINAL)
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout NOTIFY timeoutChanged FINAL)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(QObject* toolTip READ toolTip CONSTANT FINAL)

public:
    explicit ToolTipAttached(QObject* parent = nullptr);
    ~ToolTipAttached() override;

    QString text() const;
    void    setText(const QString& text);

    int  delay() const;
    void setDelay(int delay);

    int  timeout() const;
    void setTimeout(int timeout);

    bool visible() const;
    void setVisible(bool visible);

    QObject* toolTip() const;

    Q_INVOKABLE void show(const QString& text, int timeout = -1);
    Q_INVOKABLE void hide();

    Q_SIGNAL void textChanged();
    Q_SIGNAL void delayChanged();
    Q_SIGNAL void timeoutChanged();
    Q_SIGNAL void visibleChanged();

private:
    friend class ToolTipManager;

    ToolTipManager* manager(bool create) const;
    QQuickItem*     target() const;
    void            notifyVisibleChanged();

    QString m_text;
    int     m_delay { 500 };
    int     m_timeout { -1 };
};

/**
 * Provides the Material plain tooltip attached properties.
 * @ingroup control
 */
class QML_MATERIAL_API ToolTip : public QObject {
    Q_OBJECT

    QML_NAMED_ELEMENT(ToolTip)
    QML_UNCREATABLE("ToolTip is only available as an attached property")
    QML_ATTACHED(ToolTipAttached)

public:
    explicit ToolTip(QObject* parent = nullptr);

    static ToolTipAttached* qmlAttachedProperties(QObject* object);
};

} // namespace qml_material
