#pragma once

#include <QObject>
#include <QQmlEngine>

namespace qml_material
{

/**
 * @brief Attached properties exposed on carousel delegates.
 */
class CarouselAttached : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qreal itemWidth READ itemWidth NOTIFY geometryChanged FINAL)
    Q_PROPERTY(qreal itemHeight READ itemHeight NOTIFY geometryChanged FINAL)
    Q_PROPERTY(qreal maskStart READ maskStart NOTIFY geometryChanged FINAL)
    Q_PROPERTY(qreal maskEnd READ maskEnd NOTIFY geometryChanged FINAL)
    Q_PROPERTY(qreal parallaxShift READ parallaxShift NOTIFY geometryChanged FINAL)
    Q_PROPERTY(qreal visibleFraction READ visibleFraction NOTIFY geometryChanged FINAL)
    Q_PROPERTY(int sizeClass READ sizeClass NOTIFY geometryChanged FINAL)
    Q_PROPERTY(int index READ index NOTIFY geometryChanged FINAL)
    Q_PROPERTY(int carouselCount READ carouselCount NOTIFY contextChanged FINAL)
    Q_PROPERTY(int carouselOrientation READ carouselOrientation NOTIFY contextChanged FINAL)
    Q_PROPERTY(int carouselCurrentIndex READ carouselCurrentIndex NOTIFY contextChanged FINAL)
    /** @brief True when this item is the focal carousel card (title visible). */
    Q_PROPERTY(bool isActive READ isActive NOTIFY geometryChanged FINAL)
    Q_PROPERTY(bool focusRingVisible READ focusRingVisible NOTIFY focusRingChanged FINAL)
    Q_PROPERTY(bool focusRingInset READ focusRingInset NOTIFY focusRingChanged FINAL)

public:
    explicit CarouselAttached(QObject* parent = nullptr);

    qreal itemWidth() const;
    qreal itemHeight() const;
    qreal maskStart() const;
    qreal maskEnd() const;
    qreal parallaxShift() const;
    qreal visibleFraction() const;
    int   sizeClass() const;
    int   index() const;
    int   carouselCount() const;
    int   carouselOrientation() const;
    int   carouselCurrentIndex() const;
    bool  isActive() const;
    bool  focusRingVisible() const;
    bool  focusRingInset() const;

    void setGeometry(qreal width, qreal height, qreal maskStart, qreal maskEnd, qreal parallaxShift,
                     int sizeClass, int index, bool isActive);
    void setContext(int count, int orientation, int currentIndex);
    void setFocusRingState(bool suppressed, bool tabEngaged, bool inset);
    Q_SIGNAL void geometryChanged();
    Q_SIGNAL void contextChanged();
    Q_SIGNAL void focusRingChanged();

private:
    qreal m_width           = 0;
    qreal m_height          = 0;
    qreal m_mask_start      = 0;
    qreal m_mask_end        = 0;
    qreal m_parallax_shift  = 0;
    int   m_size_class      = 2;
    int   m_index           = -1;
    int   m_carousel_count  = 0;
    int   m_orientation     = static_cast<int>(Qt::Horizontal);
    int   m_current_index   = 0;
    bool  m_is_active       = false;
    bool  m_focus_ring_suppressed = true;
    bool  m_tab_focus_engaged     = false;
    bool  m_focus_ring_inset      = false;
};

} // namespace qml_material
