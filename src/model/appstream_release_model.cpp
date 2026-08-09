#include "qml_material/model/appstream_release_model.hpp"

#include <QtCore/QByteArrayView>
#include <QtCore/QFile>
#include <QtCore/QXmlStreamReader>

#include <cctype>
#include <utility>

namespace
{

QString sourcePath(const QUrl& source) {
    if (source.scheme() == QStringLiteral("qrc")) return QStringLiteral(":") + source.path();
    if (source.isLocalFile()) return source.toLocalFile();
    if (source.scheme().isEmpty()) return source.toString();
    return {};
}

QString elementText(QXmlStreamReader& reader) {
    return reader.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
}

void appendSection(QVariantList& sections, const QString& title, const QStringList& items) {
    if (items.isEmpty()) return;

    if (title.isEmpty() && ! sections.isEmpty()) {
        auto previous = sections.back().toMap();
        if (previous.value(QStringLiteral("title")).toString().isEmpty()) {
            auto previous_items = previous.value(QStringLiteral("items")).toStringList();
            previous_items.append(items);
            previous.insert(QStringLiteral("items"), previous_items);
            sections.back() = previous;
            return;
        }
    }

    sections.append(QVariantMap {
        { QStringLiteral("title"), title },
        { QStringLiteral("items"), items },
    });
}

QStringList parseList(QXmlStreamReader& reader) {
    QStringList items;
    while (reader.readNextStartElement()) {
        if (reader.name() == QStringLiteral("li")) {
            auto text = elementText(reader);
            if (! text.isEmpty()) items.append(std::move(text));
        } else {
            reader.skipCurrentElement();
        }
    }
    return items;
}

} // namespace

namespace qml_material
{

AppStreamReleaseModel::AppStreamReleaseModel(QObject* parent): QAbstractListModel(parent) {}

int AppStreamReleaseModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_releases.size());
}

QVariant AppStreamReleaseModel::data(const QModelIndex& index, int role) const {
    if (! index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};

    const auto& release = m_releases.at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case VersionRole: return release.version;
    case DateRole: return release.date;
    case SectionsRole: return release.sections;
    default: return {};
    }
}

QHash<int, QByteArray> AppStreamReleaseModel::roleNames() const {
    static const QHash<int, QByteArray> roles {
        { VersionRole, "version" },
        { DateRole, "date" },
        { SectionsRole, "sections" },
    };
    return roles;
}

QUrl AppStreamReleaseModel::source() const { return m_source; }

void AppStreamReleaseModel::setSource(const QUrl& source) {
    if (m_source == source) return;
    m_source = source;
    Q_EMIT sourceChanged();
    reload();
}

AppStreamReleaseModel::Status AppStreamReleaseModel::status() const { return m_status; }

QString AppStreamReleaseModel::errorString() const { return m_error_string; }

void AppStreamReleaseModel::reload() {
    if (m_source.isEmpty()) {
        replace({}, Null, {});
        return;
    }

    const auto path = sourcePath(m_source);
    if (path.isEmpty()) {
        replace(
            {}, Error, QStringLiteral("unsupported release source: %1").arg(m_source.toString()));
        return;
    }

    QFile file(path);
    if (! file.open(QIODevice::ReadOnly)) {
        replace({},
                Error,
                QStringLiteral("failed to open release source %1: %2")
                    .arg(m_source.toString(), file.errorString()));
        return;
    }

    std::vector<Release> releases;
    QString              error;
    if (! parseDocument(file.readAll(), releases, error)) {
        replace({}, Error, std::move(error));
        return;
    }
    replace(std::move(releases), Ready, {});
}

bool AppStreamReleaseModel::parseDocument(QByteArray xml, std::vector<Release>& releases,
                                          QString& error) {
    if (xml.startsWith("\xef\xbb\xbf")) xml.remove(0, 3);
    auto first = qsizetype { 0 };
    while (first < xml.size() && std::isspace(static_cast<unsigned char>(xml.at(first)))) ++first;
    if (xml.mid(first, 5) == QByteArrayView { "<?xml" }) {
        const auto declaration_end = xml.indexOf("?>", first + 5);
        if (declaration_end < 0) {
            error = QStringLiteral("invalid XML declaration");
            return false;
        }
        xml.remove(first, declaration_end + 2 - first);
    }

    xml.prepend("<qml-material-releases>");
    xml.append("</qml-material-releases>");

    QXmlStreamReader reader(xml);
    if (! reader.readNextStartElement()) {
        error = QStringLiteral("release source is empty");
        return false;
    }
    collectReleases(reader, releases);
    if (reader.hasError()) {
        error = QStringLiteral("release XML at %1:%2: %3")
                    .arg(reader.lineNumber())
                    .arg(reader.columnNumber())
                    .arg(reader.errorString());
        return false;
    }
    return true;
}

void AppStreamReleaseModel::collectReleases(QXmlStreamReader&     reader,
                                            std::vector<Release>& releases) {
    while (reader.readNextStartElement()) {
        if (reader.name() == QStringLiteral("release")) {
            releases.push_back(parseRelease(reader));
        } else {
            collectReleases(reader, releases);
        }
    }
}

AppStreamReleaseModel::Release AppStreamReleaseModel::parseRelease(QXmlStreamReader& reader) {
    Release release {
        reader.attributes().value(QStringLiteral("version")).toString(),
        reader.attributes().value(QStringLiteral("date")).toString(),
        {},
    };

    while (reader.readNextStartElement()) {
        if (reader.name() == QStringLiteral("description")) {
            const auto sections = parseDescription(reader);
            for (const auto& section : sections) release.sections.append(section);
        } else {
            reader.skipCurrentElement();
        }
    }
    return release;
}

QVariantList AppStreamReleaseModel::parseDescription(QXmlStreamReader& reader) {
    QVariantList sections;
    QString      pending_paragraph;

    const auto flush_paragraph = [&]() {
        if (pending_paragraph.isEmpty()) return;
        appendSection(sections, {}, { pending_paragraph });
        pending_paragraph.clear();
    };

    while (reader.readNextStartElement()) {
        if (reader.name() == QStringLiteral("p")) {
            flush_paragraph();
            pending_paragraph = elementText(reader);
        } else if (reader.name() == QStringLiteral("ul") || reader.name() == QStringLiteral("ol")) {
            appendSection(sections, pending_paragraph, parseList(reader));
            pending_paragraph.clear();
        } else {
            flush_paragraph();
            auto text = elementText(reader);
            if (! text.isEmpty()) appendSection(sections, {}, { std::move(text) });
        }
    }
    flush_paragraph();
    return sections;
}

void AppStreamReleaseModel::replace(std::vector<Release> releases, Status status, QString error) {
    const auto old_count = rowCount();
    beginResetModel();
    m_releases = std::move(releases);
    endResetModel();
    if (old_count != rowCount()) Q_EMIT countChanged();

    if (m_status != status) {
        m_status = status;
        Q_EMIT statusChanged();
    }
    if (m_error_string != error) {
        m_error_string = std::move(error);
        Q_EMIT errorStringChanged();
    }
}

} // namespace qml_material

#include <qml_material/model/moc_appstream_release_model.cpp>
