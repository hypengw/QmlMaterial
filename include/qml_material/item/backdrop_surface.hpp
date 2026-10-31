#pragma once

#include "qml_material/core/item_activity.hpp"
#include "qml_material/export.hpp"
#include <QPointer>
#include <QQuickItem>

namespace qml_material
{
/** Same-window geometry and activity for a cropped backdrop capture. */
class QML_MATERIAL_API BackdropSurface : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(BackdropSurfaceBase)
    Q_PROPERTY(
        QQuickItem* sourceItem READ sourceItem WRITE setSourceItem NOTIFY sourceItemChanged FINAL)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(qreal sampleScale READ sampleScale WRITE setSampleScale NOTIFY samplingChanged FINAL)
    Q_PROPERTY(int maximumTextureSize READ maximumTextureSize WRITE setMaximumTextureSize NOTIFY
                   samplingChanged FINAL)
    Q_PROPERTY(qreal capturePadding READ capturePadding WRITE setCapturePadding NOTIFY
                   samplingChanged FINAL)
    Q_PROPERTY(bool captureEnabled READ captureEnabled NOTIFY captureChanged FINAL)
    Q_PROPERTY(QRectF captureRect READ captureRect NOTIFY captureChanged FINAL)
    Q_PROPERTY(QSize captureSize READ captureSize NOTIFY captureChanged FINAL)
public:
    explicit BackdropSurface(QQuickItem* parent = nullptr);
    ~BackdropSurface() override;
    QQuickItem*   sourceItem() const { return m_source; }
    bool          active() const { return m_active; }
    qreal         sampleScale() const { return m_scale; }
    int           maximumTextureSize() const { return m_maximum; }
    qreal         capturePadding() const { return m_padding; }
    bool          captureEnabled() const { return m_enabled; }
    QRectF        captureRect() const { return m_rect; }
    QSize         captureSize() const { return m_size; }
    void          setSourceItem(QQuickItem*);
    void          setActive(bool);
    void          setSampleScale(qreal);
    void          setMaximumTextureSize(int);
    void          setCapturePadding(qreal);
    Q_SIGNAL void sourceItemChanged();
    Q_SIGNAL void activeChanged();
    Q_SIGNAL void samplingChanged();
    Q_SIGNAL void captureChanged();

protected:
    void geometryChange(const QRectF&, const QRectF&) override;

private:
    void                           refresh();
    void                           bindWindow();
    QPointer<QQuickItem>           m_source;
    std::unique_ptr<ItemActivity>  m_activity;
    QList<QMetaObject::Connection> m_sourceConnections;
    QMetaObject::Connection        m_frameConnection;
    QRectF                         m_rect;
    QSize                          m_size;
    qreal                          m_scale = .5, m_padding = 32;
    int                            m_maximum = 2048;
    bool                           m_active = true, m_enabled = false;
};
} // namespace qml_material
