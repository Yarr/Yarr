#ifndef STD_TRIGGER_LOOP_ACTION_H
#define STD_TRIGGER_LOOP_ACTION_H

/**
 * Base class for specific Trigger Loop.
 *
 * Used to pass expected count to histograms.
 */
class StdTriggerAction {
    public:
        virtual uint32_t getTrigCnt() const { return m_trigCnt; }
        void setTrigCnt(uint32_t cnt) { m_trigCnt = cnt; }

        virtual uint32_t getExpEvents() { return getTrigCnt(); }

        virtual ~StdTriggerAction() = default;;
    protected:
        uint32_t m_trigCnt;
};

#endif
