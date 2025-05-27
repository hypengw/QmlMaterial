#include "qml_material/util/qml_util.hpp"

#include <format>
#include <QQmlEngine>
#include <QJsonDocument>
#include <QJsonObject>

#include "qml_material/util/loggingcategory.hpp"

Q_LOGGING_CATEGORY(qml_material_logcat, "qcm.material")
namespace
{

// 2 time ease
auto easeInOut(double x) -> double { return x < 0.5 ? 2 * x * x : 1 - std::pow(-2 * x + 2, 2) / 2; }

} // namespace

namespace qml_material
{

Util::Util(QObject* parent): QObject(parent) {}
Util::~Util() {}

auto Util::cornerArray(QVariant in) -> CornersGroup {
    CornersGroup out;
    if (in.canConvert<qreal>()) {
        out = CornersGroup(in.value<qreal>());
    } else if (auto list = in.toList(); ! list.empty()) {
        switch (list.size()) {
        case 1: {
            out = CornersGroup(list[0].value<qreal>());
            break;
        }
        case 2: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[0].value<qreal>());
            out.setBottomLeft(list[1].value<qreal>());
            out.setBottomRight(list[1].value<qreal>());
            break;
        }
        case 3: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[1].value<qreal>());
            out.setBottomLeft(list[2].value<qreal>());
            out.setBottomRight(list[1].value<qreal>());
            break;
        }
        default:
        case 4: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[1].value<qreal>());
            out.setBottomLeft(list[2].value<qreal>());
            out.setBottomRight(list[3].value<qreal>());
        }
        }
    }
    return out;
}

auto Util::corner(qreal in) -> CornersGroup { return CornersGroup(in); }

auto Util::corner(qreal a, qreal b) -> CornersGroup {
    CornersGroup out;
    out.setTopLeft(a);
    out.setTopRight(a);
    out.setBottomLeft(b);
    out.setBottomRight(b);
    return out;
}

auto Util::corner(qreal tl, qreal tr, qreal bl, qreal br) -> CornersGroup {
    return CornersGroup(br, tr, bl, tl);
}

void Util::track(QVariant, Track t) {
    switch (t) {
    case TrackCreate:
        m_tracked++;
        std::printf("track create %lu", m_tracked);
        break;
    case TrackDelete:
        m_tracked--;
        std::printf("track delete %lu", m_tracked);
        break;
    }
}

QString Util::type_str(const QJSValue& obj) {
    if (obj.isQObject()) {
        return obj.toQObject()->metaObject()->className();
    }
    if (auto v = obj.toVariant(); v.isValid()) {
        return v.metaType().name();
    }
    if (auto objname = obj.property("objectName").toString(); ! objname.isEmpty()) {
        return objname;
    }
    return obj.toString();
}

void Util::print_parents(const QJSValue& obj) {
    auto cur           = obj;
    auto format_parent = ycore::y_combinator {
        [this](auto format_parent, const QJSValue& cur, i32 level) -> std::string {
            if (! cur.isNull()) {
                return std::format("    {}\n{}",
                                   type_str(cur).toStdString(),
                                   format_parent(cur.property("parent"), level + 1));
            }
            return {};
        }
    };
    qCDebug(qml_material_logcat()) << std::format(
        "{}\n{}", type_str(obj).toStdString(), format_parent(obj.property("parent"), 1));
}

auto Util::lightness(QColor color) -> qreal { return color.lightnessF(); }

auto Util::tokenElevation() -> token::Elevation { return token::Elevation(); }
auto Util::tokenShape() -> token::Shape { return token::Shape(); }
auto Util::tokenState() -> token::State { return token::State(); }

QObject* Util::createItem(const QJSValue& url_or_comp, const QVariantMap& props, QObject* parent) {
    return qcm::createItem(qmlEngine(this), url_or_comp, props, parent);
}
QObject* Util::showPopup(const QJSValue& url_or_comp, const QVariantMap& props, QObject* parent,
                         bool open_and_destry) {
    auto popup = createItem(url_or_comp, props, parent);
    if (open_and_destry) {
        QObject::connect(popup, SIGNAL(closed()), this, SLOT(on_popup_closed()));
        QMetaObject::invokeMethod(popup, "open");
    }
    return popup;
}

