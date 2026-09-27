#pragma once

#include "qml_material/control/container.hpp"
#include "qml_material/core/enum.hpp"
#include "qml_material/token/button_group.hpp"
#include <QVariantAnimation>

namespace qml_material
{
class ButtonGroupContainerAttached;
class QML_MATERIAL_API ButtonGroupContainer : public MaterialContainer {
    Q_OBJECT
    QML_NAMED_ELEMENT(ButtonGroupContainer)
    QML_ATTACHED(ButtonGroupContainerAttached)
    Q_PROPERTY(Variant variant READ variant WRITE setVariant NOTIFY variantChanged FINAL)
    Q_PROPERTY(bool animateWidth READ animateWidth WRITE setAnimateWidth RESET resetAnimateWidth
                   NOTIFY animateWidthChanged FINAL)
    Q_PROPERTY(qreal expandedRatio READ expandedRatio WRITE setExpandedRatio NOTIFY
                   expandedRatioChanged FINAL)
    Q_PROPERTY(int size READ buttonSize WRITE setButtonSize NOTIFY sizeChanged FINAL)
public:
    enum Variant
    {
        Standard,
        Connected
    };
    Q_ENUM(Variant)
    explicit ButtonGroupContainer(QQuickItem* parent = nullptr);
    ~ButtonGroupContainer() override;
    static ButtonGroupContainerAttached* qmlAttachedProperties(QObject*);
    Variant                              variant() const { return m_variant; }
    void                                 setVariant(Variant);
    bool          animateWidth() const { return m_animateWidth.value_or(m_variant == Standard); }
    void          setAnimateWidth(bool);
    void          resetAnimateWidth();
    qreal         expandedRatio() const { return m_expandedRatio; }
    void          setExpandedRatio(qreal);
    int           buttonSize() const { return m_size; }
    void          setButtonSize(int);
    Q_SIGNAL void variantChanged();
    Q_SIGNAL void animateWidthChanged();
    Q_SIGNAL void expandedRatioChanged();
    Q_SIGNAL void sizeChanged();

protected:
    bool  isContent(QQuickItem*) const override;
    void  itemAdded(QQuickItem*) override;
    void  itemRemoved(QQuickItem*) override;
    void  updatePolish() override;
    qreal defaultSpacing() const override;

private:
    Variant             m_variant = Standard;
    std::optional<bool> m_animateWidth;
    qreal               m_expandedRatio = token::ButtonGroup::expandedRatio;
    int                 m_size          = int(Enum::ButtonSize::S);
};

class QML_MATERIAL_API ButtonGroupContainerAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal preferredWidth READ preferredWidth WRITE setPreferredWidth RESET
                   resetPreferredWidth NOTIFY layoutChanged FINAL)
    Q_PROPERTY(qreal minimumWidth READ minimumWidth WRITE setMinimumWidth RESET resetMinimumWidth
                   NOTIFY layoutChanged FINAL)
    Q_PROPERTY(qreal defaultMinimumWidth READ defaultMinimumWidth WRITE setDefaultMinimumWidth
                   NOTIFY layoutChanged FINAL)
    Q_PROPERTY(qreal weight READ weight WRITE setWeight NOTIFY layoutChanged FINAL)
    Q_PROPERTY(qreal compressionLimit READ compressionLimit WRITE setCompressionLimit RESET
                   resetCompressionLimit NOTIFY layoutChanged FINAL)
    Q_PROPERTY(qreal defaultCompressionLimit READ defaultCompressionLimit WRITE
                   setDefaultCompressionLimit NOTIFY layoutChanged FINAL)
    Q_PROPERTY(int position READ position NOTIFY contextChanged FINAL)
    Q_PROPERTY(bool connected READ connected NOTIFY contextChanged FINAL)
    Q_PROPERTY(bool grouped READ grouped NOTIFY contextChanged FINAL)
    Q_PROPERTY(int size READ buttonSize NOTIFY contextChanged FINAL)
public:
    explicit ButtonGroupContainerAttached(QObject*);
    ~ButtonGroupContainerAttached() override;
    qreal preferredWidth() const { return m_preferredWidth.value_or(-1); }
    void  setPreferredWidth(qreal);
    void  resetPreferredWidth();
    qreal minimumWidth() const { return m_minimumWidth.value_or(m_defaultMinimumWidth); }
    void  setMinimumWidth(qreal);
    void  resetMinimumWidth();
    qreal defaultMinimumWidth() const { return m_defaultMinimumWidth; }
    void  setDefaultMinimumWidth(qreal);
    qreal weight() const { return m_weight; }
    void  setWeight(qreal);
    qreal compressionLimit() const {
        return m_compressionLimit.value_or(m_defaultCompressionLimit);
    }
    void          setCompressionLimit(qreal);
    void          resetCompressionLimit();
    qreal         defaultCompressionLimit() const { return m_defaultCompressionLimit; }
    void          setDefaultCompressionLimit(qreal);
    int           position() const { return m_position; }
    bool          connected() const;
    bool          grouped() const { return owner() != nullptr; }
    int           buttonSize() const;
    qreal         pressProgress() const { return m_progress; }
    Q_SIGNAL void layoutChanged();
    Q_SIGNAL void contextChanged();

private:
    friend class ButtonGroupContainer;
    void                  setPosition(int);
    void                  membershipChanged();
    void                  updatePress();
    void                  animateTo(qreal);
    void                  stopAnimation();
    ButtonGroupContainer* owner() const;
    void                  restoreWidth();
    bool                  m_managedWidth       = false;
    bool                  m_originalWidthValid = false;
    qreal                 m_originalWidth      = 0;
    std::optional<qreal>  m_preferredWidth, m_compressionLimit, m_minimumWidth;
    qreal                 m_defaultMinimumWidth = 0, m_weight = 0, m_defaultCompressionLimit = 0;
    qreal                 m_progress = 0;
    int                   m_position = int(Enum::ItemPosition::PosSingle);
    QVariantAnimation     m_animation;
    bool                  m_releasing = false;
    QList<QMetaObject::Connection> m_ownerConnections;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::ButtonGroupContainer, QML_HAS_ATTACHED_PROPERTIES)
