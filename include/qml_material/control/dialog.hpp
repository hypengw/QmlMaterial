#pragma once

#include "qml_material/control/popup.hpp"
#include "qml_material/control/page.hpp"
#include "qml_material/control/dialog_button_box.hpp"

namespace qml_material
{
class QML_MATERIAL_API Dialog : public Popup {
    Q_OBJECT
    QML_NAMED_ELEMENT(DialogBase)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QQuickItem* header READ header WRITE setHeader NOTIFY headerChanged FINAL)
    Q_PROPERTY(QQuickItem* footer READ footer WRITE setFooter NOTIFY footerChanged FINAL)
    Q_PROPERTY(
        qreal implicitHeaderWidth READ implicitHeaderWidth NOTIFY implicitHeaderWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHeaderHeight READ implicitHeaderHeight NOTIFY
                   implicitHeaderHeightChanged FINAL)
    Q_PROPERTY(
        qreal implicitFooterWidth READ implicitFooterWidth NOTIFY implicitFooterWidthChanged FINAL)
    Q_PROPERTY(qreal implicitFooterHeight READ implicitFooterHeight NOTIFY
                   implicitFooterHeightChanged FINAL)
    Q_PROPERTY(int result READ result WRITE setResult NOTIFY resultChanged FINAL)
    Q_PROPERTY(int standardButtons READ standardButtons WRITE setStandardButtons NOTIFY
                   standardButtonsChanged FINAL)
public:
    enum DialogCode
    {
        Rejected,
        Accepted
    };
    Q_ENUM(DialogCode)
    enum StandardButton
    {
        NoButton        = DialogButtonBox::NoButton,
        Ok              = DialogButtonBox::Ok,
        Save            = DialogButtonBox::Save,
        SaveAll         = DialogButtonBox::SaveAll,
        Open            = DialogButtonBox::Open,
        Yes             = DialogButtonBox::Yes,
        YesToAll        = DialogButtonBox::YesToAll,
        No              = DialogButtonBox::No,
        NoToAll         = DialogButtonBox::NoToAll,
        Abort           = DialogButtonBox::Abort,
        Retry           = DialogButtonBox::Retry,
        Ignore          = DialogButtonBox::Ignore,
        Close           = DialogButtonBox::Close,
        Cancel          = DialogButtonBox::Cancel,
        Discard         = DialogButtonBox::Discard,
        Help            = DialogButtonBox::Help,
        Apply           = DialogButtonBox::Apply,
        Reset           = DialogButtonBox::Reset,
        RestoreDefaults = DialogButtonBox::RestoreDefaults
    };
    Q_ENUM(StandardButton)
    explicit Dialog(QObject* parent = nullptr);
    ~Dialog() override;
    QString       title() const { return page()->title(); }
    void          setTitle(QString value) { page()->setTitle(value); }
    Q_SIGNAL void titleChanged();
    QQuickItem*   header() const { return page()->header(); }
    void          setHeader(QQuickItem* value) { page()->setHeader(value); }
    Q_SIGNAL void headerChanged();
    QQuickItem*   footer() const { return page()->footer(); }
    void          setFooter(QQuickItem* value) { page()->setFooter(value); }
    Q_SIGNAL void footerChanged();
    qreal         implicitHeaderWidth() const { return page()->implicitHeaderWidth(); }
    Q_SIGNAL void implicitHeaderWidthChanged();
    qreal         implicitHeaderHeight() const { return page()->implicitHeaderHeight(); }
    Q_SIGNAL void implicitHeaderHeightChanged();
    qreal         implicitFooterWidth() const { return page()->implicitFooterWidth(); }
    Q_SIGNAL void implicitFooterWidthChanged();
    qreal         implicitFooterHeight() const { return page()->implicitFooterHeight(); }
    Q_SIGNAL void implicitFooterHeightChanged();
    int           result() const { return m_result; }
    void          setResult(int);
    int           standardButtons() const { return m_standardButtons; }
    void          setStandardButtons(int);
    Q_INVOKABLE AbstractButton* standardButton(int) const;
    Q_INVOKABLE void            accept();
    Q_INVOKABLE void            reject();
    Q_INVOKABLE void            done(int);
    void                        closeFromInput() override;
    Q_SIGNAL void               resultChanged();
    Q_SIGNAL void               standardButtonsChanged();
    Q_SIGNAL void               accepted();
    Q_SIGNAL void               rejected();
    Q_SIGNAL void               applied();
    Q_SIGNAL void               reset();
    Q_SIGNAL void               discarded();
    Q_SIGNAL void               helpRequested();

private:
    Page*                          page() const { return static_cast<Page*>(surfaceItem()); }
    void                           syncButtonBoxes();
    QList<QMetaObject::Connection> m_buttonConnections;
    quint64                        m_buttonRevision  = 0;
    int                            m_result          = Rejected;
    int                            m_standardButtons = NoButton;
};
} // namespace qml_material
