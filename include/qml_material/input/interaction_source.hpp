#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QProperty>
#include <QQuickItem>
#include <functional>
#include <memory>
#include <optional>
#include "qml_material/export.hpp"

namespace qml_material
{
struct QML_MATERIAL_API Interaction {
    Q_GADGET
    QML_VALUE_TYPE(interaction)
    Q_PROPERTY(Kind kind MEMBER kind CONSTANT FINAL)
    Q_PROPERTY(Phase phase MEMBER phase CONSTANT FINAL)
    Q_PROPERTY(Device device MEMBER device CONSTANT FINAL)
    Q_PROPERTY(quint64 id MEMBER id CONSTANT FINAL)
    Q_PROPERTY(quint64 producer MEMBER producer CONSTANT FINAL)
    Q_PROPERTY(int identity MEMBER identity CONSTANT FINAL)
    Q_PROPERTY(QPointF position MEMBER position CONSTANT FINAL)
    Q_PROPERTY(QQuickItem* origin READ originItem CONSTANT FINAL)
public:
    enum Kind
    {
        None,
        Press,
        Hover,
        Focus,
        Drag
    };
    Q_ENUM(Kind)
    enum Phase
    {
        Begin,
        End,
        Cancel
    };
    Q_ENUM(Phase)
    enum Device
    {
        Unspecified,
        Mouse,
        Touch,
        Pen,
        Keyboard,
        Simulated
    };
    Q_ENUM(Device)
    Kind                   kind   = None;
    Phase                  phase  = Begin;
    Device                 device = Unspecified;
    quint64                id = 0, producer = 0;
    int                    identity = -1;
    QPointF                position;
    QPointer<QQuickItem>   coordinateItem;
    QQuickItem*            originItem() const { return coordinateItem; }
    std::optional<QPointF> positionFor(QQuickItem*) const;
    bool                   operator==(const Interaction&) const = default;
};
struct QML_MATERIAL_API InteractionEnums {
    Q_GADGET
    QML_FOREIGN_NAMESPACE(qml_material::Interaction)
    QML_NAMED_ELEMENT(Interaction)
};

class InteractionEmitter;
// GUI-thread event channel. Delivery is queued and has no active-state replay.
class QML_MATERIAL_API InteractionSource : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit InteractionSource(QObject* parent = nullptr);
    ~InteractionSource() override;
    std::unique_ptr<InteractionEmitter> emitter(QQuickItem* origin = nullptr);
    Q_SIGNAL void                       interaction(const qml_material::Interaction&);

private:
    friend class InteractionEmitter;
    void               enqueue(const Interaction&);
    void               deliver();
    QList<Interaction> m_pending;
    bool               m_scheduled = false;
};

class QML_MATERIAL_API InteractionEmitter {
public:
    ~InteractionEmitter();
    InteractionEmitter(const InteractionEmitter&)            = delete;
    InteractionEmitter& operator=(const InteractionEmitter&) = delete;
    quint64             begin(Interaction::Kind, Interaction::Device = Interaction::Unspecified,
                              int identity = -1, QPointF position = {});
    void                end(quint64, Interaction::Phase = Interaction::End);
    void                cancel();
    bool                contains(quint64) const;
    InteractionSource*  source() const { return m_source; }

private:
    friend class InteractionSource;
    InteractionEmitter(InteractionSource*, QQuickItem*);
    QPointer<InteractionSource> m_source;
    QPointer<QQuickItem>        m_origin;
    quint64                     m_producer;
    QList<Interaction>          m_active;
};

struct QML_MATERIAL_API InteractionValues {
    Q_GADGET
    QML_VALUE_TYPE(interactionValues)
    Q_PROPERTY(bool pressed MEMBER pressed CONSTANT FINAL)
    Q_PROPERTY(bool hovered MEMBER hovered CONSTANT FINAL)
    Q_PROPERTY(bool focused MEMBER focused CONSTANT FINAL)
    Q_PROPERTY(bool dragged MEMBER dragged CONSTANT FINAL)
public:
    bool pressed = false, hovered = false, focused = false, dragged = false;
    bool operator==(const InteractionValues&) const = default;
};

class QML_MATERIAL_API InteractionAccumulator {
public:
    // The context must own this accumulator and outlive its subscriptions.
    explicit InteractionAccumulator(QObject* context, std::function<void()> changed = {});
    ~InteractionAccumulator();
    InteractionAccumulator(const InteractionAccumulator&)                 = delete;
    InteractionAccumulator&      operator=(const InteractionAccumulator&) = delete;
    void                         setSource(InteractionSource*);
    InteractionSource*           source() const { return m_source; }
    InteractionValues            values() const { return m_values.value(); }
    QBindable<InteractionValues> bindableValues() const { return &m_values; }
    Interaction::Kind            latest(std::initializer_list<Interaction::Kind>) const;
    QList<Interaction>           active() const;

private:
    void                         accept(const Interaction&);
    void                         update();
    QPointer<QObject>            m_context;
    QPointer<InteractionSource>  m_source;
    QMetaObject::Connection      m_event, m_destroyed;
    QList<Interaction>           m_active;
    QProperty<quint64>           m_revision { 0 };
    QProperty<InteractionValues> m_values;
    std::function<void()>        m_changed;
};

class QML_MATERIAL_API InteractionObserver : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(InteractionSource* source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(qml_material::InteractionValues values READ values NOTIFY changed FINAL)
    Q_PROPERTY(bool pressed READ pressed NOTIFY changed FINAL)
    Q_PROPERTY(bool hovered READ hovered NOTIFY changed FINAL)
    Q_PROPERTY(bool focused READ focused NOTIFY changed FINAL)
    Q_PROPERTY(bool dragged READ dragged NOTIFY changed FINAL)
public:
    explicit InteractionObserver(QObject* parent = nullptr);
    InteractionSource* source() const { return m_input.source(); }
    void               setSource(InteractionSource*);
    InteractionValues  values() const { return m_input.values(); }
    bool               pressed() const { return values().pressed; }
    bool               hovered() const { return values().hovered; }
    bool               focused() const { return values().focused; }
    bool               dragged() const { return values().dragged; }
    Q_SIGNAL void      sourceChanged();
    Q_SIGNAL void      changed();

private:
    InteractionAccumulator  m_input;
    QMetaObject::Connection m_sourceDestroyed;
};
} // namespace qml_material
