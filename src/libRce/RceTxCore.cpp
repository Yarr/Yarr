#include <RceTxCore.h>
RceTxCore::RceTxCore():m_com(0) {
    m_trigCnt = 0;
    m_com=new RceCom();

    m_com->pgp->writeRegister("TRIGGER_MASK",0);
    m_com->pgp->writeRegister("CALIB_MODE",0);
    m_com->pgp->writeRegister("L1A_ROUTE",0);
    m_com->pgp->writeRegister("RESET_INPUT_DELAYS",1);
     m_com->pgp->writeRegister("CLOCK_SELECT",2);

    setChannelInMask(1);
    setChannelOutMask(1);
    m_com->pgp->sendCommand("SOFT_RESET");
    m_com->pgp->writeRegister("CLOCK_SELECT",2);
    m_com->pgp->sendCommand("WRITE_PHASE_CALIB");
    setChannelInMask(1);
    setChannelOutMask(1);

}

void RceTxCore::setChannelInMask(unsigned mask) {
  writeFifo(0x0);
  writeFifo(0x0);
  writeFifo(0x0);
  writeFifo(0x0);
  releaseFifo();
  m_com->pgp->writeRegister("CHANNEL_IN_MASK",mask);
}
void RceTxCore::setChannelOutMask(unsigned mask) {
  m_com->pgp->writeRegister("CHANNEL_OUT_MASK",mask);
}


RceTxCore::~RceTxCore() {
  if(m_com) delete m_com;
}

void RceTxCore::writeFifo(uint32_t value) {
    // TODO need to check channel
    m_com->write32(value);
}

void RceTxCore::setTrigCnt(uint32_t count) {
    m_trigCnt = count;
}

void RceTxCore::setTrigEnable(uint32_t value) {
    // TODO value should reflect channel mask
  m_com->pgp->sendCommand("WRITE_PHASE_CALIB");
    if(value == 0) {
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

