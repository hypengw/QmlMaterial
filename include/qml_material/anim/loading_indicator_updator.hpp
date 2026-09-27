#pragma once

#include <QtGui/QColor>
#include "qml_material/shape/material_shapes.hpp"

namespace qml_material
{

class QML_MATERIAL_API LoadingIndicatorUpdator : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(MaterialShape::Type shape READ shape NOTIFY updated FINAL)
    Q_PROPERTY(MaterialShape::Type toShape READ toShape NOTIFY updated FINAL)
    Q_PROPERTY(double shapeProgress READ shapeProgress NOTIFY updated FINAL)
    Q_PROPERTY(double rotation READ rotation NOTIFY updated FINAL)
    Q_PROPERTY(QColor color READ color NOTIFY updated FINAL)
    Q_PROPERTY(double morphFraction READ progress NOTIFY updated FINAL)
    Q_PROPERTY(double progress READ progress WRITE setProgress NOTIFY updated FINAL)
    Q_PROPERTY(QList<QColor> colors READ colors WRITE setColors NOTIFY colorsChanged FINAL)
    Q_PROPERTY(int shapeCount READ shapeCount CONSTANT FINAL)
    Q_PROPERTY(int msPerShape READ msPerShape CONSTANT FINAL)
public:
    explicit LoadingIndicatorUpdator(QObject* parent = nullptr);
    MaterialShape::Type shape() const;
    MaterialShape::Type toShape() const;
    double              shapeProgress() const { return m_shapeProgress; }
    double              rotation() const { return m_rotation; }
    QColor              color() const { return m_color; }
    double              progress() const { return m_progress; }
    void                setProgress(double progress);
    QList<QColor>       colors() const { return m_colors; }
    void                setColors(const QList<QColor>& colors);
    static int          shapeCount() { return int(MaterialShapes::loadingSequence().size()); }
    static int          msPerShape() { return 650; }
    Q_SIGNAL void       updated();
    Q_SIGNAL void       colorsChanged();

private:
    void          updateInternal();
    double        m_progress      = 0;
    double        m_shapeProgress = 0;
    double        m_rotation      = 0;
    QColor        m_color         = Qt::transparent;
    QList<QColor> m_colors;
};

} // namespace qml_material
