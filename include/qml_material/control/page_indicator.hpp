#pragma once

#include <QQmlComponent>
#include <QQmlContext>
#include "qml_material/control/control.hpp"
#include "qml_material/input/input_host.hpp"

namespace qml_material
{

/** @ingroup control */
class QML_MATERIAL_API PageIndicator : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(PageIndicatorBase)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged FINAL)
    Q_PROPERTY(
        int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(
        bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged FINAL)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(int highlightedIndex READ highlightedIndex NOTIFY highlightedIndexChanged FINAL)
    Q_PROPERTY(InteractionSource* interactionSource READ interactionSource WRITE
                   setInteractionSource NOTIFY interactionSourceChanged FINAL)
    Q_PROPERTY(bool focusVisible READ focusVisible NOTIFY focusVisibleChanged FINAL)
public:
    explicit PageIndicator(QQuickItem* parent = nullptr);
    ~PageIndicator() override;
    int                count() const { return m_count; }
    int                currentIndex() const { return m_current_index; }
    bool               interactive() const { return m_interactive; }
    int                highlightedIndex() const { return m_pressed_index; }
    InteractionSource* interactionSource() { return m_input.source(); }
    void               setInteractionSource(InteractionSource* value) { m_input.setSource(value); }
    bool               focusVisible() const { return m_input.focusVisible(); }
    QQmlComponent*     delegate() const { return m_delegate; }
    void               setCount(int);
    void               setCurrentIndex(int);
    void               setInteractive(bool);
    void               setDelegate(QQmlComponent*);
    Q_INVOKABLE QQuickItem* itemAt(int index) const;
    Q_SIGNAL void           countChanged();
    Q_SIGNAL void           currentIndexChanged();
    Q_SIGNAL void           interactiveChanged();
    Q_SIGNAL void           delegateChanged();
    Q_SIGNAL void           currentIndexChangeRequested(int index);
    Q_SIGNAL void           highlightedIndexChanged();
    Q_SIGNAL void           interactionSourceChanged();
    Q_SIGNAL void           focusVisibleChanged();

protected:
    void componentComplete() override;
    void updatePolish() override;
    bool event(QEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseUngrabEvent() override;
    void touchEvent(QTouchEvent*) override;
    void touchUngrabEvent() override;

private:
    struct Entry {
        QPointer<QQuickItem> item;
    };
    void scheduleDelegates();
    void clearDelegates();
    void setPressed(int);
    void cancelPress();
    int  hitIndex(const QPointF&) const;
    void releasePress(const QPointF&);

    QList<Entry>                   m_items;
    QPointer<QQmlComponent>        m_delegate;
    QList<QMetaObject::Connection> m_delegate_connections;
    int                            m_count         = 0;
    int                            m_current_index = 0;
    int                            m_pressed_index = -1;
    bool                           m_interactive   = false;
    bool                           m_rebuild       = false;
    bool                           m_syncing       = false;
    InputHost                      m_input;
};

} // namespace qml_material
