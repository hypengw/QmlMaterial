#pragma once

#include <QtCore/QObject>
#include <QtQml/qqmlregistration.h>

namespace qml_material::token
{

/**
 * @brief Motion duration
 *
 */
struct Duration {
    Q_GADGET
    QML_ANONYMOUS
    /// Extra short duration (50ms)
    Q_PROPERTY(qreal short1 MEMBER short1 CONSTANT FINAL)
    /// Short duration (100ms)
    Q_PROPERTY(qreal short2 MEMBER short2 CONSTANT FINAL)
    /// Short-medium duration (150ms)
    Q_PROPERTY(qreal short3 MEMBER short3 CONSTANT FINAL)
    /// Medium-short duration (200ms)
    Q_PROPERTY(qreal short4 MEMBER short4 CONSTANT FINAL)
    /// Medium duration (250ms)
    Q_PROPERTY(qreal medium1 MEMBER medium1 CONSTANT FINAL)
    /// Medium-standard duration (300ms)
    Q_PROPERTY(qreal medium2 MEMBER medium2 CONSTANT FINAL)
    /// Medium-long duration (350ms)
    Q_PROPERTY(qreal medium3 MEMBER medium3 CONSTANT FINAL)
    /// Extended medium duration (400ms)
    Q_PROPERTY(qreal medium4 MEMBER medium4 CONSTANT FINAL)
    /// Long duration (450ms)
    Q_PROPERTY(qreal long1 MEMBER long1 CONSTANT FINAL)
    /// Extended long duration (500ms)
    Q_PROPERTY(qreal long2 MEMBER long2 CONSTANT FINAL)
    /// Very long duration (550ms)
    Q_PROPERTY(qreal long3 MEMBER long3 CONSTANT FINAL)
    /// Maximum long duration (600ms)
    Q_PROPERTY(qreal long4 MEMBER long4 CONSTANT FINAL)
    /// Extra long duration (700ms)
    Q_PROPERTY(qreal extra_long1 MEMBER extra_long1 CONSTANT FINAL)
    /// Extended extra long duration (800ms)
    Q_PROPERTY(qreal extra_long2 MEMBER extra_long2 CONSTANT FINAL)
    /// Very extra long duration (900ms)
    Q_PROPERTY(qreal extra_long3 MEMBER extra_long3 CONSTANT FINAL)
    /// Maximum extra long duration (1000ms)
    Q_PROPERTY(qreal extra_long4 MEMBER extra_long4 CONSTANT FINAL)

public:
    qreal short1 { 50 };
    qreal short2 { 100 };
    qreal short3 { 150 };
    qreal short4 { 200 };
    qreal medium1 { 250 };
    qreal medium2 { 300 };
    qreal medium3 { 350 };
    qreal medium4 { 400 };
    qreal long1 { 450 };
    qreal long2 { 500 };
    qreal long3 { 550 };
    qreal long4 { 600 };
    qreal extra_long1 { 700 };
    qreal extra_long2 { 800 };
    qreal extra_long3 { 900 };
    qreal extra_long4 { 1000 };
};

} // namespace qml_material::token
