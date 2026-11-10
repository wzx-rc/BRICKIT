// BrickIt - Voxel Editor (mode DEV, maquette 5.0 validated by Wzx 10/11/2026): the connectors of a part placed by hand,
// assisted. The part alone in X-ray; an assistant that reads its mesh (studs, flat bottom, round axles and holes) and
// proposes voxels; a magnetic placement (detected shapes, the half-stud grid, the type from the face); the tools Ligne
// d'axe, Remplir une face, Miroir, Gomme; « Valider » saves the voxels (<data>/voxel_edits.json), labels the part green
// and opens the next one to do (red first, then orange, in the list of the library).
// Included by BrickUi.cpp after BrickUiStudio.inl (uses its helpers).

float acesInverse(float target8, float exposure);   // BrickPreview.cpp

static ImU32 veCol(int t, int a = 255) {
    switch (t) {
    case CONN_STUD: return IM_COL32(0xf5, 0xc3, 0x2c, a);
    case CONN_ANTISTUD: return IM_COL32(0xff, 0x8a, 0x3d, a);
    case CONN_PIN: case CONN_PINHOLE: return IM_COL32(0x43, 0xc6, 0xff, a);
    default: return IM_COL32(0xe0, 0x5c, 0xff, a);
    }
}
static bool veHollow(int t) { return t == CONN_PINHOLE || t == CONN_AXLEHOLE || t == CONN_ANTISTUD; }
static const char* veName(int t) {
    static const char* n[6] = {"Tenon", "Anti-tenon", "Pin", "Trou de pin", "Axe", "Trou d'axe"};
    return t >= 0 && t < 6 ? n[t] : "?";
}
// the marker of a voxel: filled (male), ringed (female), its colour
static void veMark(ImDrawList* dl, ImVec2 p, float r, int t, int alpha, bool dashed, float s) {
    const ImU32 c = veCol(t, alpha);
    if (dashed) {
        for (int k = 0; k < 12; k += 2) dl->PathArcTo(p, r, k * 0.5236f, (k + 1) * 0.5236f, 4), dl->PathStroke(c, 0, 1.4f * s);
        dl->AddCircleFilled(p, r * 0.55f, veCol(t, alpha / 4), 16);
        return;
    }
    if (veHollow(t)) {
        dl->AddCircleFilled(p, r, IM_COL32(10, 11, 14, alpha * 3 / 4), 18);
        dl->AddCircle(p, r - 0.8f * s, c, 18, 2.2f * s);
    } else {
        dl->AddCircleFilled(p, r, c, 18);
        dl->AddCircle(p, r, IM_COL32(255, 255, 255, alpha * 3 / 4), 18, 1.2f * s);
    }
}

void BrickApp::loadVoxEdits() {
    std::string t;
    const std::wstring d = brickitDataDir();
    if (d.empty()) return;
    FILE* f = nullptr;
    if (_wfopen_s(&f, (d + L"\\voxel_edits.json").c_str(), L"rb") != 0 || !f) return;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) t.append(buf, n);
    fclose(f);
    app::Json j;
    if (!app::Json::parse(t, j)) return;
    for (const auto& [id, e] : j.o) {
        auto& ov = m_lib.voxEdits[id];
        ov.first.clear();
        ov.second.clear();
        for (const app::Json& v : e["v"].a) ov.first.push_back(float3((float)v[0].num(), (float)v[1].num(), (float)v[2].num()));
        for (const app::Json& c : e["t"].a) ov.second.push_back((uint8_t)std::clamp((int)c.num(), 0, 5));
        if (ov.second.size() != ov.first.size()) ov.second.assign(ov.first.size(), CONN_STUD);
        const int i = m_lib.find(id);
        if (i >= 0) m_lib.applyVoxEdit(m_lib.parts[i]);
    }
}
void BrickApp::saveVoxEdits() {
    const std::wstring d = brickitDataDir();
    if (d.empty()) return;
    std::string j = "{";
    bool first = true;
    char b[64];
    for (const auto& [id, ov] : m_lib.voxEdits) {
        j += (first ? "\n  \"" : ",\n  \"") + id + "\": {\"v\": [";
        first = false;
        for (size_t k = 0; k < ov.first.size(); ++k) {
            snprintf(b, sizeof(b), "%s[%.4f, %.4f, %.4f]", k ? ", " : "", ov.first[k].x, ov.first[k].y, ov.first[k].z);
            j += b;
        }
        j += "], \"t\": [";
        for (size_t k = 0; k < ov.second.size(); ++k) j += (k ? ", " : "") + std::to_string((int)ov.second[k]);
        j += "]}";
    }
    j += "\n}\n";
    FILE* f = nullptr;
    if (_wfopen_s(&f, (d + L"\\voxel_edits.json").c_str(), L"wb") != 0 || !f) return;
    fwrite(j.data(), 1, j.size(), f);
    fclose(f);
}

void BrickApp::openVoxelEditor(int part) {
    if (part < 0 || part >= (int)m_lib.parts.size() || !m_lib.get(m_lib.parts[part].name)) return;
    const Part& p = m_lib.parts[part];
    m_vePart = part;
    m_veVox = p.voxels;
    m_veConn = p.conn;
    if (m_veConn.size() != m_veVox.size()) m_veConn.assign(m_veVox.size(), CONN_STUD);
    m_veUndo.clear();
    m_veDirty = false;
    m_veSel = -1;
    m_veLineA = false;
    m_veView = defaultThumbView();
    m_veObj = -1;
    m_veCtx = -1;
    veAnalyze();
}

