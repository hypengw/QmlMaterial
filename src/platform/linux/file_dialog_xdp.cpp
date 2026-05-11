#include "file_dialog_xdp.hpp"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusPendingCall>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusMetaType>
#include <QGuiApplication>
#include <QRandomGenerator>
#include <QLoggingCategory>

namespace qml_material
{
namespace
{
inline constexpr auto kService          = "org.freedesktop.portal.Desktop";
inline constexpr auto kObjectPath       = "/org/freedesktop/portal/desktop";
inline constexpr auto kFileChooserIface = "org.freedesktop.portal.FileChooser";
inline constexpr auto kRequestIface     = "org.freedesktop.portal.Request";

Q_LOGGING_CATEGORY(LcXdp, "qml_material.dialog.xdp")

QString generateHandleToken() {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString           out;
    out.reserve(32);
    for (int i = 0; i < 32; ++i) {
        out.append(QLatin1Char(alphabet[QRandomGenerator::global()->bounded(int(sizeof(alphabet) - 1))]));
    }
    return out;
}

QString predictRequestPath(const QDBusConnection& bus, const QString& token) {
    QString sender = bus.baseService();
    if (sender.startsWith(QLatin1Char(':'))) sender = sender.mid(1);
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    return QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);
}

QString makeParentWindowHandle(QWindow* w) {
    if (! w) return QString();
    const QString platform = QGuiApplication::platformName();
    if (platform == QLatin1String("xcb")) {
        return QStringLiteral("x11:%1").arg(w->winId(), 0, 16);
    }
    // wayland xdg-foreign export not implemented in v1
    return QString();
}

QList<XdpFilter> filtersToXdp(const QList<PortalFilter>& filters) {
    QList<XdpFilter> out;
    out.reserve(filters.size());
    for (const auto& f : filters) {
        XdpFilter xf;
        xf.name = f.name;
        xf.patterns.reserve(f.globs.size());
        for (const auto& g : f.globs) {
            xf.patterns.push_back({ 0, g });
        }
        out.push_back(std::move(xf));
    }
    return out;
}

QByteArray pathToBytes(const QUrl& url) {
    QString s = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (s.isEmpty()) return {};
    QByteArray b = s.toUtf8();
    b.append('\0');
    return b;
}
} // namespace

QDBusArgument& operator<<(QDBusArgument& a, const XdpFilterPattern& p) {
    a.beginStructure();
    a << p.type << p.glob;
    a.endStructure();
    return a;
}
const QDBusArgument& operator>>(const QDBusArgument& a, XdpFilterPattern& p) {
    a.beginStructure();
    a >> p.type >> p.glob;
    a.endStructure();
    return a;
}
QDBusArgument& operator<<(QDBusArgument& a, const XdpFilter& f) {
    a.beginStructure();
    a << f.name;
    a.beginArray(qMetaTypeId<XdpFilterPattern>());
    for (const auto& p : f.patterns) a << p;
    a.endArray();
    a.endStructure();
    return a;
}
const QDBusArgument& operator>>(const QDBusArgument& a, XdpFilter& f) {
    a.beginStructure();
    a >> f.name;
    a.beginArray();
    while (! a.atEnd()) {
        XdpFilterPattern p;
        a >> p;
        f.patterns.push_back(p);
    }
    a.endArray();
    a.endStructure();
    return a;
}

class XdpDialogCall : public QObject {
    Q_OBJECT
public:
    explicit XdpDialogCall(QObject* parent = nullptr): QObject(parent) {}

