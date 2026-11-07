// BrickIt - image files in memory: PNG / JPEG ... -> RGBA8, RGBA8 -> PNG.
// Windows: WIC (as before). Other systems (iPad, Linux test build): stb_image / stb_image_write.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace brickit {
namespace image {
bool decode(const uint8_t* data, size_t size, std::vector<uint8_t>& rgba, int& w, int& h);
} // namespace image
bool encodePng(const std::vector<uint8_t>& rgba, uint32_t w, uint32_t h, std::vector<uint8_t>& out);
} // namespace brickit
