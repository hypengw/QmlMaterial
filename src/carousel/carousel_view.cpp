#include "qml_material/carousel/carousel_view.hpp"

#include <QAbstractItemModel>
#include <QJSValue>
#include <QMetaObject>
#include <QPropertyAnimation>
#include <QAbstractAnimation>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QTimer>
#include <private/qquickflickable_p.h>
#include <QSet>
#include <QVariantMap>
#include <QtMath>
#include <limits>

namespace qml_material
{

namespace
{

constexpr int kSnapDurationMs = 400; // MD.Token.carousel.snap_duration
constexpr qreal kSnapStiffness  = 280.0;
constexpr qreal kSnapDamping    = 28.0;
constexpr qreal kSnapDt         = 1.0 / 60.0;
constexpr int kLayoutUncontained = 0;
constexpr int kLayoutFullScreen  = 4;
constexpr int kLayoutUncontainedMultiAspect = 5;
constexpr int kSizeLarge           = 2;

QQuickFlickable* asFlickable(QQuickItem* item)
{
    return qobject_cast<QQuickFlickable*>(item);
}

QJSValue modelAsJsValue(const QVariant& model)
{
    if (model.metaType() == QMetaType::fromType<QJSValue>()) {
        return model.value<QJSValue>();
    }
    return {};
}

int modelArrayLength(const QVariant& model)
{
    if (model.canConvert<QVariantList>()) {
        return model.toList().size();
    }
    const QJSValue js = modelAsJsValue(model);
    if (js.isArray()) {
        return js.property(QStringLiteral("length")).toInt();
    }
    return 0;
}

QVariant normalizeModelRow(const QVariant& row)
{
    if (row.metaType() == QMetaType::fromType<QJSValue>()) {
        return row.value<QJSValue>().toVariant(QJSValue::ConvertJSObjects);
    }
    return row;
}

QVariant modelElementAt(const QVariant& model, int index)
{
    if (model.canConvert<QVariantList>()) {
        const auto list = model.toList();
        if (index >= 0 && index < list.size()) {
            return normalizeModelRow(list.at(index));
        }
    }
    const QJSValue js = modelAsJsValue(model);
    if (js.isArray()) {
        return normalizeModelRow(js.property(index).toVariant(QJSValue::ConvertJSObjects));
    }
    if (auto* item_model = qobject_cast<QAbstractItemModel*>(model.value<QObject*>())) {
        if (index < 0 || index >= item_model->rowCount()) {
            return {};
        }
        QVariantMap row;
        const auto roles = item_model->roleNames();
        const QModelIndex idx = item_model->index(index, 0);
        for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
            row[QString::fromUtf8(it.value())] = item_model->data(idx, it.key());
        }
        return row;
    }
    return index;
}

} // namespace

CarouselView::CarouselView(QQuickItem* parent): QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setClip(true);

    auto* flick = new QQuickFlickable(this);
    m_flickable = flick;
    flick->setClip(true);
    flick->setBoundsBehavior(QQuickFlickable::StopAtBounds);
    m_content = new QQuickItem();
    m_content->setParentItem(flick->contentItem());
    connect(flick, &QQuickFlickable::contentXChanged, this, [this]() {
        if (!m_updating) {
            updateLayout();
        }
    });
    connect(flick, &QQuickFlickable::contentYChanged, this, [this]() {
        if (!m_updating) {
            updateLayout();
        }
    });
    connect(flick, &QQuickFlickable::movementEnded, this, [this]() {
        if (m_item_snapping && !m_snapping) {
            snapAfterGesture();
        }
    });

    m_snap_timer = new QTimer(this);
    m_snap_timer->setInterval(16);
    connect(m_snap_timer, &QTimer::timeout, this, &CarouselView::onSnapTick);
}

CarouselView::~CarouselView()
{
    for (auto* item : m_items) {
        if (item) {
            item->deleteLater();
        }
    }
}

CarouselAttached* CarouselView::qmlAttachedProperties(QObject* object)
{
    return new CarouselAttached(object);
}

