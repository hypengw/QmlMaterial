#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QWindow>
#include <QPointer>

namespace qml_material
{

struct PortalFilter {
    QString     name;
    QStringList globs;
};

struct PortalRequest {
    QString             title;
    QString             accept_label;
    bool                save { false };
    bool                multiple { false };
    bool                directory { false };
    QList<PortalFilter> filters;
    int                 current_filter_index { -1 };
    QUrl                current_folder;
    QString             current_name;
    QPointer<QWindow>   parent_window;
};

class FileDialogBackend : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    ~FileDialogBackend() override = default;

    virtual void open(const PortalRequest& req) = 0;
    virtual void close()                        = 0;

Q_SIGNALS:
    void accepted(QList<QUrl> urls);
    void rejected();
};

FileDialogBackend* makeFileDialogBackend(QObject* parent);

} // namespace qml_material