// ---- the assistant: what the mesh shows
// rings of vertices (the ends of the cylinders: studs, tubes, axles, pins, holes), found plane by plane on the 3 axes
struct VeRing { int axis; float at; float u, v, r; bool outward; };
static std::vector<VeRing> veRings(const vp3d::MeshData& m) {
    std::vector<VeRing> out;
    const size_t n = m.vertices.size();
    if (n < 8 || n > 400000) return out;
    for (int ax = 0; ax < 3; ++ax) {
        const int a1 = (ax + 1) % 3, a2 = (ax + 2) % 3;
        std::unordered_map<int, std::vector<uint32_t>> planes;
        for (uint32_t i = 0; i < n; ++i) {
            const float3& p = m.vertices[i].pos;
            planes[(int)std::lround((&p.x)[ax] / 0.004f)].push_back(i);
        }
        for (auto& [key, ids] : planes) {
            if (ids.size() < 8) continue;
            // the points of the plane in clusters (a grid of 0.12 cm, neighbours closer than 0.12)
            std::unordered_map<long long, std::vector<uint32_t>> grid;
            auto cell = [](float x) { return (long long)std::floor(x / 0.12f); };
            for (uint32_t i : ids) { const float3& p = m.vertices[i].pos; grid[(cell((&p.x)[a1]) << 32) ^ (cell((&p.x)[a2]) & 0xffffffff)].push_back(i); }
            std::unordered_map<uint32_t, int> lab;
            int nl = 0;
            for (uint32_t i0 : ids) {
                if (lab.count(i0)) continue;
                std::vector<uint32_t> st{i0}, cl;
                lab[i0] = nl;
                while (!st.empty() && cl.size() < 4000) {
                    const uint32_t i = st.back();
                    st.pop_back();
                    cl.push_back(i);
                    const float3& p = m.vertices[i].pos;
                    const long long cu = cell((&p.x)[a1]), cv = cell((&p.x)[a2]);
                    for (long long du = -1; du <= 1; ++du)
                        for (long long dv = -1; dv <= 1; ++dv) {
                            auto it = grid.find(((cu + du) << 32) ^ ((cv + dv) & 0xffffffff));
                            if (it == grid.end()) continue;
                            for (uint32_t j : it->second) {
                                if (lab.count(j)) continue;
                                const float3& q = m.vertices[j].pos;
                                const float d1 = (&q.x)[a1] - (&p.x)[a1], d2 = (&q.x)[a2] - (&p.x)[a2];
                                if (d1 * d1 + d2 * d2 < 0.12f * 0.12f) { lab[j] = nl; st.push_back(j); }
                            }
                        }
                }
                ++nl;
                if (cl.size() < 8) continue;
                // the centre: a circle fitted on the points (Kasa: a part of a circle too, the 3/4 holes of LDraw)
                double S[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
                double mu = 0, mv = 0;
                for (uint32_t i : cl) { mu += (&m.vertices[i].pos.x)[a1]; mv += (&m.vertices[i].pos.x)[a2]; }
                mu /= cl.size();
                mv /= cl.size();
                for (uint32_t i : cl) {
                    const double x = (&m.vertices[i].pos.x)[a1] - mu, y = (&m.vertices[i].pos.x)[a2] - mv, z = -(x * x + y * y);
                    const double r3[3] = {x, y, 1};
                    for (int a = 0; a < 3; ++a) { for (int b = 0; b < 3; ++b) S[a][b] += r3[a] * r3[b]; S[a][3] += r3[a] * z; }
                }
                for (int c = 0; c < 3; ++c) {   // (Gauss)
                    int piv = c;
                    for (int r = c + 1; r < 3; ++r) if (std::fabs(S[r][c]) > std::fabs(S[piv][c])) piv = r;
                    for (int k = 0; k < 4; ++k) std::swap(S[c][k], S[piv][k]);
                    if (std::fabs(S[c][c]) < 1e-12) break;
                    for (int r = 0; r < 3; ++r) if (r != c) { const double f = S[r][c] / S[c][c]; for (int k = 0; k < 4; ++k) S[r][k] -= f * S[c][k]; }
                }
                if (std::fabs(S[0][0]) < 1e-12 || std::fabs(S[1][1]) < 1e-12) continue;
                const float cu = (float)(mu - S[0][3] / S[0][0] * 0.5), cv = (float)(mv - S[1][3] / S[1][1] * 0.5);
                float rm = 0, outw = 0;
                for (uint32_t i : cl) {
                    const float3& p = m.vertices[i].pos;
                    const float du = (&p.x)[a1] - cu, dv = (&p.x)[a2] - cv;
                    rm += std::sqrt(du * du + dv * dv);
                    const float3& nn = m.vertices[i].nrm;
                    outw += (&nn.x)[a1] * du + (&nn.x)[a2] * dv;
                }
                rm /= cl.size();
                if (rm < 0.1f || rm > 0.3f) continue;
                float dev = 0;
                for (uint32_t i : cl) {
                    const float3& p = m.vertices[i].pos;
                    const float du = (&p.x)[a1] - cu, dv = (&p.x)[a2] - cv;
                    dev = std::max(dev, std::fabs(std::sqrt(du * du + dv * dv) - rm));
                }
                if (dev > 0.008f) continue;   // (a ring: every point at the same distance)
                out.push_back({ax, key * 0.004f, cu, cv, rm, outw > 0});
            }
        }
    }
    return out;
}

void BrickApp::veAnalyze() {
    m_veSugs.clear();
    if (m_vePart < 0) return;
    const Part& p = m_lib.parts[m_vePart];
    const vp3d::MeshData& m = p.mesh;
    auto veNear = [&](const std::vector<float3>& set, const float3& q) { for (const float3& v : set) if (length(v - q) < 0.05f) return true; return false; };
    auto pt = [](int ax, float at, float u, float v) { float3 r; (&r.x)[ax] = at; (&r.x)[(ax + 1) % 3] = u; (&r.x)[(ax + 2) % 3] = v; return r; };
    const std::vector<VeRing> rings = veRings(m);
    VeSug studs{"Tenons", "", {}, {}}, axles{"Axes ronds Ø 8", "", {}, {}}, axleHoles{"Trous ronds Ø 8", "", {}, {}}, pins{"Pins Ø 12", "", {}, {}}, pinHoles{"Trous de pin Ø 12", "", {}, {}};
    // studs: two rings r 0.24 one above the other 0.16 apart (the top and the base of a stud), outward
    for (const VeRing& a : rings) {
        if (a.axis != 1 || !a.outward || std::fabs(a.r - 0.24f) > 0.025f) continue;
        for (const VeRing& b : rings)
            if (b.axis == 1 && b.outward && std::fabs(b.r - a.r) < 0.02f && std::fabs(b.u - a.u) < 0.01f && std::fabs(b.v - a.v) < 0.01f && std::fabs(a.at - b.at - 0.16f) < 0.02f) {
                const float3 q = pt(1, b.at, a.u, a.v);
                if (!veNear(studs.v, q)) { studs.v.push_back(q); studs.t.push_back(CONN_STUD); }
            }
    }
    // cylinders: the rings of one line (same axis, same centre, same radius, same side) give one segment from the first to
    // the last; Ø 8 (r 0.16): round axles / holes, Ø 12 (r 0.24, not vertical: the studs): pins / pin holes
    {
        // a line: an axle (outward) from its first ring to its last; holes (inward) cut where the rings are apart (two
        // walls with a hole each: one voxel per wall, nothing in the air between)
        struct Line { int axis; float u, v; bool r8, outward; std::vector<float> at; };
        std::vector<Line> found;
        for (const VeRing& a : rings) {
            const bool r8 = std::fabs(a.r - 0.16f) < 0.025f, r12 = std::fabs(a.r - 0.24f) < 0.025f;
            if (!r8 && !r12) continue;
            if (r12 && a.axis == 1) continue;
            Line* l = nullptr;
            for (Line& q : found)
                if (q.axis == a.axis && q.r8 == r8 && q.outward == a.outward && std::fabs(q.u - a.u) < 0.01f && std::fabs(q.v - a.v) < 0.01f) { l = &q; break; }
            if (!l) { found.push_back({a.axis, a.u, a.v, r8, a.outward, {}}); l = &found.back(); }
            l->at.push_back(a.at);
        }
        struct Seg { int axis; float u, v; bool r8, outward; float lo, hi; int n; };
        std::vector<Seg> lines;
        for (Line& l : found) {
            std::sort(l.at.begin(), l.at.end());
            size_t i0 = 0;
            for (size_t i = 1; i <= l.at.size(); ++i)
                if (i == l.at.size() || (!l.outward && l.at[i] - l.at[i - 1] > 0.25f)) {
                    lines.push_back({l.axis, l.u, l.v, l.r8, l.outward, l.at[i0], l.at[i - 1], (int)(i - i0)});
                    i0 = i;
                }
        }
        for (Seg& l : lines) {
            if (l.n < 2) continue;
            if (l.hi - l.lo < 0.2f) l.lo = l.hi = (l.lo + l.hi) * 0.5f;   // (a hole through a thin wall: one voxel in its middle)
            VeSug& sg = l.r8 ? (l.outward ? axles : axleHoles) : (l.outward ? pins : pinHoles);
            const uint8_t t = l.r8 ? (l.outward ? CONN_AXLE : CONN_AXLEHOLE) : (l.outward ? CONN_PIN : CONN_PINHOLE);
            const int nst = l.hi - l.lo < 0.01f ? 0 : std::max(1, (int)std::lround((l.hi - l.lo) / 0.4f));
            for (int k = 0; k <= nst; ++k) {
                const float3 q = pt(l.axis, nst ? l.lo + (l.hi - l.lo) * k / nst : l.lo, l.u, l.v);
                if (!veNear(sg.v, q)) { sg.v.push_back(q); sg.t.push_back(t); }
            }
        }
    }
    // anti-studs: the cells of the stud grid covered by a flat face of the bottom
    VeSug anti{"Anti-tenons", "", {}, {}};
    {
        std::vector<std::array<float3, 3>> flat;
        for (size_t k = 0; k + 2 < m.indices.size(); k += 3) {
            const float3 &A = m.vertices[m.indices[k]].pos, &B = m.vertices[m.indices[k + 1]].pos, &C = m.vertices[m.indices[k + 2]].pos;
            if (A.y - p.bmin.y > 0.02f || B.y - p.bmin.y > 0.02f || C.y - p.bmin.y > 0.02f) continue;
            flat.push_back({A, B, C});
        }
        auto inTri = [](float x, float z, const std::array<float3, 3>& t) {
            auto d = [&](const float3& a, const float3& b) { return (x - b.x) * (a.z - b.z) - (a.x - b.x) * (z - b.z); };
            const float d1 = d(t[0], t[1]), d2 = d(t[1], t[2]), d3 = d(t[2], t[0]);
            return !((d1 < -1e-7f || d2 < -1e-7f || d3 < -1e-7f) && (d1 > 1e-7f || d2 > 1e-7f || d3 > 1e-7f));
        };
        if (!flat.empty())
            for (float cx = p.bmin.x + 0.4f; cx < p.bmax.x; cx += 0.8f)
                for (float cz = p.bmin.z + 0.4f; cz < p.bmax.z; cz += 0.8f) {
                    bool cov = false;
                    for (const auto& t : flat) {
                        for (int a = -4; a <= 4 && !cov; ++a)
                            for (int b = -4; b <= 4 && !cov; ++b) cov = inTri(cx + a * 0.0975f, cz + b * 0.0975f, t);
                        if (cov) break;
                    }
                    if (cov) { anti.v.push_back(float3(cx, p.bmin.y, cz)); anti.t.push_back(CONN_ANTISTUD); }
                }
    }
    char b[96];
    for (VeSug* sg : {&studs, &anti, &axles, &axleHoles, &pins, &pinHoles}) {
        for (size_t k = sg->v.size(); k-- > 0;)   // (what is already placed is not proposed again)
            if (veNear(m_veVox, sg->v[k])) { sg->v.erase(sg->v.begin() + k); sg->t.erase(sg->t.begin() + k); }
        if (sg->v.empty()) continue;
        snprintf(b, sizeof(b), "%d voxel%s · %s", (int)sg->v.size(), sg->v.size() > 1 ? "s" : "", sg == &anti ? "sous les faces plates du dessous" : sg == &studs ? "cylindres Ø 12 sur le dessus" : "le long des cylindres, tous les ½ tenon");
        sg->detail = b;
        m_veSugs.push_back(*sg);
    }
    // symmetry of the mesh: the mirror on X / Z switched on by itself
    m_veMirror = 0;
    if (!m.vertices.empty()) {
        std::unordered_set<long long> hs;
        auto key = [](float x, float y, float z) { return ((long long)std::lround(x * 100) * 73856093LL) ^ ((long long)std::lround(y * 100) * 19349663LL) ^ ((long long)std::lround(z * 100) * 83492791LL); };
        for (const auto& v : m.vertices) hs.insert(key(v.pos.x, v.pos.y, v.pos.z));
        const float cx = (p.bmin.x + p.bmax.x), cz = (p.bmin.z + p.bmax.z);
        int okx = 0, okz = 0, tot = 0;
        for (size_t i = 0; i < m.vertices.size(); i += std::max<size_t>(1, m.vertices.size() / 400), ++tot) {
            const float3& q = m.vertices[i].pos;
            okx += hs.count(key(cx - q.x, q.y, q.z)) ? 1 : 0;
            okz += hs.count(key(q.x, q.y, cz - q.z)) ? 1 : 0;
        }
        if (tot && okx > tot * 0.97f) m_veMirror |= 1;
        if (tot && okz > tot * 0.97f) m_veMirror |= 2;
    }
    // similar parts already right (LDCad or validated): the same size, to copy from
    m_veSimilar.clear();
    if (p.hasSizeCm)
        for (int i = 0; i < (int)m_lib.parts.size() && m_veSimilar.size() < 3; ++i) {
            const Part& q = m_lib.parts[i];
            if (i == m_vePart || !q.hasSizeCm || (q.connSrc != 1 && q.connSrc != 4)) continue;
            if (std::fabs(q.sizeCm[0] - p.sizeCm[0]) + std::fabs(q.sizeCm[1] - p.sizeCm[1]) + std::fabs(q.sizeCm[2] - p.sizeCm[2]) > 0.03f || q.category != p.category) continue;
            m_veSimilar.push_back(i);
        }
}

void BrickApp::vePush() {
    m_veUndo.push_back({m_veVox, m_veConn});
    if (m_veUndo.size() > 100) m_veUndo.erase(m_veUndo.begin());
    m_veDirty = true;
}
// a voxel added (and its mirrors), not twice at the same place
void BrickApp::veAdd(const float3& q, uint8_t t) {
    const Part& p = m_lib.parts[m_vePart];
    std::vector<float3> pts{q};
    const float cx = p.bmin.x + p.bmax.x, cz = p.bmin.z + p.bmax.z;
    if (m_veMirror & 1) pts.push_back(float3(cx - q.x, q.y, q.z));
    if (m_veMirror & 2) pts.push_back(float3(q.x, q.y, cz - q.z));
    if ((m_veMirror & 3) == 3) pts.push_back(float3(cx - q.x, q.y, cz - q.z));
    for (const float3& v : pts) {
        bool dup = false;
        for (const float3& w : m_veVox) if (length(w - v) < 0.03f) { dup = true; break; }
        if (!dup) { m_veVox.push_back(float3(std::round(v.x * 1e4f) / 1e4f, std::round(v.y * 1e4f) / 1e4f, std::round(v.z * 1e4f) / 1e4f)); m_veConn.push_back(t); }
    }
}
// the voxels kept: in the part (the scene builds with them at once), in voxel_edits.json; validated: green, saved label
void BrickApp::veSave(bool validate) {
    if (m_vePart < 0) return;
    Part& p = m_lib.parts[m_vePart];
    if (m_veDirty || validate) {
        m_lib.voxEdits[p.name] = {m_veVox, m_veConn};
        m_lib.applyVoxEdit(p);
        saveVoxEdits();
        m_veDirty = false;
    }
    if (validate) {
        p.connSrc = 4;
        saveConnLabels();
    }
}
// the next part to do in the list of the library: red first, then orange
int BrickApp::veNextPart() const {
    const std::vector<int>& list = m_stParts;
    for (int pass = 3; pass >= 2; --pass) {
        int start = 0;
        for (int k = 0; k < (int)list.size(); ++k) if (list[k] == m_vePart) start = k + 1;
        for (int n = 0; n < (int)list.size(); ++n) {
            const int i = list[(start + n) % list.size()];
            if (i != m_vePart && m_lib.parts[i].connSrc == pass) return i;
        }
    }
    return -1;
}

// the viewer: the main renderer draws the part (X-ray: the glass material) into the viewport texture
void BrickApp::renderVoxelEditor() {
    if (m_vePart < 0) return;
    if (!m_veBuilt) {
        m_veScene = Scene();
        m_veMat = setupStudio2(m_veScene, m_veGlass);
        m_veScene.geometryVersion = 0x5a000000u;
        m_veBuilt = true;
        m_veObj = -1;
    }
    const Part& p = m_lib.parts[m_vePart];
    if (m_veObj < 0 || m_veScene.objects[m_veObj].name != p.name || m_veXrayShown != m_veXray) {
        m_veScene.meshes.clear();
        m_veScene.objects.clear();
        Object o;
        o.name = p.name;
        o.mesh = m_veScene.addMesh(p.name, p.mesh);
        o.material = m_veXray ? m_veGlass : m_veMat;
        m_veObj = m_veScene.addObject(o);
        m_veXrayShown = m_veXray;
        ++m_veScene.geometryVersion;
    }
    const float bg[3] = {acesInverse(0x16, 1.12f), acesInverse(0x18, 1.12f), acesInverse(0x1d, 1.12f)};
    m_veScene.env.top = m_veScene.env.horizon = m_veScene.env.bottom = float3(bg[0], bg[1], bg[2]);
    fitThumbCamera(m_veScene.camera, p, m_veView);
    placeThumbLights(m_veScene, p, m_veView.light);
    ++m_veScene.objectsVersion;
    RenderSettings rs = thumbSettings();
    rs.pixelScale = 1;
    const int w = (int)std::max(64.0f, m_veRect.z - m_veRect.x), h = (int)std::max(64.0f, m_veRect.w - m_veRect.y);
    m_renderer->renderToTexture(m_veScene, rs, ViewRect{0, 0, w, h}, -1);
}

// a button of the editor (tracked capitals), returns true when clicked
static bool veBtn(Ui& u, ImVec2 a, const char* t, float h, int style, float s, float* wOut = nullptr) {
    const float w = stTrackW(u, 8.5f, t, 1.2f * s, true) + 24 * s;
    if (wOut) *wOut = w;
    const ImVec2 b(a.x + w, a.y + h);
    const bool hv = u.hover(a, b);
    const ImU32 bg = style == 2 ? IM_COL32(0x8e, 0xe0, 0x00, 255) : style == 1 ? C_ACCENT : IM_COL32(255, 255, 255, hv ? 22 : 10);
    u.dl->AddRectFilled(a, b, bg, 9 * s);
    if (!style) u.dl->AddRect(a, b, IM_COL32(255, 255, 255, hv ? 90 : 46), 9 * s, 0, 1.0f);
    else if (hv) u.dl->AddRect(a, b, IM_COL32(255, 255, 255, 160), 9 * s, 0, 1.0f);
    stTrack(u, ImVec2(a.x + 12 * s, (a.y + b.y) * 0.5f - u.fs(8.5f) * 0.52f), 8.5f, style ? IM_COL32(0x14, 0x16, 0x10, 255) : C_TEXT, t, 1.2f * s);
    return hv && u.pressed;
}

void BrickApp::uiVoxelEditor() {
    if (m_vePart < 0) return;
    if (m_vePart >= (int)m_lib.parts.size() || !m_lib.parts[m_vePart].loaded) { m_vePart = -1; return; }
    Ui u = ui(m_ui);
    if (m_gui->fontBold && !g_thBold) u.bold = m_gui->fontBold;
    const float s = m_ui;
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    ImGuiIO& io = ImGui::GetIO();
    Part& P = m_lib.parts[m_vePart];
    u.dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(8, 9, 11, 246));
    m_uiRects.push_back(ImVec4(0, 0, d.x, d.y));
    const bool typing = io.WantTextInput;
    const bool ctrl = io.KeyCtrl;
    // ================= header
    const ImVec2 ha(20 * s, 16 * s), hb(d.x - 20 * s, 16 * s + 58 * s);
    stGlass(u.dl, ha, hb, 16 * s, s);
    float x = ha.x + 16 * s;
    const float hy = (ha.y + hb.y) * 0.5f;
    stTrack(u, ImVec2(x, hy - u.fs(10.5f) * 0.52f), 10.5f, C_TEXT, "VOXEL EDITOR", 3 * s);
    x += stTrackW(u, 10.5f, "VOXEL EDITOR", 3 * s, true) + 16 * s;
    u.dl->AddLine(ImVec2(x, hy - 13 * s), ImVec2(x, hy + 13 * s), IM_COL32(255, 255, 255, 30), 1.0f);
    x += 16 * s;
    {
        std::string id = P.name;
        if (id.size() > 4 && id.substr(id.size() - 4) == ".dat") id = id.substr(0, id.size() - 4);
        std::string t = id + " · " + stUpper(partFr(P.label));
        t = stFit(u, 9.5f, t, d.x * 0.32f, true);
        stTrack(u, ImVec2(x, hy - 14 * s), 9.5f, C_TEXT, t.c_str(), 1.4f * s);
        char sub[160];
        snprintf(sub, sizeof(sub), "%s · %s · %d voxel%s", catFr(P.category), stConnName(P.connSrc), (int)m_veVox.size(), m_veVox.size() > 1 ? "s" : "");
        u.text(ImVec2(x, hy + 3 * s), 9.5f, C_DIM, sub);
        x += std::max(stTrackW(u, 9.5f, t.c_str(), 1.4f * s, true), u.textW(9.5f, sub)) + 18 * s;
        // the status: its colour
        const char* st = P.connSrc == 4 ? "Validée" : P.connSrc == 1 ? "LDCad" : P.connSrc == 2 ? "Auto" : "À traiter";
        const float sw = u.textW(9.5f, st) + 30 * s;
        const ImVec2 sa(x, hy - 11 * s), sb(x + sw, hy + 11 * s);
        u.dl->AddRect(sa, sb, stConnCol(P.connSrc), 11 * s, 0, 1.5f * s);
        u.dl->AddRect(ImVec2(sa.x + 9 * s, hy - 4 * s), ImVec2(sa.x + 17 * s, hy + 4 * s), stConnCol(P.connSrc), 2 * s, 0, 2 * s);
        u.text(ImVec2(sa.x + 22 * s, hy - u.fs(9.5f) * 0.55f), 9.5f, C_TEXT, st);
    }
    // the buttons, from the right
    float bx = hb.x - 14 * s, bw = 0;
    const float bh = 28 * s, by = hy - bh * 0.5f;
    {
        stTrackW(u, 8.5f, "VALIDER · V", 1.2f * s, true);
        bw = stTrackW(u, 8.5f, "VALIDER · V", 1.2f * s, true) + 24 * s;
        if (veBtn(u, ImVec2(bx - bw, by), "VALIDER · V", bh, 2, s) || (!typing && !ctrl && ImGui::IsKeyPressed(ImGuiKey_V))) {
            veSave(true);
            toast(std::string("Voxels validés : ") + P.name);
            const int nx = veNextPart();
            if (nx >= 0) openVoxelEditor(nx); else m_vePart = -1;
            return;
        }
        bx -= bw + 8 * s;
        bw = stTrackW(u, 8.5f, "SUIVANTE À TRAITER ›", 1.2f * s, true) + 24 * s;
        if (veBtn(u, ImVec2(bx - bw, by), "SUIVANTE À TRAITER ›", bh, 0, s) || (!typing && !ctrl && ImGui::IsKeyPressed(ImGuiKey_N))) {
            veSave(false);
            const int nx = veNextPart();
            if (nx >= 0) openVoxelEditor(nx); else toast("Plus de pièce rouge ni orange dans cette liste");
            return;
        }
        bx -= bw + 8 * s;
        bw = stTrackW(u, 8.5f, "FERMER", 1.2f * s, true) + 24 * s;
        if (veBtn(u, ImVec2(bx - bw, by), "FERMER", bh, 0, s) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            veSave(false);   // (what was placed is kept, the label does not change)
            m_vePart = -1;
            return;
        }
        bx -= bw + 16 * s;
        // the steps
        const bool hasSug = !m_veSugs.empty();
        const char* steps[3] = {"1 · DÉTECTER", "2 · CORRIGER", "3 · VALIDER"};
        const int cur = hasSug ? 0 : (P.connSrc == 4 && !m_veDirty ? 2 : 1);
        for (int k = 2; k >= 0; --k) {
            const float w = stTrackW(u, 8, steps[k], 1.2f * s, true) + 20 * s;
            const ImVec2 a(bx - w, hy - 12 * s), b(bx, hy + 12 * s);
            if (bx - w < x + 40 * s) break;
            u.dl->AddRectFilled(a, b, k == cur ? stA(C_ACCENT, 0.16f) : IM_COL32(255, 255, 255, 0), 12 * s);
            u.dl->AddRect(a, b, k == cur ? stA(C_ACCENT, 0.6f) : IM_COL32(255, 255, 255, 36), 12 * s, 0, 1.0f);
            stTrack(u, ImVec2(a.x + 10 * s, hy - u.fs(8) * 0.52f), 8, k == cur ? C_ACCENT_TXT : C_DIM, steps[k], 1.2f * s);
            bx -= w + 6 * s;
        }
    }
    const float top = hb.y + 16 * s, bottomH = 118 * s, bot = d.y - 20 * s - bottomH - 16 * s;
    // ================= tools (left)
    const ImVec2 la(20 * s, top), lb(20 * s + 220 * s, bot);
    stGlass(u.dl, la, lb, 16 * s, s);
    float ly = la.y + 14 * s;
    stTrack(u, ImVec2(la.x + 14 * s, ly), 7.5f, C_DIM, "OUTILS", 1.6f * s);
    ly += 18 * s;
    struct T { const char* name; const char* key; ImGuiKey k; };
    static const T tools[5] = {{"Sélection", "Q", ImGuiKey_Q}, {"Placer (aimanté)", "W", ImGuiKey_W}, {"Ligne d'axe", "L", ImGuiKey_L}, {"Remplir une face", "G", ImGuiKey_G}, {"Gomme", "E", ImGuiKey_E}};
    for (int k = 0; k < 5; ++k) {
        const ImVec2 a(la.x + 8 * s, ly), b(lb.x - 8 * s, ly + 30 * s);
        const bool on = m_veTool == k, h = u.hover(a, b);
        if (on) { u.dl->AddRectFilled(a, b, stA(C_ACCENT, 0.14f), 9 * s); u.dl->AddRect(a, b, stA(C_ACCENT, 0.55f), 9 * s, 0, 1.0f); }
        else if (h) u.dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 10), 9 * s);
        u.text(ImVec2(a.x + 12 * s, (a.y + b.y) * 0.5f - u.fs(11) * 0.55f), 11, on ? C_ACCENT_TXT : C_TEXT, tools[k].name);
        const float kw = u.textW(9, tools[k].key);
        u.dl->AddRect(ImVec2(b.x - kw - 18 * s, a.y + 7 * s), ImVec2(b.x - 8 * s, b.y - 7 * s), IM_COL32(255, 255, 255, 40), 4 * s);
        u.text(ImVec2(b.x - kw - 13 * s, (a.y + b.y) * 0.5f - u.fs(9) * 0.55f), 9, C_DIM, tools[k].key);
        if ((h && u.pressed) || (!typing && !ctrl && !io.KeyAlt && ImGui::IsKeyPressed(tools[k].k))) {
            m_veTool = k;
            m_veLineA = false;
            if (k == 1) m_veAutoType = true;
            if (k == 2 && m_veAutoType) m_veType = CONN_AXLE;
        }
        ly += 32 * s;
    }
    {   // the mirror: X, Z
        ly += 4 * s;
        u.text(ImVec2(la.x + 20 * s, ly + 6 * s), 11, C_TEXT, "Miroir");
        const char* ax[2] = {"X", "Z"};
        for (int k = 0; k < 2; ++k) {
            const ImVec2 a(lb.x - 8 * s - (2 - k) * 38 * s, ly), b(a.x + 34 * s, ly + 26 * s);
            const bool on = (m_veMirror >> k) & 1, h = u.hover(a, b);
            u.dl->AddRectFilled(a, b, on ? stA(C_ACCENT, 0.9f) : IM_COL32(255, 255, 255, h ? 18 : 8), 8 * s);
            u.text(ImVec2((a.x + b.x - u.textW(10, ax[k])) * 0.5f, (a.y + b.y) * 0.5f - u.fs(10) * 0.55f), 10, on ? C_INK : C_DIM, ax[k]);
            if (h && u.pressed) m_veMirror ^= 1 << k;
        }
        if (!typing && !ctrl && ImGui::IsKeyPressed(ImGuiKey_M)) m_veMirror = (m_veMirror + 1) & 3;
        ly += 36 * s;
    }
    stTrack(u, ImVec2(la.x + 14 * s, ly + 4 * s), 7.5f, C_DIM, "TYPE DE VOXEL", 1.6f * s);
    ly += 22 * s;
    {
        const ImVec2 a(la.x + 8 * s, ly), b(lb.x - 8 * s, ly + 26 * s);
        const bool h = u.hover(a, b);
        if (m_veAutoType) { u.dl->AddRectFilled(a, b, stA(C_ACCENT, 0.14f), 8 * s); u.dl->AddRect(a, b, stA(C_ACCENT, 0.55f), 8 * s, 0, 1.0f); }
        else if (h) u.dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 10), 8 * s);
        u.text(ImVec2(a.x + 12 * s, (a.y + b.y) * 0.5f - u.fs(10.5f) * 0.55f), 10.5f, m_veAutoType ? C_ACCENT_TXT : C_TEXT, "Auto (selon la forme)");
        if (h && u.pressed) m_veAutoType = !m_veAutoType;
        ly += 28 * s;
    }
    static const ImGuiKey nk[6] = {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5, ImGuiKey_6};
    for (int k = 0; k < 6; ++k) {
        const ImVec2 a(la.x + 8 * s, ly), b(lb.x - 8 * s, ly + 26 * s);
        const bool on = !m_veAutoType && m_veType == k, h = u.hover(a, b);
        if (on) u.dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 18), 8 * s), u.dl->AddRect(a, b, IM_COL32(255, 255, 255, 70), 8 * s, 0, 1.0f);
        else if (h) u.dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 10), 8 * s);
        veMark(u.dl, ImVec2(a.x + 18 * s, (a.y + b.y) * 0.5f), 5.5f * s, k, 255, false, s);
        u.text(ImVec2(a.x + 32 * s, (a.y + b.y) * 0.5f - u.fs(10.5f) * 0.55f), 10.5f, C_TEXT, veName(k));
        char kk[4];
        snprintf(kk, sizeof(kk), "%d", k + 1);
        u.text(ImVec2(b.x - 16 * s, (a.y + b.y) * 0.5f - u.fs(9) * 0.55f), 9, C_DIM, kk);
        const bool key = !typing && !ctrl && ImGui::IsKeyPressed(nk[k]);
        if ((h && u.pressed) || key) {
            m_veType = k;
            m_veAutoType = false;
            if (m_veSel >= 0 && m_veSel < (int)m_veConn.size() && m_veConn[m_veSel] != k) { vePush(); m_veConn[m_veSel] = (uint8_t)k; }
        }
        ly += 27 * s;
    }
    {   // X-ray
        ly += 6 * s;
        const ImVec2 a(la.x + 8 * s, ly), b(lb.x - 8 * s, ly + 26 * s);
        const bool h = u.hover(a, b);
        u.text(ImVec2(a.x + 12 * s, (a.y + b.y) * 0.5f - u.fs(10.5f) * 0.55f), 10.5f, C_TEXT, "X-ray (pièce en verre)");
        const ImVec2 sa(b.x - 40 * s, a.y + 5 * s), sb(b.x - 8 * s, b.y - 5 * s);
        u.dl->AddRectFilled(sa, sb, m_veXray ? stA(C_ACCENT, 0.85f) : IM_COL32(255, 255, 255, 30), 8 * s);
        u.dl->AddCircleFilled(ImVec2(m_veXray ? sb.x - 8 * s : sa.x + 8 * s, (sa.y + sb.y) * 0.5f), 6 * s, m_veXray ? C_INK : IM_COL32(220, 222, 226, 255), 16);
        if (h && u.pressed) m_veXray = !m_veXray;
    }
    // ================= assistant (right)
    const ImVec2 ra(d.x - 20 * s - 340 * s, top), rb(d.x - 20 * s, bot);
    stGlass(u.dl, ra, rb, 16 * s, s);
    float ry = ra.y + 14 * s;
    stIcon(u.dl, "M12 3l2.2 5.8L20 11l-5.8 2.2L12 19l-2.2-5.8L4 11l5.8-2.2z", ImVec2(ra.x + 22 * s, ry + 7 * s), 16 * s, C_ACCENT_TXT, 1.4f * s);
    stTrack(u, ImVec2(ra.x + 36 * s, ry + 7 * s - u.fs(8) * 0.52f), 8, C_TEXT, "ASSISTANT", 1.8f * s);
    if (!m_veSugs.empty()) {
        float w = 0;
        stTrackW(u, 8.5f, "TOUT ACCEPTER · ENTRÉE", 1.2f * s, true);
        w = stTrackW(u, 8.5f, "TOUT ACCEPTER · ENTRÉE", 1.2f * s, true) + 24 * s;
        if (veBtn(u, ImVec2(rb.x - 14 * s - w, ry - 5 * s), "TOUT ACCEPTER · ENTRÉE", 24 * s, 1, s) || (!typing && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
            vePush();
            const int mir = m_veMirror;
            m_veMirror = 0;
            for (const VeSug& sg : m_veSugs) for (size_t k = 0; k < sg.v.size(); ++k) veAdd(sg.v[k], sg.t[k]);
            m_veMirror = mir;
            m_veSugs.clear();
        }
    }
    ry += 32 * s;
    stTrack(u, ImVec2(ra.x + 14 * s, ry), 7.5f, C_DIM, "TROUVÉ DANS LE MAILLAGE", 1.6f * s);
    ry += 18 * s;
    if (m_veSugs.empty()) {
        u.text(ImVec2(ra.x + 14 * s, ry), 10.5f, C_DIM, "Rien de plus à proposer.");
        ry += 22 * s;
    }
    for (size_t i = 0; i < m_veSugs.size() && ry < rb.y - 160 * s; ++i) {
        VeSug& sg = m_veSugs[i];
        const ImVec2 a(ra.x + 10 * s, ry), b(rb.x - 10 * s, ry + 52 * s);
        const bool h = u.hover(a, b);
        u.dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, h ? 12 : 6), 10 * s);
        u.dl->AddRect(a, b, IM_COL32(255, 255, 255, 26), 10 * s, 0, 1.0f);
        veMark(u.dl, ImVec2(a.x + 16 * s, a.y + 17 * s), 5.5f * s, sg.t.empty() ? 0 : sg.t[0], 255, false, s);
        u.text(ImVec2(a.x + 30 * s, a.y + 9 * s), 11, C_TEXT, sg.label.c_str(), true);
        u.text(ImVec2(a.x + 14 * s, a.y + 30 * s), 9.5f, C_DIM, sg.detail.c_str());
        float w = 0;
        stTrackW(u, 8.5f, "ACCEPTER", 1.2f * s, true);
        w = stTrackW(u, 8.5f, "ACCEPTER", 1.2f * s, true) + 24 * s;
        if (veBtn(u, ImVec2(b.x - 10 * s - w, a.y + 6 * s), "ACCEPTER", 22 * s, 0, s)) {
            vePush();
            const int mir = m_veMirror;
            m_veMirror = 0;
            for (size_t k = 0; k < sg.v.size(); ++k) veAdd(sg.v[k], sg.t[k]);
            m_veMirror = mir;
            m_veSugs.erase(m_veSugs.begin() + i);
            break;
        }
        if (h) m_veSugHot = (int)i;
        ry += 58 * s;
    }
    if (!m_veSimilar.empty()) {
        ry += 6 * s;
        stTrack(u, ImVec2(ra.x + 14 * s, ry), 7.5f, C_DIM, "PIÈCES PROCHES (MÊME TAILLE)", 1.6f * s);
        ry += 18 * s;
        for (int i : m_veSimilar) {
            if (ry > rb.y - 80 * s) break;
            const Part& q = m_lib.parts[i];
            const ImVec2 a(ra.x + 10 * s, ry), b(rb.x - 10 * s, ry + 40 * s);
            u.dl->AddRect(ImVec2(a.x + 6 * s, a.y + 8 * s), ImVec2(a.x + 30 * s, b.y - 8 * s), stConnCol(q.connSrc), 5 * s, 0, 1.5f * s);
            std::string nm = stFit(u, 10, q.name + " · " + partFr(q.label), b.x - a.x - 150 * s, false);
            u.text(ImVec2(a.x + 38 * s, (a.y + b.y) * 0.5f - u.fs(10) * 0.55f), 10, C_TEXT, nm.c_str());
            float w = stTrackW(u, 8.5f, "COPIER", 1.2f * s, true) + 24 * s;
            if (veBtn(u, ImVec2(b.x - 6 * s - w, a.y + 8 * s), "COPIER", 24 * s, 0, s)) {
                Part* src = m_lib.get(q.name);
                if (src) {
                    vePush();
                    m_veVox = src->voxels;
                    m_veConn = src->conn;
                    if (m_veConn.size() != m_veVox.size()) m_veConn.assign(m_veVox.size(), CONN_STUD);
                    veAnalyze();
                }
            }
            ry += 44 * s;
        }
    }
    {   // the symmetry
        const char* t = m_veMirror == 3 ? "Symétrie X et Z : chaque voxel posé se recopie" : m_veMirror == 1 ? "Symétrie X : chaque voxel posé se recopie de l'autre côté"
                      : m_veMirror == 2 ? "Symétrie Z : chaque voxel posé se recopie de l'autre côté" : "Pas de symétrie (Miroir à gauche pour l'activer)";
        stTrack(u, ImVec2(ra.x + 14 * s, rb.y - 58 * s), 7.5f, C_DIM, "SYMÉTRIE", 1.6f * s);
        u.text(ImVec2(ra.x + 14 * s, rb.y - 40 * s), 10, m_veMirror ? C_TEXT : C_DIM, t);
    }
    // ================= the viewport
    const ImVec2 va(lb.x + 16 * s, top), vb(ra.x - 16 * s, bot);
    m_veRect = ImVec4(va.x, va.y, vb.x, vb.y);
    if (m_frozenTex) u.dl->AddImageRounded(m_gui->viewportTexture(), va, vb, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 14 * s);
    u.dl->AddRect(va, vb, IM_COL32(255, 255, 255, 22), 14 * s, 0, 1.0f);
    View v;
    {
        Camera cam;
        fitThumbCamera(cam, P, m_veView);
        v.cam = cam;
        v.ox = va.x;
        v.oy = va.y;
        v.w = std::max(1.0f, vb.x - va.x);
        v.h = std::max(1.0f, vb.y - va.y);
    }
    const bool overV = u.hover(va, vb);
    // navigation: Alt + left or the right button turn, the middle button moves, the wheel zooms
    if (overV && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle) || (io.KeyAlt && ImGui::IsMouseClicked(ImGuiMouseButton_Left)))) m_veNav = true;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)) m_veNav = false;
    if (m_veNav) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            m_veView.panX -= io.MouseDelta.x / (v.w * 0.5f) / m_veView.zoom;
            m_veView.panY += io.MouseDelta.y / (v.h * 0.5f) / m_veView.zoom;
        } else {
            m_veView.yaw -= io.MouseDelta.x * 0.01f;
            m_veView.pitch = std::clamp(m_veView.pitch + io.MouseDelta.y * 0.01f, radians(-89.0f), radians(89.0f));
        }
    }
    if (overV && io.MouseWheel != 0) m_veView.zoom = std::clamp(m_veView.zoom * std::pow(1.1f, io.MouseWheel), 0.3f, 8.0f);
    // the hover: a ray on the part, the magnet (a proposed voxel, the half-stud grid), the type from the face
    bool hit = false;
    float3 hp, hn;
    int hotVox = -1;
    if (overV && !m_veNav) {
        float3 o, dir;
        v.ray(u.mouse.x, u.mouse.y, o, dir);
        float best = 1e30f;
        const auto& M = P.mesh;
        for (size_t k = 0; k + 2 < M.indices.size(); k += 3) {
            const float3 &A = M.vertices[M.indices[k]].pos, &B = M.vertices[M.indices[k + 1]].pos, &C = M.vertices[M.indices[k + 2]].pos;
            const float3 e1 = B - A, e2 = C - A, pv = cross(dir, e2);
            const float det = dot(e1, pv);
            if (std::fabs(det) < 1e-12f) continue;
            const float inv = 1.0f / det;
            const float3 tv = o - A;
            const float uu = dot(tv, pv) * inv;
            if (uu < 0 || uu > 1) continue;
            const float3 qv = cross(tv, e1);
            const float vv = dot(dir, qv) * inv;
            if (vv < 0 || uu + vv > 1) continue;
            const float t = dot(e2, qv) * inv;
            if (t > 1e-5f && t < best) { best = t; hn = normalize(cross(e1, e2)); }
        }
        if (best < 1e30f) {
            hit = true;
            hp = o + dir * best;
            if (dot(hn, dir) > 0) hn = hn * -1.0f;
        }
        float bd = 13 * s;
        for (size_t k = 0; k < m_veVox.size(); ++k) {
            float sx, sy;
            if (!v.project(m_veVox[k], sx, sy)) continue;
            const float dd = std::hypot(sx - u.mouse.x, sy - u.mouse.y);
            if (dd < bd) { bd = dd; hotVox = (int)k; }
        }
    }
    float3 snapP;
    uint8_t snapT = (uint8_t)m_veType;
    std::string why;
    bool snapOk = false;
    if (overV && !m_veNav) {
        float bd = 18 * s;   // the magnet: a voxel proposed by the assistant
        for (const VeSug& sg : m_veSugs)
            for (size_t k = 0; k < sg.v.size(); ++k) {
                float sx, sy;
                if (!v.project(sg.v[k], sx, sy)) continue;
                const float dd = std::hypot(sx - u.mouse.x, sy - u.mouse.y);
                if (dd < bd) { bd = dd; snapP = sg.v[k]; snapT = sg.t[k]; why = "AIMANT · " + stUpper(sg.label); snapOk = true; }
            }
        if (!snapOk && hit) {   // the half-stud grid on the face, the type from its side
            const Part& p = P;
            snapP = hp;
            const bool upF = hn.y > 0.7f, downF = hn.y < -0.7f;
            if (!upF && !downF) {   // a side: the grid in the plane of the face
                snapP.y = std::round(hp.y / 0.04f) * 0.04f;
            } else snapP.y = hp.y;
            snapP.x = p.bmin.x + std::round((hp.x - p.bmin.x) / 0.4f) * 0.4f;
            snapP.z = p.bmin.z + std::round((hp.z - p.bmin.z) / 0.4f) * 0.4f;
            if (std::fabs(hn.x) > 0.7f) snapP.x = hp.x;
            if (std::fabs(hn.z) > 0.7f) snapP.z = hp.z;
            if (m_veAutoType) snapT = upF ? CONN_STUD : downF ? CONN_ANTISTUD : (uint8_t)m_veType;
            why = upF ? "GRILLE · FACE DU DESSUS" : downF ? "GRILLE · FACE DU DESSOUS" : "GRILLE · FACE DE CÔTÉ";
            snapOk = true;
        }
    }
    if (!m_veAutoType) snapT = (uint8_t)m_veType;
    // the actions of the tools
    const bool click = overV && !m_veNav && !io.KeyAlt && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (click) {
        if (m_veTool == 0) m_veSel = hotVox;
        else if (m_veTool == 4) { if (hotVox >= 0) { vePush(); m_veVox.erase(m_veVox.begin() + hotVox); m_veConn.erase(m_veConn.begin() + hotVox); m_veSel = -1; } }
        else if (m_veTool == 1 && snapOk) { vePush(); veAdd(snapP, snapT); }
        else if (m_veTool == 2 && snapOk) {
            if (!m_veLineA) { m_veLineA = true; m_veLineP = snapP; }
            else {
                vePush();
                const float L = length(snapP - m_veLineP);
                const int n = std::max(1, (int)std::lround(L / 0.4f));
                for (int k = 0; k <= n; ++k) veAdd(m_veLineP + (snapP - m_veLineP) * ((float)k / n), m_veAutoType ? (uint8_t)CONN_AXLE : (uint8_t)m_veType);
                m_veLineA = false;
            }
        } else if (m_veTool == 3 && hit) {   // the face clicked, filled: the stud grid over its flat faces (axis-aligned)
            const int ax = std::fabs(hn.y) > 0.95f ? 1 : std::fabs(hn.x) > 0.95f ? 0 : std::fabs(hn.z) > 0.95f ? 2 : -1;
            if (ax >= 0) {
                const float at = (&hp.x)[ax];
                const int a1 = (ax + 1) % 3, a2 = (ax + 2) % 3;
                std::vector<std::array<float3, 3>> flat;
                const auto& M = P.mesh;
                for (size_t k = 0; k + 2 < M.indices.size(); k += 3) {
                    const float3 &A = M.vertices[M.indices[k]].pos, &B = M.vertices[M.indices[k + 1]].pos, &C = M.vertices[M.indices[k + 2]].pos;
                    if (std::fabs((&A.x)[ax] - at) > 0.01f || std::fabs((&B.x)[ax] - at) > 0.01f || std::fabs((&C.x)[ax] - at) > 0.01f) continue;
                    flat.push_back({A, B, C});
                }
                auto inTri = [&](float x1, float x2, const std::array<float3, 3>& t) {
                    auto dd = [&](const float3& a, const float3& b) { return (x1 - (&b.x)[a1]) * ((&a.x)[a2] - (&b.x)[a2]) - ((&a.x)[a1] - (&b.x)[a1]) * (x2 - (&b.x)[a2]); };
                    const float d1 = dd(t[0], t[1]), d2 = dd(t[1], t[2]), d3 = dd(t[2], t[0]);
                    return !((d1 < -1e-7f || d2 < -1e-7f || d3 < -1e-7f) && (d1 > 1e-7f || d2 > 1e-7f || d3 > 1e-7f));
                };
                const uint8_t t = m_veAutoType ? (ax == 1 ? (hn.y > 0 ? (uint8_t)CONN_STUD : (uint8_t)CONN_ANTISTUD) : (uint8_t)m_veType) : (uint8_t)m_veType;
                vePush();
                const float s1 = ax == 1 ? 0.8f : 0.4f, s2 = 0.8f;   // (sides: a hole every half stud along, every stud across)
                for (float c1 = (&P.bmin.x)[a1] + s1 * 0.5f; c1 < (&P.bmax.x)[a1]; c1 += s1)
                    for (float c2 = (&P.bmin.x)[a2] + s2 * 0.5f; c2 < (&P.bmax.x)[a2]; c2 += s2) {
                        bool in = false;
                        for (const auto& tr : flat) if ((in = inTri(c1, c2, tr))) break;
                        if (!in) continue;
                        float3 q;
                        (&q.x)[ax] = at;
                        (&q.x)[a1] = c1;
                        (&q.x)[a2] = c2;
                        veAdd(q, t);
                    }
            }
        }
    }
    if (!typing && (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) && m_veSel >= 0 && m_veSel < (int)m_veVox.size()) {
        vePush();
        m_veVox.erase(m_veVox.begin() + m_veSel);
        m_veConn.erase(m_veConn.begin() + m_veSel);
        m_veSel = -1;
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !m_veUndo.empty()) {
        m_veVox = m_veUndo.back().first;
        m_veConn = m_veUndo.back().second;
        m_veUndo.pop_back();
        m_veSel = -1;
        m_veDirty = true;
    }
    // the drawing in the view: the voxels (far ones fainter), the proposed ones dashed, the line, the cursor
    u.dl->PushClipRect(va, vb, true);
    {
        const float3 cpos = v.cam.eye();
        std::vector<std::pair<float, int>> order;
        for (size_t k = 0; k < m_veVox.size(); ++k) order.push_back({-length(m_veVox[k] - cpos), (int)k});
        std::sort(order.begin(), order.end());
        for (const VeSug& sg : m_veSugs)
            for (size_t k = 0; k < sg.v.size(); ++k) {
                float sx, sy;
                if (v.project(sg.v[k], sx, sy)) veMark(u.dl, ImVec2(sx, sy), 6 * s, sg.t[k], 230, true, s);
            }
        for (auto& [dd, k] : order) {
            float sx, sy;
            if (!v.project(m_veVox[k], sx, sy)) continue;
            veMark(u.dl, ImVec2(sx, sy), (k == hotVox ? 7.5f : 6.0f) * s, m_veConn[k], 255, false, s);
            if (k == m_veSel) u.dl->AddCircle(ImVec2(sx, sy), 11 * s, C_ACCENT, 20, 2 * s);
            if (k == hotVox && m_veTool == 4) u.dl->AddCircle(ImVec2(sx, sy), 11 * s, IM_COL32(255, 59, 48, 255), 20, 2 * s);
        }
        if (m_veTool == 2 && m_veLineA) {
            float ax0, ay0, bx0, by0;
            if (v.project(m_veLineP, ax0, ay0)) {
                u.dl->AddCircleFilled(ImVec2(ax0, ay0), 3 * s, C_ACCENT, 12);
                if (snapOk && v.project(snapP, bx0, by0)) {
                    const float L = std::hypot(bx0 - ax0, by0 - ay0);
                    for (float t = 0; t < L; t += 8 * s) u.dl->AddLine(ImVec2(ax0 + (bx0 - ax0) * t / L, ay0 + (by0 - ay0) * t / L), ImVec2(ax0 + (bx0 - ax0) * std::min(L, t + 3 * s) / L, ay0 + (by0 - ay0) * std::min(L, t + 3 * s) / L), C_ACCENT, 1.5f * s);
                }
            }
        }
        if (snapOk && (m_veTool == 1 || m_veTool == 2) && hotVox < 0) {   // the cursor: the snapped point, its type
            float sx, sy;
            if (v.project(snapP, sx, sy)) {
                u.dl->AddCircle(ImVec2(sx, sy), 14 * s, C_ACCENT, 24, 1.5f * s);
                u.dl->AddLine(ImVec2(sx - 21 * s, sy), ImVec2(sx - 15 * s, sy), C_ACCENT, 1.5f * s);
                u.dl->AddLine(ImVec2(sx + 15 * s, sy), ImVec2(sx + 21 * s, sy), C_ACCENT, 1.5f * s);
                u.dl->AddLine(ImVec2(sx, sy - 21 * s), ImVec2(sx, sy - 15 * s), C_ACCENT, 1.5f * s);
                u.dl->AddLine(ImVec2(sx, sy + 15 * s), ImVec2(sx, sy + 21 * s), C_ACCENT, 1.5f * s);
                veMark(u.dl, ImVec2(sx, sy), 5 * s, snapT, 200, false, s);
                const std::string t2 = std::string("type : ") + veName(snapT) + (m_veMirror ? " · miroir" : "");
                const float tw = std::max(stTrackW(u, 7.6f, why.c_str(), 1.4f * s, true), u.textW(9.5f, t2.c_str())) + 22 * s;
                const ImVec2 ta(sx + 22 * s, sy - 42 * s), tb(ta.x + tw, ta.y + 38 * s);
                u.dl->AddRectFilled(ta, tb, IM_COL32(10, 11, 14, 225), 9 * s);
                u.dl->AddRect(ta, tb, IM_COL32(255, 255, 255, 36), 9 * s, 0, 1.0f);
                stTrack(u, ImVec2(ta.x + 11 * s, ta.y + 7 * s), 7.6f, C_ACCENT_TXT, why.c_str(), 1.4f * s);
                u.text(ImVec2(ta.x + 11 * s, ta.y + 20 * s), 9.5f, C_TEXT, t2.c_str());
            }
        }
    }
    {   // the hint of the view
        const char* hint = m_veTool == 2 ? (m_veLineA ? "Ligne d'axe : clique la seconde extrémité" : "Ligne d'axe : clique une extrémité")
                         : m_veTool == 3 ? "Remplir une face : clique une face plate" : m_veTool == 4 ? "Gomme : clique un voxel" : m_veTool == 0 ? "Sélection : clique un voxel, 1 à 6 change son type, Suppr l'enlève"
                         : "Placer : clique sur la pièce, le point s'aimante";
        u.text(ImVec2(va.x + 14 * s, va.y + 12 * s), 10, C_DIM, hint);
        u.text(ImVec2(va.x + 14 * s, vb.y - 22 * s), 9.5f, C_FAINT, "Clic droit ou Alt + clic gauche : tourner · clic molette : déplacer · molette : zoom · Ctrl+Z : annuler");
    }
    u.dl->PopClipRect();
    // ================= bottom: the counts, the work queue
    const ImVec2 qa(20 * s, d.y - 20 * s - bottomH), qb(d.x - 20 * s, d.y - 20 * s);
    stGlass(u.dl, qa, qb, 16 * s, s);
    {
        stTrack(u, ImVec2(qa.x + 16 * s, qa.y + 14 * s), 7.5f, C_DIM, "VOXELS DE LA PIÈCE", 1.6f * s);
        int cnt[6] = {0, 0, 0, 0, 0, 0}, sug[6] = {0, 0, 0, 0, 0, 0};
        for (uint8_t t : m_veConn) if (t < 6) ++cnt[t];
        for (const VeSug& sg : m_veSugs) for (uint8_t t : sg.t) if (t < 6) ++sug[t];
        for (int k = 0; k < 6; ++k) {
            const float cx = qa.x + 16 * s + (k % 3) * 120 * s, cy = qa.y + 40 * s + (k / 3) * 26 * s;
            veMark(u.dl, ImVec2(cx + 6 * s, cy + 7 * s), 5 * s, k, 255, false, s);
            char t[40];
            if (sug[k]) snprintf(t, sizeof(t), "%s %d + %d", veName(k), cnt[k], sug[k]); else snprintf(t, sizeof(t), "%s %d", veName(k), cnt[k]);
            u.text(ImVec2(cx + 18 * s, cy), 10, cnt[k] || sug[k] ? C_TEXT : C_FAINT, t);
        }
    }
    {
        const float x0 = qa.x + 400 * s;
        u.dl->AddLine(ImVec2(x0 - 16 * s, qa.y + 14 * s), ImVec2(x0 - 16 * s, qb.y - 14 * s), IM_COL32(255, 255, 255, 24), 1.0f);
        stTrack(u, ImVec2(x0, qa.y + 14 * s), 7.5f, C_DIM, "FILE DE TRAVAIL · LES ROUGES, PUIS LES ORANGES", 1.6f * s);
        std::vector<int> q;
        q.push_back(m_vePart);
        for (int pass = 3; pass >= 2 && q.size() < 16; --pass)
            for (int i : m_stParts) { if (q.size() >= 16) break; if (i != m_vePart && m_lib.parts[i].connSrc == pass) q.push_back(i); }
        const float cs = 58 * s;
        float cx = x0;
        for (int i : q) {
            if (cx + cs > qb.x - 16 * s) break;
            const ImVec2 a(cx, qa.y + 36 * s), b(cx + cs, qa.y + 36 * s + cs);
            const bool h = u.hover(a, b);
            u.dl->AddRectFilled(a, b, IM_COL32(18, 19, 24, 255), 9 * s);
            ImTextureID tex;
            ImVec2 uv0, uv1;
            if (thumbUv(i, tex, uv0, uv1)) {
                const ImVec2 du((uv1.x - uv0.x) * (1 - 1 / 1.4f) * 0.5f, (uv1.y - uv0.y) * (1 - 1 / 1.4f) * 0.5f);
                u.dl->AddImage(tex, ImVec2(a.x + 3 * s, a.y + 3 * s), ImVec2(b.x - 3 * s, b.y - 3 * s), ImVec2(uv0.x + du.x, uv0.y + du.y), ImVec2(uv1.x - du.x, uv1.y - du.y), IM_COL32(230, 232, 236, 255));
            }
            u.dl->AddRect(a, b, i == m_vePart ? C_ACCENT : stConnCol(m_lib.parts[i].connSrc), 9 * s, 0, i == m_vePart ? 2.5f * s : 1.5f * s);
            if (h) ImGui::SetTooltip("%s · %s", m_lib.parts[i].name.c_str(), partFr(m_lib.parts[i].label).c_str());
            if (h && u.pressed && i != m_vePart) { veSave(false); openVoxelEditor(i); return; }
            cx += cs + 8 * s;
        }
        if (q.size() == 1) u.text(ImVec2(x0, qa.y + 50 * s), 10.5f, C_DIM, "Aucune autre pièce rouge ou orange dans la liste ouverte de la bibliothèque.");
    }
}
