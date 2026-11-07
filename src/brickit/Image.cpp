#include "Image.h"
#ifdef _WIN32
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace brickit {
using Microsoft::WRL::ComPtr;

namespace image {

bool decode(const uint8_t* data, size_t size, std::vector<uint8_t>& rgba, int& w, int& h) {
    static ComPtr<IWICImagingFactory> fac;
    if (!fac) {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fac)))) return false;
    }
    ComPtr<IWICStream> st;
    ComPtr<IWICBitmapDecoder> dec;
    ComPtr<IWICBitmapFrameDecode> fr;
    ComPtr<IWICFormatConverter> cv;
    if (FAILED(fac->CreateStream(&st)) || FAILED(st->InitializeFromMemory((BYTE*)data, (DWORD)size))) return false;
    if (FAILED(fac->CreateDecoderFromStream(st.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &dec)) || FAILED(dec->GetFrame(0, &fr))) return false;
    if (FAILED(fac->CreateFormatConverter(&cv)) || FAILED(cv->Initialize(fr.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom))) return false;
    UINT uw = 0, uh = 0;
    cv->GetSize(&uw, &uh);
    w = (int)uw;
    h = (int)uh;
    rgba.resize((size_t)uw * uh * 4);
    return SUCCEEDED(cv->CopyPixels(nullptr, uw * 4, (UINT)rgba.size(), rgba.data()));
}

} // namespace image

bool encodePng(const std::vector<uint8_t>& rgba, uint32_t w, uint32_t h, std::vector<uint8_t>& out) {
    ComPtr<IWICImagingFactory> f;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&f)))) return false;
    ComPtr<IStream> mem;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &mem))) return false;
    ComPtr<IWICBitmapEncoder> enc;
    f->CreateEncoder(GUID_ContainerFormatPng, nullptr, &enc);
    if (!enc || FAILED(enc->Initialize(mem.Get(), WICBitmapEncoderNoCache))) return false;
    ComPtr<IWICBitmapFrameEncode> fr;
    ComPtr<IPropertyBag2> props;
    enc->CreateNewFrame(&fr, &props);
    fr->Initialize(props.Get());
    fr->SetSize(w, h);
    WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppRGBA;
    fr->SetPixelFormat(&fmt);
    std::vector<uint8_t> px = rgba;
    if (fmt != GUID_WICPixelFormat32bppRGBA)
        for (size_t i = 0; i + 3 < px.size(); i += 4) std::swap(px[i], px[i + 2]);
    fr->WritePixels(h, w * 4, (UINT)px.size(), px.data());
    fr->Commit();
    enc->Commit();
    STATSTG st = {};
    mem->Stat(&st, STATFLAG_NONAME);
    out.resize((size_t)st.cbSize.QuadPart);
    LARGE_INTEGER zero = {};
    mem->Seek(zero, STREAM_SEEK_SET, nullptr);
    ULONG got = 0;
    mem->Read(out.data(), (ULONG)out.size(), &got);
    return got == out.size();
}

} // namespace brickit

#else
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"
#include "stb_image_write.h"
#include <cstring>

namespace brickit {
namespace image {

bool decode(const uint8_t* data, size_t size, std::vector<uint8_t>& rgba, int& w, int& h) {
    int n = 0;
    stbi_uc* p = stbi_load_from_memory(data, (int)size, &w, &h, &n, 4);
    if (!p) return false;
    rgba.assign(p, p + (size_t)w * h * 4);
    stbi_image_free(p);
    return true;
}

} // namespace image

static void appendBytes(void* ctx, void* data, int size) {
    auto* out = (std::vector<uint8_t>*)ctx;
    out->insert(out->end(), (uint8_t*)data, (uint8_t*)data + size);
}

bool encodePng(const std::vector<uint8_t>& rgba, uint32_t w, uint32_t h, std::vector<uint8_t>& out) {
    out.clear();
    return stbi_write_png_to_func(appendBytes, &out, (int)w, (int)h, 4, rgba.data(), (int)w * 4) != 0 && !out.empty();
}

} // namespace brickit
#endif