QVariant CarouselView::model() const { return m_model; }

void CarouselView::setModel(const QVariant& model)
{
    if (m_model == model) {
        return;
    }
    m_model = model;
    updateCount();
    rebuildItems();
    Q_EMIT modelChanged();
}

QQmlComponent* CarouselView::delegate() const { return m_delegate; }

void CarouselView::setDelegate(QQmlComponent* delegate)
{
    if (m_delegate == delegate) {
        return;
    }
    m_delegate = delegate;
    rebuildItems();
    Q_EMIT delegateChanged();
}

int CarouselView::count() const { return m_count; }

int CarouselView::layout() const { return m_layout; }

void CarouselView::setLayout(int layout)
{
    if (m_layout == layout) {
        return;
    }
    m_layout = layout;
    updateLayout();
    Q_EMIT layoutChanged();
}

int CarouselView::alignment() const { return m_alignment; }

void CarouselView::setAlignment(int alignment)
{
    if (m_alignment == alignment) {
        return;
    }
    m_alignment = alignment;
    updateLayout();
    Q_EMIT alignmentChanged();
}

int CarouselView::orientation() const { return static_cast<int>(m_orientation); }

void CarouselView::setOrientation(int orientation)
{
    const auto o = orientation == Qt::Vertical ? Qt::Vertical : Qt::Horizontal;
    if (m_orientation == o) {
        return;
    }
    m_orientation = o;
    if (auto* flick = asFlickable(m_flickable)) {
        flick->setContentWidth(o == Qt::Horizontal ? m_content_width : width());
        flick->setContentHeight(o == Qt::Vertical ? m_content_height : height());
    }
    updateLayout();
    Q_EMIT orientationChanged();
}

qreal CarouselView::itemExtent() const { return m_item_extent; }

void CarouselView::setItemExtent(qreal extent)
{
    if (qFuzzyCompare(m_item_extent, extent)) {
        return;
    }
    m_item_extent = extent;
    updateLayout();
    Q_EMIT itemExtentChanged();
}

qreal CarouselView::minSmallItemWidth() const { return m_min_small_item_width; }

void CarouselView::setMinSmallItemWidth(qreal width)
{
    if (qFuzzyCompare(m_min_small_item_width, width)) {
        return;
    }
    m_min_small_item_width = width;
    updateLayout();
    Q_EMIT minSmallItemWidthChanged();
}

qreal CarouselView::maxSmallItemWidth() const { return m_max_small_item_width; }

void CarouselView::setMaxSmallItemWidth(qreal width)
{
    if (qFuzzyCompare(m_max_small_item_width, width)) {
        return;
    }
    m_max_small_item_width = width;
    updateLayout();
    Q_EMIT maxSmallItemWidthChanged();
}

QVariantList CarouselView::flexWeights() const { return m_flex_weights; }

void CarouselView::setFlexWeights(const QVariantList& weights)
{
    if (m_flex_weights == weights) {
        return;
    }
    m_flex_weights = weights;
    updateLayout();
    Q_EMIT flexWeightsChanged();
}

bool CarouselView::itemSnapping() const { return m_item_snapping; }

void CarouselView::setItemSnapping(bool snapping)
{
    if (m_item_snapping == snapping) {
        return;
    }
    m_item_snapping = snapping;
    Q_EMIT itemSnappingChanged();
}

qreal CarouselView::shrinkExtent() const { return m_shrink_extent; }

void CarouselView::setShrinkExtent(qreal extent)
{
    if (qFuzzyCompare(m_shrink_extent, extent)) {
        return;
    }
    m_shrink_extent = extent;
    updateLayout();
    Q_EMIT shrinkExtentChanged();
}

bool CarouselView::infinite() const { return m_infinite; }

void CarouselView::setInfinite(bool infinite)
{
    if (m_infinite == infinite) {
        return;
    }
    m_infinite = infinite;
    Q_EMIT infiniteChanged();
}

int CarouselView::currentIndex() const { return m_current_index; }

