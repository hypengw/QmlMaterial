#pragma once

#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/model/item_source.hpp"
#include <QQmlComponent>
#include <memory>

namespace qml_material
{

class QML_MATERIAL_API LazyList : public Flickable {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(ItemSource* source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(
        qreal cacheExtent READ cacheExtent WRITE setCacheExtent NOTIFY cacheExtentChanged FINAL)
    Q_PROPERTY(qreal estimatedItemExtent READ estimatedItemExtent WRITE setEstimatedItemExtent
                   NOTIFY estimatedItemExtentChanged FINAL)
    Q_PROPERTY(int poolLimit READ poolLimit WRITE setPoolLimit NOTIFY poolLimitChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int firstVisibleIndex READ firstVisibleIndex NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int lastVisibleIndex READ lastVisibleIndex NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int liveCount READ liveCount NOTIFY layoutChanged FINAL)
    Q_PROPERTY(bool estimatedContentHeight READ estimatedContentHeight NOTIFY layoutChanged FINAL)
    Q_PROPERTY(QString errorString READ errorString NOTIFY layoutChanged FINAL)
    Q_PROPERTY(bool positioning READ positioning NOTIFY positioningChanged FINAL)
public:
    enum PositionMode
    {
        Immediate,
        Smooth
    };
    Q_ENUM(PositionMode)
    explicit LazyList(QQuickItem* parent = nullptr);
    ~LazyList() override;
    ItemSource*      source() const;
    void             setSource(ItemSource*);
    QQmlComponent*   delegate() const;
    void             setDelegate(QQmlComponent*);
    qreal            spacing() const;
    void             setSpacing(qreal);
    qreal            cacheExtent() const;
    void             setCacheExtent(qreal);
    qreal            estimatedItemExtent() const;
    void             setEstimatedItemExtent(qreal);
    int              poolLimit() const;
    void             setPoolLimit(int);
    int              count() const;
    int              firstVisibleIndex() const;
    int              lastVisibleIndex() const;
    int              liveCount() const;
    bool             estimatedContentHeight() const;
    QString          errorString() const;
    Q_INVOKABLE bool positionAtKey(const QString&, qreal alignment = 0, qreal offset = 0,
                                   PositionMode mode = Immediate);
    Q_INVOKABLE bool positionAtIndex(int index, qreal alignment = 0, qreal offset = 0,
                                     PositionMode mode = Immediate);
    bool             positioning() const;
    void             setContentY(qreal) override;
    Q_SIGNAL void    positioningChanged();
    Q_SIGNAL void    positioningFinished(const QString& key, bool success);
    Q_SIGNAL void    sourceChanged();
    Q_SIGNAL void    delegateChanged();
    Q_SIGNAL void    spacingChanged();
    Q_SIGNAL void    cacheExtentChanged();
    Q_SIGNAL void    estimatedItemExtentChanged();
    Q_SIGNAL void    poolLimitChanged();
    Q_SIGNAL void    layoutChanged();

protected:
    void updatePolish() override;
    void componentComplete() override;
    void geometryChange(const QRectF&, const QRectF&) override;
    void viewportMoved(Qt::Orientations) override;
    void scrollInputStarted() override;
    void scrollActivityCancelled() override;

private:
    void scheduleLayout();
    void finishPositioning(bool success);
    struct Private;
    std::shared_ptr<Private> d;
};

} // namespace qml_material
