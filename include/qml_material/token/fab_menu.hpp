#pragma once

#include <QtQml/qqml.h>

namespace qml_material::token
{
struct FABMenu {
    Q_GADGET
    QML_VALUE_TYPE(fab_menu_token)
    Q_PROPERTY(qreal itemHeight MEMBER itemHeight CONSTANT FINAL)
    Q_PROPERTY(qreal itemPadding MEMBER itemPadding CONSTANT FINAL)
    Q_PROPERTY(qreal iconSize MEMBER iconSize CONSTANT FINAL)
    Q_PROPERTY(qreal iconSpacing MEMBER iconSpacing CONSTANT FINAL)
    Q_PROPERTY(qreal itemSpacing MEMBER itemSpacing CONSTANT FINAL)
    Q_PROPERTY(qreal buttonSpacing MEMBER buttonSpacing CONSTANT FINAL)
    Q_PROPERTY(qreal edgePadding MEMBER edgePadding CONSTANT FINAL)
    Q_PROPERTY(qreal buttonSize MEMBER buttonSize CONSTANT FINAL)
    Q_PROPERTY(qreal closedCorner MEMBER closedCorner CONSTANT FINAL)
    Q_PROPERTY(qreal closeIconSize MEMBER closeIconSize CONSTANT FINAL)
public:
    qreal itemHeight    = 56;
    qreal itemPadding   = 24;
    qreal iconSize      = 24;
    qreal iconSpacing   = 8;
    qreal itemSpacing   = 4;
    qreal buttonSpacing = 8;
    qreal edgePadding   = 16;
    qreal buttonSize    = 56;
    qreal closedCorner  = 16;
    qreal closeIconSize = 20;
};
} // namespace qml_material::token