void CarouselView::setCurrentIndex(int index)
{
    const int clamped = m_count > 0 ? qBound(0, index, m_count - 1) : 0;
    if (m_current_index == clamped) {
        return;
    }
    m_current_index = clamped;
    Q_EMIT currentIndexChanged();
}

int CarouselView::initialItem() const { return m_initial_item; }

void CarouselView::setInitialItem(int item)
{
    if (m_initial_item == item) {
        return;
    }
    m_initial_item = item;
    Q_EMIT initialItemChanged();
}

qreal CarouselView::spacing() const { return m_spacing; }

void CarouselView::setSpacing(qreal spacing)
{
    if (qFuzzyCompare(m_spacing, spacing)) {
        return;
    }
    m_spacing = spacing;
    updateLayout();
    Q_EMIT spacingChanged();
}

qreal CarouselView::contentPaddingStart() const { return m_content_padding_start; }

void CarouselView::setContentPaddingStart(qreal padding)
{
    if (qFuzzyCompare(m_content_padding_start, padding)) {
        return;
    }
    m_content_padding_start = padding;
    updateLayout();
    Q_EMIT contentPaddingChanged();
}

qreal CarouselView::contentPaddingEnd() const { return m_content_padding_end; }

void CarouselView::setContentPaddingEnd(qreal padding)
{
    if (qFuzzyCompare(m_content_padding_end, padding)) {
        return;
    }
    m_content_padding_end = padding;
    updateLayout();
    Q_EMIT contentPaddingChanged();
}

qreal CarouselView::contentPaddingCross() const { return m_content_padding_cross; }

void CarouselView::setContentPaddingCross(qreal padding)
{
    if (qFuzzyCompare(m_content_padding_cross, padding)) {
        return;
    }
    m_content_padding_cross = padding;
    updateLayout();
    Q_EMIT contentPaddingCrossChanged();
}

bool CarouselView::clipContainer() const { return m_clip_container; }

void CarouselView::setClipContainer(bool clip)
{
    if (m_clip_container == clip) {
        return;
    }
    m_clip_container = clip;
    setClip(clip);
    if (auto* flick = asFlickable(m_flickable)) {
        // Viewport always clips so uncontained items slide off edges instead of
        // shrinking via per-item mask.
        flick->setClip(true);
    }
    Q_EMIT clipContainerChanged();
}

bool CarouselView::reduceMotion() const { return m_reduce_motion; }

void CarouselView::setReduceMotion(bool reduce)
{
    if (m_reduce_motion == reduce) {
        return;
    }
    m_reduce_motion = reduce;
    Q_EMIT reduceMotionChanged();
    updateLayout();
}

qreal CarouselView::itemCornerRadius() const { return m_item_corner_radius; }

void CarouselView::setItemCornerRadius(qreal radius)
{
    if (qFuzzyCompare(m_item_corner_radius, radius)) {
        return;
    }
    m_item_corner_radius = radius;
    updateLayout();
    Q_EMIT itemCornerRadiusChanged();
}

qreal CarouselView::debugScrollOffset() const
{
    if (!m_flickable) {
        return 0;
    }
    auto* flick = asFlickable(m_flickable);
    return flick ? (m_orientation == Qt::Horizontal ? flick->contentX() : flick->contentY()) : 0;
}

void CarouselView::setDebugScrollOffset(qreal offset)
{
    if (!m_flickable) {
        return;
    }
    auto* flick = asFlickable(m_flickable);
    if (!flick) {
        return;
    }
    m_updating = true;
    if (m_orientation == Qt::Horizontal) {
        flick->setContentX(offset);
    } else {
        flick->setContentY(offset);
    }
    m_updating = false;
    updateLayout();
}

bool CarouselView::interactive() const { return m_interactive; }

void CarouselView::setInteractive(bool interactive)
{
    if (m_interactive == interactive) {
        return;
    }
    m_interactive = interactive;
    if (auto* flick = asFlickable(m_flickable)) {
        flick->setInteractive(interactive);
    }
    Q_EMIT interactiveChanged();
}

