#pragma once

#include <QQuickItem>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariant>
#include <QVector>
#include <memory>

#include "qml_material/carousel/carousel_attached.hpp"
#include "qml_material/carousel/carousel_strategy.hpp"


namespace qml_material
{

class CarouselView : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(CarouselAttached)

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int layout READ layout WRITE setLayout NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    Q_PROPERTY(qreal itemExtent READ itemExtent WRITE setItemExtent NOTIFY itemExtentChanged FINAL)
    Q_PROPERTY(qreal minSmallItemWidth READ minSmallItemWidth WRITE setMinSmallItemWidth NOTIFY
                   minSmallItemWidthChanged FINAL)
    Q_PROPERTY(qreal maxSmallItemWidth READ maxSmallItemWidth WRITE setMaxSmallItemWidth NOTIFY
                   maxSmallItemWidthChanged FINAL)
    Q_PROPERTY(QVariantList flexWeights READ flexWeights WRITE setFlexWeights NOTIFY flexWeightsChanged FINAL)
    Q_PROPERTY(bool itemSnapping READ itemSnapping WRITE setItemSnapping NOTIFY itemSnappingChanged FINAL)
    Q_PROPERTY(qreal shrinkExtent READ shrinkExtent WRITE setShrinkExtent NOTIFY shrinkExtentChanged FINAL)
    Q_PROPERTY(bool infinite READ infinite WRITE setInfinite NOTIFY infiniteChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(int initialItem READ initialItem WRITE setInitialItem NOTIFY initialItemChanged FINAL)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(qreal contentPaddingStart READ contentPaddingStart WRITE setContentPaddingStart NOTIFY
                   contentPaddingChanged FINAL)
    Q_PROPERTY(qreal contentPaddingEnd READ contentPaddingEnd WRITE setContentPaddingEnd NOTIFY
                   contentPaddingChanged FINAL)
    Q_PROPERTY(qreal contentPaddingCross READ contentPaddingCross WRITE setContentPaddingCross NOTIFY
                   contentPaddingCrossChanged FINAL)
    Q_PROPERTY(bool clipContainer READ clipContainer WRITE setClipContainer NOTIFY clipContainerChanged FINAL)
    Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY reduceMotionChanged FINAL)
    Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged FINAL)
    Q_PROPERTY(qreal itemCornerRadius READ itemCornerRadius WRITE setItemCornerRadius NOTIFY
                   itemCornerRadiusChanged FINAL)
    Q_PROPERTY(qreal debugScrollOffset READ debugScrollOffset WRITE setDebugScrollOffset FINAL)
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentWidthChanged FINAL)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentHeightChanged FINAL)
    Q_PROPERTY(QQuickItem* flickable READ flickable CONSTANT FINAL)

public:
    explicit CarouselView(QQuickItem* parent = nullptr);
    ~CarouselView() override;

    static CarouselAttached* qmlAttachedProperties(QObject* object);

    QVariant       model() const;
    void           setModel(const QVariant& model);
    QQmlComponent* delegate() const;
    void           setDelegate(QQmlComponent* delegate);
    int            count() const;

    int  layout() const;
    void setLayout(int layout);
    int  alignment() const;
    void setAlignment(int alignment);
    int  orientation() const;
    void setOrientation(int orientation);

    qreal itemExtent() const;
    void  setItemExtent(qreal extent);
    qreal minSmallItemWidth() const;
    void  setMinSmallItemWidth(qreal width);
    qreal maxSmallItemWidth() const;
    void  setMaxSmallItemWidth(qreal width);
    QVariantList flexWeights() const;
    void         setFlexWeights(const QVariantList& weights);
    bool itemSnapping() const;
    void setItemSnapping(bool snapping);
    qreal shrinkExtent() const;
    void  setShrinkExtent(qreal extent);
    bool infinite() const;
    void setInfinite(bool infinite);
    int  currentIndex() const;
    void setCurrentIndex(int index);
    int  initialItem() const;
    void setInitialItem(int item);
    qreal spacing() const;
    void  setSpacing(qreal spacing);
    qreal contentPaddingStart() const;
    void  setContentPaddingStart(qreal padding);
    qreal contentPaddingEnd() const;
    void  setContentPaddingEnd(qreal padding);
    qreal contentPaddingCross() const;
    void  setContentPaddingCross(qreal padding);
    bool  clipContainer() const;
    void  setClipContainer(bool clip);
    bool  reduceMotion() const;
    void  setReduceMotion(bool reduce);
    qreal itemCornerRadius() const;
    void  setItemCornerRadius(qreal radius);
    qreal debugScrollOffset() const;
    void  setDebugScrollOffset(qreal offset);
    bool interactive() const;
    void setInteractive(bool interactive);
    qreal contentWidth() const;
    qreal contentHeight() const;
    QQuickItem* flickable() const;

    Q_INVOKABLE void incrementCurrentIndex();
    Q_INVOKABLE void decrementCurrentIndex();
    Q_INVOKABLE void setCurrentIndexAnimated(int index);

    Q_SIGNAL void modelChanged();
    Q_SIGNAL void delegateChanged();
    Q_SIGNAL void countChanged();
    Q_SIGNAL void layoutChanged();
    Q_SIGNAL void alignmentChanged();
    Q_SIGNAL void orientationChanged();
    Q_SIGNAL void itemExtentChanged();
    Q_SIGNAL void minSmallItemWidthChanged();
    Q_SIGNAL void maxSmallItemWidthChanged();
    Q_SIGNAL void flexWeightsChanged();
    Q_SIGNAL void itemSnappingChanged();
    Q_SIGNAL void shrinkExtentChanged();
    Q_SIGNAL void infiniteChanged();
    Q_SIGNAL void currentIndexChanged();
    Q_SIGNAL void initialItemChanged();
    Q_SIGNAL void spacingChanged();
    Q_SIGNAL void contentPaddingChanged();
    Q_SIGNAL void contentPaddingCrossChanged();
    Q_SIGNAL void clipContainerChanged();
    Q_SIGNAL void reduceMotionChanged();
    Q_SIGNAL void itemCornerRadiusChanged();
    Q_SIGNAL void interactiveChanged();
    Q_SIGNAL void contentWidthChanged();
    Q_SIGNAL void contentHeightChanged();
    Q_SIGNAL void clicked(int index);

