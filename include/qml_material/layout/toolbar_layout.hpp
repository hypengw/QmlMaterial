/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 * Adapted from KDE Kirigami; integrated into qml_material.
 */

#pragma once

#include <QQuickItem>
#include <QQmlEngine>
#include <memory>
#include "qml_material/control/action.hpp"
#include "qml_material/export.hpp"

namespace qml_material
{

class ToolBarLayout;
class ToolBarLayoutPrivate;

/**
 * Attached property for ToolBarLayout delegates.
 */
class QML_MATERIAL_API ToolBarLayoutAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(Action* action READ action CONSTANT FINAL)
public:
    ToolBarLayoutAttached(QObject* parent = nullptr);

    Action* action() const;
    void    setAction(Action* action);

private:
    Action* m_action = nullptr;
};

/**
 * An item that creates delegates for actions and lays them out in a row.
 */
class QML_MATERIAL_API ToolBarLayout : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(ToolBarLayoutAttached)
    Q_PROPERTY(QQmlListProperty<Action> actions READ actionsProperty NOTIFY actionsChanged FINAL)
    Q_PROPERTY(QList<Action*> hiddenActions READ hiddenActions NOTIFY hiddenActionsChanged FINAL)
    Q_PROPERTY(QQmlComponent* fullDelegate READ fullDelegate WRITE setFullDelegate NOTIFY
                   fullDelegateChanged FINAL)
    Q_PROPERTY(QQmlComponent* iconDelegate READ iconDelegate WRITE setIconDelegate NOTIFY
                   iconDelegateChanged FINAL)
    Q_PROPERTY(QQmlComponent* separatorDelegate READ separatorDelegate WRITE setSeparatorDelegate
                   NOTIFY separatorDelegateChanged FINAL)
    Q_PROPERTY(QQmlComponent* moreButton READ moreButton WRITE setMoreButton NOTIFY
                   moreButtonChanged FINAL)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(
        Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)
    Q_PROPERTY(qreal visibleWidth READ visibleWidth NOTIFY visibleWidthChanged FINAL)
    Q_PROPERTY(qreal minimumWidth READ minimumWidth NOTIFY minimumWidthChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection
                   NOTIFY layoutDirectionChanged FINAL)
    Q_PROPERTY(
        HeightMode heightMode READ heightMode WRITE setHeightMode NOTIFY heightModeChanged FINAL)
    Q_PROPERTY(qint32 maxShowActionNum READ maxShowActionNum WRITE setMaxShowActionNum NOTIFY
                   maxShowActionNumChanged FINAL)

public:
    using ActionsProperty = QQmlListProperty<Action>;

    enum HeightMode
    {
        AlwaysCenter,
        AlwaysFill,
        ConstrainIfLarger,
    };
    Q_ENUM(HeightMode)

    // Folded from former kirigami DisplayHint singleton. QML never referenced
    // MD.DisplayHint directly; only ToolBarLayout consumes these flags via the
    // `displayHint` property on Action objects.
    enum DisplayHint : uint
    {
        NoPreference       = 0,
        IconOnly           = 1,
        KeepVisible        = 2,
        AlwaysHide         = 4,
        HideChildIndicator = 8,
    };
    Q_DECLARE_FLAGS(DisplayHints, DisplayHint)
    Q_FLAG(DisplayHints)

    static bool isDisplayHintSet(DisplayHints values, DisplayHint hint);

    ToolBarLayout(QQuickItem* parent = nullptr);
    ~ToolBarLayout() override;

    ActionsProperty actionsProperty() const;
    void            addAction(Action* action);
    void            removeAction(Action* action);
    void            clearActions();
    Q_SIGNAL void   actionsChanged();

    QList<Action*> hiddenActions() const;
    Q_SIGNAL void  hiddenActionsChanged();

    QQmlComponent* fullDelegate() const;
    void           setFullDelegate(QQmlComponent* newFullDelegate);
    Q_SIGNAL void  fullDelegateChanged();

    QQmlComponent* iconDelegate() const;
    void           setIconDelegate(QQmlComponent* newIconDelegate);
    Q_SIGNAL void  iconDelegateChanged();

    QQmlComponent* separatorDelegate() const;
    void           setSeparatorDelegate(QQmlComponent* newSeparatorDelegate);
    Q_SIGNAL void  separatorDelegateChanged();

    QQmlComponent* moreButton() const;
    void           setMoreButton(QQmlComponent* newMoreButton);
    Q_SIGNAL void  moreButtonChanged();

    qreal         spacing() const;
    void          setSpacing(qreal newSpacing);
    Q_SIGNAL void spacingChanged();

    Qt::Alignment alignment() const;
    void          setAlignment(Qt::Alignment newAlignment);
    Q_SIGNAL void alignmentChanged();

    qreal         visibleWidth() const;
    Q_SIGNAL void visibleWidthChanged();

    qreal         minimumWidth() const;
    Q_SIGNAL void minimumWidthChanged();

    Qt::LayoutDirection layoutDirection() const;
    void                setLayoutDirection(Qt::LayoutDirection& newLayoutDirection);
    Q_SIGNAL void       layoutDirectionChanged();

    HeightMode    heightMode() const;
    void          setHeightMode(HeightMode newHeightMode);
    Q_SIGNAL void heightModeChanged();

    qint32        maxShowActionNum() const;
    void          setMaxShowActionNum(qint32 maxShowActionNum);
    Q_SIGNAL void maxShowActionNumChanged();

    Q_SLOT void relayout();

    static ToolBarLayoutAttached* qmlAttachedProperties(QObject* object) {
        return new ToolBarLayoutAttached(object);
    }

protected:
    void componentComplete() override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData& data) override;
    void updatePolish() override;

private:
    friend class ToolBarLayoutPrivate;
    const std::unique_ptr<ToolBarLayoutPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ToolBarLayout::DisplayHints)

} // namespace qml_material
