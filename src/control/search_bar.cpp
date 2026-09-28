#include "qml_material/control/search_bar.hpp"
#include "qml_material/input/search_state.hpp"
#include "qml_material/control/search_view.hpp"

namespace qml_material
{
SearchBar::SearchBar(QQuickItem* parent): Button(parent), m_defaultState(new SearchState(this)) {
    resetSearchState();
    connect(this, &AbstractButton::clicked, this, [this] {
        if (m_searchView) m_searchView->open();
    });
}
SearchView* SearchBar::searchView() const { return m_searchView; }
void        SearchBar::setSearchView(SearchView* view) {
    if (m_searchView == view) return;
    m_searchView = view;
    Q_EMIT searchViewChanged();
}
SearchState* SearchBar::searchState() const { return m_state; }
QString      SearchBar::searchText() const { return m_state ? m_state->query() : QString(); }
void         SearchBar::setSearchText(const QString& value) {
    if (m_state) m_state->setQuery(value);
}
void SearchBar::resetSearchState() { setSearchState(m_defaultState); }
void SearchBar::setSearchState(SearchState* state) {
    if (! state) state = m_defaultState;
    if (m_state == state) return;
    disconnect(m_queryConnection);
    disconnect(m_destroyConnection);
    m_state = state;
    m_queryConnection =
        connect(state, &SearchState::queryChanged, this, &SearchBar::searchTextChanged);
    if (state != m_defaultState)
        m_destroyConnection =
            connect(state, &QObject::destroyed, this, &SearchBar::resetSearchState);
    QPointer<SearchBar> guard(this);
    Q_EMIT searchStateChanged();
    if (guard && m_state == state) Q_EMIT searchTextChanged();
}
} // namespace qml_material
