// BrickIt - the scene in the cloud (Cloud.h, Wzx 14/11/2026): the scene being built is sent a few seconds after each
// change (signed in, Réglages › Cloud › Synchro auto); every 20 s its version in the cloud is read, and a newer one sent
// by the other device (iPad, PC) replaces it here when nothing changed here since. Both changed: this one goes on as a
// copy (nothing is lost). Scène › Scènes du cloud: every scene of the account, to open (or delete).
#include "BrickApp.h"
#include "Platform.h"
#include "imgui.h"

namespace brickit {

// a scene that is not the one of the cloud any more (new, opened from a file, imported): a new scene of the cloud at
// its next change
void BrickApp::cloudForget() {
    m_cloudId.clear();
    m_cloudVer = 0;
    m_cloudSentKey.clear();
}

void BrickApp::cloudInit() {
    m_cloud.init(plat::dataDir());
    m_cloudPollAt = 0;   // (its version in the cloud read at once)
}

void BrickApp::cloudNoteChange() {
    if (m_cloudLoading || !m_cloudAuto || !userSession() || !m_cloud.signedIn()) return;
    m_cloudDirty = true;
    m_cloudDirtyAt = ImGui::GetTime();
    ++m_cloudChanges;
}

void BrickApp::cloudTick() {
    m_cloud.pump();
    if (!userSession() || !m_cloud.signedIn()) return;
    const double t = ImGui::GetTime();
    if (m_cloudDirty && !m_cloudSending && !m_cloudChecking && !m_holding && t - m_cloudDirtyAt > 2.5 && t >= m_cloudSendAt) cloudSend();   // (one send per minute at most, Wzx 14/11/2026)
    else if (m_cloudAuto && !m_cloudDirty && !m_cloudSending && !m_cloudChecking && !m_cloudId.empty() && t > m_cloudPollAt) {
        m_cloudPollAt = t + 20;
        cloudCheck();
    }
}

void BrickApp::cloudSend() {
    // (the camera alone does not make a new version: the scene compared without it)
    const std::string key = m_build.toJson(vp3d::Camera());
    if (!m_cloudId.empty() && key == m_cloudSentKey) { m_cloudDirty = false; return; }
    if (m_cloudId.empty()) { m_cloudId = Cloud::newId(); m_cloudVer = 0; savePrefs(); }
    CloudScene meta;
    meta.id = m_cloudId;
    meta.name = m_modelName.empty() ? std::string("Ma scène") : m_modelName;
    meta.bricks = (int)m_bricks.size();
    m_cloudSending = true;
    m_cloudMsg = "envoi…";
    const int changes = m_cloudChanges;
    const std::string id = m_cloudId;
    m_cloud.upload(meta, m_build.toJson(m_scene.camera), m_cloudVer, [this, changes, id, key, meta](int res, int ver, const std::string& err) {
        m_cloudSending = false;
        if (id != m_cloudId) return;   // (another scene since)
        if (res == 0) {
            m_cloudVer = ver;
            m_cloudSentKey = key;
            if (m_cloudChanges == changes) m_cloudDirty = false;
            m_cloudOkAt = ImGui::GetTime();
            m_cloudSendAt = m_cloudOkAt + 60;
            m_cloudMsg = "à jour";
            savePrefs();
        } else if (res == 1) {   // changed on the other device too: this one goes on as a copy, sent at once
            m_cloudId = Cloud::newId();
            m_cloudVer = 0;
            m_cloudSentKey.clear();
            m_modelName = meta.name + " (copie " + plat::deviceName() + ")";
            m_cloudDirty = true;
            m_cloudDirtyAt = 0;
            m_cloudSendAt = 0;
            toast("Cloud : cette scène a aussi changé sur l'autre appareil. Ta version continue en copie : « " + m_modelName + " »");
        } else {
            m_cloudMsg = "envoi impossible : " + err;
            m_cloudDirtyAt = ImGui::GetTime() + 15;   // (tried again later)
        }
    });
}

void BrickApp::cloudCheck() {
    m_cloudChecking = true;
    const std::string id = m_cloudId;
    m_cloud.head(id, [this, id](bool ok, bool found, const CloudScene& c, const std::string& err) {
        m_cloudChecking = false;
        if (!ok) { m_cloudMsg = err; return; }
        if (id != m_cloudId) return;
        if (!found) {   // deleted from the other device: this scene goes on as a new one at its next change
            if (m_cloudVer > 0) cloudForget();
            return;
        }
        if (c.version > m_cloudVer && !m_cloudDirty && !m_holding) cloudOpen(c, true);
    });
}

// a scene of the cloud opened here (fromPoll: the newer version of this one, sent by the other device: the camera stays)
void BrickApp::cloudOpen(const CloudScene& c, bool fromPoll) {
    if (!fromPoll && m_cloudDirty && !m_cloudSending) cloudSend();   // (this one sent first: the requests run in order)
    m_cloudChecking = true;
    m_cloud.download(c.id, [this, c, fromPoll](bool ok, const std::string& json, const std::string& err) {
        m_cloudChecking = false;
        if (!ok) { toast("Cloud : " + err); return; }
        if (fromPoll && (m_cloudDirty || c.id != m_cloudId || m_holding)) return;   // (changed here meanwhile: the conflict at the next send)
        if (!loadSceneText(json, c.name, !fromPoll)) return;
        m_cloudId = c.id;
        m_cloudVer = c.version;
        m_cloudSentKey = m_build.toJson(vp3d::Camera());
        m_cloudDirty = false;
        m_cloudOkAt = ImGui::GetTime();
        m_cloudMsg = "à jour";
        savePrefs();
        toast(fromPoll ? "Cloud : scène mise à jour depuis " + (c.device.empty() ? std::string("l'autre appareil") : c.device)
                       : "Cloud : « " + c.name + " » ouverte");
    });
}

bool BrickApp::loadSceneText(const std::string& text, const std::string& name, bool camera) {
    std::vector<Brick> before = m_bricks;
    std::string msg;
    m_cloudLoading = true;
    const bool ok = m_build.fromJson(text, camera ? &m_scene.camera : nullptr, &msg);
    if (ok) {
        m_undo.push_back(before);
        m_redo.clear();
        m_selected = -1;
        m_multi.clear();
        m_scenePath.clear();
        m_modelName = name == "Ma scène" ? std::string() : name;
        rebuildScene();
        autosave();
    }
    m_cloudLoading = false;
    if (!ok) toast(msg);
    return ok;
}

void BrickApp::cloudRefreshList() {
    if (!m_cloud.signedIn() || m_cloudListBusy) return;
    m_cloudListBusy = true;
    m_cloud.list([this](bool ok, const std::vector<CloudScene>& v, const std::string& err) {
        m_cloudListBusy = false;
        m_cloudListErr = ok ? std::string() : err;
        if (ok) m_cloudList = v;
    });
}

}  // namespace brickit
