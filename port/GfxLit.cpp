#include "GfxLit.h"
#include "imgui.h"

#include "Graphics/GraphicsTools/interface/MapHelper.hpp"

#include <algorithm>
#include <cstring>

using namespace Diligent;

namespace brickit {

// --------------------------------------------------------------------------- renderer

bool GfxRendererLit::init(IRenderDevice* device, IDeviceContext* context, IEngineFactory* factory, ISwapChain* swapChain,
                          uint32_t width, uint32_t height, TEXTURE_FORMAT format, const std::string& shaderDirs,
                          std::string* error) {
    m_device = device;
    m_ctx = context;
    m_swapChain = swapChain;
    m_format = swapChain ? swapChain->GetDesc().ColorBufferFormat : format;
    if (!m_lit.init(device, context, factory, shaderDirs, m_format, error)) return false;
    resize(swapChain ? swapChain->GetDesc().Width : width, swapChain ? swapChain->GetDesc().Height : height);
    return true;
}

void GfxRendererLit::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    m_w = width;
    m_h = height;
    if (m_swapChain) {
        const SwapChainDesc& d = m_swapChain->GetDesc();
        if (d.Width != width || d.Height != height) m_swapChain->Resize(width, height);
        return;
    }
    TextureDesc td;
    td.Name = "BrickIt back buffer";
    td.Type = RESOURCE_DIM_TEX_2D;
    td.Width = width;
    td.Height = height;
    td.Format = m_format;
    td.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    m_offscreen.Release();
    m_device->CreateTexture(td, nullptr, &m_offscreen);
}

ITexture* GfxRendererLit::backBuffer() const {
    if (m_swapChain) {
        ITextureView* v = m_swapChain->GetCurrentBackBufferRTV();
        return v ? v->GetTexture() : nullptr;
    }
    return m_offscreen;
}

ITextureView* GfxRendererLit::backBufferRTV() const {
    if (m_swapChain) return m_swapChain->GetCurrentBackBufferRTV();
    return m_offscreen ? m_offscreen->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) : nullptr;
}

ITextureView* GfxRendererLit::viewportSRV() const { return m_view ? m_view->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE) : nullptr; }

void GfxRendererLit::shutdown() {
    waitIdle();
    m_lit.shutdown();
    m_view.Release();
    m_offscreen.Release();
    m_swapChain.Release();
    m_ctx.Release();
    m_device.Release();
}

void GfxRendererLit::waitIdle() {
    if (m_ctx) m_ctx->WaitForIdle();
}

void GfxRendererLit::renderView(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, uint32_t w, uint32_t h) {
    w = std::max(1u, w);
    h = std::max(1u, h);
    if (!m_view || m_view->GetDesc().Width != w || m_view->GetDesc().Height != h) {
        TextureDesc td;
        td.Name = "BrickIt viewport";
        td.Type = RESOURCE_DIM_TEX_2D;
        td.Width = w;
        td.Height = h;
        td.Format = m_format;
        td.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
        m_view.Release();
        m_device->CreateTexture(td, nullptr, &m_view);
    }
    ::lit::Settings s;
    s.exposure = rs.exposure;
    s.vignette = rs.vignette;
    s.tonemapper = rs.tonemapper;
    const auto t0 = std::chrono::steady_clock::now();
    m_lit.render(scene, s, m_view->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET), w, h);
    m_ms = m_ms * 0.9f + 0.1f * std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

