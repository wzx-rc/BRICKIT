#include "ArchTextures.h"
#include "vp3d/shaders/Shared.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace archviz {

using namespace vp3d;

namespace {

uint32_t hash(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return x;
}
float rnd(uint32_t a, uint32_t b = 0, uint32_t c = 0) { return (hash(a * 73856093U ^ hash(b * 19349663U ^ hash(c + 83492791U))) & 0xFFFFFF) / 16777215.0f; }

// tileable value noise: period p cells over the unit square
float vnoise(float x, float y, int p, uint32_t seed) {
    x *= p; y *= p;
    const int xi = (int)std::floor(x), yi = (int)std::floor(y);
    const float fx = x - xi, fy = y - yi;
    auto at = [&](int i, int j) { return rnd(((i % p) + p) % p, ((j % p) + p) % p, seed); };
    const float sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    const float a = at(xi, yi) + (at(xi + 1, yi) - at(xi, yi)) * sx, b = at(xi, yi + 1) + (at(xi + 1, yi + 1) - at(xi, yi + 1)) * sx;
    return a + (b - a) * sy;
}
float fbm(float x, float y, int p, uint32_t seed, int oct = 4) {
    float s = 0, a = 0.5f, n = 0;
    for (int o = 0; o < oct; ++o) { s += vnoise(x, y, p << o, seed + o) * a; n += a; a *= 0.5f; }
    return s / n;
}

struct Img {
    int n;
    std::vector<uint8_t> px;
    explicit Img(int size) : n(size), px((size_t)size * size * 4, 255) {}
    void set(int x, int y, float r, float g, float b) {
        uint8_t* p = &px[((size_t)y * n + x) * 4];
        p[0] = (uint8_t)std::clamp(r * 255.0f, 0.0f, 255.0f); p[1] = (uint8_t)std::clamp(g * 255.0f, 0.0f, 255.0f); p[2] = (uint8_t)std::clamp(b * 255.0f, 0.0f, 255.0f);
    }
};

// planks / boards: rows of width w (texture units) along u, lengths L staggered per row, grain along u
void boards(Img& im, int rows, float lenUnits, float r, float g, float b, float vary, bool vertical, uint32_t seed) {
    for (int y = 0; y < im.n; ++y)
        for (int x = 0; x < im.n; ++x) {
            float u = (x + 0.5f) / im.n, v = (y + 0.5f) / im.n;
            if (vertical) std::swap(u, v);
            const int row = (int)(v * rows);
            const float off = rnd(row, 7, seed);
            const float uu = u + off;
            const int seg = (int)std::floor(uu / lenUnits);
            const float k = (rnd(row, seg, seed + 1) - 0.5f) * vary;
            const float grain = fbm(uu * 1.0f, v * rows * 0.25f, 8, seed + row * 3 + seg) * 0.55f + vnoise(uu * 6.0f, v * rows * 4.0f, 8, seed + 9) * 0.25f;
            const float rv = v * rows - row, ru = uu / lenUnits - seg;
            const float gap = (rv < 0.035f || ru < 0.004f) ? 0.55f : 1.0f;
            const float t = (0.82f + grain * 0.36f + k) * gap;
            im.set(vertical ? y : x, vertical ? x : y, r * t, g * t, b * t);
        }
}

Texture make(int t) {
    Texture T;
    const int N = (t == kTexParquet || t == kTexBrick || t == kTexPaving) ? 1024 : 512;
    Img im(N);
    const uint32_t seed = 1234 + t * 77;
    switch (t) {
    case kTexParquet: boards(im, 13, 0.5f, 0.64f, 0.49f, 0.34f, 0.13f, false, seed); break;
    case kTexCladding: boards(im, 10, 2.0f, 0.62f, 0.47f, 0.32f, 0.16f, true, seed); break;
    case kTexTiles:   // 2 x 2 tiles, light (the material tints them), joints 3 % of a tile
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x) {
                const float u = (x + 0.5f) / N * 2, v = (y + 0.5f) / N * 2;
                const float fu = u - std::floor(u), fv = v - std::floor(v);
                const bool joint = fu < 0.009f || fu > 0.991f || fv < 0.009f || fv > 0.991f;
                const float k = 0.94f + (rnd((int)u, (int)v, seed) - 0.5f) * 0.05f + (fbm(u * 0.5f, v * 0.5f, 4, seed) - 0.5f) * 0.04f;
                if (joint) im.set(x, y, 0.74f, 0.73f, 0.70f);
                else im.set(x, y, k, k, k * 0.99f);
            }
        break;
    case kTexConcrete:
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x) {
                const float u = (x + 0.5f) / N, v = (y + 0.5f) / N;
                float k = 0.62f + (fbm(u, v, 4, seed, 5) - 0.5f) * 0.18f;
                if (rnd(x, y, seed) > 0.997f) k *= 0.7f;   // pores
                im.set(x, y, k, k * 0.99f, k * 0.96f);
            }
        break;
    case kTexCarpet:
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x) {
                const float u = (x + 0.5f) / N, v = (y + 0.5f) / N;
                const float k = 0.80f + (rnd(x, y, seed) - 0.5f) * 0.22f + (fbm(u, v, 8, seed) - 0.5f) * 0.12f;
                im.set(x, y, k, k, k);
            }
        break;
    case kTexBrick:   // 4 bricks across, 12 courses up (0.9 m), running bond, mortar
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x) {
                const float u = (x + 0.5f) / N, v = (y + 0.5f) / N;
                const int row = (int)(v * 12);
                const float uu = u * 4 + (row & 1) * 0.5f;
                const int col = (int)std::floor(uu);
                const float fu = uu - col, fv = v * 12 - row;
                const bool mortar = fv < 0.13f || fu < 0.045f;
                const float k = (rnd(row, ((col % 4) + 4) % 4, seed) - 0.5f) * 0.22f + (fbm(u * 2, v * 2, 8, seed) - 0.5f) * 0.18f;
                if (mortar) { const float m = 0.74f + (rnd(x, y, seed) - 0.5f) * 0.06f; im.set(x, y, m, m * 0.98f, m * 0.94f); }
                else im.set(x, y, 0.58f + k, 0.30f + k * 0.6f, 0.22f + k * 0.4f);
            }
        break;
    case kTexGrass:
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x) {
                const float u = (x + 0.5f) / N, v = (y + 0.5f) / N;
                // blades (fine noise), patches of greener / drier grass (low frequencies), a few dry blades
                const float f = fbm(u, v, 4, seed, 5), dry = fbm(u, v, 2, seed + 5, 3), s = rnd(x, y, seed);
                const float k = 0.78f + (f - 0.5f) * 0.45f + (s - 0.5f) * 0.4f;
                const float d = std::clamp((dry - 0.45f) * 1.6f, 0.0f, 1.0f) * 0.6f + (s > 0.985f ? 0.5f : 0.0f);
                im.set(x, y, (0.25f + d * 0.16f) * k, (0.34f + d * 0.04f) * k, (0.15f + d * 0.02f) * k);
            }
        break;
    case kTexGravel:   // pebbles on a jittered grid
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x) {
                const float u = (x + 0.5f) / N * 40, v = (y + 0.5f) / N * 40;
                float best = 1e9f, second = 1e9f;
                int bi = 0, bj = 0;
                for (int j = -1; j <= 1; ++j)
                    for (int i = -1; i <= 1; ++i) {
                        const int ci = (int)std::floor(u) + i, cj = (int)std::floor(v) + j;
                        const int wi = ((ci % 40) + 40) % 40, wj = ((cj % 40) + 40) % 40;
                        const float px = ci + rnd(wi, wj, seed), py = cj + rnd(wi, wj, seed + 1);
                        const float d = (px - u) * (px - u) + (py - v) * (py - v);
                        if (d < best) { second = best; best = d; bi = wi; bj = wj; } else if (d < second) second = d;
                    }
                const float edge = std::sqrt(second) - std::sqrt(best);
                const float tone = 0.55f + rnd(bi, bj, seed + 2) * 0.35f;
                const float k = edge < 0.08f ? 0.35f : tone * (0.85f + std::min(edge, 0.5f) * 0.3f);
                im.set(x, y, k, k * 0.97f, k * 0.9f);
            }
        break;
    case kTexPaving:   // 50 cm slabs, rows offset by half
        for (int y = 0; y < N; ++y)
            for (int x = 0; x < N; ++x) {
                const float u = (x + 0.5f) / N * 4, v = (y + 0.5f) / N * 4;
                const int row = (int)v;
                const float uu = u + (row & 1) * 0.5f;
                const int col = (int)std::floor(uu);
                const float fu = uu - col, fv = v - row;
                const bool joint = fu < 0.012f || fv < 0.012f;
                const float k = 0.66f + (rnd(row, ((col % 4) + 4) % 4, seed) - 0.5f) * 0.1f + (fbm(u * 0.25f, v * 0.25f, 4, seed, 5) - 0.5f) * 0.12f;
                if (joint) im.set(x, y, 0.36f, 0.35f, 0.33f);
                else im.set(x, y, k, k * 0.98f, k * 0.94f);
            }
        break;
    }
    static const char* names[] = {"parquet", "tiles", "concrete", "carpet", "brick", "cladding", "grass", "gravel", "paving"};
    T.name = std::string("archviz:") + names[std::clamp(t, 0, kBuildTexCount - 1)];
    T.width = T.height = (uint32_t)N;
    T.rgba = std::move(im.px);
    return T;
}

} // namespace

float buildTexMetres(int t) {
    static const float m[] = {2.0f, 0.0f /* tiles: twice the tile */, 2.0f, 1.0f, 0.9f, 1.2f, 3.5f, 1.0f, 2.0f};
    return m[std::clamp(t, 0, kBuildTexCount - 1)];
}

int buildTexture(Scene& scene, int t) {
    static const char* names[] = {"parquet", "tiles", "concrete", "carpet", "brick", "cladding", "grass", "gravel", "paving"};
    const std::string name = std::string("archviz:") + names[std::clamp(t, 0, kBuildTexCount - 1)];
    for (size_t i = 0; i < scene.textures.size(); ++i)
        if (scene.textures[i].name == name) return (int)i;
    if (scene.textures.size() >= MAX_TEXTURES) return -1;
    return scene.addTexture(make(t));
}

} // namespace archviz
