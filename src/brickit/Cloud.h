// BrickIt - the cloud: the scenes of a user between the iPad and the PC (Wzx 14/11/2026: « une synchro de travail et
// sauvegarde entre l'app iPad et PC »). Supabase: Auth (e-mail and password), a table public.scenes (name, device,
// version, date) and the private bucket « scenes » (<user>/<scene>.brick, the JSON of the .brick files); the rules of
// supabase/migrations: each user reads and writes only his own scenes. Only the publishable key is in the app.
// Every request runs on one worker thread, in order; its answer comes back on the main thread (pump, each frame).
#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace brickit {

struct CloudScene {
    std::string id, name, device, updated;   // updated: ISO 8601 (UTC)
    int bricks = 0, version = 0;
};

class Cloud {
public:
    ~Cloud();
    void init(const std::wstring& dataDir);   // the session of the last time (cloud.json), refreshed on first use
    bool signedIn() const;
    std::string email() const;
    int pending() const { return m_pending; }   // requests not answered yet (main thread)
    // the answers run on the main thread, from pump()
    void signIn(const std::string& email, const std::string& password, std::function<void(bool ok, const std::string& msg)> done);
    void signUp(const std::string& email, const std::string& password, std::function<void(bool ok, const std::string& msg)> done);
    void signOut();
    void list(std::function<void(bool ok, const std::vector<CloudScene>& scenes, const std::string& err)> done);
    void head(const std::string& id, std::function<void(bool ok, bool found, const CloudScene& s, const std::string& err)> done);
    // result: 0 sent (newVersion), 1 conflict (the cloud has another version than baseVersion: nothing sent), -1 error
    void upload(const CloudScene& meta, const std::string& json, int baseVersion,
                std::function<void(int result, int newVersion, const std::string& err)> done);
    void download(const std::string& id, std::function<void(bool ok, const std::string& json, const std::string& err)> done);
    void remove(const std::string& id, std::function<void(bool ok, const std::string& err)> done);
    void pump();

    static std::string newId();                         // a random UUID v4
    static std::string localDate(const std::string& iso);   // « 14/11 12:03 » (local time), « aujourd'hui 12:03 »

private:
    struct Session { std::string access, refresh, uid, email; double expires = 0; };
    void post(std::function<void()> work);              // to the worker
    void answer(std::function<void()> f);               // to the main thread
    bool ensureToken(std::string& err);                 // worker: a valid access token (refreshed when old)
    bool tokenRequest(const std::string& grant, const std::string& body, std::string& err);   // worker
    std::vector<std::string> headers(bool json) const;  // worker
    bool call(const char* method, const std::string& path, const std::vector<std::string>& extra, const std::string& body,
              int& status, std::string& out, std::string& err);   // worker: with the session, refreshed once on 401
    void saveSession();                                  // worker

    std::wstring m_file;
    Session m_s;                      // worker (m_mx for the copies read by the main thread)
    mutable std::mutex m_mx;
    std::string m_uidCopy, m_emailCopy;
    std::thread m_thread;
    std::mutex m_qmx;
    std::condition_variable m_cv;
    std::deque<std::function<void()>> m_jobs;
    std::vector<std::function<void()>> m_answers;
    bool m_quit = false;
    int m_pending = 0;
};

}  // namespace brickit
