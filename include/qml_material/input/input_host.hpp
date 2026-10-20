#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QTimer>
#include <memory>
#include <optional>
#include "qml_material/input/interaction_source.hpp"

namespace qml_material
{
class InputHost;
class QtInputAdapter;
class QtDragInputAdapter;

struct QML_MATERIAL_API RepeatClickInput {
    int delay = 350, interval = 80;

private:
    friend class ClickInput;
    std::unique_ptr<QTimer> timer;
};

struct PointerInput {
    Interaction::Device device   = Interaction::Unspecified;
    int                 identity = -1;
    QPointF             position;
    qint64              timestamp = 0;
};

class QML_MATERIAL_API CombinedClickInput {
public:
    bool                  longClickEnabled   = false;
    bool                  doubleClickEnabled = false;
    int                   longClickInterval  = 0;
    std::function<void()> longClicked, doubleClicked;

private:
    friend class ClickInput;
    struct Tap {
        qint64 released, deadline;
    };
    QHash<quint64, Tap>     taps;
    std::unique_ptr<QTimer> holdTimer, tapTimer;
};

class QML_MATERIAL_API ClickInput {
public:
    explicit ClickInput(InputHost&);
    ~ClickInput();
    std::function<void()>                    clicked;
    std::function<void(const PointerInput&)> pointerStarted;
    std::function<void(QPointF)>             pointerClicked, pointerMoved;
    std::function<void()>                    pointerCancelled;
    bool                                     keyboardEnabled = true;
    CombinedClickInput&                      combined();
    RepeatClickInput&                        repeat();
    void                                     resetCombined();
    void                                     activate();

private:
    friend class InputHost;
    friend class QtInputAdapter;
    friend class QtDragInputAdapter;
    struct Candidate {
        PointerInput input;
        quint64      token          = 0;
        qint64       deadline       = -1;
        qint64       repeatDeadline = -1;
        bool         held = false, second = false, ignored = false, dragging = false;
        bool         feedbackEligible = false;
    };
    void                                begin(const PointerInput&);
    void                                move(const PointerInput&, bool notifyDrag = true);
    bool                                startDrag();
    void                                finish(const PointerInput&);
    void                                cancelPointer(bool notify);
    void                                cancelKeys();
    void                                cancel(bool notify);
    bool                                key(int, bool down, bool repeat, qint64 timestamp);
    bool                                prepare(Candidate&);
    void                                feedback(Candidate&);
    void                                feedbackTimeout();
    void                                release(Candidate&, bool cancelled);
    void                                complete(const Candidate&);
    void                                scheduleHold();
    void                                holdTimeout();
    void                                scheduleRepeat();
    void                                repeatTimeout();
    void                                scheduleTap();
    void                                tapTimeout();
    static quint64                      tapIdentity(const Candidate&);
    InputHost&                          m_host;
    std::optional<Candidate>            m_pointer;
    QHash<int, Candidate>               m_keys;
    std::unique_ptr<QTimer>             m_feedbackTimer;
    std::unique_ptr<CombinedClickInput> m_combined;
    std::unique_ptr<RepeatClickInput>   m_repeat;
};

class QML_MATERIAL_API DragInput {
public:
    Qt::Orientations                      axes             = Qt::Horizontal | Qt::Vertical;
    qreal                                 threshold        = -1;
    bool                                  startImmediately = false;
    std::function<bool(QPointF, QPointF)> canStart;
    std::function<void(QPointF)>          started, moved;
    std::function<void(QPointF, bool)>    finished;
    QPointF                               velocity;

private:
    friend class ClickInput;
    friend class QtDragInputAdapter;
    quint64               token = 0;
    std::function<bool()> requestGrab;
};

class HoverInput {
    friend class InputHost;
    quint64 token = 0;
};
class FocusInput {
    friend class InputHost;
    quint64 token = 0;
};

class QML_MATERIAL_API InputHost {
public:
    // Embed in the target item so input is detached before the item is destroyed.
    explicit InputHost(QQuickItem*, std::function<void()> sourceChanged = {});
    ~InputHost();
    InputHost(const InputHost&)                       = delete;
    InputHost&            operator=(const InputHost&) = delete;
    QQuickItem*           item() const { return m_item; }
    InteractionSource*    source();
    InteractionSource*    existingSource() const { return m_source; }
    void                  setSource(InteractionSource*);
    bool                  event(QEvent*);
    bool                  observeEvent(QEvent*);
    void                  mouseUngrabbed();
    void                  touchUngrabbed();
    ClickInput&           click();
    void                  setHoverEnabled(bool);
    void                  observeHover(bool);
    void                  setFocusEnabled(bool);
    DragInput&            drag();
    void                  resetDrag();
    void                  observeChildPointerInput();
    void                  cancelActivation();
    quint64               activationRevision() const { return m_generation; }
    void                  cancel();
    bool                  available() const;
    std::function<bool()> acceptsInput;
    bool                  focusVisible() const { return m_focusVisible; }
    std::function<void()> focusVisibleChanged;

private:
    friend class ClickInput;
    friend class QtInputAdapter;
    friend class QtDragInputAdapter;
    quint64                             begin(Interaction::Kind, const PointerInput& = {});
    void                                end(quint64, bool cancelled = false);
    void                                hover(bool);
    void                                focus();
    void                                blur();
    void                                focusReason(Qt::FocusReason);
    void                                observeWindow();
    void                                reset(bool notify);
    void                                attachSource(InteractionSource*);
    QPointer<QQuickItem>                m_item;
    QPointer<InteractionSource>         m_source;
    std::unique_ptr<InteractionSource>  m_ownedSource;
    std::unique_ptr<InteractionEmitter> m_emitter;
    QList<QMetaObject::Connection>      m_connections;
    QMetaObject::Connection             m_sourceDestroyed, m_windowActive, m_windowVisibility;
    std::function<void()>               m_sourceChanged;
    std::optional<ClickInput>           m_click;
    std::unique_ptr<DragInput>          m_drag;
    std::unique_ptr<QtDragInputAdapter> m_pointerAdapter;
    std::shared_ptr<const bool>         m_lifetime = std::make_shared<const bool>(true);
    HoverInput                          m_hover;
    FocusInput                          m_focus;
    quint64                             m_editorPress = 0;
    QElapsedTimer                       m_elapsed;
    bool                                m_focusVisible = false, m_detaching = false;
    bool                                m_focusEnabled = true;
    Qt::FocusReason                     m_focusReason  = Qt::OtherFocusReason;
    quint64                             m_generation   = 0;
};

class QML_MATERIAL_API QtInputAdapter {
public:
    static bool deliver(InputHost&, QEvent*);
    // The editor owns acceptance, selection and grabs; this only observes its result.
    static bool deliverToEditor(InputHost&, QEvent*, const std::function<bool()>& receiver);
};
} // namespace qml_material
