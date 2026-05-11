#pragma once

#include <QPointer>
#include <QtDBus/QDBusArgument>

#include "dialog/file_dialog_p.hpp"

namespace qml_material
{

struct XdpFilterPattern {
    quint32 type { 0 };
    QString glob;
};

struct XdpFilter {
    QString                 name;
    QList<XdpFilterPattern> patterns;
};

QDBusArgument&       operator<<(QDBusArgument&, const XdpFilterPattern&);
const QDBusArgument& operator>>(const QDBusArgument&, XdpFilterPattern&);
QDBusArgument&       operator<<(QDBusArgument&, const XdpFilter&);
const QDBusArgument& operator>>(const QDBusArgument&, XdpFilter&);

class XdpDialogCall;

class XdpFileDialogBackend : public FileDialogBackend {
    Q_OBJECT
public:
    explicit XdpFileDialogBackend(QObject* parent = nullptr);
    ~XdpFileDialogBackend() override;

    void open(const PortalRequest& req) override;
    void close() override;

private:
    QPointer<XdpDialogCall> m_current;
};

} // namespace qml_material

Q_DECLARE_METATYPE(qml_material::XdpFilterPattern)
Q_DECLARE_METATYPE(qml_material::XdpFilter)
Q_DECLARE_METATYPE(QList<qml_material::XdpFilter>)
