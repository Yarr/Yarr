#include "ProgressBar.h"

#include <algorithm>
#include <cstdio>
#include <unistd.h>
#include <sys/ioctl.h>

ProgressBar::ProgressBar() {
    m_enabled = (isatty(STDOUT_FILENO) != 0);
}

ProgressBar& ProgressBar::instance() {
    static ProgressBar bar;
    return bar;
}

int ProgressBar::terminalWidth() {
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return ws.ws_col;
    }
    return 80;
}

std::string ProgressBar::renderBar(double fraction, int width) {
    fraction = std::min(1.0, std::max(0.0, fraction));
    int barWidth = std::max(1, width);
    int filled = static_cast<int>(fraction * barWidth);
    std::string bar;
    bar.reserve(barWidth + 2);
    bar += '[';
    for (int i = 0; i < barWidth; i++) {
        bar += (i < filled) ? '#' : '-';
    }
    bar += ']';
    return bar;
}

void ProgressBar::eraseLocked() {
    if (m_drawnLines == 0) return;
    // Move cursor up over all previously drawn lines, then clear from there to end of screen.
    std::fprintf(stdout, "\033[%zuA\033[J", m_drawnLines);
    m_drawnLines = 0;
}

void ProgressBar::renderLocked() {
    eraseLocked();

    std::vector<std::string> lines;
    for (const auto &section : m_sections) {
        for (const auto &line : section.second) {
            lines.push_back(line);
        }
    }

    if (lines.empty()) {
        std::fflush(stdout);
        return;
    }

    for (const auto &line : lines) {
        std::fprintf(stdout, "%s\n", line.c_str());
    }
    std::fflush(stdout);
    m_drawnLines = lines.size();
}

void ProgressBar::setSection(const std::string &name, std::vector<std::string> lines) {
    if (!m_enabled) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    bool found = false;
    for (auto &section : m_sections) {
        if (section.first == name) {
            section.second = std::move(lines);
            found = true;
            break;
        }
    }
    if (!found) {
        m_sections.emplace_back(name, std::move(lines));
    }

    auto now = std::chrono::steady_clock::now();
    if (now - m_lastRender >= m_minInterval) {
        renderLocked();
        m_lastRender = now;
    }
}

void ProgressBar::clearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    eraseLocked();
    std::fflush(stdout);
    m_sections.clear();
}

void ProgressBar::aroundExternalWrite(const std::function<void()> &fn) {
    if (!m_enabled) {
        fn();
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    eraseLocked();
    fn();
    renderLocked();
}
