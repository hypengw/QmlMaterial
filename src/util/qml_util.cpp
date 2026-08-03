#include "qml_material/util/qml_util.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QQmlContext>
#include <QQmlEngine>
#include <format>

#include "qml_material/util/loggingcategory.hpp"
#include "qml_material/util/pool.hpp"

using namespace std::string_view_literals;

Q_LOGGING_CATEGORY(qml_material_logcat, "qcm.material", QtMsgType::QtInfoMsg)
namespace
{

// 2 time ease
auto easeInOut(double x) -> double { return x < 0.5 ? 2 * x * x : 1 - std::pow(-2 * x + 2, 2) / 2; }

} // namespace

namespace qml_material
{

Util::Util(QObject* parent): QObject(parent) {}

Util::~Util() {}

void Util::openUrlExternally(const QString& url) { sysOpenUrl(url); }
void Util::openFolderExternally(const QString& url) { sysOpenFolder(url); }

bool Util::hasIcon(const QJSValue& v) const {
    auto name   = v.property("name");
    auto source = v.property("source");
    if (name.isString() && source.toVariant().isValid()) {
        return ! name.toString().isEmpty() || ! source.toString().isEmpty();
    }
    return false;
}

auto Util::transparent(QColor in, float alpha) noexcept -> QColor {
    in.setAlphaF(alpha);
    return in;
}

auto Util::hoverColor(QColor in) noexcept -> QColor {
    in.setAlphaF(0.08f);
    return in;
}

auto Util::pressColor(QColor in) noexcept -> QColor {
    in.setAlphaF(0.18f);
    return in;
}

void Util::closePopup(QObject* obj) {
    if (! obj) return;
    do {
        auto meta = obj->metaObject();
        do {
            auto cn = meta->className();
            if (cn == "QQuickPopup"sv) {
                QMetaObject::invokeMethod(obj, "close");
                return;
            }
        } while (meta = meta->superClass(), meta);
    } while (obj = obj->parent(), obj);
}

auto Util::devicePixelRatio(QQuickItem* in) -> qreal {
    return in ? in->window() ? in->window()->devicePixelRatio() : 1.0 : 1.0;
}

auto Util::listCorners(qint32 idx, qint32 count, qint32 radius) noexcept -> CornersGroup {
    return corners(idx == 0 ? radius : 0, idx + 1 == count ? radius : 0);
}

auto Util::tableWithHeaderCorners(qint32 row, qint32 column, qint32 rows, qint32 columns,
                                  qint32 radius) noexcept -> CornersGroup {
    CornersGroup out;
    bool         row_end      = row + 1 == rows;
    bool         column_start = column == 0;
    bool         column_end   = column + 1 == columns;
    out.setBottomLeft(row_end && column_start ? radius : 0);
    out.setBottomRight(row_end && column_end ? radius : 0);
    return out;
}

auto Util::tableCorners(qint32 row, qint32 column, qint32 rows, qint32 columns,
                        qint32 radius) noexcept -> CornersGroup {
    CornersGroup out;
    bool         row_start    = row == 0;
    bool         row_end      = row + 1 == rows;
    bool         column_start = column == 0;
    bool         column_end   = column + 1 == columns;
    out.setTopLeft(row_start && column_start ? radius : 0);
    out.setTopRight(row_start && column_end ? radius : 0);
    out.setBottomLeft(row_end && column_start ? radius : 0);
    out.setBottomRight(row_end && column_end ? radius : 0);
    return out;
}

auto Util::cornerArray(QVariant in) noexcept -> CornersGroup {
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

auto Util::corners(qreal in) noexcept -> CornersGroup { return CornersGroup(in); }

auto Util::corners(qreal a, qreal b) noexcept -> CornersGroup {
    CornersGroup out;
    out.setTopLeft(a);
    out.setTopRight(a);
    out.setBottomLeft(b);
    out.setBottomRight(b);
    return out;
}

auto Util::corners(qreal tl, qreal tr, qreal bl, qreal br) noexcept -> CornersGroup {
    return CornersGroup(br, tr, bl, tl);
}

void Util::track(QVariant, Track t) {
    switch (t) {
    case TrackCreate:
        m_tracked++;
        std::printf("track create %zu", m_tracked);
        break;
    case TrackDelete:
        m_tracked--;
        std::printf("track delete %zu", m_tracked);
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

auto Util::lightness(QColor color) noexcept -> qreal { return color.lightnessF(); }

auto Util::tokenElevation() noexcept -> token::Elevation { return token::Elevation(); }
auto Util::tokenShape() noexcept -> token::Shape { return token::Shape(); }
auto Util::tokenState() noexcept -> token::State { return token::State(); }

QObject* Util::createItem(const QJSValue& url_or_comp, const QVariantMap& props, QObject* parent) {
    auto* context = parent ? qmlContext(parent) : qmlContext(this);
    return qcm::createItem(qmlEngine(this), url_or_comp, props, parent, context);
}
QObject* Util::showPopup(const QJSValue& url_or_comp, const QVariantMap& props, QObject* parent,
                         bool open_and_destry) {
    auto popup = createItem(url_or_comp, props, parent);
    if (! popup) {
        qCWarning(qml_material_logcat()) << "cannot show popup: creation failed";
        return nullptr;
    }
    if (open_and_destry) {
        QObject::connect(popup, SIGNAL(closed()), this, SLOT(onPopupFinished()));
        if (popup->metaObject()->indexOfSignal("openRejected(QString)") >= 0) {
            QObject::connect(popup, SIGNAL(openRejected(QString)), this, SLOT(onPopupFinished()));
        }
        const auto* meta   = popup->metaObject();
        const char* method = nullptr;
        if (meta->indexOfMethod("requestOpen()") >= 0)
            method = "requestOpen";
        else if (meta->indexOfMethod("open()") >= 0)
            method = "open";

        if (! method || ! QMetaObject::invokeMethod(popup, method))
            qCWarning(qml_material_logcat()) << "cannot show popup: no callable open method";
    }
    return popup;
}

void Util::onPopupFinished() {
    auto s = sender();
    if (! s || s->property("_qcm_destroy_scheduled").toBool()) return;
    s->setProperty("_qcm_destroy_scheduled", true);
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
        } else {
            props[el] = val.toString();
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

double Util::clamp(double t, double low, double heigh) { return std::clamp(t, low, heigh); }

double Util::bezierY(double t, double p1x, double p1y, double p2x, double p2y) noexcept {
    // Newton's method to find s such that x(s) = t, then return y(s).
    double s = std::clamp(t, 0.0, 1.0);
    for (int i = 0; i < 6; ++i) {
        const double omt = 1.0 - s;
        const double x   = 3.0 * omt * omt * s * p1x + 3.0 * omt * s * s * p2x + s * s * s;
        const double dx =
            3.0 * omt * omt * p1x + 6.0 * omt * s * (p2x - p1x) + 3.0 * s * s * (1.0 - p2x);
        if (std::abs(dx) < 1e-6) break;
        s -= (x - t) / dx;
        s = std::clamp(s, 0.0, 1.0);
    }
    const double omt = 1.0 - s;
    return 3.0 * omt * omt * s * p1y + 3.0 * omt * s * s * p2y + s * s * s;
}

double Util::segFrac(double playtime, double delay, double duration, double p1x, double p1y,
                     double p2x, double p2y) noexcept {
    if (duration <= 0.0) return 0.0;
    const double f = std::clamp((playtime - delay) / duration, 0.0, 1.0);
    return std::clamp(bezierY(f, p1x, p1y, p2x, p2y), 0.0, 1.0);
}
double Util::teleportCurve(double t, double left, double right) {
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

auto Util::stateText(bool enabled, bool pressed, bool hovered, bool focused) noexcept -> QString {
    if (! enabled) return QStringLiteral("disabled");
    if (pressed) return QStringLiteral("pressed");
    if (hovered) return QStringLiteral("hovered");
    if (focused) return QStringLiteral("focus");
    return {};
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

void Util::cellHoveredOn(QQuickItem* item, bool hovered, qint32 row, qint32 column) {
    if (item == nullptr) return;
    if (! hovered) return;

    auto meta       = item->metaObject();
    auto signal_sig = QMetaObject::normalizedSignature("cellHovered(qint32, qint32)");
    auto idx        = meta->indexOfSignal(signal_sig);
    if (idx != -1) {
        auto signal = meta->method(idx);
        signal.invoke(item, row, column);
    }
}

QObject* Util::getParent(QObject* obj) { return obj ? obj->parent() : nullptr; }
bool     Util::disconnectAll(QObject* obj, const QString& name) {
    if (! obj) {
        qCWarning(qml_material_logcat()) << "disconnectAll: obj is null";
        return false;
    }
    auto signal_idx = obj->metaObject()->indexOfSignal(
        QMetaObject::normalizedSignature(name.toUtf8().constData()));
    if (signal_idx == -1) {
        qCWarning(qml_material_logcat()) << "disconnectAll: signal not found" << name;
        return false;
    }
    auto signal = obj->metaObject()->method(signal_idx);
    return QObject::disconnect(obj, signal, nullptr, QMetaMethod {});
}

quint32 Util::poolObjectCount() noexcept { return static_cast<quint32>(pool_object_count()); }
qint32  Util::i32Max() noexcept { return std::numeric_limits<qint32>::max(); }

} // namespace qml_material

auto qml_material::resolveComponentSource(const QVariant& source, QQmlEngine* engine,
                                          QQmlContext* context, QQmlComponent::CompilationMode mode)
    -> ComponentSource {
    ComponentSource result;
    if (auto* component = source.value<QQmlComponent*>()) {
        result.component = component;
        return result;
    }
    if (! engine) {
        result.errorString = QStringLiteral("cannot resolve a component without a QML engine");
        return result;
    }

    auto loadUrl = [&](QUrl url) {
        if (url.isRelative() && QFileInfo(url.toString()).isAbsolute()) {
            url = QUrl::fromLocalFile(url.toString());
        } else if (url.isRelative() && context) {
            url = context->resolvedUrl(url);
        }
        if (! url.isValid() || url.isEmpty()) {
            result.errorString = QStringLiteral("component URL is empty or invalid");
            return;
        }
        result.ownedComponent = std::make_unique<QQmlComponent>(engine);
        result.component      = result.ownedComponent.get();
        result.component->loadUrl(url, mode);
    };

    if (source.typeId() == QMetaType::QUrl) {
        loadUrl(source.toUrl());
        return result;
    }
    if (source.typeId() != QMetaType::QString) {
        const auto* typeName = source.metaType().name();
        result.errorString   = QStringLiteral("unsupported component source type: %1")
                                   .arg(QString::fromUtf8(typeName ? typeName : "unknown"));
        return result;
    }

    const auto value = source.toString().trimmed();
    const QUrl url(value);
    const bool isUrl = value.endsWith(u".qml", Qt::CaseInsensitive) || value.startsWith(u"./") ||
                       value.startsWith(u"../") || value.startsWith(u'/') ||
                       value.startsWith(u":/") || ! url.scheme().isEmpty();
    if (isUrl) {
        loadUrl(value.startsWith(u":/") ? QUrl(QStringLiteral("qrc") + value) : url);
        return result;
    }

    const auto parts = value.split(u'/');
    if (parts.size() != 2 || parts[0].isEmpty() || parts[1].isEmpty()) {
        result.errorString =
            QStringLiteral("component source must be a URL or module/type: %1").arg(value);
        return result;
    }
    result.ownedComponent = std::make_unique<QQmlComponent>(engine);
    result.component      = result.ownedComponent.get();
    result.component->loadFromModule(parts[0], parts[1], mode);
    return result;
}

namespace qcm
{

auto qml_dyn_count() -> std::atomic<i32>& {
    static std::atomic<i32> n { 0 };
    return n;
}

auto createItem(QQmlEngine* engine, const QJSValue& url_or_comp, const QVariantMap& props,
                QObject* parent, QQmlContext* context) -> QObject* {
    QVariant source = url_or_comp.toVariant();
    if (auto* component = qobject_cast<QQmlComponent*>(url_or_comp.toQObject())) {
        source = QVariant::fromValue(component);
    }
    if (! source.isValid() || source.isNull()) {
        qCWarning(qml_material_logcat()) << "cannot create component from an empty source";
        return nullptr;
    }

    auto resolved = qml_material::resolveComponentSource(
        source, engine, context, QQmlComponent::PreferSynchronous);
    if (! resolved) {
        qCWarning(qml_material_logcat()) << resolved.errorString;
        return nullptr;
    }
    auto* comp = resolved.component.data();

    switch (comp->status()) {
    case QQmlComponent::Status::Ready: {
        QObject* obj { nullptr };
        QMetaObject::invokeMethod(comp,
                                  "createObject",
                                  Q_RETURN_ARG(QObject*, obj),
                                  Q_ARG(QObject*, parent),
                                  Q_ARG(const QVariantMap&, props));
        if (obj != nullptr) {
            qml_dyn_count()++;
            // auto name = obj->metaObject()->className();
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

#include "qml_material/util/moc_qml_util.cpp"
