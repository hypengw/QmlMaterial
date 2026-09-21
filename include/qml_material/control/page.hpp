#pragma once

#include "qml_material/control/panel.hpp"

namespace qml_material
{

class QML_MATERIAL_API Page : public Panel {
    Q_OBJECT
    QML_NAMED_ELEMENT(PageBase)
    Q_PROPERTY(QString title READ title WRITE setTitle RESET resetTitle NOTIFY titleChanged FINAL)
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
    Q_PROPERTY(bool canBack READ canBack WRITE setCanBack NOTIFY canBackChanged FINAL)
    Q_PROPERTY(bool scrolling READ scrolling WRITE setScrolling NOTIFY scrollingChanged FINAL)

public:
    explicit Page(QQuickItem* parent = nullptr);
    ~Page() override;
    QString     title() const;
    void        setTitle(const QString& value);
    void        resetTitle();
    QQuickItem* header() const;
    QQuickItem* footer() const;
    void        setHeader(QQuickItem* item);
    void        setFooter(QQuickItem* item);
    qreal       implicitHeaderWidth() const;
    qreal       implicitHeaderHeight() const;
    qreal       implicitFooterWidth() const;
    qreal       implicitFooterHeight() const;
    bool        canBack() const;
    void        setCanBack(bool value);
    bool        scrolling() const;
    void        setScrolling(bool value);

    Q_SIGNAL void titleChanged();
    Q_SIGNAL void headerChanged();
    Q_SIGNAL void footerChanged();
    Q_SIGNAL void implicitHeaderWidthChanged();
    Q_SIGNAL void implicitHeaderHeightChanged();
    Q_SIGNAL void implicitFooterWidthChanged();
    Q_SIGNAL void implicitFooterHeightChanged();
    Q_SIGNAL void canBackChanged();
    Q_SIGNAL void scrollingChanged();

protected:
    QRectF contentRect() const override;
    void   componentComplete() override;
    void   geometryChange(const QRectF& geometry, const QRectF& oldGeometry) override;

private:
    struct Slot {
        QPointer<QQuickItem>           item;
        QSizeF                         implicitSize { 0, 0 };
        QList<QMetaObject::Connection> connections;
    };
    void                    setSlot(Slot& slot, QQuickItem* item);
    void                    updateSlot(Slot& slot);
    void                    relayout();
    Slot                    m_header;
    Slot                    m_footer;
    QString                 m_title;
    bool                    m_can_back      = false;
    bool                    m_scrolling     = false;
    bool                    m_layout_active = false;
    QMetaObject::Connection m_spacing_connection;
};

} // namespace qml_material
