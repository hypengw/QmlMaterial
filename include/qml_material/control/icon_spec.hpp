#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <QPointer>
#include <QUrl>
#include <optional>
#include <qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API ActionIcon : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by Action")
    Q_PROPERTY(QString name READ name WRITE setName RESET resetName NOTIFY nameChanged FINAL)
    Q_PROPERTY(QUrl source READ source WRITE setSource RESET resetSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor RESET resetColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(bool cache READ cache WRITE setCache RESET resetCache NOTIFY cacheChanged FINAL)
    Q_PROPERTY(bool fill READ fill WRITE setFill RESET resetFill NOTIFY fillChanged FINAL)
    Q_PROPERTY(int weight READ weight WRITE setWeight RESET resetWeight NOTIFY weightChanged FINAL)
    Q_PROPERTY(int renderType READ renderType WRITE setRenderType RESET resetRenderType NOTIFY
                   renderTypeChanged FINAL)
    Q_PROPERTY(QUrl resolvedSource READ resolvedSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY contentChanged FINAL)
    Q_PROPERTY(bool image READ isImage NOTIFY contentChanged FINAL)
public:
    explicit ActionIcon(QObject* parent = nullptr): QObject(parent) {}
    QString       name() const { return m_name; }
    void          setName(const QString& value);
    void          resetName();
    Q_SIGNAL void nameChanged();
    QUrl          source() const { return m_source; }
    void          setSource(const QUrl& value);
    void          resetSource();
    Q_SIGNAL void sourceChanged();
    QColor        color() const { return m_color; }
    void          setColor(const QColor& value);
    void          resetColor();
    Q_SIGNAL void colorChanged();
    bool          cache() const { return m_cache; }
    void          setCache(bool value);
    void          resetCache();
    Q_SIGNAL void cacheChanged();
    bool          fill() const { return m_fill; }
    void          setFill(bool value);
    void          resetFill();
    Q_SIGNAL void fillChanged();
    int           weight() const { return m_weight; }
    void          setWeight(int value);
    void          resetWeight();
    Q_SIGNAL void weightChanged();
    int           renderType() const { return m_renderType; }
    void          setRenderType(int value);
    void          resetRenderType();
    Q_SIGNAL void renderTypeChanged();
    QUrl          resolvedSource() const;
    bool          isEmpty() const { return m_name.isEmpty() && m_source.isEmpty(); }
    bool          isImage() const { return ! m_source.isEmpty(); }
    Q_SIGNAL void contentChanged();
    Q_SIGNAL void changed();

protected:
    void setOverride(ActionIcon*);

private:
    void                           refresh();
    const ActionIcon*              contentOwner() const;
    std::optional<QString>         m_local_name;
    std::optional<QUrl>            m_local_source;
    std::optional<QColor>          m_local_color;
    std::optional<bool>            m_local_cache;
    std::optional<bool>            m_local_fill;
    std::optional<int>             m_local_weight;
    std::optional<int>             m_local_renderType;
    QString                        m_name       = QString();
    QUrl                           m_source     = QUrl();
    QColor                         m_color      = QColor(Qt::transparent);
    bool                           m_cache      = true;
    bool                           m_fill       = false;
    int                            m_weight     = QFont::Normal;
    int                            m_renderType = 2;
    QUrl                           m_resolved_source;
    QPointer<ActionIcon>           m_override;
    QList<QMetaObject::Connection> m_connections;
};

class QML_MATERIAL_API IconSpec : public ActionIcon {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int width READ width WRITE setWidth RESET resetWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(int height READ height WRITE setHeight RESET resetHeight NOTIFY heightChanged FINAL)
public:
    explicit IconSpec(QObject* parent = nullptr): ActionIcon(parent) {}
    void          setActionIcon(ActionIcon* value) { setOverride(value); }
    int           width() const { return m_width.value_or(0); }
    int           height() const { return m_height.value_or(0); }
    void          setWidth(int);
    void          setHeight(int);
    void          resetWidth();
    void          resetHeight();
    Q_SIGNAL void widthChanged();
    Q_SIGNAL void heightChanged();

private:
    std::optional<int> m_width, m_height;
};
} // namespace qml_material