qreal CarouselView::contentWidth() const { return m_content_width; }
qreal CarouselView::contentHeight() const { return m_content_height; }

QQuickItem* CarouselView::flickable() const { return m_flickable; }

void CarouselView::incrementCurrentIndex()
{
    if (m_count <= 0) {
        return;
    }
    setCurrentIndexAnimated(qMin(m_count - 1, m_current_index + 1));
}

void CarouselView::decrementCurrentIndex()
{
    if (m_count <= 0) {
        return;
    }
    setCurrentIndexAnimated(qMax(0, m_current_index - 1));
}

void CarouselView::setCurrentIndexAnimated(int index)
{
    const int clamped = m_count > 0 ? qBound(0, index, m_count - 1) : 0;
    if (!m_flickable || m_snap_offsets.isEmpty()) {
        setCurrentIndex(clamped);
        updateLayout();
        return;
    }
    qreal target = 0;
    if (clamped == 0) {
        target = 0;
    } else if (clamped == m_count - 1) {
        target = m_end_snap_offset;
    } else if (clamped >= 0 && clamped < m_snap_offsets.size()) {
        target = m_snap_offsets.at(clamped);
    }
    applySnapAnimation(target);
}

void CarouselView::componentComplete()
{
    QQuickItem::componentComplete();
    m_completed = true;
    setClip(m_clip_container);
    if (auto* flick = asFlickable(m_flickable)) {
        flick->setInteractive(m_interactive);
        flick->setClip(true);
    }
    updateCount();
    rebuildItems();
    if (m_count > 0 && m_initial_item > 0) {
        setCurrentIndexAnimated(qBound(0, m_initial_item, m_count - 1));
    }
}

void CarouselView::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (m_flickable) {
        m_flickable->setWidth(newGeometry.width());
        m_flickable->setHeight(newGeometry.height());
    }
    updateLayout();
}

void CarouselView::itemChange(ItemChange change, const ItemChangeData& value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemEnabledHasChanged && m_flickable) {
        m_flickable->setEnabled(value.boolValue);
    }
}

void CarouselView::updateCount()
{
    int new_count = 0;
    if (m_model.userType() == QMetaType::Int || m_model.userType() == QMetaType::UInt
        || m_model.userType() == QMetaType::LongLong) {
        new_count = m_model.toInt();
    } else if (auto* model = qobject_cast<QAbstractItemModel*>(m_model.value<QObject*>())) {
        new_count = model->rowCount();
    } else {
        new_count = modelArrayLength(m_model);
    }
    if (new_count != m_count) {
        m_count = new_count;
        Q_EMIT countChanged();
    }
}

QVariant CarouselView::modelDataAt(int index) const
{
    if (m_model.userType() == QMetaType::Int || m_model.userType() == QMetaType::UInt
        || m_model.userType() == QMetaType::LongLong) {
        return index;
    }
    return modelElementAt(m_model, index);
}

QVariantMap CarouselView::initialPropertiesForDelegate(int index) const
{
    QVariantMap initial;
    initial[QStringLiteral("index")] = index;

    const QVariant row = modelDataAt(index);
    if (auto* engine = qmlEngine(this)) {
        initial[QStringLiteral("model")] = QVariant::fromValue(engine->toScriptValue(row));
    } else {
        initial[QStringLiteral("model")] = row;
    }

    const QVariantMap row_map = row.toMap();
    for (auto it = row_map.cbegin(); it != row_map.cend(); ++it) {
        initial[it.key()] = it.value();
    }
    return initial;
}

void CarouselView::applyDelegateProperties(QQuickItem* item, int index) const
{
    if (!item) {
        return;
    }
    const auto initial = initialPropertiesForDelegate(index);
    for (auto it = initial.cbegin(); it != initial.cend(); ++it) {
        item->setProperty(it.key().toUtf8().constData(), it.value());
    }
}

