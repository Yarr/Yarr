#include <RceTxCore.h>
RceTxCore::RceTxCore(RceCom *com) {
    m_com = com;
    m_trigCnt = 0;
}

RceTxCore::RceTxCore() {
    m_com = NULL;
    m_trigCnt = 0;
}

RceTxCore::~RceTxCore() {}

void RceTxCore::writeFifo(uint32_t value) {
    // TODO need to check channel
  std::cout <<  "w= "<< std::hex << value <<std::dec << " ";
    m_com->write32(value);
}

void RceTxCore::setTrigCnt(uint32_t count) {
    m_trigCnt = count;
}

void RceTxCore::setTrigEnable(uint32_t value) {
    // TODO value should reflect channel mask
    if(value == 0) {
        // setTrigEnable(0) is called to disable the trigger, once its done
        triggerProc.join();
    } else {
        trigProcRunning = true;
        switch (m_trigCfg) {
            case INT_TIME:
            case EXT_TRIGGER:
                triggerProc = std::thread(&RceTxCore::doTriggerTime, this);
                break;
            case INT_COUNT:
                triggerProc = std::thread(&RceTxCore::doTriggerCnt, this);
                break;
            default:
                // Should not occur, else stuck
                break;
        }
    }

}

void RceTxCore::doTriggerCnt() {
    for(unsigned i=0; i<m_trigCnt; i++) {
        m_com->write32(m_trigWord[3]);
        m_com->write32(m_trigWord[2]);
        m_com->write32(m_trigWord[1]);
        m_com->write32(m_trigWord[0]);
        m_com->write32(0x0);
        this->releaseFifo();
        std::this_thread::sleep_for(std::chrono::microseconds((int)(1e6/m_trigFreq))); // Frequency in Hz
        while(!m_com->isEmpty()); // Should not be the case
    }
    // TODO Might need a little sleep here
    trigProcRunning = false; // TODO Could be replaces with joinable?
}

void RceTxCore::doTriggerTime() {
    auto start = std::chrono::system_clock::now(); // This Ok for RCE?
    auto cur = std::chrono::system_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(cur - start).count() < m_trigTime) {
        m_com->write32(m_trigWord[3]);
        m_com->write32(m_trigWord[2]);
        m_com->write32(m_trigWord[1]);
        m_com->write32(m_trigWord[0]);
        m_com->write32(0x0);
        this->releaseFifo();
        std::this_thread::sleep_for(std::chrono::microseconds((int)(1000/m_trigFreq))); // Frequency in kHz
        while(!m_com->isEmpty()); // Should not be the case
        cur = std::chrono::system_clock::now();
    }
    // TODO Might need a little sleep here
    trigProcRunning = false; // TODO Could be replaces with joinable?
}

