#pragma once
#include <QCoreApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSignalSpy>
#include "qml_material/control/button_control.hpp"

inline void flushInteractions() { QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall); }

namespace qmtest
{
inline void mouse(QQuickItem& item, QEvent::Type type, QPointF point, quint64 timestamp = 0) {
    QMouseEvent event(type,
                      point,
                      point,
                      type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                      Qt::NoModifier);
    event.setTimestamp(timestamp);
    QCoreApplication::sendEvent(&item, &event);
    flushInteractions();
}

inline void key(QQuickItem& item, QEvent::Type type, int code, bool repeat = false,
                quint64 timestamp = 0) {
    QKeyEvent event(type, code, Qt::NoModifier, {}, repeat);
    event.setTimestamp(timestamp);
    QCoreApplication::sendEvent(&item, &event);
    flushInteractions();
}

inline QList<qml_material::Interaction> events(const QSignalSpy&               spy,
                                               qml_material::Interaction::Kind kind) {
    QList<qml_material::Interaction> result;
    for (const auto& args : spy) {
        const auto event = args[0].value<qml_material::Interaction>();
        if (event.kind == kind) result.append(event);
    }
    return result;
}
} // namespace qmtest

class TestInteractionEmitter {
public:
    TestInteractionEmitter(qml_material::InteractionSource* source, QQuickItem* origin)
        : m_emitter(source->emitter(origin)) {}
    quint64 begin(qml_material::Interaction::Kind   kind,
                  qml_material::Interaction::Device device = qml_material::Interaction::Unspecified,
                  int identity = -1, QPointF point = {}) {
        const auto id = m_emitter->begin(kind, device, identity, point);
        flushInteractions();
        return id;
    }
    void end(quint64 id, qml_material::Interaction::Phase phase = qml_material::Interaction::End) {
        m_emitter->end(id, phase);
        flushInteractions();
    }
    void set(qml_material::Interaction::Kind kind, bool active) {
        if (active && ! m_tokens.contains(kind)) m_tokens.insert(kind, begin(kind));
        if (! active && m_tokens.contains(kind)) end(m_tokens.take(kind));
    }
    void clear() {
        m_tokens.clear();
        m_emitter->cancel();
        flushInteractions();
    }

private:
    std::unique_ptr<qml_material::InteractionEmitter> m_emitter;
    QHash<qml_material::Interaction::Kind, quint64>   m_tokens;
};

inline void setButtonPressed(qml_material::ButtonControl* button, bool pressed) {
    if (! pressed) {
        const QPointF outside(button->width() + 1, button->height() + 1);
        QMouseEvent   cancel(
            QEvent::MouseMove, outside, outside, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(button, &cancel);
        flushInteractions();
        return;
    }
    const QPointF point(button->width() / 2, button->height() / 2);
    QMouseEvent   event(
        QEvent::MouseButtonPress, point, point, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(button, &event);
    flushInteractions();
}
