#include <QtCore/qtsymbolmacros.h>
#include <QtQml/qqmlextensionplugin.h>

#include <QtCore/QVariantAnimation>
#include "qml_material/util/corner.hpp"

using namespace qml_material;

QT_DECLARE_EXTERN_SYMBOL_VOID(qml_register_types_Qcm_Material)

class Q_DECL_EXPORT Qcm_MaterialPlugin : public QQmlEngineExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

public:
    Qcm_MaterialPlugin(QObject* parent = nullptr): QQmlEngineExtensionPlugin(parent) {
        QT_KEEP_SYMBOL(qml_register_types_Qcm_Material)
        qRegisterAnimationInterpolator<CornersGroup>(&CornersGroup::interpolated);
    }
};

#include "plugin.moc"
