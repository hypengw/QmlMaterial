#include "file_dialog_p.hpp"

#include <QLoggingCategory>
#include <QTimer>

namespace qml_material
{
namespace
{
Q_LOGGING_CATEGORY(LcStub, "qml_material.dialog.stub")
}

class StubFileDialogBackend : public FileDialogBackend {
    Q_OBJECT
public:
    using FileDialogBackend::FileDialogBackend;
    void open(const PortalRequest&) override {
        qCWarning(LcStub) << "FileDialog: no portal backend on this platform";
        QTimer::singleShot(0, this, [this] { Q_EMIT rejected(); });
    }
    void close() override {}
};

FileDialogBackend* makeFileDialogBackend(QObject* parent) {
    return new StubFileDialogBackend(parent);
}

} // namespace qml_material

#include "file_dialog_stub.moc"
