// BrickIt - PdfReader on the systems other than Windows (Pdf.cpp): the same worker thread and job queue, the pages
// opened and rendered by the system (iPad: CoreGraphics, PdfApple.mm). The Linux test build has no PDF renderer.
#ifndef _WIN32
#include "Pdf.h"
#include <algorithm>
#include <memory>

namespace brickit {

// backend: an opened document (null on failure, `error` set), its page sizes in points, a page as RGBA8
std::shared_ptr<void> pdfBackendOpen(const std::wstring& path, std::vector<std::pair<float, float>>& sizes, std::string& error);
bool pdfBackendRender(void* doc, int page, int width, PdfReader::Image& img);

#ifndef __APPLE__
std::shared_ptr<void> pdfBackendOpen(const std::wstring&, std::vector<std::pair<float, float>>&, std::string& error) {
    error = "lecture PDF non disponible sur ce système";
    return nullptr;
}
bool pdfBackendRender(void*, int, int, PdfReader::Image&) { return false; }
#endif

PdfReader::PdfReader() { m_thread = std::thread([this] { run(); }); }

PdfReader::~PdfReader() {
    {
        std::lock_guard<std::mutex> l(m_mx);
        m_quit = true;
    }
    m_cv.notify_all();
    if (m_thread.joinable()) m_thread.join();
}

void PdfReader::open(const std::wstring& path) {
    std::lock_guard<std::mutex> l(m_mx);
    ++m_gen;
    m_jobs.clear();
    m_done.clear();
    m_sizes.clear();
    m_ready = m_failed = false;
    m_error.clear();
    m_jobs.push_back({0, path, 0, 0, m_gen});
    m_cv.notify_all();
}

void PdfReader::request(int page, int width) {
    std::lock_guard<std::mutex> l(m_mx);
    for (const Job& j : m_jobs)
        if (j.kind == 1 && j.page == page && j.width == width) return;
    m_jobs.push_back({1, L"", page, width, m_gen});
    m_cv.notify_all();
}

bool PdfReader::take(Image& out) {
    std::lock_guard<std::mutex> l(m_mx);
    if (m_done.empty()) return false;
    out = std::move(m_done.front());
    m_done.pop_front();
    return true;
}

void PdfReader::run() {
    std::shared_ptr<void> doc;
    int docGen = -1, pages = 0;
    for (;;) {
        Job job;
        {
            std::unique_lock<std::mutex> l(m_mx);
            m_cv.wait(l, [this] { return m_quit || !m_jobs.empty(); });
            if (m_quit) break;
            job = m_jobs.front();
            m_jobs.pop_front();
            if (job.gen != m_gen) continue;
        }
        if (job.kind == 0) {
            std::vector<std::pair<float, float>> sizes;
            std::string err;
            doc = pdfBackendOpen(job.path, sizes, err);
            docGen = job.gen;
            pages = (int)sizes.size();
            std::lock_guard<std::mutex> l(m_mx);
            if (job.gen != m_gen) continue;
            if (doc) { m_sizes = sizes; m_ready = true; }
            else { m_failed = true; m_error = err.empty() ? "erreur inconnue" : err; }
        } else if (doc && docGen == job.gen && job.page >= 0 && job.page < pages) {
            Image img;
            img.page = job.page;
            if (!pdfBackendRender(doc.get(), job.page, std::max(16, job.width), img)) continue;
            std::lock_guard<std::mutex> l(m_mx);
            if (job.gen == m_gen) m_done.push_back(std::move(img));
        }
    }
}

} // namespace brickit
#endif
