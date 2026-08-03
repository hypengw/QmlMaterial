#pragma once

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <qqmlregistration.h>
#include <vector>

#include "qml_material/export.hpp"
#include "qml_material/util/pool.hpp"

class QQuickItem;

namespace qml_material
{

class PopupPresenter;
class PresentationAttached;

class QML_MATERIAL_API PopupPresentation : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(PopupPresentation)
    QML_UNCREATABLE("PopupPresentation objects are returned by PopupPresenter.present()")

    Q_PROPERTY(Status status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged FINAL)
    Q_PROPERTY(QVariant result READ result NOTIFY resultChanged FINAL)

public:
    enum Status
    {
        Loading,
        WaitingForReady,
        Opening,
        Open,
        Closing,
        Closed,
        Error,
        Cancelled,
    };
    Q_ENUM(Status)

    ~PopupPresentation() override;

    auto status() const -> Status;
    auto active() const -> bool;
    auto errorString() const -> QString;
    auto result() const -> QVariant;

    Q_INVOKABLE void close();
    Q_INVOKABLE void cancel();

    Q_SIGNAL void statusChanged();
    Q_SIGNAL void activeChanged();
    Q_SIGNAL void errorStringChanged();
    Q_SIGNAL void resultChanged();
    Q_SIGNAL void opened();
    Q_SIGNAL void completed(const QVariant& result);
    Q_SIGNAL void closed();
    Q_SIGNAL void failed(const QString& error);
    Q_SIGNAL void cancelled();

private Q_SLOTS:
    void onPopupOpened();
    void onPopupClosed();

private:
    friend class PopupPresenter;
    friend class PresentationAttached;

    enum class Finish
    {
        None,
        Closed,
        Error,
        Cancelled,
    };

    explicit PopupPresentation(PopupPresenter* presenter);

    void setStatus(Status value);
    void setErrorString(QString value);
    void setResult(QVariant value);

    QPointer<PopupPresenter>       m_presenter;
    QPointer<PoolRequest>          m_request;
    QPointer<QObject>              m_popup;
    QPointer<PresentationAttached> m_attached;
    Status                         m_status { Loading };
    Finish                         m_finish { Finish::None };
    QString                        m_errorString;
    QVariant                       m_result;
    bool                           m_terminal { false };
    bool                           m_completed { false };
};

class QML_MATERIAL_API PresentationAttached final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool ready READ ready WRITE setReady NOTIFY readyChanged FINAL)

public:
    explicit PresentationAttached(QObject* parent = nullptr);

    auto ready() const -> bool;
    void setReady(bool value);

    Q_INVOKABLE void fail(const QString& error);
    Q_INVOKABLE void complete(const QVariant& result = {});

    Q_SIGNAL void readyChanged();
    Q_SIGNAL void failureReported(const QString& error);
    Q_SIGNAL void completionRequested(const QVariant& result);

private:
    friend class PopupPresenter;

    void bind(PopupPresentation* presentation);
    void unbind(PopupPresentation* presentation);

    QPointer<PopupPresentation> m_presentation;
    QString                     m_errorString;
    bool                        m_ready { true };
    bool                        m_failed { false };
};

class QML_MATERIAL_API Presentation : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Presentation)
    QML_UNCREATABLE("Presentation is only available as an attached property")
    QML_ATTACHED(PresentationAttached)

public:
    explicit Presentation(QObject* parent = nullptr);

    static PresentationAttached* qmlAttachedProperties(QObject* object);
};

class QML_MATERIAL_API PopupPresenter : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickItem* host READ host WRITE setHost NOTIFY hostChanged FINAL)
    Q_PROPERTY(Pool::IncubationMode incubationMode READ incubationMode WRITE setIncubationMode
                   NOTIFY incubationModeChanged FINAL)

public:
    explicit PopupPresenter(QObject* parent = nullptr);
    ~PopupPresenter() override;

    auto host() const -> QQuickItem*;
    void setHost(QQuickItem* value);

    auto incubationMode() const -> Pool::IncubationMode;
    void setIncubationMode(Pool::IncubationMode value);

    Q_INVOKABLE PopupPresentation* present(const QVariant&    source,
                                           const QVariantMap& initialProperties = {});
    Q_INVOKABLE void               closeAll();

    Q_SIGNAL void hostChanged();
    Q_SIGNAL void incubationModeChanged();

private:
    friend class PopupPresentation;

    void requestClose(PopupPresentation* presentation, PopupPresentation::Finish finish);
    void handleRequest(PopupPresentation* presentation);
    void attachPopup(PopupPresentation* presentation, QObject* popup);
    void openPopup(PopupPresentation* presentation);
    void failPresentation(PopupPresentation* presentation, QString error);
    void finishPresentation(PopupPresentation* presentation, PopupPresentation::Finish finish);
    void completePresentation(PopupPresentation* presentation, QVariant result);
    void popupOpened(PopupPresentation* presentation);
    void popupClosed(PopupPresentation* presentation);
    void popupDestroyed(PopupPresentation* presentation);
    void presentationDestroyed(PopupPresentation* presentation);
    void retire(PopupPresentation* presentation);
    void ensurePoolContext();

    QPointer<QQuickItem>                     m_host;
    Pool*                                    m_pool;
    Pool::IncubationMode                     m_incubationMode { Pool::AsynchronousIfNested };
    std::vector<QPointer<PopupPresentation>> m_presentations;
    bool                                     m_destroying { false };
};

} // namespace qml_material
