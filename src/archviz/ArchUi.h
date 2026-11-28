// ArchViz - the palette and the small helpers shared by the interface files (entre-deux, light).
#pragma once
#include "imgui.h"
#include <algorithm>

namespace archviz {

static const ImU32 kInk = IM_COL32(31, 39, 51, 255), kInk2 = IM_COL32(106, 115, 131, 255), kInk3 = IM_COL32(154, 162, 175, 255);
static const ImU32 kAccent = IM_COL32(47, 107, 255, 255), kAccentSoft = IM_COL32(47, 107, 255, 40);
static const ImU32 kDraw = IM_COL32(240, 138, 28, 255), kOk = IM_COL32(35, 120, 65, 255);
static const ImU32 kLine = IM_COL32(225, 229, 235, 255), kHollow = IM_COL32(237, 240, 244, 255);
static const ImU32 kWhite = IM_COL32(255, 255, 255, 255);

inline bool inRect(ImVec2 p, const ImVec4& r) { return r.z > r.x && p.x >= r.x && p.y >= r.y && p.x < r.z && p.y < r.w; }
inline ImU32 alpha(ImU32 c, float a) { return (c & 0x00FFFFFF) | ((ImU32)(std::clamp(a, 0.0f, 1.0f) * ((c >> 24) & 255)) << 24); }

// ArchApp.cpp: a row of segment buttons (cur: the one on), a value in metres typed (Enter), clamped
bool segButtons(const char* id, const char* const* items, int n, int& cur, float ui, ImFont* bold);
bool fieldM(const char* label, float* v, float ui, float lo = 0.01f, float hi = 100.0f);

} // namespace archviz
