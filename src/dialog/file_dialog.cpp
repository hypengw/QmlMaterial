#include "qml_material/dialog/file_dialog.hpp"

#include <QFileInfo>
#include <QRegularExpression>

#include "file_dialog_p.hpp"

namespace qml_material
{
namespace
{
PortalFilter parseNameFilter(const QString& s) {
    PortalFilter out;
    out.name = s;
    int open = s.lastIndexOf(QLatin1Char('('));
    int close = s.lastIndexOf(QLatin1Char(')'));
    if (open >= 0 && close > open) {
        const QString inside = s.mid(open + 1, close - open - 1).trimmed();
        static const QRegularExpression sep(QStringLiteral("\\s+"));
        for (const auto& tok : inside.split(sep, Qt::SkipEmptyParts)) {
            out.globs.push_back(tok);
        }
    } else {
        // Bare pattern, e.g. "*.png"
        out.globs.push_back(s.trimmed());
    }
    return out;
}

QList<PortalFilter> parseNameFilters(const QStringList& list) {
    QList<PortalFilter> out;
    out.reserve(list.size());
    for (const auto& s : list) out.push_back(parseNameFilter(s));
    return out;
}

QUrl applyDefaultSuffix(const QUrl& url, const QString& suffix) {
    if (suffix.isEmpty()) return url;
    if (! url.isLocalFile()) return url;
    QFileInfo fi(url.toLocalFile());
    if (! fi.suffix().isEmpty()) return url;
    QString p = url.toLocalFile() + QLatin1Char('.') + suffix;
    return QUrl::fromLocalFile(p);
}
} // namespace

// ---- FileDialogSelectedNameFilter ----

FileDialogSelectedNameFilter::FileDialogSelectedNameFilter(FileDialog* parent)
    : QObject(parent), m_dialog(parent) {}

int FileDialogSelectedNameFilter::index() const { return m_index; }

void FileDialogSelectedNameFilter::setIndex(int i) {
    if (i == m_index) return;
    m_index = i;
    Q_EMIT indexChanged();
    Q_EMIT nameChanged();
}

QString FileDialogSelectedNameFilter::name() const {
    if (! m_dialog) return {};
    const auto& filters = m_dialog->nameFilters();
    if (m_index < 0 || m_index >= filters.size()) return {};
    return filters[m_index];
}

// ---- FileDialog ----

FileDialog::FileDialog(QObject* parent)
    : QObject(parent), m_selected_name_filter(new FileDialogSelectedNameFilter(this)) {}

FileDialog::~FileDialog() = default;

QString FileDialog::title() const { return m_title; }
void    FileDialog::setTitle(const QString& v) {
    if (v == m_title) return;
    m_title = v;
    Q_EMIT titleChanged();
}

QString FileDialog::acceptLabel() const { return m_accept_label; }
void    FileDialog::setAcceptLabel(const QString& v) {
    if (v == m_accept_label) return;
    m_accept_label = v;
    Q_EMIT acceptLabelChanged();
}

FileDialog::FileMode FileDialog::fileMode() const { return m_file_mode; }
void                 FileDialog::setFileMode(FileMode v) {
    if (v == m_file_mode) return;
    m_file_mode = v;
    Q_EMIT fileModeChanged();
}

QStringList FileDialog::nameFilters() const { return m_name_filters; }
void        FileDialog::setNameFilters(const QStringList& v) {
    if (v == m_name_filters) return;
    m_name_filters = v;
    Q_EMIT nameFiltersChanged();
    Q_EMIT m_selected_name_filter->nameChanged();
}

FileDialogSelectedNameFilter* FileDialog::selectedNameFilter() const {
    return m_selected_name_filter;
}

QUrl FileDialog::currentFolder() const { return m_current_folder; }
void FileDialog::setCurrentFolder(const QUrl& v) {
    if (v == m_current_folder) return;
    m_current_folder = v;
    Q_EMIT currentFolderChanged();
}

QString FileDialog::defaultSuffix() const { return m_default_suffix; }
void    FileDialog::setDefaultSuffix(const QString& v) {
    if (v == m_default_suffix) return;
    m_default_suffix = v;
    Q_EMIT defaultSuffixChanged();
}

QUrl        FileDialog::selectedFile() const {
    return m_selected_files.isEmpty() ? QUrl() : m_selected_files.first();
}
QList<QUrl> FileDialog::selectedFiles() const { return m_selected_files; }

QWindow* FileDialog::parentWindow() const { return m_parent_window.data(); }
void     FileDialog::setParentWindow(QWindow* w) {
    if (w == m_parent_window) return;
    m_parent_window = w;
    Q_EMIT parentWindowChanged();
}

void FileDialog::ensureBackend() {
    if (m_backend) return;
    m_backend = makeFileDialogBackend(this);
    connect(m_backend, &FileDialogBackend::accepted, this, &FileDialog::applyResult);
    connect(m_backend, &FileDialogBackend::rejected, this, [this] {
        if (! m_selected_files.isEmpty()) {
            m_selected_files.clear();
            Q_EMIT selectedFilesChanged();
        }
        Q_EMIT rejected();
    });
}

void FileDialog::applyResult(const QList<QUrl>& urls) {
    QList<QUrl> out = urls;
    if (m_file_mode == SaveFile && ! out.isEmpty()) {
        out[0] = applyDefaultSuffix(out[0], m_default_suffix);
    }
    m_selected_files = std::move(out);
    Q_EMIT selectedFilesChanged();
    Q_EMIT accepted();
}

void FileDialog::open() {
    ensureBackend();
    PortalRequest req;
    req.title         = m_title;
    req.accept_label  = m_accept_label;
    req.save          = (m_file_mode == SaveFile);
    req.multiple      = (m_file_mode == OpenFiles);
    req.directory     = false;
    req.filters       = parseNameFilters(m_name_filters);
    req.current_filter_index = m_selected_name_filter->index();
    req.current_folder       = m_current_folder;
    if (req.save && ! m_selected_files.isEmpty() && m_selected_files.first().isLocalFile()) {
        req.current_name = QFileInfo(m_selected_files.first().toLocalFile()).fileName();
    }
    req.parent_window = m_parent_window.data();
    m_backend->open(req);
}

void FileDialog::close() {
    if (m_backend) m_backend->close();
}

// ---- FolderDialog ----

FolderDialog::FolderDialog(QObject* parent): QObject(parent) {}
FolderDialog::~FolderDialog() = default;

QString FolderDialog::title() const { return m_title; }
void    FolderDialog::setTitle(const QString& v) {
    if (v == m_title) return;
    m_title = v;
    Q_EMIT titleChanged();
}

QString FolderDialog::acceptLabel() const { return m_accept_label; }
void    FolderDialog::setAcceptLabel(const QString& v) {
    if (v == m_accept_label) return;
    m_accept_label = v;
    Q_EMIT acceptLabelChanged();
}

QUrl FolderDialog::currentFolder() const { return m_current_folder; }
void FolderDialog::setCurrentFolder(const QUrl& v) {
    if (v == m_current_folder) return;
    m_current_folder = v;
    Q_EMIT currentFolderChanged();
}

QUrl     FolderDialog::selectedFolder() const { return m_selected_folder; }
QWindow* FolderDialog::parentWindow() const { return m_parent_window.data(); }
void     FolderDialog::setParentWindow(QWindow* w) {
    if (w == m_parent_window) return;
    m_parent_window = w;
    Q_EMIT parentWindowChanged();
}

void FolderDialog::ensureBackend() {
    if (m_backend) return;
    m_backend = makeFileDialogBackend(this);
    connect(m_backend, &FileDialogBackend::accepted, this, [this](QList<QUrl> urls) {
        m_selected_folder = urls.isEmpty() ? QUrl() : urls.first();
        Q_EMIT selectedFolderChanged();
        Q_EMIT accepted();
    });
    connect(m_backend, &FileDialogBackend::rejected, this, [this] {
        if (! m_selected_folder.isEmpty()) {
            m_selected_folder = QUrl();
            Q_EMIT selectedFolderChanged();
        }
        Q_EMIT rejected();
    });
}

void FolderDialog::open() {
    ensureBackend();
    PortalRequest req;
    req.title          = m_title;
    req.accept_label   = m_accept_label;
    req.directory      = true;
    req.current_folder = m_current_folder;
    req.parent_window  = m_parent_window.data();
    m_backend->open(req);
}

void FolderDialog::close() {
    if (m_backend) m_backend->close();
}

} // namespace qml_material
