#pragma once
#include <QtQml/QQmlEngine>

namespace qml_material
{

class WidthProvider : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qint32 width READ width NOTIFY widthChanged FINAL)
    Q_PROPERTY(qint32 minimum READ minimum WRITE setMinimum NOTIFY minimumChanged FINAL)
    Q_PROPERTY(qint32 spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(qint32 total READ total WRITE setTotal NOTIFY totalChanged FINAL)
public:
    explicit WidthProvider(QObject* parent = nullptr);

    auto          width() const noexcept -> qint32;
    Q_SIGNAL void widthChanged();

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
    qint32 m_width;
    qint32 m_total;
    qint32 m_minimum;
    qint32 m_spacing;
};

} // namespace qml_material