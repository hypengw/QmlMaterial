#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QColor>
#include <QQuickItem>
#include <QPointer>
#include <QQuickWindow>

#include "qml_material/corner.h"
#include "qml_material/enum.h"
#include "qml_material/token.h"
#include "qml_material/core.h"

namespace qml_material
{

class Util : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
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

    Q_INVOKABLE bool hasIcon(const QJSValue& v) {
        auto name   = v.property("name");
        auto source = v.property("source");
        if (name.isString() && source.toVariant().isValid()) {
            return ! name.toString().isEmpty() || ! source.toString().isEmpty();
        }
        return false;
    }

    Q_INVOKABLE QColor transparent(QColor in, float alpha) {
        in.setAlphaF(alpha);
        return in;
    }

    Q_INVOKABLE void closePopup(QObject* obj) {
        do {
            auto meta = obj->metaObject();
            do {
                if (meta->className() == std::string("QQuickPopup")) {
                    QMetaObject::invokeMethod(obj, "close");
                    return;
                }
            } while (meta = meta->superClass(), meta);
        } while (obj = obj->parent(), obj);
    }

    Q_INVOKABLE QColor hoverColor(QColor in) {
        in.setAlphaF(0.08);
        return in;
    }

    Q_INVOKABLE QColor pressColor(QColor in) {
        in.setAlphaF(0.18);
        return in;
    }

    Q_INVOKABLE qreal devicePixelRatio(QQuickItem* in) {
        return in ? in->window() ? in->window()->devicePixelRatio() : 1.0 : 1.0;
    }

    // tl tr bl br
    Q_INVOKABLE CornersGroup cornerArray(QVariant in);
    Q_INVOKABLE CornersGroup corner(qreal in);
    Q_INVOKABLE CornersGroup corner(qreal, qreal);
    Q_INVOKABLE CornersGroup corner(qreal tl, qreal tr, qreal bl, qreal br);

    QString          type_str(const QJSValue&);
    Q_INVOKABLE void print_parents(const QJSValue&);

    Q_INVOKABLE qreal lightness(QColor color);

    Q_INVOKABLE token::Elevation token_elevation();
    Q_INVOKABLE token::Shape token_shape();
    Q_INVOKABLE token::State token_state();
    Q_INVOKABLE QObject*     create_item(const QJSValue& url_or_comp, const QVariantMap& props,
                                         QObject* parent = nullptr);

    Q_INVOKABLE QObject* show_popup(const QJSValue& url_or_comp, const QVariantMap& props,
                                    QObject* parent = nullptr, bool open_and_destry = true);

    Q_INVOKABLE QString params_string(const QVariantMap& props);

private:
    Q_SLOT void on_popup_closed();

private:
    usize m_tracked { 0 };
};
} // namespace qml_material

namespace qcm
{
auto qml_dyn_count() -> std::atomic<i32>&;
auto create_item(QQmlEngine* engine, const QJSValue& url_or_comp, const QVariantMap& props,
                 QObject* parent) -> QObject*;
} // namespace qcm
