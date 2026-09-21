#pragma once

#include <QtQuick/private/qquicktableview_p.h>
#include "qml_material/model/header_model.hpp"

namespace qml_material
{
class HeaderDragHandler;
class QML_MATERIAL_API HeaderView : public QQuickTableView {
    Q_OBJECT
    QML_NAMED_ELEMENT(HeaderViewBase)
    QML_UNCREATABLE("Use HorizontalHeaderViewBase or VerticalHeaderViewBase")
    Q_PROPERTY(QVariant model READ headerModel WRITE setHeaderModel RESET resetHeaderModel NOTIFY
                   modelChanged FINAL)
    Q_PROPERTY(QString textRole READ textRole WRITE setTextRole NOTIFY textRoleChanged FINAL)
    Q_PROPERTY(HeaderModel::SourceMode sourceMode READ sourceMode WRITE setSourceMode NOTIFY
                   sourceModeChanged FINAL)
    Q_PROPERTY(QStringList headerTexts READ headerTexts NOTIFY headerContentChanged FINAL)
    Q_PROPERTY(int sectionCount READ sectionCount NOTIFY headerContentChanged FINAL)
    Q_PROPERTY(QList<int> visualSections READ visualSections NOTIFY sectionOrderChanged FINAL)
public:
    ~HeaderView() override;
    bool                    movableSections() const { return m_movable; }
    void                    setMovableSections(bool);
    QVariant                headerModel() const { return m_model.model(); }
    void                    setHeaderModel(const QVariant& value) { m_model.setModel(value); }
    void                    resetHeaderModel() { m_model.resetModel(); }
    QString                 textRole() const { return m_model.textRole(); }
    void                    setTextRole(const QString& value) { m_model.setTextRole(value); }
    HeaderModel::SourceMode sourceMode() const { return m_model.sourceMode(); }
    void          setSourceMode(HeaderModel::SourceMode value) { m_model.setSourceMode(value); }
    QStringList   headerTexts() const { return m_model.texts(); }
    int           sectionCount() const { return m_model.sectionCount(); }
    QList<int>    visualSections() const;
    Q_SIGNAL void textRoleChanged();
    Q_SIGNAL void sourceModeChanged();
    Q_SIGNAL void headerContentChanged();
    Q_SIGNAL void movableSectionsChanged();
    Q_SIGNAL void sectionOrderChanged();

protected:
    HeaderView(Qt::Orientation orientation, QQuickItem* parent);

private:
    void                    syncSource();
    HeaderModel             m_model;
    QMetaObject::Connection m_syncChanged, m_syncDestroyed;
    HeaderDragHandler*      m_drag    = nullptr;
    bool                    m_movable = false;
};

class QML_MATERIAL_API HorizontalHeaderView : public HeaderView {
    Q_OBJECT
    QML_NAMED_ELEMENT(HorizontalHeaderViewBase)
    Q_PROPERTY(bool movableColumns READ movableSections WRITE setMovableSections NOTIFY
                   movableSectionsChanged FINAL)
public:
    explicit HorizontalHeaderView(QQuickItem* parent = nullptr);
};

class QML_MATERIAL_API VerticalHeaderView : public HeaderView {
    Q_OBJECT
    QML_NAMED_ELEMENT(VerticalHeaderViewBase)
    Q_PROPERTY(bool movableRows READ movableSections WRITE setMovableSections NOTIFY
                   movableSectionsChanged FINAL)
public:
    explicit VerticalHeaderView(QQuickItem* parent = nullptr);
};
} // namespace qml_material
