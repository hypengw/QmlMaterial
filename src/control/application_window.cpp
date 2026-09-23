#include "qml_material/control/application_window.hpp"
#include "qml_material/util/qml_util.hpp"

namespace qml_material
{

ApplicationWindow::ApplicationWindow(QWindow* parent): QQuickWindowQmlImpl(parent) {
    m_frame = new Page;
    m_frame->setParent(this);
    m_frame->setParentItem(QQuickWindow::contentItem());
    m_frame->setFocus(true);

    m_body = new Page;
    m_body->setParent(this);
    m_body->setFocus(true);
    m_frame->setContentItem(m_body);

    connect(this, &QQuickWindow::widthChanged, this, &ApplicationWindow::relayout);
    connect(this, &QQuickWindow::heightChanged, this, &ApplicationWindow::relayout);
    if (auto* root = QQuickWindow::contentItem()) {
        connect(root, &QQuickItem::widthChanged, this, &ApplicationWindow::relayout);
        connect(root, &QQuickItem::heightChanged, this, &ApplicationWindow::relayout);
    }

    connect(m_frame, &Page::backgroundChanged, this, &ApplicationWindow::backgroundChanged);
    connect(m_frame, &Page::headerChanged, this, &ApplicationWindow::menuBarChanged);
    connect(m_frame, &Page::fontChanged, this, &ApplicationWindow::fontChanged);
    connect(m_frame, &Page::localeChanged, this, &ApplicationWindow::localeChanged);
    connect(m_body, &Page::headerChanged, this, &ApplicationWindow::headerChanged);
    connect(m_body, &Page::footerChanged, this, &ApplicationWindow::footerChanged);
    connect(m_body, &Page::topPaddingChanged, this, &ApplicationWindow::topPaddingChanged);
    connect(m_body, &Page::leftPaddingChanged, this, &ApplicationWindow::leftPaddingChanged);
    connect(m_body, &Page::rightPaddingChanged, this, &ApplicationWindow::rightPaddingChanged);
    connect(m_body, &Page::bottomPaddingChanged, this, &ApplicationWindow::bottomPaddingChanged);
    connect(this,
            &QQuickWindow::activeFocusItemChanged,
            this,
            &ApplicationWindow::updateActiveFocusControl);

    relayout();
}

QQuickItem* ApplicationWindow::contentItem() const { return m_body->contentItem(); }

QQmlListProperty<QObject> ApplicationWindow::contentData() { return m_body->contentData(); }

QQuickItem* ApplicationWindow::background() const { return m_frame->background(); }

void ApplicationWindow::setBackground(QQuickItem* item) { m_frame->setBackground(item); }

QQuickItem* ApplicationWindow::menuBar() const { return m_frame->header(); }

void ApplicationWindow::setMenuBar(QQuickItem* item) { m_frame->setHeader(item); }

QQuickItem* ApplicationWindow::header() const { return m_body->header(); }

void ApplicationWindow::setHeader(QQuickItem* item) { m_body->setHeader(item); }

QQuickItem* ApplicationWindow::footer() const { return m_body->footer(); }

void ApplicationWindow::setFooter(QQuickItem* item) { m_body->setFooter(item); }

QQuickItem* ApplicationWindow::activeFocusControl() const { return m_activeFocusControl; }

QFont ApplicationWindow::font() const { return m_frame->font(); }

void ApplicationWindow::setFont(const QFont& font) { m_frame->setFont(font); }

void ApplicationWindow::resetFont() { m_frame->resetFont(); }

QLocale ApplicationWindow::locale() const { return m_frame->locale(); }

void ApplicationWindow::setLocale(const QLocale& locale) { m_frame->setLocale(locale); }

void ApplicationWindow::resetLocale() { m_frame->resetLocale(); }

qreal ApplicationWindow::topPadding() const { return m_body->topPadding(); }

void ApplicationWindow::setTopPadding(qreal value) { m_body->setTopPadding(value); }

void ApplicationWindow::resetTopPadding() { m_body->resetTopPadding(); }

qreal ApplicationWindow::leftPadding() const { return m_body->leftPadding(); }

void ApplicationWindow::setLeftPadding(qreal value) { m_body->setLeftPadding(value); }

void ApplicationWindow::resetLeftPadding() { m_body->resetLeftPadding(); }

qreal ApplicationWindow::rightPadding() const { return m_body->rightPadding(); }

void ApplicationWindow::setRightPadding(qreal value) { m_body->setRightPadding(value); }

void ApplicationWindow::resetRightPadding() { m_body->resetRightPadding(); }

qreal ApplicationWindow::bottomPadding() const { return m_body->bottomPadding(); }

void ApplicationWindow::setBottomPadding(qreal value) { m_body->setBottomPadding(value); }

void ApplicationWindow::resetBottomPadding() { m_body->resetBottomPadding(); }

void ApplicationWindow::relayout() {
    if (! m_frame) return;
    auto* root = QQuickWindow::contentItem();
    if (root && m_frame->parentItem() != root) m_frame->setParentItem(root);
    m_frame->setX(0);
    m_frame->setY(0);
    m_frame->setSize(root ? root->size() : QSizeF(width(), height()));
}

void ApplicationWindow::updateActiveFocusControl() {
    auto* control = Util::controlAncestor(activeFocusItem());
    if (control == m_frame || control == m_body) control = nullptr;
    if (m_activeFocusControl == control) return;
    m_activeFocusControl = control;
    Q_EMIT activeFocusControlChanged();
}

} // namespace qml_material