    bool run(const PortalRequest& req) {
        auto bus = QDBusConnection::sessionBus();
        if (! bus.isConnected()) {
            qCWarning(LcXdp) << "session bus not connected";
            Q_EMIT rejected();
            deleteLater();
            return false;
        }

        m_handle_token = generateHandleToken();
        m_request_path = predictRequestPath(bus, m_handle_token);

        bool ok = bus.connect(kService,
                              m_request_path,
                              kRequestIface,
                              QStringLiteral("Response"),
                              this,
                              SLOT(onResponse(uint, QVariantMap)));
        if (! ok) {
            qCWarning(LcXdp) << "failed to connect Response signal on" << m_request_path;
            Q_EMIT rejected();
            deleteLater();
            return false;
        }

        const QString method = req.save ? QStringLiteral("SaveFile") : QStringLiteral("OpenFile");
        QDBusMessage msg = QDBusMessage::createMethodCall(
            kService, kObjectPath, kFileChooserIface, method);

        QVariantMap options;
        options.insert(QStringLiteral("handle_token"), m_handle_token);
        options.insert(QStringLiteral("modal"), true);
        if (! req.accept_label.isEmpty())
            options.insert(QStringLiteral("accept_label"), req.accept_label);
        if (req.multiple) options.insert(QStringLiteral("multiple"), true);
        if (req.directory) options.insert(QStringLiteral("directory"), true);

        if (! req.directory && ! req.filters.isEmpty()) {
            auto xdp_filters = filtersToXdp(req.filters);
            options.insert(QStringLiteral("filters"), QVariant::fromValue(xdp_filters));
            if (req.current_filter_index >= 0
                && req.current_filter_index < xdp_filters.size()) {
                options.insert(QStringLiteral("current_filter"),
                               QVariant::fromValue(xdp_filters[req.current_filter_index]));
            }
        }

        if (req.save && ! req.current_name.isEmpty())
            options.insert(QStringLiteral("current_name"), req.current_name);

        if (! req.current_folder.isEmpty()) {
            auto bytes = pathToBytes(req.current_folder);
            if (! bytes.isEmpty())
                options.insert(QStringLiteral("current_folder"), bytes);
        }

        const QString parent_window = makeParentWindowHandle(req.parent_window);
        msg << parent_window << req.title << options;

        auto pending = bus.asyncCall(msg);
        auto watcher = new QDBusPendingCallWatcher(pending, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
            QDBusPendingReply<QDBusObjectPath> reply = *w;
            w->deleteLater();
            if (reply.isError()) {
                qCWarning(LcXdp) << "FileChooser call failed:" << reply.error().message();
                disconnectResponse();
                Q_EMIT rejected();
                deleteLater();
                return;
            }
            const QString actual = reply.value().path();
            if (actual != m_request_path) {
                // resubscribe on the real path. Disconnect predicted, connect actual.
                auto bus = QDBusConnection::sessionBus();
                bus.disconnect(kService,
                               m_request_path,
                               kRequestIface,
                               QStringLiteral("Response"),
                               this,
                               SLOT(onResponse(uint, QVariantMap)));
                m_request_path = actual;
                bool ok2 = bus.connect(kService,
                                       m_request_path,
                                       kRequestIface,
                                       QStringLiteral("Response"),
                                       this,
                                       SLOT(onResponse(uint, QVariantMap)));
                if (! ok2) {
                    qCWarning(LcXdp) << "failed to resubscribe Response on" << m_request_path;
                    Q_EMIT rejected();
                    deleteLater();
                }
            }
        });

        return true;
    }

    void cancel() {
        if (m_request_path.isEmpty()) return;
        auto bus = QDBusConnection::sessionBus();
        auto msg = QDBusMessage::createMethodCall(
            kService, m_request_path, kRequestIface, QStringLiteral("Close"));
        bus.asyncCall(msg);
    }

public Q_SLOTS:
    void onResponse(uint code, QVariantMap results) {
        disconnectResponse();
        if (code != 0) {
            Q_EMIT rejected();
            deleteLater();
            return;
        }
        auto        v = results.value(QStringLiteral("uris"));
        QStringList uri_strings;
        if (v.canConvert<QStringList>()) {
            uri_strings = v.toStringList();
        } else if (v.canConvert<QDBusArgument>()) {
            auto arg = v.value<QDBusArgument>();
            arg.beginArray();
            while (! arg.atEnd()) {
                QString s;
                arg >> s;
                uri_strings.push_back(s);
            }
            arg.endArray();
        }
        QList<QUrl> urls;
        urls.reserve(uri_strings.size());
        for (const auto& s : uri_strings) urls.push_back(QUrl(s));
        if (urls.isEmpty()) {
            Q_EMIT rejected();
        } else {
            Q_EMIT accepted(std::move(urls));
        }
        deleteLater();
    }

Q_SIGNALS:
    void accepted(QList<QUrl> urls);
    void rejected();

private:
    void disconnectResponse() {
        if (m_request_path.isEmpty()) return;
        auto bus = QDBusConnection::sessionBus();
        bus.disconnect(kService,
                       m_request_path,
                       kRequestIface,
                       QStringLiteral("Response"),
                       this,
                       SLOT(onResponse(uint, QVariantMap)));
        m_request_path.clear();
    }

    QString m_handle_token;
    QString m_request_path;
};

XdpFileDialogBackend::XdpFileDialogBackend(QObject* parent): FileDialogBackend(parent) {
    static bool registered = [] {
        qDBusRegisterMetaType<XdpFilterPattern>();
        qDBusRegisterMetaType<XdpFilter>();
        qDBusRegisterMetaType<QList<XdpFilter>>();
        return true;
    }();
    Q_UNUSED(registered);
}

XdpFileDialogBackend::~XdpFileDialogBackend() = default;

void XdpFileDialogBackend::open(const PortalRequest& req) {
    if (m_current) {
        m_current->cancel();
        m_current->deleteLater();
        m_current.clear();
    }
    auto* call = new XdpDialogCall(this);
    m_current  = call;
    connect(call, &XdpDialogCall::accepted, this, [this](QList<QUrl> urls) {
        Q_EMIT accepted(std::move(urls));
    });
    connect(call, &XdpDialogCall::rejected, this, [this] { Q_EMIT rejected(); });
    call->run(req);
}

void XdpFileDialogBackend::close() {
    if (m_current) {
        m_current->cancel();
    }
}

FileDialogBackend* makeFileDialogBackend(QObject* parent) {
    return new XdpFileDialogBackend(parent);
}

} // namespace qml_material

#include "file_dialog_xdp.moc"
