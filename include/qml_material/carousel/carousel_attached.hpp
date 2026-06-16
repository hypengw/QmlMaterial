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
  /** @brief True when this item is the focal carousel card (title visible). */
    Q_PROPERTY(bool isActive READ isActive NOTIFY geometryChanged FINAL)

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
    bool  isActive() const;

    void setGeometry(qreal width, qreal height, qreal maskStart, qreal maskEnd, qreal parallaxShift,
                     int sizeClass, int index, bool isActive);
    Q_SIGNAL void geometryChanged();

private:
    qreal m_width           = 0;
    qreal m_height          = 0;
    qreal m_mask_start      = 0;
    qreal m_mask_end        = 0;
    qreal m_parallax_shift  = 0;
    int   m_size_class      = 2;
    int   m_index           = -1;
    bool  m_is_active       = false;
};

} // namespace qml_material
