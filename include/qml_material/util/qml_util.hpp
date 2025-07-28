#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QColor>
#include <QQuickItem>
#include <QPointer>
#include <QQuickWindow>

#include "qml_material/util/corner.hpp"
#include "qml_material/enum.hpp"
#include "qml_material/token/token.hpp"
#include "qml_material/core.hpp"

namespace qml_material
{

class Util : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(UtilCpp)
public:
    Util(QObject* parent = nullptr);
    ~Util();

    enum Track
    {
        TrackCreate = 0,
        TrackDelete
    };
    Q_ENUMS(Track)

    Q_INVOKABLE void track(QVariant, Track);

    Q_INVOKABLE bool hasIcon(const QJSValue& v) const;

    Q_INVOKABLE void closePopup(QObject* obj) const;

    Q_INVOKABLE QColor transparent(QColor in, float alpha) const noexcept;
    Q_INVOKABLE QColor hoverColor(QColor in) const noexcept;
    Q_INVOKABLE QColor pressColor(QColor in) const noexcept;

    Q_INVOKABLE qreal devicePixelRatio(QQuickItem* in) const;

    Q_INVOKABLE CornersGroup listCorners(qint32 idx, qint32 count, qint32 radius) const noexcept;
    Q_INVOKABLE CornersGroup tableCorners(qint32 row, qint32 column, qint32 rows, qint32 columns,
                                          qint32 radius) const noexcept;
    Q_INVOKABLE CornersGroup tableWithHeaderCorners(qint32 row, qint32 column, qint32 rows,
                                                    qint32 columns, qint32 radius) const noexcept;

    // tl tr bl br
    Q_INVOKABLE CornersGroup cornerArray(QVariant in) const noexcept;
    Q_INVOKABLE CornersGroup corners(qreal in) const noexcept;
    Q_INVOKABLE CornersGroup corners(qreal, qreal) const noexcept;
    Q_INVOKABLE CornersGroup corners(qreal tl, qreal tr, qreal bl, qreal br) const noexcept;

    QString          type_str(const QJSValue&);
    Q_INVOKABLE void print_parents(const QJSValue&);

    Q_INVOKABLE qreal lightness(QColor color);

    Q_INVOKABLE token::Elevation tokenElevation();
    Q_INVOKABLE token::Shape tokenShape();
    Q_INVOKABLE token::State tokenState();
    Q_INVOKABLE QObject*     createItem(const QJSValue& url_or_comp, const QVariantMap& props,
                                        QObject* parent = nullptr);

    Q_INVOKABLE QObject* showPopup(const QJSValue& url_or_comp, const QVariantMap& props,
                                   QObject* parent = nullptr, bool open_and_destry = true);

    Q_INVOKABLE QString paramsString(const QVariantMap& props);
    Q_INVOKABLE void    setCursor(QQuickItem* item, Qt::CursorShape shape);

    Q_INVOKABLE double clamp(double t, double low, double high) const;
    Q_INVOKABLE double teleportCurve(double t, double left, double right) const;

    Q_INVOKABLE void forceSetImplicitHeight(QQuickItem* item, qreal height);
    Q_INVOKABLE void forceSetImplicitWidth(QQuickItem* item, qreal width);

    Q_INVOKABLE void cellHoveredOn(QQuickItem* item, bool hovered, qint32 row, qint32 column) const;
    Q_INVOKABLE QObject* getParent(QObject* obj) const;
    Q_INVOKABLE bool     disconnectAll(QObject* obj, const QString&) const;

private:
    Q_SLOT void on_popup_closed();

private:
    usize m_tracked { 0 };
};

auto tryCreateComponent(const QVariant& val, QQmlComponent::CompilationMode useAsync,
                        const std::function<QQmlComponent*()>& createComponent) -> QQmlComponent*;
} // namespace qml_material

namespace qcm
{
auto qml_dyn_count() -> std::atomic<i32>&;
auto createItem(QQmlEngine* engine, const QJSValue& url_or_comp, const QVariantMap& props,
                QObject* parent) -> QObject*;

} // namespace qcm
