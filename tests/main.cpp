#include <QCoreApplication>
#include <QGuiApplication>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#define QM_TEST_GROUP(name, application) int run_##name(int argc, char** argv);
#include "test_groups.inc"
#undef QM_TEST_GROUP

namespace
{
enum class Application
{
    None,
    Core,
    Gui
};

struct TestGroup {
    const char* name;
    Application application;
    int (*run)(int, char**);
};

const TestGroup groups[] = {
#define QM_TEST_GROUP(name, application) { #name, Application::application, run_##name },
#include "test_groups.inc"
#undef QM_TEST_GROUP
};

void printGroups() {
    for (const auto& group : groups) std::puts(group.name);
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::strcmp(argv[1], "--help") == 0) {
        std::puts("Usage: qm_tests <group> [QtTest arguments]\n       qm_tests --list");
        printGroups();
        return argc < 2 ? 2 : 0;
    }
    if (std::strcmp(argv[1], "--list") == 0) {
        printGroups();
        return 0;
    }
    const TestGroup* selected = nullptr;
    for (const auto& group : groups) {
        if (std::strcmp(argv[1], group.name) == 0) {
            selected = &group;
            break;
        }
    }
    if (! selected) {
        std::fprintf(stderr, "Unknown test group: %s\n", argv[1]);
        return 2;
    }

    std::vector<char*> arguments { argv[0] };
    for (int i = 2; i < argc; ++i) arguments.push_back(argv[i]);
    int count = int(arguments.size());
    arguments.push_back(nullptr);
    if (std::strcmp(selected->name, "control_layout") == 0) {
        qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
        qputenv("QT_SCALE_FACTOR", "1");
    }
    std::unique_ptr<QCoreApplication> app;
    if (selected->application == Application::Gui)
        app = std::make_unique<QGuiApplication>(count, arguments.data());
    else if (selected->application == Application::Core)
        app = std::make_unique<QCoreApplication>(count, arguments.data());
    return selected->run(count, arguments.data());
}
