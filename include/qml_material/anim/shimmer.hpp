#pragma once

#include "qml_material/export.hpp"
#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <memory>

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE
Q_MOC_INCLUDE("QtQuick/QQuickItem")

namespace qml_material
{
class ShimmerSubscription;

/** Shared sweep timing and reference coordinates. @ingroup qml */
class QML_MATERIAL_API Shimmer : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        BoundsMode boundsMode READ boundsMode WRITE setBoundsMode NOTIFY configurationChanged FINAL)
    Q_PROPERTY(QQuickItem* boundsItem READ boundsItem WRITE setBoundsItem NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged FINAL)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled WRITE setAnimationsEnabled NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY configurationChanged FINAL)
    Q_PROPERTY(int pauseDuration READ pauseDuration WRITE setPauseDuration NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY configurationChanged FINAL)
    Q_PROPERTY(qreal bandWidth READ bandWidth WRITE setBandWidth NOTIFY configurationChanged FINAL)
    Q_PROPERTY(qreal minimumOpacity READ minimumOpacity WRITE setMinimumOpacity NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(qreal maximumOpacity READ maximumOpacity WRITE setMaximumOpacity NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(qreal phase READ phase NOTIFY phaseChanged FINAL)
    Q_PROPERTY(bool animating READ animating NOTIFY animatingChanged FINAL)
public:
    enum BoundsMode
    {
        View,
        Window,
        Item
    };
    Q_ENUM(BoundsMode)
    explicit Shimmer(QObject* parent = nullptr);
    ~Shimmer() override;
    BoundsMode       boundsMode() const;
    QQuickItem*      boundsItem() const;
    bool             running() const;
    bool             animationsEnabled() const;
    int              duration() const;
    int              pauseDuration() const;
    qreal            angle() const;
    qreal            bandWidth() const;
    qreal            minimumOpacity() const;
    qreal            maximumOpacity() const;
    qreal            phase() const;
    bool             animating() const;
    void             setBoundsMode(BoundsMode);
    void             setBoundsItem(QQuickItem*);
    void             setRunning(bool);
    void             setAnimationsEnabled(bool);
    void             setDuration(int);
    void             setPauseDuration(int);
    void             setAngle(qreal);
    void             setBandWidth(qreal);
    void             setMinimumOpacity(qreal);
    void             setMaximumOpacity(qreal);
    Q_INVOKABLE void seek(qreal phase);
    Q_INVOKABLE void restart();
    Q_SIGNAL void    configurationChanged();
    Q_SIGNAL void    runningChanged();
    Q_SIGNAL void    phaseChanged();
    Q_SIGNAL void    animatingChanged();

private:
    friend class ShimmerSubscription;
    void setConsumerActive(QQuickItem*, bool);
    void reconcile();
    void publish();
    void resetSweep();
    void suspendRest();
    void observeBounds();
    struct Private;
    std::unique_ptr<Private> d;
};
} // namespace qml_material
