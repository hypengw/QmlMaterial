#include "qml_material/util.h"

#include <format>
#include <QQmlEngine>


#include "qml_material/loggingcategory.h"

Q_LOGGING_CATEGORY(qml_material_logcat, "qcm.material")
namespace
{

} // namespace

namespace qml_material
{

Util::Util(QObject* parent): QObject(parent) {}
Util::~Util() {}

CornersGroup Util::corner(QVariant in) {
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

CornersGroup Util::corner(qreal br, qreal tr, qreal bl, qreal tl) {
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
    if (obj.isVariant()) {
        return obj.toVariant().metaType().name();
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
                return std::format(
                    "    {}\n{}", type_str(cur), format_parent(cur.property("parent"), level + 1));
            }
            return {};
        }
    };
    // DEBUG_LOG("{}\n{}", type_str(obj), format_parent(obj.property("parent"), 1));
}

auto Util::lightness(QColor color) -> qreal { return color.lightnessF(); }

auto Util::token_elevation() -> token::Elevation { return token::Elevation(); }
auto Util::token_shape() -> token::Shape { return token::Shape(); }
auto Util::token_state() -> token::State { return token::State(); }

QObject* Util::create_item(const QJSValue& url_or_comp, const QVariantMap& props, QObject* parent) {
    return qcm::create_item(qmlEngine(this), url_or_comp, props, parent);
}
QObject* Util::show_popup(const QJSValue& url_or_comp, const QVariantMap& props, QObject* parent,
                          bool open_and_destry) {
    auto popup = create_item(url_or_comp, props, parent);
    if (open_and_destry) {
        QObject::connect(popup, SIGNAL(closed()), this, SLOT(on_popup_closed()));
        QMetaObject::invokeMethod(popup, "open");
    }
    return popup;
}

void Util::on_popup_closed() {
    auto s = sender();
    if (s != nullptr) {
        auto js = qmlEngine(s)->toManagedValue(s);
        if (auto p = js.property("destroy"); p.isCallable()) {
            p.call({ 1000 });
        }
    }
}

} // namespace qml_material

namespace qcm
{

auto qml_dyn_count() -> std::atomic<i32>& {
    static std::atomic<i32> n { 0 };
    return n;
}

auto create_item(QQmlEngine* engine, const QJSValue& url_or_comp, const QVariantMap& props,
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