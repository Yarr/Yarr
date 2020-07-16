#ifndef STD_TRIGGER_LOOP_ACTION_H
#define STD_TRIGGER_LOOP_ACTION_H

/**
 * Base class for specific Trigger Loop.
 *
 * Used to pass expected count to histograms.
 */
class StdTriggerAction {
    public:
        uint32_t getTrigCnt() { return m_trigCnt; }
        void setTrigCnt(uint32_t cnt) { m_trigCnt = cnt; }
    private:
        uint32_t m_trigCnt;
};

#endif