void CarouselView::createDelegate(int index)
{
    if (!m_delegate || index < 0 || index >= m_count) {
        return;
    }
    while (m_items.size() <= index) {
        m_items.append(nullptr);
    }
    if (m_items[index]) {
        applyDelegateProperties(m_items[index], index);
        const qreal aspect = m_items[index]->property("itemAspectRatio").toDouble();
        if (aspect > 0) {
            while (m_item_aspects.size() < m_count) {
                m_item_aspects.append(1.0);
            }
            if (!qFuzzyCompare(m_item_aspects[index], aspect)) {
                m_item_aspects[index] = aspect;
                updateLayout();
            }
        }
        return;
    }

    QQmlContext* ctx = m_delegate->creationContext();
    QQmlContext* child_ctx = ctx ? new QQmlContext(ctx) : new QQmlContext(qmlEngine(this));

    QObject* obj = m_delegate->createWithInitialProperties(initialPropertiesForDelegate(index), child_ctx);
    child_ctx->setParent(obj);
    auto* item = qobject_cast<QQuickItem*>(obj);
    if (!item) {
        delete obj;
        return;
    }
    item->setParentItem(m_content);

    item->setProperty("_carouselIndex", index);
    connect(item, SIGNAL(clicked()), this, SLOT(onDelegateClicked()));
    connect(item, SIGNAL(aspectRatioChanged()), this, SLOT(onDelegateAspectRatioChanged()));

    m_items[index] = item;
}

void CarouselView::onDelegateAspectRatioChanged()
{
    auto* item = qobject_cast<QQuickItem*>(sender());
    if (!item) {
        return;
    }
    const int index = item->property("_carouselIndex").toInt();
    if (index < 0 || index >= m_count) {
        return;
    }
    const qreal aspect = item->property("itemAspectRatio").toDouble();
    if (aspect <= 0) {
        return;
    }
    while (m_item_aspects.size() < m_count) {
        m_item_aspects.append(1.0);
    }
    if (qFuzzyCompare(m_item_aspects[index], aspect)) {
        return;
    }
    m_item_aspects[index] = aspect;
    updateLayout();
}

void CarouselView::onDelegateClicked()
{
    auto* sender_item = qobject_cast<QQuickItem*>(sender());
    if (!sender_item) {
        return;
    }
    Q_EMIT clicked(sender_item->property("_carouselIndex").toInt());
}

void CarouselView::destroyDelegate(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return;
    }
    if (auto* item = m_items[index]) {
        item->deleteLater();
        m_items[index] = nullptr;
    }
}

void CarouselView::rebuildItems()
{
    for (int i = 0; i < m_items.size(); ++i) {
        destroyDelegate(i);
    }
    m_items.clear();
    m_items.resize(m_count);
    m_item_aspects.resize(m_count);
    m_item_aspects.fill(1.0);
    updateLayout();
}

void CarouselView::positionItem(QQuickItem* item, const CarouselItemGeometry& geom, bool isActive)
{
    if (!item) {
        return;
    }
    const qreal cross = qMax(0.0, (m_orientation == Qt::Horizontal ? height() : width())
        - 2.0 * m_content_padding_cross);

    if (m_orientation == Qt::Horizontal) {
        item->setX(geom.position);
        item->setY(m_content_padding_cross);
        item->setWidth(geom.size);
        item->setHeight(cross);
    } else {
        item->setX(m_content_padding_cross);
        item->setY(geom.position);
        item->setWidth(cross);
        item->setHeight(geom.size);
    }

    item->setZ(geom.index);

    auto* attached = qobject_cast<CarouselAttached*>(qmlAttachedPropertiesObject<CarouselView>(item, true));
    if (attached) {
        attached->setGeometry(geom.size, cross, geom.mask_start, geom.mask_end, geom.parallax_shift,
                              geom.size_class, geom.index, isActive);
    }
}

