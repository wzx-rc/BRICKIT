// BrickIt - PDF pages on the iPad (and macOS): CoreGraphics, for PdfPortable.cpp.
#include "Pdf.h"
#include "Platform.h"
#import <CoreGraphics/CoreGraphics.h>
#include <memory>

namespace brickit {

std::shared_ptr<void> pdfBackendOpen(const std::wstring& path, std::vector<std::pair<float, float>>& sizes, std::string& error) {
    const std::string p = plat::nativePath(path);
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(nullptr, (const UInt8*)p.c_str(), (CFIndex)p.size(), false);
    CGPDFDocumentRef doc = url ? CGPDFDocumentCreateWithURL(url) : nullptr;
    if (url) CFRelease(url);
    if (!doc) { error = "fichier PDF illisible"; return nullptr; }
    const size_t n = CGPDFDocumentGetNumberOfPages(doc);
    for (size_t i = 1; i <= n; ++i) {
        CGRect r = CGPDFPageGetBoxRect(CGPDFDocumentGetPage(doc, i), kCGPDFCropBox);
        const int rot = CGPDFPageGetRotationAngle(CGPDFDocumentGetPage(doc, i));
        if (rot % 180) std::swap(r.size.width, r.size.height);
        sizes.push_back({(float)r.size.width, (float)r.size.height});
    }
    return std::shared_ptr<void>((void*)doc, [](void* d) { CGPDFDocumentRelease((CGPDFDocumentRef)d); });
}

bool pdfBackendRender(void* d, int pageIndex, int width, PdfReader::Image& img) {
    CGPDFPageRef page = CGPDFDocumentGetPage((CGPDFDocumentRef)d, (size_t)pageIndex + 1);
    if (!page) return false;
    CGRect box = CGPDFPageGetBoxRect(page, kCGPDFCropBox);
    const int rot = CGPDFPageGetRotationAngle(page);
    const float pw = (float)(rot % 180 ? box.size.height : box.size.width), ph = (float)(rot % 180 ? box.size.width : box.size.height);
    if (pw <= 0 || ph <= 0) return false;
    img.w = width;
    img.h = std::max(1, (int)(width * ph / pw + 0.5f));
    img.rgba.assign((size_t)img.w * img.h * 4, 255);
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(img.rgba.data(), img.w, img.h, 8, (size_t)img.w * 4, cs,
                                             kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(cs);
    if (!ctx) return false;
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);   // white paper, as the Windows renderer
    CGContextFillRect(ctx, CGRectMake(0, 0, img.w, img.h));
    CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
    CGContextConcatCTM(ctx, CGPDFPageGetDrawingTransform(page, kCGPDFCropBox, CGRectMake(0, 0, img.w, img.h), 0, true));
    CGContextDrawPDFPage(ctx, page);
    CGContextRelease(ctx);
    return true;
}

} // namespace brickit
