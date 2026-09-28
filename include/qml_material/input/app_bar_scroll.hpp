#pragma once

#include "qml_material/input/nested_scroll_connection.hpp"
#include <QQmlParserStatus>
#include <optional>

namespace qml_material
{
class QML_MATERIAL_API AppBarScroll : public NestedScrollConnection, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
    Q_PROPERTY(qreal collapseDistance READ collapseDistance WRITE setCollapseDistance NOTIFY
                   collapseDistanceChanged FINAL)
    Q_PROPERTY(
        qreal heightOffset READ heightOffset WRITE setHeightOffset NOTIFY heightOffsetChanged FINAL)
    Q_PROPERTY(qreal collapsedFraction READ collapsedFraction NOTIFY heightOffsetChanged FINAL)
    Q_PROPERTY(bool overlapped READ overlapped NOTIFY overlappedChanged FINAL)
    Q_PROPERTY(bool contentAtStart READ contentAtStart WRITE setContentAtStart NOTIFY
                   contentAtStartChanged FINAL)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool reverseLayout READ reverseLayout WRITE setReverseLayout NOTIFY
                   reverseLayoutChanged FINAL)
public:
    enum Mode
    {
        Pinned,
        EnterAlways,
        ExitUntilCollapsed
    };
    Q_ENUM(Mode)
    explicit AppBarScroll(QObject* parent = nullptr);
    void    classBegin() override;
    void    componentComplete() override;
    Mode    mode() const { return m_mode; }
    qreal   collapseDistance() const { return m_distance; }
    qreal   heightOffset() const { return m_offset; }
    qreal   collapsedFraction() const { return m_distance > 0 ? -m_offset / m_distance : 0; }
    bool    overlapped() const { return m_enabled && (! m_contentAtStart || m_offset < 0); }
    bool    contentAtStart() const { return m_contentAtStart; }
    bool    active() const { return m_active; }
    bool    enabled() const { return m_enabled; }
    bool    reverseLayout() const { return m_reverseLayout; }
    void    setMode(Mode);
    void    setCollapseDistance(qreal);
    void    setHeightOffset(qreal);
    void    setContentAtStart(bool);
    void    setEnabled(bool);
    void    setReverseLayout(bool);
    QPointF preScroll(QPointF, Source) override;
    QPointF postScroll(QPointF, QPointF, Source) override;
    bool    canConsume(QPointF, Source) const override;
    void    begin(Source) override;
    void    end(bool cancelled) override;
    Q_INVOKABLE void reset();
    Q_SIGNAL void    modeChanged();
    Q_SIGNAL void    collapseDistanceChanged();
    Q_SIGNAL void    heightOffsetChanged();
    Q_SIGNAL void    contentAtStartChanged();
    Q_SIGNAL void    overlappedChanged();
    Q_SIGNAL void    activeChanged();
    Q_SIGNAL void    enabledChanged();
    Q_SIGNAL void    reverseLayoutChanged();
    Q_SIGNAL void    inputStarted();

private:
    QPointF              consume(QPointF);
    void                 update(qreal, bool);
    Mode                 m_mode           = ExitUntilCollapsed;
    qreal                m_distance       = 0;
    qreal                m_offset         = 0;
    bool                 m_contentAtStart = true;
    bool                 m_active         = false;
    bool                 m_enabled        = true;
    bool                 m_reverseLayout  = false;
    bool                 m_initializing   = false;
    std::optional<qreal> m_initialOffset;
};
} // namespace qml_material
