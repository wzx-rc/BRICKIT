#include "Cloud.h"
#include "CrashLog.h"
#include "Platform.h"
#include "app/Json.h"
#include <chrono>
#include <cstdio>
#include <ctime>
#include <random>

namespace brickit {

// the project of BrickIt (Supabase, Wzx 14/11/2026): its address and its publishable key (public by design: the rules of
// the database do the protection)
static const char* kUrl = "https://yrvbjyrwfaxiwjghrjdq.supabase.co";
static const char* kKey = "sb_publishable__5SBfHTejXcUhs05KE7sTg_Bx-FNkSe";

static double nowSec() { return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); }

static std::string isoNow() {
    const std::time_t t = std::time(nullptr);
    std::tm g{};
#ifdef _WIN32
    gmtime_s(&g, &t);
#else
    gmtime_r(&t, &g);
#endif
    char b[32];
    std::strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%SZ", &g);
    return b;
}

static std::string q(const std::string& s) { return "\"" + app::jsonEscape(s) + "\""; }

// the message of an error answer of Supabase (auth, PostgREST, storage), in French for the usual ones
static std::string errorOf(int status, const std::string& body, const std::string& netErr) {
    if (status == 0) return netErr.empty() ? std::string("pas de connexion") : "pas de connexion (" + netErr + ")";
    app::Json j;
    std::string code, msg;
    if (app::Json::parse(body, j)) {
        code = j["error_code"].str(j["code"].str(j["error"].str()));
        msg = j["msg"].str(j["message"].str(j["error_description"].str()));
    }
    if (code == "invalid_credentials" || msg == "Invalid login credentials") return "e-mail ou mot de passe incorrect";
    if (code == "email_not_confirmed") return "e-mail pas encore confirmé : clique sur le lien reçu par e-mail";
    if (code == "user_already_exists") return "ce compte existe déjà : connecte-toi";
    if (code == "weak_password") return "mot de passe trop court (6 caractères au moins)";
    if (code == "over_email_send_rate_limit") return "trop d'e-mails envoyés : réessaie dans un moment";
    if (code == "42P01" || msg.find("does not exist") != std::string::npos || msg.find("schema cache") != std::string::npos)
        return "la table des scènes n'existe pas encore (le SQL de supabase/migrations à coller dans Supabase)";
    if (msg.find("Bucket not found") != std::string::npos) return "le bucket « scenes » n'existe pas encore (le SQL de supabase/migrations)";
    char b[48];
    snprintf(b, sizeof(b), "erreur %d", status);
    return msg.empty() ? std::string(b) : std::string(b) + " : " + msg;
}

Cloud::~Cloud() {
    {
        std::lock_guard<std::mutex> l(m_qmx);
        m_quit = true;
    }
    m_cv.notify_all();
    if (m_thread.joinable()) m_thread.join();
}

void Cloud::init(const std::wstring& dataDir) {
    m_file = dataDir + L"\\cloud.json";
    FILE* f = nullptr;
    if (_wfopen_s(&f, m_file.c_str(), L"rb") == 0 && f) {
        std::string t;
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) t.append(buf, n);
        fclose(f);
        app::Json j;
        if (app::Json::parse(t, j)) {
            m_s.email = j["email"].str();
            m_s.uid = j["uid"].str();
            m_s.refresh = plat::unprotectSecret(j["refresh"].str());
            if (m_s.refresh.empty()) m_s.uid.clear();   // (another Windows account: signed out)
        }
    }
    {
        std::lock_guard<std::mutex> l(m_mx);
        m_uidCopy = m_s.uid;
        m_emailCopy = m_s.email;
    }
    m_thread = std::thread([this] {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> l(m_qmx);
                m_cv.wait(l, [this] { return m_quit || !m_jobs.empty(); });
                if (m_quit) return;
                job = std::move(m_jobs.front());
                m_jobs.pop_front();
            }
            job();
        }
    });
}

bool Cloud::signedIn() const {
    std::lock_guard<std::mutex> l(m_mx);
    return !m_uidCopy.empty();
}

