#pragma once

#include "qml_material/control/popup.hpp"

Q_MOC_INCLUDE("qml_material/control/search_bar.hpp")
Q_MOC_INCLUDE("qml_material/input/search_state.hpp")

namespace qml_material
{
class SearchBar;
class SearchState;
class Page;

class QML_MATERIAL_API SearchView : public Popup {
    Q_OBJECT
    QML_NAMED_ELEMENT(SearchViewBase)
    Q_PROPERTY(SearchBar* searchBar READ searchBar WRITE setSearchBar NOTIFY searchBarChanged FINAL)
    Q_PROPERTY(SearchState* searchState READ searchState NOTIFY searchStateChanged FINAL)
    Q_PROPERTY(
        QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged FINAL)
    Q_PROPERTY(Presentation presentation READ presentation WRITE setPresentation NOTIFY
                   presentationChanged FINAL)
    Q_PROPERTY(QQuickItem* header READ header WRITE setHeader NOTIFY headerChanged FINAL)
    Q_PROPERTY(
        QQuickItem* inputField READ inputField WRITE setInputField NOTIFY inputFieldChanged FINAL)
    Q_PROPERTY(qreal expansion READ expansion WRITE setExpansion NOTIFY expansionChanged FINAL)
    Q_PROPERTY(bool autoShowKeyboard READ autoShowKeyboard WRITE setAutoShowKeyboard NOTIFY
                   autoShowKeyboardChanged FINAL)
public:
    enum Presentation
    {
        Docked,
        FullScreen
    };
    Q_ENUM(Presentation)
    explicit SearchView(QObject* parent = nullptr);
    ~SearchView() override;
    SearchBar*       searchBar() const;
    void             setSearchBar(SearchBar*);
    SearchState*     searchState() const;
    QString          searchText() const;
    void             setSearchText(const QString&);
    Presentation     presentation() const { return m_presentation; }
    void             setPresentation(Presentation);
    QQuickItem*      header() const;
    void             setHeader(QQuickItem*);
    QQuickItem*      inputField() const { return m_inputField; }
    void             setInputField(QQuickItem*);
    qreal            expansion() const { return m_expansion; }
    void             setExpansion(qreal);
    bool             autoShowKeyboard() const { return m_autoShowKeyboard; }
    void             setAutoShowKeyboard(bool);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void submit();
    void             open() override;
    void             close() override;
    Q_SIGNAL void    searchBarChanged();
    Q_SIGNAL void    searchStateChanged();
    Q_SIGNAL void    searchTextChanged();
    Q_SIGNAL void    presentationChanged();
    Q_SIGNAL void    headerChanged();
    Q_SIGNAL void    inputFieldChanged();
    Q_SIGNAL void    expansionChanged();
    Q_SIGNAL void    autoShowKeyboardChanged();
    Q_SIGNAL void    accepted();

protected:
    void                    componentComplete() override;
    QPointF                 surfacePosition() const override;
    QSizeF                  surfaceSize() const override;
    QList<TransitionTarget> transitionTargets(bool) const override;
    void                    finalizeTransition(bool) override;

private:
    Page*                          page() const;
    QRectF                         originRect() const;
    void                           observeState();
    void                           focusInput();
    QPointer<SearchBar>            m_bar;
    SearchState*                   m_defaultState;
    QPointer<QQuickItem>           m_inputField;
    QList<QMetaObject::Connection> m_barConnections;
    QMetaObject::Connection        m_queryConnection, m_inputConnection;
    Presentation                   m_presentation = Docked;
    qreal                          m_expansion    = 0;
    bool    m_autoShowKeyboard = true, m_componentComplete = false, m_initialRequest = false;
    quint64 m_revision = 0;
};
} // namespace qml_material
