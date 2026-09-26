#pragma once

#include <QPointer>
#include <QQuickItem>
#include <qqmlregistration.h>
#include "qml_material/control/popup.hpp"
#include "qml_material/item/item_proxy.hpp"

namespace qml_material
{
class PresentationSite;

class QML_MATERIAL_API AdaptivePresenter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickItem* content READ content WRITE setContent NOTIFY contentChanged FINAL)
    Q_PROPERTY(PresentationSite* destination READ destination WRITE setDestination NOTIFY
                   destinationChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(PresentationSite* currentSite READ currentSite NOTIFY currentSiteChanged FINAL)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged FINAL)
public:
    enum Status
    {
        Inactive,
        Mounted,
        Opening,
        Open,
        Closing,
        Switching,
        Error
    };
    Q_ENUM(Status)
    explicit AdaptivePresenter(QObject* parent = nullptr);
    ~AdaptivePresenter() override;
    QQuickItem*       content() const { return m_content; }
    PresentationSite* destination() const;
    PresentationSite* currentSite() const;
    bool              enabled() const { return m_enabled; }
    Status            status() const { return m_status; }
    QString           errorString() const { return m_error; }
    void              setContent(QQuickItem*);
    void              setDestination(PresentationSite*);
    void              setEnabled(bool);
    Q_INVOKABLE void  present();
    Q_INVOKABLE void  dismiss();
    Q_SIGNAL void     contentChanged();
    Q_SIGNAL void     destinationChanged();
    Q_SIGNAL void     enabledChanged();
    Q_SIGNAL void     currentSiteChanged();
    Q_SIGNAL void     statusChanged();
    Q_SIGNAL void     errorStringChanged();
    Q_SIGNAL void     dismissed();
    Q_SIGNAL void     aboutToRelocate(PresentationSite* from, PresentationSite* to);

private:
    friend class PresentationSite;
    void                              schedule();
    void                              reconcile(bool activationRequest = false);
    QString                           siteError(PresentationSite*) const;
    void                              detach();
    void                              siteChanging(PresentationSite*);
    void                              observePopup();
    void                              updateStatus();
    void                              setStatus(Status);
    void                              setError(const QString&);
    void                              completeDismiss(quint64);
    QPointer<QQuickItem>              m_content;
    QPointer<PresentationSite>        m_destination, m_current;
    QPointer<Popup>                   m_popup;
    QList<QPointer<PresentationSite>> m_sites;
    QList<QMetaObject::Connection>    m_contentConnections, m_popupConnections;
    QMetaObject::Connection           m_destinationConnection;
    Status                            m_status = Inactive;
    QString                           m_error;
    quint64                           m_generation = 0, m_closeGeneration = 0;
    bool m_enabled = true, m_queued = false, m_destroying = false, m_relocating = false;
    bool m_suppressed = false, m_lostControl = false;
    bool m_openRequested = false, m_dismissRequested = false;
    bool m_session = false, m_explicit = false, m_reachedOpen = false;
};

class QML_MATERIAL_API PresentationSite : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(AdaptivePresenter* presenter READ presenter WRITE setPresenter NOTIFY
                   presenterChanged FINAL)
    Q_PROPERTY(Popup* popup READ popup WRITE setPopup NOTIFY popupChanged FINAL)
    Q_PROPERTY(bool autoOpen READ autoOpen WRITE setAutoOpen NOTIFY autoOpenChanged FINAL)
    Q_PROPERTY(bool current READ current NOTIFY currentChanged FINAL)
    Q_PROPERTY(bool activationEnabled READ activationEnabled WRITE setActivationEnabled NOTIFY
                   activationEnabledChanged FINAL)
public:
    explicit PresentationSite(QQuickItem* parent = nullptr);
    ~PresentationSite() override;
    AdaptivePresenter* presenter() const { return m_presenter; }
    Popup*             popup() const { return m_popup; }
    bool               autoOpen() const { return m_autoOpen; }
    bool               current() const { return m_proxy->controlling(); }
    bool               activationEnabled() const { return m_activationEnabled; }
    void               setPresenter(AdaptivePresenter*);
    void               setPopup(Popup*);
    void               setAutoOpen(bool);
    void               setActivationEnabled(bool);
    Q_SIGNAL void      presenterChanged();
    Q_SIGNAL void      popupChanged();
    Q_SIGNAL void      autoOpenChanged();
    Q_SIGNAL void      currentChanged();
    Q_SIGNAL void      activationEnabledChanged();
    Q_SIGNAL void      activationRequested();

protected:
    void geometryChange(const QRectF&, const QRectF&) override;
    void componentComplete() override;

private:
    friend class AdaptivePresenter;
    void                           changed();
    void                           observePopupParent();
    bool                           acquirePopup();
    ItemProxy*                     m_proxy;
    QPointer<AdaptivePresenter>    m_presenter;
    QPointer<Popup>                m_popup;
    QList<QMetaObject::Connection> m_popupConnections;
    QMetaObject::Connection        m_windowConnection;
    bool                           m_autoOpen = true, m_popupRequired = false, m_destroying = false;
    bool                           m_activationEnabled = false;
};
} // namespace qml_material
