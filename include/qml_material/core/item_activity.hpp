#pragma once

#include <QObject>
#include <memory>
#include "qml_material/export.hpp"

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

namespace qml_material
{
/** Event-driven visibility within a window and all ancestor clips. */
class QML_MATERIAL_API ItemActivity : public QObject {
    Q_OBJECT
public:
    explicit ItemActivity(QQuickItem* target);
    ~ItemActivity() override;
    bool          active() const;
    void          refresh();
    Q_SIGNAL void changed();

private:
    struct Private;
    std::unique_ptr<Private> d;
    void                     observe();
};
} // namespace qml_material
