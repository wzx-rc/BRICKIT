// ------------------------------------------------------------------ Crème: the kit of the iPad, updated (Wzx 22/11/2026)
// The interface of the theme Crème is the cream touch kit (BrickUiIpad.inl), on the PC and the iPad. Its drawer takes the
// families and the categories of the library of today (stFams of BrickUiStudio.inl, as Brique and Classic).

int BrickApp::crFamCount() { return (int)stFams().size(); }
std::string BrickApp::crFamKey(int i) { return stFams()[std::clamp(i, 0, crFamCount() - 1)].key; }
const char* BrickApp::crFamLabel(int i) { return stFams()[std::clamp(i, 0, crFamCount() - 1)].label; }
int BrickApp::crFamFirst(int i) { return stFamFirst(crFamKey(i)); }

static const StFam* crFam(const std::string& key) {
    for (const StFam& f : stFams()) if (key == f.key) return &f;
    return nullptr;
}
int BrickApp::crSubCount() { const StFam* f = crFam(m_stFam); return f ? (int)f->subs.size() : 0; }
const char* BrickApp::crSubLabel(int j) { const StFam* f = crFam(m_stFam); return f && j >= 0 && j < (int)f->subs.size() ? f->subs[j].label : ""; }

// the parts of the family / category chosen (cached; the same rules as the drawer of the Studio interface)
void BrickApp::crLibParts() {
    const StFam* fam = crFam(m_stFam);
    if (!fam) { m_stFam = "basique"; m_stSub = -1; fam = crFam(m_stFam); }
    if (!fam) return;
    if (m_stSub >= (int)fam->subs.size()) m_stSub = -1;
    const StSub* sub = !m_searchOpen && m_stSub >= 0 ? &fam->subs[m_stSub] : nullptr;
    int avail = 0;
    for (const Part& p : m_lib.parts) avail += p.available() ? 1 : 0;
    const std::string key = "cr|" + m_stFam + "|" + std::to_string(m_stSub) + "|" + (m_searchOpen ? m_search : std::string()) + "|" + std::to_string(avail) + "|" +
                            std::to_string(m_recent.size()) + "|" + std::to_string(m_fav.size()) + (m_hideDeco ? "|d" : "");
    if (key == m_stKey) return;
    m_stKey = key;
    m_stParts.clear();
    if (m_searchOpen) m_stParts = visibleParts();
    else if (m_stFam == "recent") m_stParts = m_recent;
    else if (m_stFam == "fav") m_stParts = m_fav;
    else
        for (int i = 0; i < (int)m_lib.parts.size(); ++i) {
            const Part& p = m_lib.parts[i];
            if (!p.available() || (m_hideDeco && partDeco(p)) || partDuplo(p)) continue;
            const std::string& c = partCat(i);
            bool in = false;
            if (sub) in = stInSub(*sub, c, p.sub);
            else for (const StSub& q : fam->subs) if (stInSub(q, c, p.sub)) { in = true; break; }
            if (in) m_stParts.push_back(i);
        }
}
