#include "qml_material/carousel/carousel_strategy.hpp"
#include "qml_material/carousel/carousel_view.hpp"

#include <QAbstractListModel>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <cstdio>
#include <cstdlib>

using namespace qml_material;

namespace
{

struct TestCarouselView : CarouselView
{
    using CarouselView::CarouselView;

    void complete()
    {
        componentComplete();
    }
};

class TestListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        TitleRole = Qt::UserRole + 1,
    };

    explicit TestListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
        m_titles = { QStringLiteral("Alpha"), QStringLiteral("Beta"), QStringLiteral("Gamma") };
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_titles.size();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_titles.size()) {
            return {};
        }
        if (role == TitleRole) {
            return m_titles.at(index.row());
        }
        return {};
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        if (!index.isValid() || role != TitleRole) {
            return false;
        }
        m_titles[index.row()] = value.toString();
        Q_EMIT dataChanged(index, index, { TitleRole });
        return true;
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return { { TitleRole, "title" } };
    }

    void setTitle(int row, const QString& title)
    {
        setData(index(row, 0), title, TitleRole);
    }

private:
    QStringList m_titles;
};

auto fail(const char* msg) -> int
{
    std::fprintf(stderr, "FAIL: %s\n", msg);
    return EXIT_FAILURE;
}

auto findDelegateItem(CarouselView* view, int index) -> QQuickItem*
{
    auto* flick = view ? view->flickable() : nullptr;
    if (!flick) {
        return nullptr;
    }
    for (QQuickItem* content_item : flick->childItems()) {
        for (QQuickItem* row_content : content_item->childItems()) {
            for (QQuickItem* item : row_content->childItems()) {
                const QVariant index_value = item->property("_carouselIndex");
                if (!index_value.isValid() || index_value.toInt() != index) {
                    continue;
                }
                return item;
            }
        }
    }
    return nullptr;
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QQmlEngine engine;
    QQuickWindow window;
    window.resize(480, 196);

    QQmlComponent root_component(&engine);
    root_component.setData(QByteArrayLiteral("import QtQuick\nItem { }"), QUrl());
    if (root_component.isError()) {
        std::fprintf(stderr, "FAIL: root QML: %s\n", qPrintable(root_component.errorString()));
        return EXIT_FAILURE;
    }
    auto* root = qobject_cast<QQuickItem*>(root_component.create());
    if (!root) {
        return fail("expected engine root item");
    }
    root->setParentItem(window.contentItem());
    QQmlEngine::setObjectOwnership(root, QQmlEngine::CppOwnership);

    QQmlComponent delegate_component(&engine);
    delegate_component.setData(
        QByteArrayLiteral(
            "import QtQuick\n"
            "Item {\n"
            "    property var model\n"
            "    property string title: \"\"\n"
            "}\n"),
        QUrl());
    if (delegate_component.isError()) {
        std::fprintf(stderr, "FAIL: delegate QML: %s\n",
                     qPrintable(delegate_component.errorString()));
        return EXIT_FAILURE;
    }

    TestListModel model;
    TestCarouselView view;
    view.setParentItem(root);
    view.setWidth(480);
    view.setHeight(196);
    view.setLayout(CarouselLayoutId::Uncontained);
    view.setDelegate(&delegate_component);
    view.setModel(QVariant::fromValue<QObject*>(&model));
    view.setItemExtent(140);
    view.setSpacing(8);
    view.setContentPaddingStart(16);
    view.complete();

    if (view.count() != 3) {
        std::fprintf(stderr, "FAIL: count=%d expected=3\n", view.count());
        return EXIT_FAILURE;
    }

    auto* delegate = findDelegateItem(&view, 0);
    if (!delegate) {
        return fail("expected delegate item at index 0");
    }

    if (delegate->property("title").toString() != QStringLiteral("Alpha")) {
        std::fprintf(stderr, "FAIL: initial title=%s expected=Alpha\n",
                     qPrintable(delegate->property("title").toString()));
        return EXIT_FAILURE;
    }

    model.setTitle(0, QStringLiteral("Updated"));
    if (delegate->property("title").toString() != QStringLiteral("Updated")) {
        std::fprintf(stderr, "FAIL: dataChanged title=%s expected=Updated\n",
                     qPrintable(delegate->property("title").toString()));
        return EXIT_FAILURE;
    }

    std::printf("PASS model_data_changed\n");
    return EXIT_SUCCESS;
}

#include "model_data_changed.moc"
