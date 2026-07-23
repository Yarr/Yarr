#ifndef YARR_PROGRESSBAR_H
#define YARR_PROGRESSBAR_H

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

/**
 * Renders a multi-line status block at the bottom of the terminal
 * (e.g. scan loop progress, clipboard queue depths) without
 * corrupting interleaved log output.
 *
 * Content is organised into named sections (e.g. "loop", "clipboards")
 * that different threads can update independently; the full block is
 * erased and redrawn whenever any section changes.
 *
 * Automatically disables itself when stdout is not a terminal.
 */
class ProgressBar {
    public:
        static ProgressBar& instance();

        ProgressBar(const ProgressBar&) = delete;
        ProgressBar& operator=(const ProgressBar&) = delete;

        /// Replace the lines shown for a named section and redraw (rate-limited).
        void setSection(const std::string &name, std::vector<std::string> lines);

        /// Erase the progress block and forget all section content.
        void clearAll();

        /// Run fn() with the progress block erased beforehand and redrawn after,
        /// so e.g. a log line can be written without corrupting the display.
        void aroundExternalWrite(const std::function<void()> &fn);

        /// Current terminal width in columns, or 80 if it cannot be determined.
        static int terminalWidth();

        /// Build a "[#####-----] 42%" style bar string of the given total width.
        static std::string renderBar(double fraction, int width);

    private:
        ProgressBar();

        /// Caller must hold m_mutex.
        void eraseLocked();
        /// Caller must hold m_mutex.
        void renderLocked();

        std::mutex m_mutex;
        std::vector<std::string> m_sectionOrder;
        std::vector<std::pair<std::string, std::vector<std::string>>> m_sections;
        std::size_t m_drawnLines = 0;
        bool m_enabled;
        std::chrono::steady_clock::time_point m_lastRender{};
        std::chrono::milliseconds m_minInterval{200};
};

#endif