int CarouselView::activeIndexForLayout(const CarouselLayoutOutput& out) const
{
    if (m_layout == kLayoutUncontained || m_layout == kLayoutFullScreen
        || m_layout == kLayoutUncontainedMultiAspect) {
        // Leading snap item; at end-of-list prefer the leftmost fully unmasked visible item.
        int   best_index = out.leading_index;
        qreal best_frac  = -1.0;
        for (const auto& g : out.items) {
            if (g.mask_start > 0.001) {
                continue;
            }
            const qreal frac = qMax(0.0, 1.0 - g.mask_start - g.mask_end);
            if (frac > best_frac + 0.001
                || (qFuzzyCompare(frac, best_frac) && g.index < best_index)) {
                best_frac  = frac;
                best_index = g.index;
            }
        }
        return best_index;
    }

    int   best_index = out.leading_index;
    qreal best_frac  = -1.0;
    for (const auto& g : out.items) {
        if (g.size_class != kSizeLarge) {
            continue;
        }
        const qreal frac = qMax(0.0, 1.0 - g.mask_start - g.mask_end);
        if (frac > best_frac + 0.001
            || (qFuzzyCompare(frac, best_frac) && g.index == out.leading_index)) {
            best_frac  = frac;
            best_index = g.index;
        }
    }
    return best_index;
}

void CarouselView::updateLayout()
{
    if (!m_completed || !m_flickable || m_count <= 0) {
        return;
    }

    CarouselLayoutInput input;
    input.layout        = m_layout;
    input.alignment     = m_alignment;
    input.orientation   = m_orientation;
    input.viewport_size = m_orientation == Qt::Horizontal ? width() : height();
    input.cross_size    = m_orientation == Qt::Horizontal ? height() : width();
    input.cross_size    = m_orientation == Qt::Horizontal ? height() : width();
    auto* flick = asFlickable(m_flickable);
    input.scroll_offset = flick
        ? (m_orientation == Qt::Horizontal ? flick->contentX() : flick->contentY())
        : 0;
    input.item_extent   = m_item_extent;
    input.spacing       = m_spacing;
    input.shrink_extent = m_shrink_extent;
    input.content_padding_start = m_content_padding_start;
    input.content_padding_end   = m_content_padding_end;
    input.content_padding_cross = m_content_padding_cross;
    input.small_item_min  = m_min_small_item_width;
    input.small_item_max  = m_max_small_item_width;
    input.min_peek_px     = 16;
    input.item_corner_radius = m_item_corner_radius;
    input.parallax_ratio  = m_reduce_motion ? 0
        : (m_layout == kLayoutUncontained || m_layout == kLayoutUncontainedMultiAspect ? 0.5 : 0.35);
    input.count         = m_count;
    input.item_aspects  = m_item_aspects;
    while (input.item_aspects.size() < m_count) {
        input.item_aspects.append(1.0);
    }
    for (const auto& w : m_flex_weights) {
        input.flex_weights.append(w.toInt());
    }

    const auto out  = CarouselStrategy::compute(input);
    m_geometries    = out.items;
    m_snap_offsets  = out.snap_offsets;
    m_max_scroll_offset = out.max_scroll_offset;
    m_end_snap_offset   = out.end_snap_offset;
    m_scroll_step       = out.scroll_step > 0 ? out.scroll_step : (m_item_extent + m_spacing);

    const qreal new_w = m_orientation == Qt::Horizontal ? out.content_size : width();
    const qreal new_h = m_orientation == Qt::Vertical ? out.content_size : height();
    if (!qFuzzyCompare(m_content_width, new_w)) {
        m_content_width = new_w;
        Q_EMIT contentWidthChanged();
    }
    if (!qFuzzyCompare(m_content_height, new_h)) {
        m_content_height = new_h;
        Q_EMIT contentHeightChanged();
    }

    m_updating = true;
    if (flick) {
        flick->setContentWidth(m_orientation == Qt::Horizontal ? m_content_width : width());
        flick->setContentHeight(m_orientation == Qt::Vertical ? m_content_height : height());

        const qreal viewport = m_orientation == Qt::Horizontal ? width() : height();
        const qreal max_offset = m_max_scroll_offset > 0
            ? m_max_scroll_offset
            : qMax(0.0, (m_orientation == Qt::Horizontal ? m_content_width : m_content_height) - viewport);
        const bool user_scrolling = flick->isMoving() || m_snapping;
        if (!user_scrolling) {
            if (m_orientation == Qt::Horizontal) {
                flick->setContentX(qBound(0.0, flick->contentX(), max_offset));
            } else {
                flick->setContentY(qBound(0.0, flick->contentY(), max_offset));
            }
        }
    }
    m_updating = false;

    QSet<int> visible;
    const int active_index = activeIndexForLayout(out);
    for (const auto& g : out.items) {
        visible.insert(g.index);
        createDelegate(g.index);
        positionItem(m_items[g.index], g, g.index == active_index);
    }

    for (int i = 0; i < m_items.size(); ++i) {
        if (!visible.contains(i) && m_items[i]) {
            m_items[i]->setVisible(false);
        } else if (m_items[i]) {
            m_items[i]->setVisible(true);
        }
    }

    if (usesFreeScrollSnap() && active_index != m_current_index) {
        m_current_index = active_index;
        Q_EMIT currentIndexChanged();
    }
}