void Util::on_popup_closed() {
    auto s = sender();
    if (auto engine = qmlEngine(s)) {
        auto js = engine->toManagedValue(s);
        if (auto p = js.property("destroy"); p.isCallable()) {
            p.call({ 1000 });
        }
    }
}

auto Util::paramsString(const QVariantMap& in_props) -> QString {
    auto props = in_props;
    for (const auto& el : props.keys()) {
        auto val = props.value(el);
        if (auto pp = get_if<QObject*>(&val)) {
            props[el] = QString::number((std::intptr_t)(*pp));
        } else if (auto pp = get_if<QVariantMap>(&val)) {
            props[el] = paramsString(*pp);
        }
    }
    QJsonDocument doc;
    doc.setObject(QJsonObject::fromVariantMap(props));
    return doc.toJson(QJsonDocument::Compact);
}

void Util::setCursor(QQuickItem* item, Qt::CursorShape shape) {
    if (item) {
        item->setCursor(shape);
    }
}

auto Util::clamp(double t, double low, double heigh) const -> double {
    return std::clamp(t, low, heigh);
}
auto Util::teleportCurve(double t, double left, double right) const -> double {
    if (t < left) {
        double x = t / left;
        return 1.0 - easeInOut(x);
    } else if (t < right) {
        return 0.0;
    } else if (t <= 1.0) {
        double x = (t - right) / (1.0 - right);
        return easeInOut(x);
    } else {
        return 1.0;
    }
}

void Util::forceSetImplicitHeight(QQuickItem* item, qreal height) {
    if (item) {
        item->setImplicitHeight(height);
    }
}
void Util::forceSetImplicitWidth(QQuickItem* item, qreal width) {
    if (item) {
        item->setImplicitWidth(width);
    }
}

} // namespace qml_material

namespace qcm
{

auto qml_dyn_count() -> std::atomic<i32>& {
    static std::atomic<i32> n { 0 };
    return n;
}

auto createItem(QQmlEngine* engine, const QJSValue& url_or_comp, const QVariantMap& props,
                QObject* parent) -> QObject* {
    std::unique_ptr<QQmlComponent, void (*)(QQmlComponent*)> comp { nullptr, nullptr };
    if (auto p = qobject_cast<QQmlComponent*>(url_or_comp.toQObject())) {
        comp = decltype(comp)(p, [](QQmlComponent*) {
        });
    } else if (auto p = url_or_comp.toVariant(); ! p.isNull()) {
        QUrl url;
        if (p.canConvert<QUrl>()) {
            url = p.toUrl();
        } else if (p.canConvert<QString>()) {
            url = p.toString();
        }
        comp = decltype(comp)(new QQmlComponent(engine, url, nullptr), [](QQmlComponent* q) {
            delete q;
        });
    } else {
        qCCritical(qml_material_logcat) << "url not valid";
        return nullptr;
    }

    switch (comp->status()) {
    case QQmlComponent::Status::Ready: {
        QObject* obj { nullptr };
        QMetaObject::invokeMethod(comp.get(),
                                  "createObject",
                                  Q_RETURN_ARG(QObject*, obj),
                                  Q_ARG(QObject*, parent),
                                  Q_ARG(const QVariantMap&, props));
        if (obj != nullptr) {
            qml_dyn_count()++;
            auto name = obj->metaObject()->className();
            QObject::connect(obj, &QObject::destroyed, [](QObject*) {
                qml_dyn_count()--;
            });
        } else {
            qCCritical(qml_material_logcat) << comp->errorString();
        }
        return obj;
        break;
    }
    case QQmlComponent::Status::Error: {
        qCCritical(qml_material_logcat) << comp->errorString();
        break;
    }
    default: break;
    }
    return nullptr;
}

} // namespace qcm

#include <qml_material/util/moc_qml_util.cpp>