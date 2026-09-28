#include "qml_material/control/search_view.hpp"
#include "qml_material/control/search_bar.hpp"
#include "qml_material/control/page.hpp"
#include "qml_material/input/search_state.hpp"
#include "popup_surface.hpp"
#include <QGuiApplication>
#include <QInputMethod>
#include <QQmlInfo>
#include <algorithm>
#include <cmath>
#include <utility>

namespace qml_material
{
SearchView::SearchView(QObject* parent)
    : Popup(new PopupSurface<Page>(this), parent), m_defaultState(new SearchState(this)) {
    setFocus(true);
    connect(page(), &Page::headerChanged, this, &SearchView::headerChanged);
    connect(this, &Popup::aboutToShow, this, [this] {
        QPointer<SearchView> guard(this);
        const bool           initial = std::exchange(m_initialRequest, false);
        if (initial) setExpansion(1);
        if (guard && entering()) focusInput();
    });
    connect(this, &Popup::aboutToHide, this, [this] {
        if (m_inputField && m_inputField->hasActiveFocus() && qGuiApp)
            QGuiApplication::inputMethod()->hide();
    });
    observeState();
}
SearchView::~SearchView() {
    for (const auto& connection : m_barConnections) disconnect(connection);
    disconnect(m_queryConnection);
    disconnect(m_inputConnection);
    disconnect(page(), nullptr, this, nullptr);
    if (m_bar) m_bar->setSearchView(nullptr);
}
Page*        SearchView::page() const { return static_cast<Page*>(surfaceItem()); }
SearchBar*   SearchView::searchBar() const { return m_bar; }
SearchState* SearchView::searchState() const {
    return m_bar ? m_bar->searchState() : m_defaultState;
}
QString SearchView::searchText() const { return searchState()->query(); }
void    SearchView::setSearchText(const QString& text) { searchState()->setQuery(text); }
void    SearchView::observeState() {
    disconnect(m_queryConnection);
    m_queryConnection =
        connect(searchState(), &SearchState::queryChanged, this, &SearchView::searchTextChanged);
    QPointer<SearchView> guard(this);
    Q_EMIT searchStateChanged();
    if (guard) Q_EMIT searchTextChanged();
}
void SearchView::setSearchBar(SearchBar* bar) {
    if (m_bar == bar) return;
    if (bar && surfaceItem()->isAncestorOf(bar)) {
        qmlWarning(this) << "SearchBar must be outside its SearchView";
        return;
    }
    if (bar && bar->searchView() && bar->searchView() != this) {
        qmlWarning(this) << "SearchBar already has a SearchView";
        return;
    }
    QPointer<SearchView> guard(this);
    QPointer<SearchBar>  next(bar);
    const auto           revision = ++m_revision;
    if (m_componentComplete) {
        m_initialRequest = false;
        dismissImmediately();
    }
    if (! guard || revision != m_revision) return;
    for (const auto& connection : m_barConnections) disconnect(connection);
    m_barConnections.clear();
    auto previous = m_bar;
    m_bar         = next;
    if (previous) previous->setSearchView(nullptr);
    if (! guard || revision != m_revision) return;
    if (next) {
        m_barConnections << connect(
            next, &SearchBar::searchStateChanged, this, &SearchView::observeState);
        m_barConnections << connect(this, &SearchView::accepted, next, &SearchBar::accepted);
        m_barConnections << connect(next, &QObject::destroyed, this, [this] {
            QPointer<SearchView> guard(this);
            ++m_revision;
            m_bar = nullptr;
            dismissImmediately();
            if (! guard) return;
            observeState();
            if (guard) Q_EMIT searchBarChanged();
        });
        next->setSearchView(this);
    }
    if (! guard || revision != m_revision) return;
    observeState();
    if (guard && revision == m_revision) Q_EMIT searchBarChanged();
}
void SearchView::setPresentation(Presentation value) {
    if ((value != Docked && value != FullScreen) || m_presentation == value) return;
    m_presentation = value;
    Q_EMIT presentationChanged();
}
QQuickItem* SearchView::header() const { return page()->header(); }
void        SearchView::setHeader(QQuickItem* item) { page()->setHeader(item); }
void        SearchView::setInputField(QQuickItem* item) {
    if (m_inputField == item) return;
    disconnect(m_inputConnection);
    m_inputField = item;
    if (item)
        m_inputConnection = connect(item, &QObject::destroyed, this, [this] {
            m_inputField = nullptr;
            Q_EMIT inputFieldChanged();
        });
    Q_EMIT inputFieldChanged();
}
void SearchView::setExpansion(qreal value) {
    if (! std::isfinite(value)) return;
    value = std::clamp(value, qreal(0), qreal(1));
    if (m_expansion == value) return;
    m_expansion = value;
    QPointer<SearchView> guard(this);
    reposition();
    if (guard) Q_EMIT expansionChanged();
}
void SearchView::setAutoShowKeyboard(bool value) {
    if (m_autoShowKeyboard == value) return;
    m_autoShowKeyboard = value;
    Q_EMIT autoShowKeyboardChanged();
}
void SearchView::focusInput() {
    if (! m_inputField || ! isVisible() || closing() || ! surfaceItem()->isAncestorOf(m_inputField))
        return;
    QPointer<SearchView> guard(this);
    m_inputField->forceActiveFocus(Qt::PopupFocusReason);
    if (guard && m_autoShowKeyboard && m_inputField && m_inputField->hasActiveFocus() && qGuiApp)
        QGuiApplication::inputMethod()->show();
}
void SearchView::clear() {
    QPointer<SearchView> guard(this);
    setSearchText({});
    if (guard) focusInput();
}
void SearchView::submit() { Q_EMIT accepted(); }
void SearchView::open() {
    if (! m_componentComplete) m_initialRequest = true;
    Popup::open();
}
void SearchView::close() {
    m_initialRequest = false;
    Popup::close();
}
void SearchView::componentComplete() {
    QPointer<SearchView> guard(this);
    Popup::componentComplete();
    if (guard) m_componentComplete = true;
}
QRectF SearchView::originRect() const {
    if (m_bar && overlayItem() && m_bar->window() == overlayItem()->window())
        return m_bar->mapRectToItem(overlayItem(), m_bar->boundingRect());
    return { Popup::surfacePosition(), QSizeF(width(), std::min<qreal>(56, height())) };
}
QPointF SearchView::surfacePosition() const {
    return originRect().topLeft() * (1 - m_expansion) + Popup::surfacePosition() * m_expansion;
}
QSizeF SearchView::surfaceSize() const {
    return originRect().size() * (1 - m_expansion) + QSizeF(width(), height()) * m_expansion;
}
QList<Popup::TransitionTarget> SearchView::transitionTargets(bool opening) const {
    return { { QStringLiteral("expansion"), opening ? 1.0 : 0.0 } };
}
void SearchView::finalizeTransition(bool opening) { setExpansion(opening ? 1 : 0); }
} // namespace qml_material
