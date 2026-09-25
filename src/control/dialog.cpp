#include "qml_material/control/dialog.hpp"
#include "popup_surface.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
Dialog::Dialog(QObject* parent): Popup(new PopupSurface<Page>(this), parent) {
    setFocus(true);
    connect(page(), &Page::titleChanged, this, &Dialog::titleChanged);
    connect(page(), &Page::headerChanged, this, &Dialog::headerChanged);
    connect(page(), &Page::footerChanged, this, &Dialog::footerChanged);
    connect(page(), &Page::implicitHeaderWidthChanged, this, &Dialog::implicitHeaderWidthChanged);
    connect(page(), &Page::implicitHeaderHeightChanged, this, &Dialog::implicitHeaderHeightChanged);
    connect(page(), &Page::implicitFooterWidthChanged, this, &Dialog::implicitFooterWidthChanged);
    connect(page(), &Page::implicitFooterHeightChanged, this, &Dialog::implicitFooterHeightChanged);
    connect(page(), &Page::headerChanged, this, &Dialog::syncButtonBoxes);
    connect(page(), &Page::footerChanged, this, &Dialog::syncButtonBoxes);
}
Dialog::~Dialog() {
    utils::disconnectAll(m_buttonConnections);
    disconnect(page(), nullptr, this, nullptr);
}
void Dialog::setResult(int value) {
    if (m_result == value) return;
    m_result = value;
    Q_EMIT resultChanged();
}
void Dialog::accept() { done(Accepted); }
void Dialog::reject() { done(Rejected); }
void Dialog::closeFromInput() { reject(); }
void Dialog::done(int value) {
    QPointer<Dialog> guard(this);
    setResult(value);
    if (! guard) return;
    if (value == Accepted)
        Q_EMIT accepted();
    else if (value == Rejected)
        Q_EMIT rejected();
    if (guard) close();
}
void Dialog::setStandardButtons(int value) {
    if (m_standardButtons == value) return;
    m_standardButtons = value;
    QPointer<Dialog> guard(this);
    for (QPointer<QQuickItem> item :
         { QPointer<QQuickItem>(header()), QPointer<QQuickItem>(footer()) }) {
        if (auto box = qobject_cast<DialogButtonBox*>(item)) box->setStandardButtons(value);
        if (! guard || m_standardButtons != value) return;
    }
    Q_EMIT standardButtonsChanged();
}
AbstractButton* Dialog::standardButton(int button) const {
    for (auto item : { footer(), header() }) {
        if (auto box = qobject_cast<DialogButtonBox*>(item)) {
            if (auto found = box->standardButton(button)) return found;
        }
    }
    return nullptr;
}
void Dialog::syncButtonBoxes() {
    const auto revision = ++m_buttonRevision;
    utils::disconnectAll(m_buttonConnections);
    QPointer<Dialog> guard(this);
    for (QPointer<QQuickItem> item :
         { QPointer<QQuickItem>(header()), QPointer<QQuickItem>(footer()) }) {
        auto box = qobject_cast<DialogButtonBox*>(item);
        if (! box) continue;
        m_buttonConnections.append(connect(box, &DialogButtonBox::accepted, this, &Dialog::accept));
        m_buttonConnections.append(connect(box, &DialogButtonBox::rejected, this, &Dialog::reject));
        m_buttonConnections.append(connect(box, &DialogButtonBox::applied, this, &Dialog::applied));
        m_buttonConnections.append(connect(box, &DialogButtonBox::reset, this, &Dialog::reset));
        m_buttonConnections.append(
            connect(box, &DialogButtonBox::helpRequested, this, &Dialog::helpRequested));
        m_buttonConnections.append(connect(box, &DialogButtonBox::discarded, this, [this] {
            QPointer<Dialog> guard(this);
            Q_EMIT discarded();
            if (guard) close();
        }));
        box->setStandardButtons(m_standardButtons);
        if (! guard || revision != m_buttonRevision) return;
    }
}
} // namespace qml_material
