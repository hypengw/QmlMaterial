#include "qml_material/carousel/carousel_view.hpp"

#include <QAbstractListModel>
#include <QGuiApplication>
#include <QStringList>
#include <cstdio>
#include <cstdlib>
#include <utility>

using namespace qml_material;

namespace
{

class StringModel : public QAbstractListModel
{
public:
    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : m_values.size();
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        return index.isValid() && role == Qt::DisplayRole ? m_values.at(index.row()) : QVariant {};
    }

    void prepend(QString value)
    {
        beginInsertRows({}, 0, 0);
        m_values.prepend(std::move(value));
        endInsertRows();
    }

    void moveToFront(int row)
    {
        beginMoveRows({}, row, row, {}, 0);
        m_values.move(row, 0);
        endMoveRows();
    }

    void remove(int row)
    {
        beginRemoveRows({}, row, row);
        m_values.removeAt(row);
        endRemoveRows();
    }

    QString value(int row) const { return m_values.at(row); }

private:
    QStringList m_values { QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c") };
};

auto fail(const char* message) -> int
{
    std::fprintf(stderr, "FAIL: %s\n", message);
    return EXIT_FAILURE;
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    StringModel  model;
    CarouselView view;
    view.setModel(QVariant::fromValue<QObject*>(&model));
    view.setCurrentIndex(1);

    model.prepend(QStringLiteral("x"));
    if (view.currentIndex() != 2 || model.value(view.currentIndex()) != QStringLiteral("b")) {
        return fail("front insertion changed the logical current item");
    }

    model.moveToFront(2);
    if (view.currentIndex() != 0 || model.value(view.currentIndex()) != QStringLiteral("b")) {
        return fail("row move changed the logical current item");
    }

    model.remove(0);
    if (view.currentIndex() != 0 || model.value(view.currentIndex()) != QStringLiteral("x")) {
        return fail("removing the current item did not select its neighbor");
    }

    std::printf("PASS model_current_item\n");
    return EXIT_SUCCESS;
}
