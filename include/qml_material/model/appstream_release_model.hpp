#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QByteArray>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QVariantList>
#include <QtQml/QQmlEngine>

#include <vector>

#include "qml_material/export.hpp"

class QXmlStreamReader;

namespace qml_material
{

class QML_MATERIAL_API AppStreamReleaseModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Status
    {
        Null,
        Ready,
        Error,
    };
    Q_ENUM(Status)

    enum Role
    {
        VersionRole = Qt::UserRole + 1,
        DateRole,
        SectionsRole,
    };

    explicit AppStreamReleaseModel(QObject* parent = nullptr);

    int                    rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant               data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QUrl    source() const;
    void    setSource(const QUrl& source);
    Status  status() const;
    QString errorString() const;

    Q_INVOKABLE void reload();

Q_SIGNALS:
    void sourceChanged();
    void statusChanged();
    void errorStringChanged();
    void countChanged();

private:
    struct Release {
        QString      version;
        QString      date;
        QVariantList sections;
    };

    static bool    parseDocument(QByteArray xml, std::vector<Release>& releases, QString& error);
    static void    collectReleases(QXmlStreamReader& reader, std::vector<Release>& releases);
    static Release parseRelease(QXmlStreamReader& reader);
    static QVariantList parseDescription(QXmlStreamReader& reader);

    void replace(std::vector<Release> releases, Status status, QString error);

    QUrl                 m_source;
    Status               m_status { Null };
    QString              m_error_string;
    std::vector<Release> m_releases;
};

} // namespace qml_material
