#pragma once

#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/model/item_source.hpp"
#include "qml_material/model/item_demand.hpp"
#include <QQmlComponent>
#include <memory>

namespace qml_material
{

class LazyViewLayout;

class QML_MATERIAL_API LazyView : public Flickable {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged FINAL)
    Q_PROPERTY(ItemSource* source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(
        qreal cacheExtent READ cacheExtent WRITE setCacheExtent NOTIFY cacheExtentChanged FINAL)
    Q_PROPERTY(int poolLimit READ poolLimit WRITE setPoolLimit NOTIFY poolLimitChanged FINAL)
    Q_PROPERTY(
        bool asynchronous READ asynchronous WRITE setAsynchronous NOTIFY asynchronousChanged FINAL)
    Q_PROPERTY(qreal cachePreparationBudget READ cachePreparationBudget WRITE
                   setCachePreparationBudget NOTIFY cachePreparationBudgetChanged FINAL)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int firstVisibleIndex READ firstVisibleIndex NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int lastVisibleIndex READ lastVisibleIndex NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int anchorIndex READ anchorIndex NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int liveCount READ liveCount NOTIFY layoutChanged FINAL)
    Q_PROPERTY(bool estimatedContentWidth READ estimatedContentWidth NOTIFY layoutChanged FINAL)
    Q_PROPERTY(bool estimatedContentHeight READ estimatedContentHeight NOTIFY layoutChanged FINAL)
    Q_PROPERTY(QString errorString READ errorString NOTIFY layoutChanged FINAL)
    Q_PROPERTY(bool positioning READ positioning NOTIFY positioningChanged FINAL)
    Q_PROPERTY(qml_material::ItemDemand currentDemand READ currentDemand NOTIFY itemsNeeded FINAL)
    Q_PROPERTY(QString currentKey READ currentKey WRITE setCurrentKey NOTIFY currentChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged FINAL)
public:
    enum PositionMode
    {
        Immediate,
        Smooth
    };
    Q_ENUM(PositionMode)
    ~LazyView() override;
    Qt::Orientation     orientation() const;
    void                setOrientation(Qt::Orientation);
    Q_SIGNAL void       orientationChanged();
    ItemSource*         source() const;
    void                setSource(ItemSource*);
    QQmlComponent*      delegate() const;
    void                setDelegate(QQmlComponent*);
    qreal               cacheExtent() const;
    void                setCacheExtent(qreal);
    int                 poolLimit() const;
    void                setPoolLimit(int);
    bool                asynchronous() const;
    void                setAsynchronous(bool);
    qreal               cachePreparationBudget() const;
    void                setCachePreparationBudget(qreal);
    int                 pendingCount() const;
    int                 count() const;
    int                 firstVisibleIndex() const;
    int                 lastVisibleIndex() const;
    int                 anchorIndex() const;
    int                 liveCount() const;
    bool                estimatedContentHeight() const;
    bool                estimatedContentWidth() const;
    QString             errorString() const;
    Q_INVOKABLE bool    positionAtKey(const QString&, qreal alignment = 0, qreal offset = 0,
                                      PositionMode mode = Immediate);
    Q_INVOKABLE bool    positionAtIndex(int index, qreal alignment = 0, qreal offset = 0,
                                        PositionMode mode = Immediate);
    QString             currentKey() const;
    void                setCurrentKey(const QString&);
    int                 currentIndex() const;
    void                setCurrentIndex(int);
    Q_INVOKABLE QString keyAtIndex(int) const;
    Q_INVOKABLE QRectF  itemRect(int) const;
    Q_SIGNAL void       currentChanged();
    ItemDemand          currentDemand() const;
    Q_SIGNAL void       itemsNeeded(const qml_material::ItemDemand& demand);
    bool                positioning() const;
    void                setContentY(qreal) override;
    void                setContentX(qreal) override;
    Q_SIGNAL void       positioningChanged();
    Q_SIGNAL void       positioningFinished(const QString& key, bool success);
    Q_SIGNAL void       sourceChanged();
    Q_SIGNAL void       delegateChanged();
    Q_SIGNAL void       cacheExtentChanged();
    Q_SIGNAL void       poolLimitChanged();
    Q_SIGNAL void       asynchronousChanged();
    Q_SIGNAL void       cachePreparationBudgetChanged();
    Q_SIGNAL void       layoutChanged();

protected:
    explicit LazyView(std::unique_ptr<LazyViewLayout>, QQuickItem* parent);
    LazyViewLayout& layoutEngine() const;
    void            invalidateLayout();
    void            keyPressEvent(QKeyEvent*) override;
    void            updatePolish() override;
    void            componentComplete() override;
    void            geometryChange(const QRectF&, const QRectF&) override;
    void            viewportMoved(Qt::Orientations) override;
    void            scrollInputStarted() override;
    void            scrollActivityCancelled() override;

private:
    void invalidateDemand();
    void scheduleLayout();
    void schedulePreparation();
    void cancelPreparations();
    void finishPositioning(bool success);
    struct Private;
    std::shared_ptr<Private> d;
};

} // namespace qml_material
