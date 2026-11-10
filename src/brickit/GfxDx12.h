// BrickIt PC backend: WZX Engine's DirectX 12 renderer (vp3d::Renderer) and its Dear ImGui layer behind Gfx.h.
// Plain forwarding: BrickIt on PC renders exactly as before the iPad port.
#pragma once
#ifdef _WIN32
#include "Gfx.h"
#include <unordered_set>
#include "app/ImGuiLayer.h"
#include "vp3d/Renderer.h"

namespace brickit {

class GfxRendererDx12 final : public GfxRenderer {
public:
    vp3d::Renderer r;

    void resize(uint32_t w, uint32_t h) override { r.resize(w, h); }
    void shutdown() override { r.shutdown(); }
    void waitIdle() override { r.waitIdle(); }
    bool ready() const override { return r.device() != nullptr; }
    uint32_t width() const override { return r.width(); }
    uint32_t height() const override { return r.height(); }
    void setExtraSelection(const std::vector<int>& objects) override { r.setExtraSelection(objects); }
    void setEdgeOutline(const std::vector<int>& objects, bool red, float opacity) override { r.setEdgeOutline(objects, red, opacity); }
    void render(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, const vp3d::ViewRect& vp, int selectedObject,
                bool toBackBuffer = true) override { r.render(scene, rs, vp, selectedObject, toBackBuffer); }
    void renderToTexture(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, const vp3d::ViewRect& vp,
                         int selectedObject) override { r.renderToTexture(scene, rs, vp, selectedObject); }
    bool readViewport(std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) override { return r.readViewport(rgba, w, h); }
    bool readBackBuffer(std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) override { return r.readBackBuffer(rgba, w, h); }
    void present(bool vsync) override { r.present(vsync); }
    void resetAccumulation() override { r.resetAccumulation(); }
    uint32_t sampleCount() const override { return r.sampleCount(); }
    float gpuFrameMs() const override { return r.gpuFrameMs(); }
    uint64_t rasterTriangles() const override { return r.rasterTriangles(); }
    float giUpdateMs() const override { return r.giUpdateMs(); }
    float giBlendMs() const override { return r.giBlendMs(); }
    bool pathTracing() const override { return true; }
};

class GfxGuiDx12 final : public GfxGui {
public:
    app::ImGuiLayer l;

    bool init(HWND hwnd, GfxRendererDx12& renderer, float dpiScale) {
        if (!l.init(hwnd, renderer.r, dpiScale)) return false;
        sync();
        return true;
    }
    void shutdown() override { l.shutdown(); }
    void newFrame() override { l.beforeNewFrame = beforeNewFrame; l.newFrame(); sync(); }
    void render(bool viewportTextureInUse) override { l.render(viewportTextureInUse); }
    void onResize() override { l.onResize(); }
    ImTextureID viewportTexture() override { return l.viewportTexture(); }
    // the interface has 256 texture slots (engine ImGuiLayer), shared with the font atlas of ImGui, the viewport and the
    // views: the textures of BrickIt stop at 200, so that the font atlas, when it grows, always finds a free slot (a
    // full heap gave it a slot already taken: all the text garbled, then a hang)
    ImTextureID createTexture(const uint8_t* rgba, uint32_t w, uint32_t h) override {
        // (counted by their ids: a texture let go twice no longer lowers the count, which let it pass the limit; the limit
        // 170 leaves the font atlas of ImGui room to grow: a crash in ImGui_ImplDX12_UpdateTexture, Wzx 10/11/2026)
        if ((int)m_ids.size() >= 170) return (ImTextureID)0;
        const ImTextureID id = l.createTexture(rgba, w, h);
        if (id) m_ids.insert(id);
        return id;
    }
    void releaseTexture(ImTextureID id) override { if (id && m_ids.erase(id)) l.releaseTexture(id); }
    int liveTextures() const override { return (int)m_ids.size(); }

private:
    std::unordered_set<ImTextureID> m_ids;
    void sync() { fontBold = l.fontBold; fontSmall = l.fontSmall; dpi = l.dpi; }
};

} // namespace brickit
#endif
