#include "qml_material/control/search_bar.hpp"
#include "qml_material/input/search_state.hpp"
#include "qml_material/control/search_view.hpp"

namespace qml_material
{
SearchBar::SearchBar(QQuickItem* parent)
    : CommandButton(parent), m_defaultState(new SearchState(this)) {
    resetSearchState();
}
void SearchBar::activate() {
    if (! canActivate()) return;
    const QPointer<SearchBar> guard(this);
    const auto                target   = m_searchView;
    const auto                revision = m_revision;
    CommandButton::activate();
    if (guard && revision == m_revision && target) target->open();
}
SearchView* SearchBar::searchView() const { return m_searchView; }
void        SearchBar::setSearchView(SearchView* view) {
    if (m_searchView == view) return;
    m_searchView                       = view;
    const auto                revision = ++m_revision;
    const QPointer<SearchBar> guard(this);
    cancelActivation();
    if (guard && revision == m_revision) Q_EMIT searchViewChanged();
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
    const auto          revision = ++m_revision;
    QPointer<SearchBar> guard(this);
    cancelActivation();
    if (! guard || revision != m_revision) return;
    Q_EMIT searchStateChanged();
    if (guard && revision == m_revision && m_state == state) Q_EMIT searchTextChanged();
}
} // namespace qml_material
