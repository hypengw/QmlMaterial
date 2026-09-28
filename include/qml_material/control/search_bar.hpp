#pragma once

#include "qml_material/control/button.hpp"
#include <QPointer>

Q_MOC_INCLUDE("qml_material/input/search_state.hpp")
Q_MOC_INCLUDE("qml_material/control/search_view.hpp")

namespace qml_material
{
class SearchState;
class SearchView;

class QML_MATERIAL_API SearchBar : public Button {
    Q_OBJECT
    QML_NAMED_ELEMENT(SearchBarBase)
    Q_PROPERTY(SearchState* searchState READ searchState WRITE setSearchState RESET resetSearchState
                   NOTIFY searchStateChanged FINAL)
    Q_PROPERTY(
        QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged FINAL)
    Q_PROPERTY(SearchView* searchView READ searchView NOTIFY searchViewChanged FINAL)
public:
    explicit SearchBar(QQuickItem* parent = nullptr);
    SearchState*  searchState() const;
    void          setSearchState(SearchState*);
    void          resetSearchState();
    QString       searchText() const;
    void          setSearchText(const QString&);
    SearchView*   searchView() const;
    Q_SIGNAL void searchViewChanged();
    Q_SIGNAL void searchStateChanged();
    Q_SIGNAL void searchTextChanged();
    Q_SIGNAL void accepted();

private:
    friend class SearchView;
    void                    setSearchView(SearchView*);
    QPointer<SearchView>    m_searchView;
    SearchState*            m_defaultState;
    QPointer<SearchState>   m_state;
    QMetaObject::Connection m_queryConnection, m_destroyConnection;
};
} // namespace qml_material