void GfxRendererLit::render(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, const vp3d::ViewRect& vp, int,
                            bool toBackBuffer) {
    renderView(scene, rs, (uint32_t)vp.w, (uint32_t)vp.h);
    if (!toBackBuffer) return;
    ITexture* bb = backBuffer();
    if (!bb) return;
    // the image into its rectangle of the back buffer (clipped)
    const int x0 = std::max(0, vp.x), y0 = std::max(0, vp.y);
    const int x1 = std::min((int)m_w, vp.x + vp.w), y1 = std::min((int)m_h, vp.y + vp.h);
    if (x1 <= x0 || y1 <= y0) return;
    Box src(Uint32(x0 - vp.x), Uint32(x1 - vp.x), Uint32(y0 - vp.y), Uint32(y1 - vp.y));
    CopyTextureAttribs c(m_view, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, bb, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    c.pSrcBox = &src;
    c.DstX = (Uint32)x0;
    c.DstY = (Uint32)y0;
    m_ctx->CopyTexture(c);
}

void GfxRendererLit::renderToTexture(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, const vp3d::ViewRect& vp, int) {
    renderView(scene, rs, (uint32_t)vp.w, (uint32_t)vp.h);
    StateTransitionDesc b(m_view, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE);
    m_ctx->TransitionResourceStates(1, &b);
}

bool GfxRendererLit::readTexture(ITexture* tex, std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) {
    if (!tex) return false;
    const TextureDesc& d = tex->GetDesc();
    TextureDesc sd;
    sd.Name = "BrickIt readback";
    sd.Type = RESOURCE_DIM_TEX_2D;
    sd.Width = d.Width;
    sd.Height = d.Height;
    sd.Format = d.Format;
    sd.Usage = USAGE_STAGING;
    sd.CPUAccessFlags = CPU_ACCESS_READ;
    RefCntAutoPtr<ITexture> staging;
    m_device->CreateTexture(sd, nullptr, &staging);
    if (!staging) return false;
    CopyTextureAttribs c(tex, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, staging, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_ctx->CopyTexture(c);
    m_ctx->WaitForIdle();
    MappedTextureSubresource m;
    m_ctx->MapTextureSubresource(staging, 0, 0, MAP_READ, MAP_FLAG_DO_NOT_WAIT, nullptr, m);
    if (!m.pData) return false;
    w = d.Width;
    h = d.Height;
    rgba.resize((size_t)w * h * 4);
    const bool bgra = d.Format == TEX_FORMAT_BGRA8_UNORM || d.Format == TEX_FORMAT_BGRA8_UNORM_SRGB;
    for (uint32_t y = 0; y < h; ++y) {
        const uint8_t* s = (const uint8_t*)m.pData + (size_t)y * m.Stride;
        uint8_t* o = &rgba[(size_t)y * w * 4];
        std::memcpy(o, s, (size_t)w * 4);
        if (bgra)
            for (uint32_t x = 0; x < w; ++x) std::swap(o[x * 4], o[x * 4 + 2]);
    }
    m_ctx->UnmapTextureSubresource(staging, 0, 0);
    return true;
}

bool GfxRendererLit::readViewport(std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) { return readTexture(m_view, rgba, w, h); }

bool GfxRendererLit::readBackBuffer(std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) {
    return readTexture(backBuffer(), rgba, w, h);
}

void GfxRendererLit::present(bool vsync) {
    if (m_swapChain) m_swapChain->Present(vsync ? 1 : 0);
    else m_ctx->Flush();
}

// --------------------------------------------------------------------------- Dear ImGui

bool GfxGuiLit::init(GfxRendererLit& renderer, float dpiScale, const std::string& fontDir, std::string* error) {
    m_r = &renderer;
    dpi = dpiScale;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // as app::ImGuiLayer (PC)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = nullptr;
    io.BackendPlatformName = "brickit_lit";

    // Fonts: the PC uses Segoe UI (+ Segoe UI Symbol), not redistributable; Inter + DejaVu Sans here, same sizes.
    const float size = 15.0f * dpi;
    auto addFont = [&](const char* file, float px) -> ImFont* {
        static const ImWchar textRange[] = {0x0020, 0x017F, 0x2000, 0x206F, 0x2190, 0x22FF, 0x25A0, 0x27BF, 0};
        ImFont* f = io.Fonts->AddFontFromFileTTF((fontDir + "/" + file).c_str(), px, nullptr, textRange);
        if (!f) return nullptr;
        ImFontConfig sc;
        sc.MergeMode = true;
        io.Fonts->AddFontFromFileTTF((fontDir + "/DejaVuSans.ttf").c_str(), px, &sc, textRange);
        return f;
    };
    if (!addFont("Inter-Regular.otf", size)) io.Fonts->AddFontDefault();
    fontBold = addFont("Inter-SemiBold.otf", size);
    fontSmall = addFont("Inter-Regular.otf", 13.0f * dpi);
    if (!fontBold) fontBold = io.Fonts->Fonts[0];
    if (!fontSmall) fontSmall = io.Fonts->Fonts[0];
    return m_imgui.init(renderer.device(), renderer.context(), renderer.format(), error);
}

void GfxGuiLit::shutdown() {
    m_textures.clear();
    m_imgui.shutdown();
    if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
}

void GfxGuiLit::newFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)m_r->width(), (float)m_r->height());
    const auto now = std::chrono::steady_clock::now();
    io.DeltaTime = m_started ? std::max(1e-4f, std::chrono::duration<float>(now - m_last).count()) : 1.0f / 60.0f;
    m_last = now;
    m_started = true;
    if (beforeNewFrame) beforeNewFrame();
    ImGui::NewFrame();
}

void GfxGuiLit::render(bool) {
    ImGui::Render();
    ITextureView* rtv = m_r->backBufferRTV();
    if (!rtv) return;
    IDeviceContext* ctx = m_r->context();
    ctx->SetRenderTargets(1, &rtv, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    const float clear[4] = {0x13 / 255.0f, 0x14 / 255.0f, 0x17 / 255.0f, 1.0f};   // the dark stage behind the panels
    ctx->ClearRenderTarget(rtv, clear, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_imgui.render(ImGui::GetDrawData(), rtv);
}

ImTextureID GfxGuiLit::viewportTexture() { return (ImTextureID)(uintptr_t)m_r->viewportSRV(); }

ImTextureID GfxGuiLit::createTexture(const uint8_t* rgba, uint32_t w, uint32_t h) {
    if (!rgba || !w || !h) return (ImTextureID)0;
    TextureDesc td;
    td.Name = "BrickIt UI image";
    td.Type = RESOURCE_DIM_TEX_2D;
    td.Width = w;
    td.Height = h;
    td.Format = TEX_FORMAT_RGBA8_UNORM;
    td.BindFlags = BIND_SHADER_RESOURCE;
    td.Usage = USAGE_IMMUTABLE;
    TextureSubResData sub;
    sub.pData = rgba;
    sub.Stride = (Uint64)w * 4;
    TextureData data(&sub, 1);
    RefCntAutoPtr<ITexture> tex;
    m_r->device()->CreateTexture(td, &data, &tex);
    if (!tex) return (ImTextureID)0;
    const ImTextureID id = (ImTextureID)(uintptr_t)tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    m_textures[id] = tex;
    return id;
}

void GfxGuiLit::releaseTexture(ImTextureID id) { m_textures.erase(id); }

} // namespace brickit