bool CarouselView::usesSingleAdvanceFling() const
{
    return m_layout == 2 || m_layout == 3 || m_layout == 4; // Hero, HeroCenter, FullScreen
}

bool CarouselView::usesFreeScrollSnap() const
{
    return m_layout == kLayoutUncontained || m_layout == kLayoutUncontainedMultiAspect;
}

int CarouselView::snapIndexForFling(qreal offset, qreal velocity) const
{
    if (usesSingleAdvanceFling() && qAbs(velocity) > 50) {
        return velocity > 0 ? qMin(m_count - 1, m_current_index + 1) : qMax(0, m_current_index - 1);
    }

    const qreal stride   = m_scroll_step > 0 ? m_scroll_step : 1.0;
    const qreal end_zone = m_max_scroll_offset - stride * 0.25;

    if (offset >= end_zone) {
        return m_count > 0 ? m_count - 1 : 0;
    }

    if (qAbs(velocity) > 50) {
        if (velocity > 0 && offset >= m_max_scroll_offset - stride * 0.5) {
            return m_count > 0 ? m_count - 1 : 0;
        }
        int   best   = m_current_index;
        qreal best_d = std::numeric_limits<qreal>::max();
        for (int i = 0; i < m_snap_offsets.size(); ++i) {
            const qreal d = m_snap_offsets.at(i) - offset;
            if ((velocity > 0 && d > 0) || (velocity < 0 && d < 0)) {
                const qreal ad = qAbs(d);
                if (ad < best_d) {
                    best_d = ad;
                    best   = i;
                }
            }
        }
        if (best != m_current_index) {
            return best;
        }
    }
    return snapIndexForOffset(offset);
}

qreal CarouselView::snapTargetForGesture(qreal offset, qreal velocity) const
{
    const qreal stride      = m_scroll_step > 0 ? m_scroll_step : 1.0;
    const qreal end_zone    = m_max_scroll_offset - stride * 0.25;
    const qreal start_zone  = stride * 0.25;

    if (offset >= end_zone || (qAbs(velocity) > 50 && velocity > 0
                              && offset >= m_max_scroll_offset - stride * 0.5)) {
        return m_end_snap_offset;
    }

    if (offset <= start_zone && velocity <= 50) {
        return 0;
    }

    const int best = snapIndexForFling(offset, velocity);
    if (best == m_count - 1) {
        return m_end_snap_offset;
    }
    if (best == 0) {
        return 0;
    }
    if (best >= 0 && best < m_snap_offsets.size()) {
        return m_snap_offsets.at(best);
    }
    return offset;
}

