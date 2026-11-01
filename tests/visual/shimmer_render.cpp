#include <QtTest>
#include <QtGui/rhi/qrhi.h>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QSGTextureProvider>
#include "qml_material/anim/shimmer.hpp"
#include "qml_material/item/shimmer_effect.hpp"

using namespace qml_material;

namespace
{
class Scene {
public:
    QQuickRenderControl         control;
    QQuickWindow                window { &control };
    QQmlEngine                  engine;
    std::unique_ptr<QQuickItem> root;
    QRhiTexture*                texture = nullptr;
    QRhiTextureRenderTarget*    target  = nullptr;
    QRhiRenderPassDescriptor*   pass    = nullptr;
    QRhiRenderBuffer*           depth   = nullptr;
    int                         dpr     = 1;

    ~Scene() {
        root.reset();
        control.invalidate();
        delete target;
        delete pass;
        delete depth;
        delete texture;
    }
    bool create(const QByteArray& body, int scale = 1) {
        dpr = scale;
        window.resize(320, 240);
        if (! control.initialize()) return false;
        auto*       rhi = control.rhi();
        const QSize pixels(320 * dpr, 240 * dpr);
        texture = rhi->newTexture(QRhiTexture::RGBA8,
                                  pixels,
                                  1,
                                  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
        depth   = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixels);
        if (! texture->create() || ! depth->create()) return false;
        QRhiTextureRenderTargetDescription description(texture);
        description.setDepthStencilBuffer(depth);
        target = rhi->newTextureRenderTarget(description);
        pass   = target->newCompatibleRenderPassDescriptor();
        target->setRenderPassDescriptor(pass);
        if (! target->create()) return false;
        auto renderTarget = QQuickRenderTarget::fromRhiRenderTarget(target);
        renderTarget.setDevicePixelRatio(dpr);
        window.setRenderTarget(renderTarget);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport Qcm.Material as MD\nRectangle {id:root; "
                          "width:320; height:240; color:'white'; " +
                              body + "}",
                          QUrl("qrc:/tests/shimmer-render.qml"));
        root.reset(qobject_cast<QQuickItem*>(component.create()));
        if (! root) qWarning() << component.errors();
        if (! root) return false;
        root->setParentItem(window.contentItem());
        return true;
    }
    QImage render() {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        control.polishItems();
        control.beginFrame();
        control.sync();
        control.render();
        QRhiReadbackResult result;
        auto*              batch = control.rhi()->nextResourceUpdateBatch();
        batch->readBackTexture(QRhiReadbackDescription(texture), &result);
        control.commandBuffer()->resourceUpdate(batch);
        control.endFrame();
        control.rhi()->finish();
        if (result.data.isEmpty()) return {};
        QImage image(reinterpret_cast<const uchar*>(result.data.constData()),
                     result.pixelSize.width(),
                     result.pixelSize.height(),
                     QImage::Format_RGBA8888_Premultiplied);
        return control.rhi()->isYUpInFramebuffer() ? image.mirrored(false, true) : image.copy();
    }
    QImage settle() {
        render();
        render();
        return render();
    }
    QColor at(const QImage& image, int x, int y) const {
        return image.pixelColor(x * dpr, y * dpr);
    }
    Shimmer*       group() const { return root->findChild<Shimmer*>("group"); }
    ShimmerEffect* effect(const QString& name) const {
        const auto visit = [&](auto&& self, QQuickItem* item) -> ShimmerEffect* {
            if (auto* effect = qobject_cast<ShimmerEffect*>(item);
                effect && effect->objectName() == name)
                return effect;
            for (auto* child : item->childItems())
                if (auto* found = self(self, child)) return found;
            return nullptr;
        };
        return visit(visit, root.get());
    }
};
} // namespace

