#include "qml_material/scenegraph/texture_cache.h"

#include <cmath>
#include <cstring>
#include <vector>

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QSGTexture>

#include "qml_material/math/gaussian.hpp"

namespace qml_material::sg
{

namespace
{
constexpr int   kProfileSize = 128;
constexpr int   kFadeoffSize = 128;
constexpr float kMinSigma    = 0.5f;
constexpr float kMinRadius   = 0.5f;

struct CornerKey {
    int sigma_q;
    int radius_q;
    bool operator==(const CornerKey&) const = default;
};

inline std::size_t qHash(const CornerKey& k, std::size_t seed = 0) noexcept {
    return ::qHashMulti(seed, k.sigma_q, k.radius_q);
}

struct WindowEntry {
    QSGTexture*                          profile = nullptr;
    QSGTexture*                          fadeoff = nullptr;
    QHash<CornerKey, QSGTexture*>        corners;
};

QMutex& cache_mutex() {
    static QMutex m;
    return m;
}

// Heap-allocated entries so we can transfer ownership out of the map for
// destruction without holding the lock.
QHash<QQuickWindow*, WindowEntry*>& cache_map() {
    static QHash<QQuickWindow*, WindowEntry*> m;
    return m;
}

void release_textures_locked(WindowEntry* e) {
    if (! e) return;
    delete e->profile;
    e->profile = nullptr;
    delete e->fadeoff;
    e->fadeoff = nullptr;
    for (auto* t : std::as_const(e->corners)) delete t;
    e->corners.clear();
}

WindowEntry* ensure_entry_locked(QQuickWindow* win) {
    auto& map = cache_map();
    auto  it  = map.find(win);
    if (it != map.end()) return *it;

    auto* entry = new WindowEntry {};
    map.insert(win, entry);

    // Release GPU resources when the scene graph is torn down (context loss,
    // window hide, or window destruction). Emitted on the render thread under
    // the threaded loop — DirectConnection keeps the QSGTexture deletes on the
    // owning render thread. Qt guarantees this fires before ~QQuickWindow.
    QObject::connect(win, &QQuickWindow::sceneGraphInvalidated, win, [win]() {
        QMutexLocker lk(&cache_mutex());
        auto&        map = cache_map();
        auto         it  = map.find(win);
        if (it != map.end()) release_textures_locked(*it);
    }, Qt::DirectConnection);

    // Drop the bookkeeping entry on the GUI thread after destruction. By the
    // time this fires, sceneGraphInvalidated has already cleared the textures
    // so there is nothing GPU-related left to free here.
    QObject::connect(win, &QObject::destroyed, [win]() {
        WindowEntry* victim = nullptr;
        {
            QMutexLocker lk(&cache_mutex());
            auto&        map = cache_map();
            auto         it  = map.find(win);
            if (it != map.end()) {
                victim = *it;
                map.erase(it);
            }
        }
        delete victim;
    });

    return entry;
}

QSGTexture* build_profile_texture(QQuickWindow* win) {
    QImage image(kProfileSize, 1, QImage::Format_Grayscale8);
    math::fill_unit_cdf_profile(
        std::span<std::uint8_t> { image.scanLine(0), static_cast<std::size_t>(kProfileSize) });
    return win->createTextureFromImage(image);
}

QSGTexture* build_fadeoff_texture(QQuickWindow* win) {
    QImage image(kFadeoffSize, 1, QImage::Format_Grayscale8);
    uchar* row = image.scanLine(0);
    for (int i = 0; i < kFadeoffSize; ++i) {
        const float d = math::k_scalar_one - i / float(kFadeoffSize - 1);
        row[i] = math::round_to_int((std::exp(-4.0f * d * d) - 0.018f) * 255.0f);
    }
    return win->createTextureFromImage(image);
}

QSGTexture* build_corner_texture(QQuickWindow* win, float sigma, float radius) {
    const int n = math::rrect_corner_blur_size(sigma, radius);
    if (n <= 0) return nullptr;
    QImage                    image(n, n, QImage::Format_Grayscale8);
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(n) * n);
    math::fill_rrect_corner_blur(buf, sigma, radius);
    for (int j = 0; j < n; ++j) {
        std::memcpy(image.scanLine(j),
                    buf.data() + static_cast<std::size_t>(j) * n,
                    static_cast<std::size_t>(n));
    }
    return win->createTextureFromImage(image);
}

} // namespace

QSGTexture* shared_blur_profile_texture(QQuickWindow* win) {
    if (! win) return nullptr;
    QMutexLocker lk(&cache_mutex());
    auto*        e = ensure_entry_locked(win);
    if (! e->profile) e->profile = build_profile_texture(win);
    return e->profile;
}

QSGTexture* shared_shadow_fadeoff_texture(QQuickWindow* win) {
    if (! win) return nullptr;
    QMutexLocker lk(&cache_mutex());
    auto*        e = ensure_entry_locked(win);
    if (! e->fadeoff) e->fadeoff = build_fadeoff_texture(win);
    return e->fadeoff;
}

QSGTexture* shared_rrect_corner_blur_texture(QQuickWindow* win, float sigma, float radius) {
    if (! win) return nullptr;
    if (sigma < kMinSigma || radius < kMinRadius) return nullptr;
    const CornerKey key {
        .sigma_q  = static_cast<int>(std::round(sigma * 2.0f)),
        .radius_q = static_cast<int>(std::round(radius * 2.0f)),
    };
    QMutexLocker lk(&cache_mutex());
    auto*        e = ensure_entry_locked(win);
    auto         it = e->corners.find(key);
    if (it != e->corners.end()) return *it;
    QSGTexture* t = build_corner_texture(win, sigma, radius);
    e->corners.insert(key, t);
    return t;
}

} // namespace qml_material::sg
