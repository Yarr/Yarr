#ifndef YARR_SCANPROGRESS_H
#define YARR_SCANPROGRESS_H

#include <chrono>
#include <string>
#include <vector>

#include "LoopStatus.h"

class ScanLoopInfo;

/**
 * Tracks completion of the nested scan-loop hierarchy (mask/parameter loops)
 * and feeds a human-readable status block to ProgressBar.
 *
 * Trigger/data loops are not tracked individually (they just repeat within
 * a single step of an outer mask/parameter loop), so the reported fraction
 * is an approximation based on the mask/parameter loop levels only.
 */
class ScanProgress {
    public:
        static ScanProgress& instance();

        /// Snapshot the loop hierarchy (min/max/step per level) before the scan starts.
        void init(const ScanLoopInfo &scan);

        /// Called on every loop step with the current position in the hierarchy.
        void update(const LoopStatus &status);

        /// Remove the progress display at the end of the scan.
        void finish();

    private:
        ScanProgress() = default;

        struct LevelInfo {
            unsigned index = 0; ///< level index within the loop hierarchy
            int min = 0;
            int max = 0;
            unsigned step = 1;
            unsigned count = 1; ///< number of distinct steps, i.e. (max-min)/step + 1
            std::string tag;    ///< short label, e.g. "Mask" or "Param"
        };

        std::vector<LevelInfo> m_tracked;
        std::chrono::steady_clock::time_point m_start;
        bool m_initialized = false;
};

#endif