std::string Cloud::email() const {
    std::lock_guard<std::mutex> l(m_mx);
    return m_emailCopy;
}

void Cloud::post(std::function<void()> work) {
    ++m_pending;
    {
        std::lock_guard<std::mutex> l(m_qmx);
        m_jobs.push_back(std::move(work));
    }
    m_cv.notify_one();
}

void Cloud::answer(std::function<void()> f) {
    std::lock_guard<std::mutex> l(m_qmx);
    m_answers.push_back([this, f] { --m_pending; f(); });
}

void Cloud::pump() {
    std::vector<std::function<void()>> a;
    {
        std::lock_guard<std::mutex> l(m_qmx);
        a.swap(m_answers);
    }
    for (auto& f : a) f();
}

void Cloud::saveSession() {
    std::string t = "{\"email\": " + q(m_s.email) + ", \"uid\": " + q(m_s.uid) + ", \"refresh\": " + q(m_s.uid.empty() ? std::string() : plat::protectSecret(m_s.refresh)) + "}\n";
    FILE* f = nullptr;
    if (_wfopen_s(&f, m_file.c_str(), L"wb") == 0 && f) {
        fwrite(t.data(), 1, t.size(), f);
        fclose(f);
    }
    std::lock_guard<std::mutex> l(m_mx);
    m_uidCopy = m_s.uid;
    m_emailCopy = m_s.email;
}

std::vector<std::string> Cloud::headers(bool json) const {
    std::vector<std::string> h = {std::string("apikey: ") + kKey};
    if (!m_s.access.empty()) h.push_back("Authorization: Bearer " + m_s.access);
    if (json) h.push_back("Content-Type: application/json");
    return h;
}

// worker: password or refresh_token grant; the session kept
bool Cloud::tokenRequest(const std::string& grant, const std::string& body, std::string& err) {
    const plat::HttpReply r = plat::http("POST", std::string(kUrl) + "/auth/v1/token?grant_type=" + grant,
                                         {std::string("apikey: ") + kKey, "Content-Type: application/json"}, body);
    if (r.status != 200) {
        err = errorOf(r.status, r.body, r.error);
        if (grant == "refresh_token" && r.status >= 400 && r.status < 500) { m_s = Session{}; saveSession(); err = "session expirée : reconnecte-toi"; }
        return false;
    }
    app::Json j;
    if (!app::Json::parse(r.body, j) || j["access_token"].str().empty()) { err = "réponse inattendue"; return false; }
    m_s.access = j["access_token"].str();
    m_s.refresh = j["refresh_token"].str();
    m_s.expires = nowSec() + j["expires_in"].num(3600);
    m_s.uid = j["user"]["id"].str(m_s.uid);
    m_s.email = j["user"]["email"].str(m_s.email);
    saveSession();
    crumb("Cloud : session ouverte (%s)", m_s.email.c_str());
    return true;
}

bool Cloud::ensureToken(std::string& err) {
    if (m_s.uid.empty() || m_s.refresh.empty()) { err = "pas connecté"; return false; }
    if (!m_s.access.empty() && nowSec() < m_s.expires - 60) return true;
    return tokenRequest("refresh_token", "{\"refresh_token\": " + q(m_s.refresh) + "}", err);
}

bool Cloud::call(const char* method, const std::string& path, const std::vector<std::string>& extra, const std::string& body, int& status,
                 std::string& out, std::string& err) {
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!ensureToken(err)) return false;
        std::vector<std::string> h = headers(false);
        h.insert(h.end(), extra.begin(), extra.end());
        const plat::HttpReply r = plat::http(method, std::string(kUrl) + path, h, body);
        status = r.status;
        out = r.body;
        if (r.status == 401 && attempt == 0) { m_s.access.clear(); continue; }   // (expired: refreshed once)
        if (r.status >= 200 && r.status < 300) return true;
        err = errorOf(r.status, r.body, r.error);
        return false;
    }
    return false;
}

