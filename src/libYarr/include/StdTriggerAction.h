#ifndef STD_TRIGGER_LOOP_ACTION_H
#define STD_TRIGGER_LOOP_ACTION_H

/**
 * Base class for specific Trigger Loop.
 *
 * Used to pass expected count to histograms.
 */
class StdTriggerAction {
    public:
        virtual uint32_t getTrigCnt() { return m_trigCnt; }
        void setTrigCnt(uint32_t cnt) { m_trigCnt = cnt; }
        virtual uint32_t getTrigMulti() { return m_trigMulti; }
        void setTrigMulti(uint32_t multi) { m_trigMulti = multi; }

        virtual ~StdTriggerAction() = default;;
    protected:
        uint32_t m_trigCnt;
	    uint32_t m_trigMulti;
};

#endif
