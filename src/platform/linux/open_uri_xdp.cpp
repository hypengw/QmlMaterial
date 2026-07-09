#include "qml_material/util/qml_util.hpp"
#include "qml_material/util/loggingcategory.hpp"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusPendingCall>
#include <QtDBus/QDBusUnixFileDescriptor>
#include <QFile>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QVariantMap>

#include <fcntl.h>
#include <unistd.h>

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

auto localPathFromUrl(const QString& url) -> QString {
    const auto parsed = QUrl { url };
    if (parsed.isValid() && parsed.isLocalFile()) return parsed.toLocalFile();
    if (parsed.isValid() && ! parsed.scheme().isEmpty()) return {};
    return url;
}

auto fileManagerUrl(const QString& url) -> QString {
    const auto localPath = localPathFromUrl(url);
    if (localPath.isEmpty()) return url;
    return QUrl::fromLocalFile(localPath).toString();
}

auto tryOpenDirectoryPortal(const QString& url) -> bool {
    if (! QDBusUnixFileDescriptor::isSupported()) return false;

    const auto localPath = localPathFromUrl(url);
    if (localPath.isEmpty()) return false;

    const auto encoded = QFile::encodeName(localPath);
    const int  fd      = ::open(encoded.constData(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;

    const QDBusUnixFileDescriptor descriptor { fd };
    ::close(fd);
    if (! descriptor.isValid()) return false;

    auto        msg = QDBusMessage::createMethodCall(QString::fromLatin1(kService),
                                                     QString::fromLatin1(kObjectPath),
                                                     QString::fromLatin1(kOpenUriInterface),
                                                     QStringLiteral("OpenDirectory"));
    QVariantMap options;
    msg.setArguments({ QString {}, QVariant::fromValue(descriptor), options });

    const auto reply = QDBusConnection::sessionBus().call(msg, QDBus::BlockWithGui, 3000);
    return reply.type() != QDBusMessage::ErrorMessage;
}
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
    if (tryOpenDirectoryPortal(url)) return;

    auto msg = QDBusMessage::createMethodCall(QString::fromLatin1(kFileManagerService),
                                              QString::fromLatin1(kFileManagerPath),
                                              QString::fromLatin1(kFileManagerInterface),
                                              QStringLiteral("ShowFolders"));
    msg.setArguments({ QStringList { fileManagerUrl(url) }, QString {} });
    QDBusConnection::sessionBus().asyncCall(msg);
}

} // namespace qml_material
