#include <RogueTxCore.h>
RogueTxCore::RogueTxCore():m_com(0) {
    m_trigCnt = 0;
    m_com=RogueCom::getInstance();
}


RogueTxCore::~RogueTxCore() {
}

void RogueTxCore::writeFifo(uint32_t value) {
  m_com->write32(value);
}

void RogueTxCore::setTrigCnt(uint32_t count) {
 m_trigCnt = count;
}

void RogueTxCore::setTrigTime(double time) {
//	std::cout<<"zixu in setTrigTime: "<<time<<", trigFreq="<<m_trigFreq<<std::endl;
//	double tmpB;std::cin>>tmpB;
	if(time>0.001){
 uint32_t count=time* m_trigFreq;
 setTrigCnt(count);
	}
}

void RogueTxCore::setTrigEnable(uint32_t value) {
  if(value == 0) {
    if(triggerProc.joinable() ) triggerProc.join();
  } else {
    trigProcRunning = true;
    switch (m_trigCfg) {
    case INT_TIME:
    case EXT_TRIGGER:
      triggerProc = std::thread(&RogueTxCore::doTriggerTime, this);
      break;
    case INT_COUNT:
      triggerProc = std::thread(&RogueTxCore::doTriggerCnt, this);
      break;
    default:
      // Should not occur, else stuck
      break;
    }
  }
}
void  RogueTxCore::setTrigWordLength(uint32_t length) {
  m_trigWordLength=length;
}

#define SW_TRIGGER
void RogueTxCore::doTriggerCnt() {

  if(!m_com->getFirmwareTrigger()) {
    for(unsigned i=0;i<m_trigCnt;i++) {
      for(unsigned j=0;j<m_trigWordLength;j++)
	m_com->write32(m_trigWord[m_trigWordLength-1-j]);
      m_com->releaseFifo();
      std::this_thread::sleep_for(std::chrono::microseconds((int)(1e6/m_trigFreq)));    
    } 
  } else {
    m_com->setTrigEmu(m_trigWord,m_trigWordLength,m_trigFreq,m_trigCnt);
    m_com->enableTrig();  
    double delay=1000/m_trigFreq*m_trigCnt*1.1;
    uint32_t count=0;
    while(m_com->trigBusy())  {count++; std::this_thread::sleep_for(std::chrono::milliseconds((unsigned)delay));}
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  trigProcRunning = false;
}

void RogueTxCore::doTriggerTime() {
  
}

