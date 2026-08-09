#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "qml_material/model/appstream_release_model.hpp"

using qml_material::AppStreamReleaseModel;

class AppStreamReleaseModelTest : public QObject {
    Q_OBJECT

private:
    static QUrl writeXml(QTemporaryDir& directory, const QByteArray& xml) {
        const auto path = directory.filePath(QStringLiteral("releases.xml"));
        QFile      file(path);
        if (! file.open(QIODevice::WriteOnly) || file.write(xml) != xml.size()) return {};
        return QUrl::fromLocalFile(path);
    }

private Q_SLOTS:
    void parsesReleaseFragment() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto source = writeXml(directory, R"(
            <release version="2.0.0" date="2026-08-09">
              <description>
                <p>Added:</p>
                <ul><li>First feature.</li><li>Second feature.</li></ul>
                <p>Standalone note.</p>
              </description>
            </release>
            <release version="1.0.0" date="2026-01-01"/>
        )");
        QVERIFY(! source.isEmpty());

        AppStreamReleaseModel model;
        model.setSource(source);

        QCOMPARE(model.status(), AppStreamReleaseModel::Ready);
        QCOMPARE(model.errorString(), QString {});
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0), AppStreamReleaseModel::VersionRole).toString(),
                 QStringLiteral("2.0.0"));
        QCOMPARE(model.data(model.index(0), AppStreamReleaseModel::DateRole).toString(),
                 QStringLiteral("2026-08-09"));

        const auto sections =
            model.data(model.index(0), AppStreamReleaseModel::SectionsRole).toList();
        QCOMPARE(sections.size(), 2);
        QCOMPARE(sections.at(0).toMap().value(QStringLiteral("title")).toString(),
                 QStringLiteral("Added:"));
        QCOMPARE(
            sections.at(0).toMap().value(QStringLiteral("items")).toStringList(),
            QStringList({ QStringLiteral("First feature."), QStringLiteral("Second feature.") }));
        QCOMPARE(sections.at(1).toMap().value(QStringLiteral("items")).toStringList(),
                 QStringList({ QStringLiteral("Standalone note.") }));
    }

    void parsesReleasesContainer() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto source = writeXml(directory, R"(
            <?xml version="1.0" encoding="UTF-8"?>
            <releases>
              <release version="3.0.0" date="2026-08-09">
                <description><ul><li>Container entry.</li></ul></description>
              </release>
            </releases>
        )");

        AppStreamReleaseModel model;
        model.setSource(source);

        QCOMPARE(model.status(), AppStreamReleaseModel::Ready);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), AppStreamReleaseModel::VersionRole).toString(),
                 QStringLiteral("3.0.0"));
    }

    void rejectsMalformedInputAtomically() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto source = writeXml(directory, R"(
            <release version="2.0.0"><description><p>Valid prefix.</p></description></release>
            <release version="1.0.0"><description><ul><li>Broken</description></release>
        )");

        AppStreamReleaseModel model;
        model.setSource(source);

        QCOMPARE(model.status(), AppStreamReleaseModel::Error);
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(model.errorString().contains(QStringLiteral("release XML")));
    }
};

QTEST_GUILESS_MAIN(AppStreamReleaseModelTest)

#include "appstream_release_model.moc"
