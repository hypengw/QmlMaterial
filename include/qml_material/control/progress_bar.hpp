#pragma once

#include "qml_material/control/control.hpp"

namespace qml_material
{

/** @ingroup control */
class QML_MATERIAL_API ProgressBar : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(ProgressBarBase)
    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(bool indeterminate READ indeterminate WRITE setIndeterminate NOTIFY
                   indeterminateChanged FINAL)

public:
    explicit ProgressBar(QQuickItem* parent = nullptr);
    qreal from() const { return m_from; }
    qreal to() const { return m_to; }
    qreal value() const { return m_value; }
    qreal position() const;
    qreal visualPosition() const;
    bool  indeterminate() const { return m_indeterminate; }
    void  setFrom(qreal value);
    void  setTo(qreal value);
    void  setValue(qreal value);
    void  setIndeterminate(bool value);

    Q_SIGNAL void fromChanged();
    Q_SIGNAL void toChanged();
    Q_SIGNAL void valueChanged();
    Q_SIGNAL void positionChanged();
    Q_SIGNAL void visualPositionChanged();
    Q_SIGNAL void indeterminateChanged();

protected:
    void componentComplete() override;

private:
    qreal m_from          = 0;
    qreal m_to            = 1;
    qreal m_value         = 0;
    bool  m_indeterminate = false;
};

} // namespace qml_material