void CarouselView::snapAfterGesture()
{
    if (!m_flickable || m_snap_offsets.isEmpty()) {
        return;
    }
    auto* flick = asFlickable(m_flickable);
    if (!flick) {
        return;
    }
    const qreal offset = m_orientation == Qt::Horizontal ? flick->contentX() : flick->contentY();
    const qreal velocity =
        m_orientation == Qt::Horizontal ? flick->horizontalVelocity() : flick->verticalVelocity();

    const qreal target = snapTargetForGesture(offset, velocity);
    applySnapAnimation(target);
}

int CarouselView::snapIndexForOffset(qreal offset) const
{
    int   best   = m_current_index;
    qreal best_d = std::numeric_limits<qreal>::max();
    for (int i = 0; i < m_snap_offsets.size(); ++i) {
        const qreal d = qAbs(m_snap_offsets.at(i) - offset);
        if (d < best_d) {
            best_d = d;
            best   = i;
        }
    }
    return best;
}

void CarouselView::applySnapAnimation(qreal targetOffset)
{
    if (!m_flickable) {
        return;
    }
    auto* flick = asFlickable(m_flickable);
    if (!flick) {
        return;
    }

    const qreal clamped_target = qBound(0.0, targetOffset, m_max_scroll_offset);

    const qreal current = m_orientation == Qt::Horizontal ? flick->contentX() : flick->contentY();
    if (qFuzzyCompare(current, clamped_target)) {
        finishSnap(clamped_target);
        return;
    }

    if (m_reduce_motion) {
        m_updating = true;
        if (m_orientation == Qt::Horizontal) {
            flick->setContentX(clamped_target);
        } else {
            flick->setContentY(clamped_target);
        }
        m_updating = false;
        finishSnap(clamped_target);
        return;
    }

    if (usesFreeScrollSnap()) {
        const QByteArray prop = m_orientation == Qt::Horizontal ? QByteArrayLiteral("contentX")
                                                                : QByteArrayLiteral("contentY");
        auto* anim = new QPropertyAnimation(flick, prop, this);
        anim->setDuration(kSnapDurationMs);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->setStartValue(current);
        anim->setEndValue(clamped_target);
        m_snapping = true;
        connect(anim, &QPropertyAnimation::finished, this, [this, anim, clamped_target]() {
            m_snapping = false;
            anim->deleteLater();
            finishSnap(clamped_target);
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        return;
    }

    m_snap_pos      = current;
    m_snap_velocity = 0;
    m_snap_target   = clamped_target;
    m_snapping      = true;
    m_snap_timer->start();
}

void CarouselView::onSnapTick()
{
    if (!m_flickable) {
        m_snap_timer->stop();
        m_snapping = false;
        return;
    }
    auto* flick = asFlickable(m_flickable);
    if (!flick) {
        m_snap_timer->stop();
        m_snapping = false;
        return;
    }

    const qreal force = -kSnapStiffness * (m_snap_pos - m_snap_target) - kSnapDamping * m_snap_velocity;
    m_snap_velocity += force * kSnapDt;
    m_snap_pos += m_snap_velocity * kSnapDt;

    m_updating = true;
    if (m_orientation == Qt::Horizontal) {
        flick->setContentX(m_snap_pos);
    } else {
        flick->setContentY(m_snap_pos);
    }
    m_updating = false;
    updateLayout();

    if (qAbs(m_snap_pos - m_snap_target) < 0.5 && qAbs(m_snap_velocity) < 5.0) {
        m_snap_timer->stop();
        m_updating = true;
        if (m_orientation == Qt::Horizontal) {
            flick->setContentX(m_snap_target);
        } else {
            flick->setContentY(m_snap_target);
        }
        m_updating = false;
        finishSnap(m_snap_target);
    }
}

void CarouselView::finishSnap(qreal targetOffset)
{
    m_snapping = false;
    if (m_count > 0 && qAbs(targetOffset - m_end_snap_offset) < 1.0) {
        setCurrentIndex(m_count - 1);
    } else if (qAbs(targetOffset) < 1.0) {
        setCurrentIndex(0);
    } else {
        setCurrentIndex(snapIndexForOffset(targetOffset));
    }
    updateLayout();
}

} // namespace qml_material
