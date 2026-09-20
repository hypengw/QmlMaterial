#pragma once

#include <QQmlListProperty>

#include "qml_material/control/control.hpp"

namespace qml_material
{

/** @ingroup control
 * Content container without a predefined visual background.
 */
class QML_MATERIAL_API Panel : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(PanelBase)
    Q_CLASSINFO("DefaultProperty", "contentData")
    Q_PROPERTY(QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickItem> contentChildren READ contentChildren NOTIFY
                   contentChildrenChanged FINAL)
    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth RESET resetContentWidth
                   NOTIFY contentWidthChanged FINAL)
    Q_PROPERTY(qreal contentHeight READ contentHeight WRITE setContentHeight RESET
                   resetContentHeight NOTIFY contentHeightChanged FINAL)

public:
    explicit Panel(QQuickItem* parent = nullptr);
    ~Panel() override;

    QQmlListProperty<QObject>    contentData();
    QQmlListProperty<QQuickItem> contentChildren();
    qreal                        contentWidth() const;
    qreal                        contentHeight() const;
    void                         setContentWidth(qreal value);
    void                         setContentHeight(qreal value);
    void                         resetContentWidth();
    void                         resetContentHeight();

    Q_SIGNAL void contentWidthChanged();
    Q_SIGNAL void contentHeightChanged();
    Q_SIGNAL void contentChildrenChanged();

protected:
    QSizeF measureImplicitContent() const override;
    void   componentComplete() override;
    void   mousePressEvent(QMouseEvent* event) override;
    void   mouseReleaseEvent(QMouseEvent* event) override;
    void   mouseMoveEvent(QMouseEvent* event) override;
    void   touchEvent(QTouchEvent* event) override;

private:
    QSizeF calculateContentSize() const;
    void   observeContentItem();
    void   observeChildren();
    void   updateContentSize();

    QSizeF                         m_content_size { 0, 0 };
    std::optional<qreal>           m_content_width;
    std::optional<qreal>           m_content_height;
    QMetaObject::Connection        m_slot_connection;
    QList<QMetaObject::Connection> m_host_connections;
    QList<QMetaObject::Connection> m_child_connections;
};

} // namespace qml_material
