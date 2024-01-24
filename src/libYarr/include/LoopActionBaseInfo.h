#ifndef LOOPACTIONBASE_INFO_H
#define LOOPACTIONBASE_INFO_H

#include <string>

#include "LoopStatus.h"

/**
 * Information about a scan LoopAction.
 */
class LoopActionBaseInfo {
    public:
        explicit LoopActionBaseInfo(LoopStyle s) : m_style(s) {}
        virtual ~LoopActionBaseInfo() = default;

        /// Is this a loop over a parameter change
        bool isParameterLoop() const {
            return m_style == LOOP_STYLE_PARAMETER;
        }

        /// Is this loop over mask changes
        bool isMaskLoop() const {
            return m_style == LOOP_STYLE_MASK;
        }

        /// Is this loop where data is collected
        bool isDataLoop() const {
            return m_style == LOOP_STYLE_DATA;
        }

        /// Is this loop where a trigger is sent
        bool isTriggerLoop() const {
            return m_style == LOOP_STYLE_TRIGGER;
        }

        /// Is this a requiring feedback at the pixel level
        /*
         * In this case feedback is provided via a histogram (feedbackStep).
         */
        bool isPixelFeedbackLoop() const {
            return m_style == LOOP_STYLE_PIXEL_FEEDBACK;
        }

        /// Is this a requiring feedback at the pixel level
        /*
         * In this case only simple feedback is provided (feedbackBinary).
         */
        bool isGlobalFeedbackLoop() const {
            return m_style == LOOP_STYLE_GLOBAL_FEEDBACK;
        }

        /// Retrieve loop style directly
        LoopStyle getStyle() const {
            return m_style;
        }

        /// Retrieve lowest parameter value
        unsigned getMin() const {
            return min;
        }

        /// Retrieve highest parameter value
        unsigned getMax() const {
            return max;
        }

        /// Retrieve parameter step
        unsigned getStep() const {
            return step;
        }

    protected:
        LoopStyle m_style;
        int min;
        int max;
        unsigned step;
};

#endif