void Cloud::signIn(const std::string& email, const std::string& password, std::function<void(bool, const std::string&)> done) {
    post([this, email, password, done] {
        std::string err;
        const bool ok = tokenRequest("password", "{\"email\": " + q(email) + ", \"password\": " + q(password) + "}", err);
        answer([done, ok, err] { done(ok, ok ? std::string("connecté") : err); });
    });
}

void Cloud::signUp(const std::string& email, const std::string& password, std::function<void(bool, const std::string&)> done) {
    post([this, email, password, done] {
        const plat::HttpReply r = plat::http("POST", std::string(kUrl) + "/auth/v1/signup", {std::string("apikey: ") + kKey, "Content-Type: application/json"},
                                             "{\"email\": " + q(email) + ", \"password\": " + q(password) + "}");
        bool ok = r.status == 200;
        std::string msg;
        if (!ok) msg = errorOf(r.status, r.body, r.error);
        else {
            app::Json j;
            app::Json::parse(r.body, j);
            if (!j["access_token"].str().empty()) {   // (no confirmation asked by the project: signed in at once)
                std::string err;
                ok = tokenRequest("password", "{\"email\": " + q(email) + ", \"password\": " + q(password) + "}", err);
                msg = ok ? "compte créé, connecté" : err;
            } else msg = "compte créé : confirme ton e-mail (lien reçu), puis connecte-toi";
        }
        answer([done, ok, msg] { done(ok, msg); });
    });
}

void Cloud::signOut() {
    post([this] {
        if (!m_s.access.empty()) plat::http("POST", std::string(kUrl) + "/auth/v1/logout", headers(true), "{}");
        m_s = Session{};
        saveSession();
        answer([] {});
    });
}

static CloudScene sceneOf(const app::Json& r) {
    CloudScene c;
    c.id = r["id"].str();
    c.name = r["name"].str();
    c.device = r["device"].str();
    c.updated = r["updated_at"].str();
    c.bricks = (int)r["bricks"].num();
    c.version = (int)r["version"].num();
    return c;
}

void Cloud::list(std::function<void(bool, const std::vector<CloudScene>&, const std::string&)> done) {
    post([this, done] {
        int st = 0;
        std::string out, err;
        std::vector<CloudScene> v;
        const bool ok = call("GET", "/rest/v1/scenes?select=id,name,device,bricks,version,updated_at&deleted=eq.false&order=updated_at.desc", {}, "", st, out, err);
        if (ok) {
            app::Json j;
            if (app::Json::parse(out, j)) for (const app::Json& r : j.a) v.push_back(sceneOf(r));
        }
        answer([done, ok, v, err] { done(ok, v, err); });
    });
}

void Cloud::head(const std::string& id, std::function<void(bool, bool, const CloudScene&, const std::string&)> done) {
    post([this, id, done] {
        int st = 0;
        std::string out, err;
        CloudScene c;
        bool found = false;
        const bool ok = call("GET", "/rest/v1/scenes?select=id,name,device,bricks,version,updated_at,deleted&id=eq." + id, {}, "", st, out, err);
        if (ok) {
            app::Json j;
            if (app::Json::parse(out, j) && !j.a.empty() && !(j.a[0]["deleted"].type == app::Json::Bool && j.a[0]["deleted"].b)) { c = sceneOf(j.a[0]); found = true; }
        }
        answer([done, ok, found, c, err] { done(ok, found, c, err); });
    });
}

