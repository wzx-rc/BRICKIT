// BrickIt iPad backend (also the Linux test build): WZX Engine's Lit renderer and Dear ImGui drawn through Diligent
// (Metal through MoltenVK on iOS, Vulkan on Linux) behind brickit::Gfx. Raster only: the path tracing, GI and outline
// settings of RenderSettings are ignored (image quality is the only difference with the PC, see Gfx.h).
#pragma once
#include "Gfx.h"
#include "lit/LitRenderer.h"
#include "ImGuiDiligent.h"

#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#include <chrono>
#include <map>
#include <string>

namespace brickit {

class GfxRendererLit final : public GfxRenderer {
public:
    // swapChain null: headless, the back buffer is an offscreen texture of width x height (Linux tests).
    bool init(Diligent::IRenderDevice* device, Diligent::IDeviceContext* context, Diligent::IEngineFactory* factory,
              Diligent::ISwapChain* swapChain, uint32_t width, uint32_t height, Diligent::TEXTURE_FORMAT format,
              const std::string& shaderDirs, std::string* error);

    Diligent::IRenderDevice* device() const { return m_device; }
    Diligent::IDeviceContext* context() const { return m_ctx; }
    Diligent::TEXTURE_FORMAT format() const { return m_format; }
    Diligent::ITexture* backBuffer() const;
    Diligent::ITextureView* backBufferRTV() const;
    Diligent::ITextureView* viewportSRV() const;   // image of the last renderToTexture
    uint32_t drawCalls() const { return m_lit.drawCalls(); }

    void resize(uint32_t width, uint32_t height) override;
    void shutdown() override;
    void waitIdle() override;
    bool ready() const override { return m_device != nullptr; }
    uint32_t width() const override { return m_w; }
    uint32_t height() const override { return m_h; }
    void setExtraSelection(const std::vector<int>&) override {}
    void setEdgeOutline(const std::vector<int>&, bool, float) override {}
    void render(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, const vp3d::ViewRect& vp, int selectedObject,
                bool toBackBuffer = true) override;
    void renderToTexture(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, const vp3d::ViewRect& vp,
                         int selectedObject) override;
    bool readViewport(std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) override;
    bool readBackBuffer(std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) override;
    void present(bool vsync) override;
    void resetAccumulation() override {}
    uint32_t sampleCount() const override { return 1u << 20; }   // a raster image is complete at once
    float gpuFrameMs() const override { return m_ms; }
    uint64_t rasterTriangles() const override { return 0; }
    float giUpdateMs() const override { return 0; }
    float giBlendMs() const override { return 0; }
    bool pathTracing() const override { return false; }

private:
    void renderView(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, uint32_t w, uint32_t h);
    bool readTexture(Diligent::ITexture* tex, std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h);

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> m_device;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_ctx;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> m_swapChain;
    Diligent::RefCntAutoPtr<Diligent::ITexture> m_offscreen;   // headless back buffer
    Diligent::RefCntAutoPtr<Diligent::ITexture> m_view;        // the 3D image (viewport size)
    Diligent::TEXTURE_FORMAT m_format = Diligent::TEX_FORMAT_RGBA8_UNORM;
    ::lit::LitRenderer m_lit;
    uint32_t m_w = 0, m_h = 0;
    float m_ms = 0;
};

class GfxGuiLit final : public GfxGui {
public:
    // fontDir: Inter (text), DejaVu Sans (symbols), see assets_portable/fonts
    bool init(GfxRendererLit& renderer, float dpiScale, const std::string& fontDir, std::string* error);
    void shutdown() override;
    void newFrame() override;
    void render(bool viewportTextureInUse) override;
    void onResize() override {}
    ImTextureID viewportTexture() override;
    ImTextureID createTexture(const uint8_t* rgba, uint32_t w, uint32_t h) override;
    void releaseTexture(ImTextureID id) override;

private:
    GfxRendererLit* m_r = nullptr;
    player::ImGuiDiligent m_imgui;
    std::map<ImTextureID, Diligent::RefCntAutoPtr<Diligent::ITexture>> m_textures;
    std::chrono::steady_clock::time_point m_last;
    bool m_started = false;
};

} // namespace brickit
