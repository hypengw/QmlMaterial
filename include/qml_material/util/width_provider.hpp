#pragma once
#include <QtQml/QQmlEngine>

namespace qml_material
{

class WidthProvider : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qint32 width READ width NOTIFY widthChanged FINAL)
    Q_PROPERTY(qint32 column READ column NOTIFY columnChanged FINAL)
    Q_PROPERTY(qint32 minimum READ minimum WRITE setMinimum NOTIFY minimumChanged FINAL)
    Q_PROPERTY(qint32 leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged FINAL)
    Q_PROPERTY(
        qint32 rightMargin READ rightMargin WRITE setrightMargin NOTIFY rightMarginChanged FINAL)
    Q_PROPERTY(qint32 spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(qint32 total READ total WRITE setTotal NOTIFY totalChanged FINAL)
public:
    explicit WidthProvider(QObject* parent = nullptr);

    Q_INVOKABLE qint32 calculateX(qint32 column) noexcept;

    auto          width() const noexcept -> qint32;
    Q_SIGNAL void widthChanged();

    auto          column() const noexcept -> qint32;
    Q_SIGNAL void columnChanged();

    auto          leftMargin() const noexcept -> qint32;
    void          setLeftMargin(qint32);
    Q_SIGNAL void leftMarginChanged();

    auto          rightMargin() const noexcept -> qint32;
    void          setrightMargin(qint32);
    Q_SIGNAL void rightMarginChanged();

    auto          minimum() const noexcept -> qint32;
    void          setMinimum(qint32);
    Q_SIGNAL void minimumChanged();

    auto          spacing() const noexcept -> qint32;
    void          setSpacing(qint32);
    Q_SIGNAL void spacingChanged();

    auto          total() const noexcept -> qint32;
    void          setTotal(qint32);
    Q_SIGNAL void totalChanged();

    Q_SLOT void refresh();

private:
    qint32 m_column;
    qint32 m_width;
    qint32 m_total;
    qint32 m_minimum;
    qint32 m_spacing;
    qint32 m_left_margin;
    qint32 m_right_margin;
};

} // namespace qml_material