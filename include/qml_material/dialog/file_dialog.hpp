#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QList>
#include <QStringList>
#include <QWindow>
#include <QQmlEngine>

#include "qml_material/export.hpp"

namespace qml_material
{

class FileDialog;
class FileDialogBackend;

class QML_MATERIAL_API FileDialogSelectedNameFilter : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged FINAL)
public:
    explicit FileDialogSelectedNameFilter(FileDialog* parent);

    int     index() const;
    void    setIndex(int);
    QString name() const;

    Q_SIGNAL void indexChanged();
    Q_SIGNAL void nameChanged();

private:
    friend class FileDialog;
    FileDialog* m_dialog;
    int         m_index { 0 };
};

class QML_MATERIAL_API FileDialog : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(FileDialog)

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString acceptLabel READ acceptLabel WRITE setAcceptLabel NOTIFY acceptLabelChanged
                   FINAL)
    Q_PROPERTY(FileMode fileMode READ fileMode WRITE setFileMode NOTIFY fileModeChanged FINAL)
    Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters NOTIFY
                   nameFiltersChanged FINAL)
    Q_PROPERTY(FileDialogSelectedNameFilter* selectedNameFilter READ selectedNameFilter CONSTANT
                   FINAL)
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY
                   currentFolderChanged FINAL)
    Q_PROPERTY(QString defaultSuffix READ defaultSuffix WRITE setDefaultSuffix NOTIFY
                   defaultSuffixChanged FINAL)
    Q_PROPERTY(QUrl selectedFile READ selectedFile NOTIFY selectedFilesChanged FINAL)
    Q_PROPERTY(QList<QUrl> selectedFiles READ selectedFiles NOTIFY selectedFilesChanged FINAL)
    Q_PROPERTY(QUrl currentFile READ selectedFile NOTIFY selectedFilesChanged FINAL)
    Q_PROPERTY(QList<QUrl> currentFiles READ selectedFiles NOTIFY selectedFilesChanged FINAL)
    Q_PROPERTY(
        QWindow* parentWindow READ parentWindow WRITE setParentWindow NOTIFY parentWindowChanged)

public:
    enum FileMode {
        OpenFile  = 0,
        OpenFiles = 1,
        SaveFile  = 2,
    };
    Q_ENUM(FileMode)

    explicit FileDialog(QObject* parent = nullptr);
    ~FileDialog() override;

    QString  title() const;
    void     setTitle(const QString&);
    QString  acceptLabel() const;
    void     setAcceptLabel(const QString&);
    FileMode fileMode() const;
    void     setFileMode(FileMode);
    QStringList nameFilters() const;
    void        setNameFilters(const QStringList&);
    FileDialogSelectedNameFilter* selectedNameFilter() const;
    QUrl     currentFolder() const;
    void     setCurrentFolder(const QUrl&);
    QString  defaultSuffix() const;
    void     setDefaultSuffix(const QString&);
    QUrl         selectedFile() const;
    QList<QUrl>  selectedFiles() const;
    QWindow* parentWindow() const;
    void     setParentWindow(QWindow*);

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();

    Q_SIGNAL void titleChanged();
    Q_SIGNAL void acceptLabelChanged();
    Q_SIGNAL void fileModeChanged();
    Q_SIGNAL void nameFiltersChanged();
    Q_SIGNAL void currentFolderChanged();
    Q_SIGNAL void defaultSuffixChanged();
    Q_SIGNAL void selectedFilesChanged();
    Q_SIGNAL void parentWindowChanged();
    Q_SIGNAL void accepted();
    Q_SIGNAL void rejected();

private:
    void ensureBackend();
    void applyResult(const QList<QUrl>& urls);

    QString                       m_title;
    QString                       m_accept_label;
    FileMode                      m_file_mode { OpenFile };
    QStringList                   m_name_filters;
    FileDialogSelectedNameFilter* m_selected_name_filter;
    QUrl                          m_current_folder;
    QString                       m_default_suffix;
    QList<QUrl>                   m_selected_files;
    QPointer<QWindow>             m_parent_window;
    FileDialogBackend*            m_backend { nullptr };
};

class QML_MATERIAL_API FolderDialog : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(FolderDialog)

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString acceptLabel READ acceptLabel WRITE setAcceptLabel NOTIFY acceptLabelChanged
                   FINAL)
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY
                   currentFolderChanged FINAL)
    Q_PROPERTY(QUrl selectedFolder READ selectedFolder NOTIFY selectedFolderChanged FINAL)
    Q_PROPERTY(
        QWindow* parentWindow READ parentWindow WRITE setParentWindow NOTIFY parentWindowChanged)

public:
    explicit FolderDialog(QObject* parent = nullptr);
    ~FolderDialog() override;

    QString title() const;
    void    setTitle(const QString&);
    QString acceptLabel() const;
    void    setAcceptLabel(const QString&);
    QUrl    currentFolder() const;
    void    setCurrentFolder(const QUrl&);
    QUrl    selectedFolder() const;
    QWindow* parentWindow() const;
    void     setParentWindow(QWindow*);

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();

    Q_SIGNAL void titleChanged();
    Q_SIGNAL void acceptLabelChanged();
    Q_SIGNAL void currentFolderChanged();
    Q_SIGNAL void selectedFolderChanged();
    Q_SIGNAL void parentWindowChanged();
    Q_SIGNAL void accepted();
    Q_SIGNAL void rejected();

private:
    void ensureBackend();

    QString            m_title;
    QString            m_accept_label;
    QUrl               m_current_folder;
    QUrl               m_selected_folder;
    QPointer<QWindow>  m_parent_window;
    FileDialogBackend* m_backend { nullptr };
};

} // namespace qml_material