protected:
    void componentComplete() override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private Q_SLOTS:
    void onDelegateClicked();
    void onDelegateAspectRatioChanged();
    void onSnapTick();

private:
    void rebuildItems();
    void updateLayout();
    void updateCount();
    void snapAfterGesture();
    void applySnapAnimation(qreal targetOffset);
    void finishSnap(qreal targetOffset);
    qreal snapTargetForGesture(qreal offset, qreal velocity) const;
    int  snapIndexForOffset(qreal offset) const;
    int  snapIndexForFling(qreal offset, qreal velocity) const;
    bool usesSingleAdvanceFling() const;
    bool usesFreeScrollSnap() const;
    QVariant    modelDataAt(int index) const;
    QVariantMap initialPropertiesForDelegate(int index) const;
    void        applyDelegateProperties(QQuickItem* item, int index) const;
    void        createDelegate(int index);
    void     destroyDelegate(int index);
    void     positionItem(QQuickItem* item, const CarouselItemGeometry& geom, bool isActive);
    int      activeIndexForLayout(const CarouselLayoutOutput& out) const;

    QVariant                         m_model;
    QQmlComponent*                   m_delegate = nullptr;
    int                              m_count    = 0;
    int                              m_layout   = 0;
    int                              m_alignment = 0;
    Qt::Orientation                  m_orientation = Qt::Horizontal;
    qreal                            m_item_extent = 180;
    qreal                            m_min_small_item_width = 40;
    qreal                            m_max_small_item_width = 56;
    qreal                            m_item_corner_radius = 28;
    QVariantList                     m_flex_weights;
    bool                             m_item_snapping = true;
    qreal                            m_shrink_extent = 0;
    bool                             m_infinite      = false;
    int                              m_current_index = 0;
    int                              m_initial_item  = 0;
    qreal                            m_spacing       = 8;
    qreal                            m_content_padding_start = 0;
    qreal                            m_content_padding_end   = 0;
    qreal                            m_content_padding_cross = 0;
    bool                             m_clip_container = true;
    bool                             m_reduce_motion  = false;
    bool                             m_interactive   = true;
    bool                             m_snapping      = false;
    qreal                            m_content_width  = 0;
    qreal                            m_content_height = 0;
    bool                             m_completed     = false;

    qreal                            m_snap_pos       = 0;
    qreal                            m_snap_velocity  = 0;
    qreal                            m_snap_target    = 0;
    class QTimer*                    m_snap_timer     = nullptr;

    QQuickItem*                      m_flickable = nullptr;
    QQuickItem*                      m_content    = nullptr;
    QVector<QQuickItem*>             m_items;
    QVector<CarouselItemGeometry>    m_geometries;
    QVector<qreal>                   m_snap_offsets;
    QVector<qreal>                   m_item_aspects;
    qreal                            m_max_scroll_offset = 0;
    qreal                            m_end_snap_offset   = 0;
    qreal                            m_scroll_step       = 0;
    bool                             m_updating    = false;
};

} // namespace qml_material
