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
}

void RceTxCore::doTrigger() {
    for(unsigned i=0; i<m_trigCnt; i++) {
        m_com->write32(0x1D000000 + i);
    }
    m_com->write32(0x0);
    while(!m_com->isEmpty());
}
