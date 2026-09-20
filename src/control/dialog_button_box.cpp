#include "qml_material/control/dialog_button_box.hpp"
#include "qml_material/util/qt.hpp"
#include <QQmlContext>
#include <QQmlInfo>
#include <QCoreApplication>
#include <algorithm>

namespace qml_material
{
namespace
{
struct StandardEntry {
    int                         button;
    DialogButtonBox::ButtonRole role;
    const char*                 text;
};
// Material dialogs use trailing affirmative actions.
const StandardEntry entries[] {
    { DialogButtonBox::Help,
      DialogButtonBox::HelpRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Help") },
    { DialogButtonBox::Reset,
      DialogButtonBox::ResetRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Reset") },
    { DialogButtonBox::RestoreDefaults,
      DialogButtonBox::ResetRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Restore Defaults") },
    { DialogButtonBox::Discard,
      DialogButtonBox::DestructiveRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Discard") },
    { DialogButtonBox::No, DialogButtonBox::NoRole, QT_TRANSLATE_NOOP("QPlatformTheme", "No") },
    { DialogButtonBox::NoToAll,
      DialogButtonBox::NoRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "No to All") },
    { DialogButtonBox::Cancel,
      DialogButtonBox::RejectRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Cancel") },
    { DialogButtonBox::Close,
      DialogButtonBox::RejectRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Close") },
    { DialogButtonBox::Abort,
      DialogButtonBox::RejectRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Abort") },
    { DialogButtonBox::Apply,
      DialogButtonBox::ApplyRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Apply") },
    { DialogButtonBox::Ignore,
      DialogButtonBox::AcceptRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Ignore") },
    { DialogButtonBox::Retry,
      DialogButtonBox::AcceptRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Retry") },
    { DialogButtonBox::Open,
      DialogButtonBox::AcceptRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Open") },
    { DialogButtonBox::Save,
      DialogButtonBox::AcceptRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Save") },
    { DialogButtonBox::SaveAll,
      DialogButtonBox::AcceptRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Save All") },
    { DialogButtonBox::Yes, DialogButtonBox::YesRole, QT_TRANSLATE_NOOP("QPlatformTheme", "Yes") },
    { DialogButtonBox::YesToAll,
      DialogButtonBox::YesRole,
      QT_TRANSLATE_NOOP("QPlatformTheme", "Yes to All") },
    { DialogButtonBox::Ok, DialogButtonBox::AcceptRole, QT_TRANSLATE_NOOP("QPlatformTheme", "OK") }
};
} // namespace
DialogButtonBox::DialogButtonBox(QQuickItem* parent): Panel(parent) {
    connect(this, &Panel::contentChildrenChanged, this, &DialogButtonBox::observe);
    connect(this, &Control::availableWidthChanged, this, &QQuickItem::polish);
    connect(this, &Control::availableHeightChanged, this, &QQuickItem::polish);
    connect(this, &Control::spacingChanged, this, &QQuickItem::polish);
    connect(this, &Control::mirroredChanged, this, &QQuickItem::polish);
}
DialogButtonBox::~DialogButtonBox() {
    utils::disconnectAll(m_connections);
    for (const auto& button : m_buttons) {
        if (! button) continue;
        auto* attached = qobject_cast<DialogButtonBoxAttached*>(
            qmlAttachedPropertiesObject<DialogButtonBox>(button, false));
        if (attached) attached->m_box = nullptr;
    }
}
DialogButtonBoxAttached* DialogButtonBox::qmlAttachedProperties(QObject* object) {
    return new DialogButtonBoxAttached(object);
}
void DialogButtonBox::setStandardButtons(int buttons) {
    if (m_standard_buttons == buttons) return;
    m_standard_buttons = buttons;
    QPointer<DialogButtonBox> guard(this);
    rebuild();
    if (guard) Q_EMIT standardButtonsChanged();
}
void DialogButtonBox::setDelegate(QQmlComponent* value) {
    if (m_delegate == value) return;
    m_delegate = value;
    QPointer<DialogButtonBox> guard(this);
    const auto                old = m_standard_items;
    m_standard_items.clear();
    for (const auto& item : old) {
        if (item) {
            item->setParentItem(nullptr);
            if (item) item->deleteLater();
        }
        if (! guard) return;
    }
    rebuild();
    if (guard) Q_EMIT delegateChanged();
}
void DialogButtonBox::setAlignment(Qt::Alignment value) {
    if (m_alignment == value) return;
    m_alignment = value;
    polish();
    Q_EMIT alignmentChanged();
}
AbstractButton* DialogButtonBox::standardButton(int value) const {
    auto* button = m_standard_items.value(value).data();
    return button && button->parentItem() == contentItem() ? button : nullptr;
}
AbstractButton* DialogButtonBox::itemAt(int index) const { return m_buttons.value(index); }
void            DialogButtonBox::addItem(AbstractButton* item) {
    if (item) item->setParentItem(contentItem());
}
void DialogButtonBox::removeItem(AbstractButton* item) {
    if (item && item->parentItem() == contentItem()) item->setParentItem(nullptr);
}
void DialogButtonBox::componentComplete() {
    Panel::componentComplete();
    m_complete = true;
    rebuild();
}
void DialogButtonBox::rebuild() {
    if (! m_complete || m_rebuilding) return;
    m_rebuilding                        = true;
    const auto                requested = m_standard_buttons;
    QPointer<DialogButtonBox> guard(this);
    const auto                keys = m_standard_items.keys();
    for (const auto key : keys) {
        if (! (m_standard_buttons & key)) {
            auto button = m_standard_items.take(key);
            if (button) {
                button->setParentItem(nullptr);
                if (button) button->deleteLater();
            }
            if (! guard) return;
        }
    }
    for (const auto& entry : entries) {
        if (! (m_standard_buttons & entry.button) || m_standard_items.value(entry.button) ||
            ! m_delegate)
            continue;
        auto* context = m_delegate->creationContext();
        auto* object  = m_delegate->createWithInitialProperties(
            { { "text", QCoreApplication::translate("QPlatformTheme", entry.text) } },
            context ? context : qmlContext(this));
        if (! guard) {
            if (object) object->deleteLater();
            return;
        }
        auto* button = qobject_cast<AbstractButton*>(object);
        if (! button) {
            if (object) object->deleteLater();
            qmlWarning(this) << "delegate must create AbstractButtonBase";
            continue;
        }
        button->setParent(this);
        m_standard_items.insert(entry.button, button);
        auto* attached = qobject_cast<DialogButtonBoxAttached*>(
            qmlAttachedPropertiesObject<DialogButtonBox>(button, true));
        QPointer<AbstractButton> alive(button);
        attached->setButtonRole(entry.role);
        if (! guard) return;
        if (! alive) continue;
        button->setParentItem(contentItem());
        if (! guard) return;
    }
    m_rebuilding = false;
    observe();
    if (guard && requested != m_standard_buttons)
        QMetaObject::invokeMethod(this, &DialogButtonBox::rebuild, Qt::QueuedConnection);
}
void DialogButtonBox::observe() {
    const auto                revision = ++m_observation;
    QPointer<DialogButtonBox> guard(this);
    utils::disconnectAll(m_connections);
    const auto old = m_buttons;
    m_buttons.clear();
    if (contentItem()) {
        for (auto* item : contentItem()->childItems()) {
            if (auto* button = qobject_cast<AbstractButton*>(item)) m_buttons.append(button);
        }
    }
    std::stable_sort(
        m_buttons.begin(), m_buttons.end(), [this](const auto& left, const auto& right) {
            const auto rank = [this](const auto& button) {
                const int key   = m_standard_items.key(button, 0);
                int       index = 0;
                for (const auto& entry : entries) {
                    if (entry.button == key) return index;
                    ++index;
                }
                return index;
            };
            return rank(left) < rank(right);
        });
    for (const auto& button : old) {
        if (! button || m_buttons.contains(button)) continue;
        auto* attached = qobject_cast<DialogButtonBoxAttached*>(
            qmlAttachedPropertiesObject<DialogButtonBox>(button, false));
        if (attached) {
            attached->m_box = nullptr;
            Q_EMIT attached->buttonBoxChanged();
            if (! guard || revision != m_observation) return;
        }
    }
    const auto current = m_buttons;
    for (const auto& button : current) {
        if (! button || button->parentItem() != contentItem()) continue;
        auto* attached = qobject_cast<DialogButtonBoxAttached*>(
            qmlAttachedPropertiesObject<DialogButtonBox>(button, true));
        if (attached->m_box != this) {
            attached->m_box = this;
            Q_EMIT attached->buttonBoxChanged();
            if (! guard || revision != m_observation) return;
            if (! button || button->parentItem() != contentItem()) continue;
        }
        m_connections.append(connect(button, &AbstractButton::clicked, this, [this, button]() {
            if (button) activate(button);
        }));
        m_connections.append(
            connect(button, &QQuickItem::implicitWidthChanged, this, &QQuickItem::polish));
        m_connections.append(
            connect(button, &QQuickItem::implicitHeightChanged, this, &QQuickItem::polish));
        m_connections.append(
            connect(button, &QQuickItem::visibleChanged, this, &QQuickItem::polish));
    }
    polish();
    if (old != m_buttons) Q_EMIT countChanged();
}
void DialogButtonBox::updatePolish() {
    QPointer<DialogButtonBox> guard(this);
    Panel::updatePolish();
    if (! guard) return;
    qreal                           total = 0, height = 0;
    QList<QPointer<AbstractButton>> visible;
    for (const auto& button : m_buttons)
        if (button && (! isVisible() || button->isVisible())) {
            if (! visible.isEmpty()) total += spacing();
            total += button->implicitWidth();
            height = std::max(height, button->implicitHeight());
            visible.append(button);
        }
    setContentWidth(total);
    if (! guard) return;
    setContentHeight(height);
    if (! guard) return;
    qreal x = m_alignment.testFlag(Qt::AlignHCenter) ? (availableWidth() - total) / 2
              : m_alignment.testFlag(Qt::AlignRight) ? availableWidth() - total
                                                     : 0;
    if (mirrored()) std::reverse(visible.begin(), visible.end());
    for (const auto& button : visible) {
        if (! button) continue;
        button->setSize(QSizeF(button->implicitWidth(), button->implicitHeight()));
        if (! guard) return;
        if (! button) continue;
        button->setPosition(QPointF(x, (availableHeight() - button->height()) / 2));
        if (! guard) return;
        if (! button) continue;
        x += button->width() + spacing();
    }
}
void DialogButtonBox::activate(AbstractButton* button) {
    auto* attached = qobject_cast<DialogButtonBoxAttached*>(
        qmlAttachedPropertiesObject<DialogButtonBox>(button, true));
    const auto                role = attached->buttonRole();
    QPointer<DialogButtonBox> guard(this);
    Q_EMIT clicked(button);
    if (! guard) return;
    switch (role) {
    case AcceptRole:
    case YesRole: Q_EMIT accepted(); break;
    case RejectRole:
    case NoRole: Q_EMIT rejected(); break;
    case ApplyRole: Q_EMIT applied(); break;
    case ResetRole: Q_EMIT reset(); break;
    case DestructiveRole: Q_EMIT discarded(); break;
    case HelpRole: Q_EMIT helpRequested(); break;
    default: break;
    }
}
} // namespace qml_material