class ShimmerRenderTest : public QObject {
    Q_OBJECT
private slots:
    void masks_data() {
        QTest::addColumn<int>("dpr");
        QTest::newRow("dpr1") << 1;
        QTest::newRow("dpr2") << 2;
    }
    void masks() {
        QFETCH(int, dpr);
        Scene scene;
        QVERIFY(scene.create(R"(
            property color ink: "black"
            MD.Shimmer { id:g; objectName:"group"; boundsMode:MD.Shimmer.Item; boundsItem:root
                running:false; angle:0; bandWidth:80; minimumOpacity:0
            }
            MD.Skeleton {x:20; y:20; width:280; height:32; radius:8; color:root.ink; shimmer:g}
            MD.Skeleton {x:100; y:70; width:140; height:32; radius:8; color:root.ink; shimmer:g}
            MD.Skeleton {x:150; y:120; width:80; height:32; radius:8; color:root.ink; shimmer:g}
            Item {x:20; y:180; width:280; height:32; layer.enabled:true
                layer.effect:MD.ShimmerEffect {shimmer:g}
                Rectangle {anchors.fill:parent; radius:8; color:root.ink}
            }
        )",
                             dpr));
        scene.group()->seek(.5);
        auto image = scene.settle();
        QVERIFY(! image.isNull());
        QCOMPARE(image.size(), QSize(320 * dpr, 240 * dpr));
        QVERIFY(scene.at(image, 160, 36).red() < 8);
        QCOMPARE(scene.at(image, 160, 36), scene.at(image, 160, 86));
        QCOMPARE(scene.at(image, 160, 36), scene.at(image, 160, 136));
        QCOMPARE(scene.at(image, 160, 36), scene.at(image, 160, 196));
        QCOMPARE(scene.at(image, 140, 36), scene.at(image, 140, 196));
        QCOMPARE(scene.at(image, 40, 196), QColor(Qt::white));
        QCOMPARE(scene.at(image, 40, 36), QColor(Qt::white));
        QCOMPARE(scene.at(image, 20, 20), QColor(Qt::white));
        scene.group()->seek(0);
        image = scene.render();
        QCOMPARE(scene.at(image, 160, 36), QColor(Qt::white));
        scene.group()->seek(.5);
        image = scene.render();
        QVERIFY(scene.at(image, 160, 36).red() < 8);
        scene.group()->setAnimationsEnabled(false);
        image = scene.render();
        QCOMPARE(scene.at(image, 40, 36), QColor(Qt::black));
        scene.root->setProperty("ink", QColor(96, 128, 192, 128));
        image            = scene.render();
        const auto mixed = scene.at(image, 40, 36);
        QVERIFY(std::abs(mixed.red() - 175) < 3);
        QVERIFY(std::abs(mixed.green() - 191) < 3);
        QVERIFY(std::abs(mixed.blue() - 223) < 3);
        scene.group()->setAnimationsEnabled(true);
        scene.group()->setBoundsItem(nullptr);
        image = scene.render();
        QCOMPARE(scene.at(image, 40, 36), mixed);
    }
    void textureAndRecovery() {
        Scene scene;
        QVERIFY(scene.create(R"(
            MD.Shimmer {id:g; objectName:"group"; running:false; angle:0; bandWidth:80; minimumOpacity:0}
            Item {x:20; y:20; width:80; height:80; opacity:.5
                layer.enabled:true
                layer.effect:MD.ShimmerEffect { objectName:"alphaEffect"; shimmer:g }
                Rectangle {anchors.fill:parent; radius:40; color:"#80ff0000"}
            }
            MD.Shape {x:120; y:20; width:80; height:80
                objectName: "layeredShape"
                layer.enabled:true
                layer.effect:MD.ShimmerEffect { shimmer:g }
                MD.MaterialShapePath {shape:MD.MaterialShape.Cookie6Sided; size:Qt.size(80,80); fillColor:"blue"; strokeWidth:0}
            }
            MD.Shape {x:120; y:120; width:80; height:80
                MD.MaterialShapePath {shape:MD.MaterialShape.Cookie6Sided; size:Qt.size(80,80); fillColor:"blue"; strokeWidth:0}
            }
            Item {x:220; y:20; width:80; height:80
                layer.enabled:true
                layer.effect:MD.ShimmerEffect { shimmer:g }
                Rectangle {width:80; height:40; color:"red"}
                Rectangle {y:40; width:80; height:40; color:"blue"}
            }
        )"));
        scene.group()->seek(.5);
        auto image = scene.settle();
        QVERIFY(! image.isNull());
        const auto alpha = scene.at(image, 60, 60);
        QCOMPARE(alpha.red(), 255);
        QVERIFY(std::abs(alpha.green() - 192) < 4);
        QCOMPARE(scene.at(image, 20, 20), QColor(Qt::white));
        QVERIFY(scene.at(image, 160, 60).blue() > 245);
        QVERIFY(scene.at(image, 160, 60).red() < 10);
        QCOMPARE(scene.at(image, 120, 20), QColor(Qt::white));
        const auto top = scene.at(image, 260, 30), bottom = scene.at(image, 260, 90);
        QVERIFY(top.red() > 245 && top.blue() < 10);
        QVERIFY(bottom.blue() > 245 && bottom.red() < 10);
        auto* effect = scene.effect(QStringLiteral("alphaEffect"));
        QVERIFY(effect && effect->source());
        auto* provider = effect->source()->textureProvider();
        QVERIFY(provider);
        QSignalSpy captures(provider, &QSGTextureProvider::textureChanged);
        scene.group()->seek(0);
        image = scene.render();
        QCOMPARE(scene.at(image, 160, 60), QColor(Qt::white));
        QVERIFY(captures.isEmpty());
        scene.group()->seek(.5);
        scene.control.invalidate();
        QVERIFY(scene.control.initialize());
        image = scene.settle();
        QCOMPARE(scene.at(image, 60, 60), alpha);
        // Compare with an unmodified Shape: Qt's curve renderer also owns resources here.
        QVERIFY(std::abs(scene.at(image, 160, 60).red() - scene.at(image, 160, 160).red()) < 5);
        scene.root->setParentItem(nullptr);
        scene.root->setParentItem(scene.window.contentItem());
        image = scene.settle();
        QVERIFY(scene.at(image, 160, 60).red() < 10);
        QVERIFY(scene.at(image, 160, 160).red() < 10);
    }
    void sourceReplacement() {
        Scene scene;
        QVERIFY(scene.create(R"(
            property bool blue: false
            property color firstColor: "red"
            MD.Shimmer {id:g; objectName:"group"; running:false; angle:0; bandWidth:80; minimumOpacity:0}
            Rectangle {id:first; width:80; height:80; color:root.firstColor}
            Rectangle {id:second; width:80; height:80; color:"blue"}
            ShaderEffectSource {id:a; sourceItem:first; hideSource:true; visible:false}
            ShaderEffectSource {id:b; sourceItem:second; hideSource:true; visible:false}
            MD.ShimmerEffect {x:100; y:20; width:80; height:80; shimmer:g; source:root.blue ? b : a}
        )"));
        scene.group()->seek(.5);
        auto image = scene.settle();
        QVERIFY(scene.at(image, 140, 60).red() > 245);
        QVERIFY(scene.at(image, 140, 60).blue() < 10);
        scene.root->setProperty("firstColor", QColor(Qt::green));
        image = scene.render();
        QVERIFY(scene.at(image, 140, 60).green() > 120);
        scene.root->setProperty("blue", true);
        image = scene.settle();
        QVERIFY(scene.at(image, 140, 60).blue() > 245);
        QVERIFY(scene.at(image, 140, 60).red() < 10);
    }
};
int run_shimmer_render(int argc, char** argv) {
    ShimmerRenderTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "shimmer_render.moc"
