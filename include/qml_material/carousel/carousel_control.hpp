#pragma once

#include "qml_material/carousel/carousel_view.hpp"
#include "qml_material/control/command_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API CarouselControl : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(CarouselBase)
    Q_PROPERTY(CarouselView* view READ view WRITE setView NOTIFY viewChanged FINAL)
    Q_PROPERTY(
        int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(CommandButton* previousControl READ previousControl WRITE setPreviousControl NOTIFY
                   previousControlChanged FINAL)
    Q_PROPERTY(CommandButton* nextControl READ nextControl WRITE setNextControl NOTIFY
                   nextControlChanged FINAL)
public:
    explicit CarouselControl(QQuickItem* parent = nullptr);
    ~CarouselControl() override;
    CarouselView*    view() const { return m_view; }
    void             setView(CarouselView*);
    CommandButton*   previousControl() const { return m_previous; }
    void             setPreviousControl(CommandButton*);
    CommandButton*   nextControl() const { return m_next; }
    void             setNextControl(CommandButton*);
    int              currentIndex() const;
    Q_INVOKABLE void setCurrentIndex(int index);
    Q_INVOKABLE void incrementCurrentIndex();
    Q_INVOKABLE void decrementCurrentIndex();
    Q_INVOKABLE void setCurrentIndex(int index, bool animated);
    Q_SIGNAL void    viewChanged();
    Q_SIGNAL void    previousControlChanged();
    Q_SIGNAL void    nextControlChanged();
    Q_SIGNAL void    currentIndexChanged();
    Q_SIGNAL void    clicked(int index);
    Q_SIGNAL void    indexChanged(int index);

private:
    void                           invalidateCommands();
    void                           navigate(int direction);
    QPointer<CarouselView>         m_view;
    QPointer<CommandButton>        m_previous;
    QPointer<CommandButton>        m_next;
    QList<QMetaObject::Connection> m_viewConnections;
    QList<QMetaObject::Connection> m_previousConnections;
    QList<QMetaObject::Connection> m_nextConnections;
    quint64                        m_revision = 0;
    std::optional<int>             m_pendingIndex;
};
} // namespace qml_material
