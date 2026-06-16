#include "qml_material/util/qml_util.hpp"
#include "qml_material/util/loggingcategory.hpp"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusPendingCall>
#include <QStringList>
#include <QVariantMap>

namespace qml_material
{

namespace
{
inline constexpr auto kService              = "org.freedesktop.portal.Desktop";
inline constexpr auto kObjectPath           = "/org/freedesktop/portal/desktop";
inline constexpr auto kOpenUriInterface     = "org.freedesktop.portal.OpenURI";
inline constexpr auto kFileManagerService   = "org.freedesktop.FileManager1";
inline constexpr auto kFileManagerPath      = "/org/freedesktop/FileManager1";
inline constexpr auto kFileManagerInterface = "org.freedesktop.FileManager1";
} // namespace

// xdg-desktop-portal `OpenURI.OpenURI(parent_window, uri, options)`.
// `parent_window` is intentionally empty (no native handle); the portal
// just uses the user's default handler. Fire-and-forget — failures land
// in the log without blocking the caller.
void sysOpenUrl(const QString& url) {
    if (url.isEmpty()) return;
    auto        msg = QDBusMessage::createMethodCall(QString::fromLatin1(kService),
                                                     QString::fromLatin1(kObjectPath),
                                                     QString::fromLatin1(kOpenUriInterface),
                                                     QStringLiteral("OpenURI"));
    QVariantMap options;
    msg.setArguments({ QString {}, url, options });
    QDBusConnection::sessionBus().asyncCall(msg);
}

void sysOpenFolder(const QString& url) {
    if (url.isEmpty()) return;
    auto msg = QDBusMessage::createMethodCall(QString::fromLatin1(kFileManagerService),
                                              QString::fromLatin1(kFileManagerPath),
                                              QString::fromLatin1(kFileManagerInterface),
                                              QStringLiteral("ShowFolders"));
    msg.setArguments({ QStringList { url }, QString {} });
    QDBusConnection::sessionBus().asyncCall(msg);
}

} // namespace qml_material