void Cloud::upload(const CloudScene& meta, const std::string& json, int baseVersion, std::function<void(int, int, const std::string&)> done) {
    post([this, meta, json, baseVersion, done] {
        int st = 0;
        std::string out, err;
        int result = -1, ver = baseVersion;
        // 1. its version in the cloud: another device sent one since this one was read -> a conflict, nothing sent
        if (call("GET", "/rest/v1/scenes?select=version,deleted&id=eq." + meta.id, {}, "", st, out, err)) {
            app::Json j;
            app::Json::parse(out, j);
            const int remote = j.a.empty() ? 0 : (int)j.a[0]["version"].num();
            if (remote != baseVersion && !j.a.empty()) { result = 1; ver = remote; }
            else {
                // 2. the file (replaced), 3. its line (inserted or updated)
                const std::string path = "/storage/v1/object/scenes/" + m_s.uid + "/" + meta.id + ".brick";
                if (call("POST", path, {"Content-Type: application/json", "x-upsert: true"}, json, st, out, err)) {
                    ver = baseVersion + 1;
                    const std::string row = "{\"id\": " + q(meta.id) + ", \"name\": " + q(meta.name) + ", \"device\": " + q(plat::deviceName()) +
                                            ", \"bricks\": " + std::to_string(meta.bricks) + ", \"version\": " + std::to_string(ver) +
                                            ", \"deleted\": false, \"updated_at\": " + q(isoNow()) + "}";
                    if (call("POST", "/rest/v1/scenes?on_conflict=id", {"Content-Type: application/json", "Prefer: resolution=merge-duplicates,return=minimal"}, row,
                             st, out, err))
                        result = 0;
                }
            }
        }
        if (result == 0) crumb("Cloud : scène %s envoyée (version %d)", meta.id.c_str(), ver);
        answer([done, result, ver, err] { done(result, ver, err); });
    });
}

void Cloud::download(const std::string& id, std::function<void(bool, const std::string&, const std::string&)> done) {
    post([this, id, done] {
        int st = 0;
        std::string out, err;
        const bool ok = call("GET", "/storage/v1/object/authenticated/scenes/" + m_s.uid + "/" + id + ".brick", {}, "", st, out, err);
        answer([done, ok, out, err] { done(ok, out, err); });
    });
}

void Cloud::remove(const std::string& id, std::function<void(bool, const std::string&)> done) {
    post([this, id, done] {
        int st = 0;
        std::string out, err;
        bool ok = call("PATCH", "/rest/v1/scenes?id=eq." + id, {"Content-Type: application/json", "Prefer: return=minimal"},
                       "{\"deleted\": true, \"updated_at\": " + q(isoNow()) + "}", st, out, err);
        if (ok) call("DELETE", "/storage/v1/object/scenes/" + m_s.uid + "/" + id + ".brick", {}, "", st, out, err);   // (the line stays, marked)
        answer([done, ok, err] { done(ok, err); });
    });
}

std::string Cloud::newId() {
    static std::mt19937_64 rng(std::random_device{}() ^ (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    uint64_t a = rng(), b = rng();
    a = (a & 0xffffffffffff0fffull) | 0x0000000000004000ull;   // version 4
    b = (b & 0x3fffffffffffffffull) | 0x8000000000000000ull;   // variant 10
    char s[40];
    snprintf(s, sizeof(s), "%08x-%04x-%04x-%04x-%012llx", (unsigned)(a >> 32), (unsigned)((a >> 16) & 0xffff), (unsigned)(a & 0xffff),
             (unsigned)(b >> 48), (unsigned long long)(b & 0xffffffffffffull));
    return s;
}

std::string Cloud::localDate(const std::string& iso) {
    int Y, M, D, h, m, sec = 0;
    if (sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d", &Y, &M, &D, &h, &m, &sec) < 5) return iso;
    std::tm g{};
    g.tm_year = Y - 1900;
    g.tm_mon = M - 1;
    g.tm_mday = D;
    g.tm_hour = h;
    g.tm_min = m;
    g.tm_sec = sec;
#ifdef _WIN32
    const std::time_t t = _mkgmtime(&g);
#else
    const std::time_t t = timegm(&g);
#endif
    std::tm l{}, n{};
    const std::time_t now = std::time(nullptr);
#ifdef _WIN32
    localtime_s(&l, &t);
    localtime_s(&n, &now);
#else
    localtime_r(&t, &l);
    localtime_r(&now, &n);
#endif
    char b[40];
    if (l.tm_year == n.tm_year && l.tm_yday == n.tm_yday) snprintf(b, sizeof(b), "aujourd'hui %02d:%02d", l.tm_hour, l.tm_min);
    else snprintf(b, sizeof(b), "%02d/%02d %02d:%02d", l.tm_mday, l.tm_mon + 1, l.tm_hour, l.tm_min);
    return b;
}

}  // namespace brickit
