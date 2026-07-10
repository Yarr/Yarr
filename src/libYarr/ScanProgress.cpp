#include "ScanProgress.h"

#include <algorithm>
#include <cstdio>

#include "ScanLoopInfo.h"
#include "ProgressBar.h"

namespace {
    std::string formatDuration(std::chrono::seconds secs) {
        long long total = secs.count();
        if (total < 0) total = 0;
        long long h = total / 3600;
        long long m = (total % 3600) / 60;
        long long s = total % 60;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02lld:%02lld:%02lld", h, m, s);
        return std::string(buf);
    }
}

ScanProgress& ScanProgress::instance() {
    static ScanProgress progress;
    return progress;
}

void ScanProgress::init(const ScanLoopInfo &scan) {
    m_tracked.clear();

    for (unsigned i = 0; i < scan.size(); i++) {
        const LoopActionBaseInfo *loop = scan.getLoop(i);
        if (loop == nullptr) continue;
        if (!loop->isMaskLoop() && !loop->isParameterLoop()) continue;

        LevelInfo info;
        info.index = i;
        info.min = loop->getMin();
        info.max = loop->getMax();
        info.step = std::max(1u, loop->getStep());
        info.count = static_cast<unsigned>((info.max - info.min) / (int)info.step) + 1;
        info.tag = loop->isMaskLoop() ? "Mask" : "Param";

        m_tracked.push_back(info);
    }

    m_start = std::chrono::steady_clock::now();
    m_initialized = !m_tracked.empty();
}

void ScanProgress::update(const LoopStatus &status) {
    if (!m_initialized) return;

    unsigned long long total = 1;
    unsigned long long elapsed = 0;
    std::string label;

    for (const auto &level : m_tracked) {
        unsigned val = status.get(level.index);
        unsigned step = std::min<unsigned>(
            level.count - 1,
            (val >= (unsigned)level.min) ? (val - level.min) / level.step : 0);

        elapsed = elapsed * level.count + step;
        total *= level.count;

        if (!label.empty()) label += " ";
        label += level.tag + ": " + std::to_string(step + 1) + "/" + std::to_string(level.count);
    }

    double fraction = total > 0 ? (double)elapsed / (double)total : 0.0;

    auto now = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(now - m_start);

    std::string eta = "--:--:--";
    if (fraction > 0.0) {
        auto totalEst = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::duration<double>(elapsedTime.count() / fraction));
        eta = formatDuration(totalEst - elapsedTime);
    }

    int width = ProgressBar::terminalWidth();
    int barWidth = std::max(10, std::min(40, width - 60));
    std::string bar = ProgressBar::renderBar(fraction, barWidth);

    char pctBuf[8];
    std::snprintf(pctBuf, sizeof(pctBuf), "%3d%%", (int)(fraction * 100));

    std::string line = "[Scan] " + label + " " + bar + " " + pctBuf +
                        "  elapsed " + formatDuration(elapsedTime) +
                        "  ETA " + eta;

    ProgressBar::instance().setSection("loop", {line});
}

void ScanProgress::finish() {
    m_initialized = false;
    m_tracked.clear();
}
