/*
 */

#include "NetioTxCore.h"
#include "BitStream.h"
#include <unistd.h>
#include <bitset>
#include "include/StarTriggerLoop.h"

StarTriggerLoop::StarTriggerLoop() : LoopActionBase() {

	m_trigCnt = 50; // Maximum number of triggers to send
	m_trigDelay = 45; // L0_delay 34
	m_trigFreq = 1e3; // 1kHz
	m_trigTime = 10; // 10s
	//    m_trigWord[0] = 0x00;
	//    m_trigWord[1] = TRIG_CMD;
	//    m_trigWord[2] = 0x00;
	//    m_trigWord[3] = CAL_CMD;
	m_noInject = false;
	m_extTrigger = false;
	isInner = false;
	min = 0;
	max = 0;
	step = 1;
	loopType = typeid(this);
}

void StarTriggerLoop::init() {
	m_done = false;
	if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
	// Setup Trigger
	//this->setTrigDelay(m_trigDelay);  //set using StarCfg class
	if (m_trigCnt > 0) {
		g_tx->setTrigConfig(INT_COUNT); //use internal charge injection
	} else {
		g_tx->setTrigConfig(INT_TIME);  //external trigger
	}
	g_tx->setTrigFreq(m_trigFreq);
	g_tx->setTrigCnt(m_trigCnt);
	//    g_tx->setTrigWordLength(m_trigWordLength);
	//    g_tx->setTrigWord(m_trigWord,4);
	g_tx->setTrigTime(m_trigTime);


	prepareL0Trigger();
	prepareL1Trigger();

	//std::this_thread::sleep_for(std::chrono::microseconds(50));
//	for ( FrontEnd* fe : keeper->feList ) {
////		if (dynamic_cast<FrontEndCfg*>(fe)->getName().find("hcc") != std::string::npos) {continue;}  ///fix me, maybe @@@
//		if (!fe->isActive()) {continue;}
//		static_cast<StarChips*> (fe)->setRunMode();
//		//g_tx->setTrigChannel(fe->getTxChannel(),true);
//	}

	// Workaround: Put everything into run mode, active rx channels will sort this out
	//    g_tx->setCmdEnable(keeper->getTxMask());
	//    keeper->globalFe<Star>()->setRunMode(true);
	//    usleep(100); // Empty could be delayed
	while(!g_tx->isCmdEmpty());
}

void StarTriggerLoop::end() {
	if (verbose)
		std::cout << __PRETTY_FUNCTION__ << std::endl;

	// Go back to general state of FE, do something here (if needed)
	while(!g_tx->isCmdEmpty());
}

void StarTriggerLoop::execPart1() {
	if (verbose)
		std::cout << __PRETTY_FUNCTION__ << std::endl;
	// Enable Trigger
	g_tx->setTrigEnable(0x1);
}

void StarTriggerLoop::execPart2() {
	if (verbose)
		std::cout << __PRETTY_FUNCTION__ << std::endl;
	while(!g_tx->isTrigDone());
	// Disable Trigger
	g_tx->setTrigEnable(0x0);
	m_done = true;
}

void StarTriggerLoop::setTrigCnt(unsigned int cnt) {
	m_trigCnt = cnt;
}

unsigned int StarTriggerLoop::getTrigCnt() {
	return m_trigCnt;
}

void StarTriggerLoop::setTrigDelay(unsigned int delay) {
	unsigned pos = (delay-1)%32; // subtract 8 bit long trig cmd
	unsigned word = (delay-1)/32; // Select word in array
	m_trigWord[0] = 0;
	m_trigWord[1] = 0;
	m_trigWord[2] = 0;
	m_trigWord[3] = CAL_CMD;
	if ((word < 3 && pos <= 27) || word < 2) {
		m_trigWord[2-word] = (TRIG_CMD>>pos);
		if (pos > 27) // In case we shifted over word border
			m_trigWord[2-1-word] = (TRIG_CMD<<(5-(32-pos)));
		m_trigDelay = delay;
	}
	m_trigWordLength = 32 + delay;
}

void StarTriggerLoop::setNoInject() {
	m_trigWord[0] = 0;
	m_trigWord[1] = 0;
	m_trigWord[2] = 0;
	m_trigWord[3] = TRIG_CMD;
	m_trigWordLength = 4;
}

void StarTriggerLoop::setNoWord() {
	m_trigWord[0] = 0;
	m_trigWord[1] = 0;
	m_trigWord[2] = 0;
	m_trigWord[3] = 0;
}

unsigned int StarTriggerLoop::getTrigDelay() {
	return m_trigDelay;
}

void StarTriggerLoop::setTrigFreq(double freq) {
	m_trigFreq = freq;
}

double StarTriggerLoop::getTrigFreq() {
	return m_trigFreq;
}

void StarTriggerLoop::setTrigTime(double time) {
	m_trigTime = time;
}

double StarTriggerLoop::getTrigTime() {
	return m_trigTime;
}

/*
void StarTriggerLoop::setIsInner(bool itis) {
    isInner = itis;
}

bool StarTriggerLoop::getIsInner() {
    return isInner;
}
 */


//void NetioTxCore::triggerL1(unsigned l0ID) {
//
//	uint64_t cmd = (0b110 << 9) + ((l0ID & 0xff) << 1);
//	//writeFifo(2, interleaveL0CMD(cmd));
//	writeFifo(2, cmd); //fix me, hardcoded elink chn
//	this->releaseFifo((uint32_t) 2);	//fix me, hardcoded elink chn
//
//}

void StarTriggerLoop::prepareL0Trigger() {
	//std::cout << __PRETTY_FUNCTION__ <<  std::endl;

	std::string cmd = interleaveL0CMD(0x2FBFF0100000001, m_trigDelay); //broadcast
	//reg_0 (broadcast hccID=11111, abcID=11111): 1011 111 0 11111 11111 00000001 00000000000000000000000000000001+ m_trigDelay
//	std::string cmd = interleaveL0CMD(0x2FA550100000001, m_trigDelay);//chip 21 only
//	std::string cmd = interleaveL0CMD(0x2FA570100000001, m_trigDelay);//chip 23 only

//	std::string cmd = interleaveL0CMD(0x2FA760100000001, m_trigDelay); //@@@ hardcoded trig cmd(same hcc=19/abc ID=22): 001011111010011101100000000100000000000000000000000000000001?
	BitStream bs;
	bs.FromString(cmd);
	bs.Pack();


    NetioTxCore* txcore = dynamic_cast<NetioTxCore*> (g_tx);
//    std::cout << __PRETTY_FUNCTION__ << "  ---------------netiotxcore: " << txcore << std::endl;
//    std::map<uint64_t,bool>::iterator it;
//    for(it=txcore->m_elinks.begin();it!=txcore->m_elinks.end();it++){
//    	txcore->m_trigFifo[it->first].clear();
//    	for (unsigned i=0; i<bs.GetSize(); ++i){
//    		txcore->writeFifo(&(txcore->m_trigFifo[it->first]), bs.GetWord(i));
//    	}
//    }








}



void StarTriggerLoop::prepareL1Trigger(unsigned l0ID) {
	//std::cout << __PRETTY_FUNCTION__ <<  std::endl;
//	uint64_t cmd = (0b110 << 9) + ((l0ID & 0xff) << 1);
	std::string cmd = interleaveL0CMD((0b110 << 9) + ((l0ID & 0xff) << 1));

	BitStream bs;
	bs.FromString(cmd);
	bs.Pack();

	NetioTxCore* txcore = dynamic_cast<NetioTxCore*> (g_tx);
	//    std::cout << __PRETTY_FUNCTION__ << "  ---------------netiotxcore: " << txcore << std::endl;
//	std::map<uint64_t,bool>::iterator it;
//	for(it=txcore->m_trigElinks.begin();it!=txcore->m_trigElinks.end();it++){
//		txcore->m_trigFifoL1[it->first].clear();
//		for (unsigned i=0; i<bs.GetSize(); ++i){
//			txcore->writeFifo(&(txcore->m_trigFifoL1[it->first]), bs.GetWord(i));
//		}
//	}



}


//if R3_L1 and L0_CMD is not time-multiplexed, then this is not needed.
std::string StarTriggerLoop::interleaveL0CMD(uint64_t cmd, unsigned l0_delay) { //l0_delay is explicitly required to be passed manually


	int cmd_len = bitlen(cmd);
	if (verbose)
		std::cout << "cmd length: " << cmd_len << " _ " << std::bitset<64>(cmd)
				<< std::endl;

	std::string cmd_for_fup = "";
	while (cmd_len % 4 != 0) {//The command for fupload is in unite of Byte
		cmd <<= 1;
		cmd_len += 1;
	}

	for (int i = 0; i < int(bitlen(cmd) / 4); i++) {
		int bit_a = (cmd >> (cmd_len - i * 4 - 1)) & 0b1;
		int bit_b = (cmd >> (cmd_len - i * 4 - 2)) & 0b1;
		int bit_c = (cmd >> (cmd_len - i * 4 - 3)) & 0b1;
		int bit_d = (cmd >> (cmd_len - i * 4 - 4)) & 0b1;
		// bit order swap in one Byte data, due to LSB/MSB difference
		std::bitset<8> bit_all(bit_a << 1 | bit_b << 3 | bit_c << 5 | bit_d
				<< 7);
		cmd_for_fup += bit_all.to_string();

		//		cmd_for_fup+=int_to_hex<int>(bit_a << 1 | bit_b << 3 | bit_c << 5 | bit_d << 7);
		//		std::cout << "@@@----> "<< std::hex << (int)(bit_a << 1 | bit_b << 3 | bit_c << 5 | bit_d << 7) << std::endl;
	}

	if (l0_delay > 3) {
		l0_delay -= 2;
		while (l0_delay > 4) {
			//cmd_for_fup.append(0);
			cmd_for_fup += "00000000";
			l0_delay -= 4;
		}

		std::bitset<8> bit_l0Trig(1 << ((l0_delay - 1) * 2));
		cmd_for_fup.append(bit_l0Trig.to_string());
	} else if (l0_delay > 0 && l0_delay < 3) {
		std::cout << "Wrong trigger delay value" << std::endl;
	}

	if (verbose)
		std::cout << "cmd after :" << cmd_for_fup << std::endl;
	return cmd_for_fup;
}


void StarTriggerLoop::writeConfig(json &config) {
	config["count"] = m_trigCnt;
	config["frequency"] = m_trigFreq;
	config["time"] = m_trigTime;
	config["delay"] = m_trigDelay;
	config["noInject"] = m_noInject;
	config["extTrigger"] = m_extTrigger;
}

void StarTriggerLoop::loadConfig(json &config) {

	m_trigCnt = config["trig_count"];
	m_trigFreq = config["trig_frequency"];
	//m_trigTime = config["trig_time"];
	m_trigDelay = config["l0_delay"];
	verbose = config["verbose"];
	// TODO these two don't do anything yet
//	m_noInject = config["noInject"];
//	m_extTrigger = config["extTrigger"];

	std::cout << "------trig_count: " <<  m_trigCnt
			  << "------trig_frequency: " << m_trigFreq
			  << "------l0_delay: " << m_trigDelay
			  <<std::endl;


}


void StarTriggerLoop::setTrigWord(uint32_t word[4]) {
	m_trigWord[0] = word[0];
	m_trigWord[1] = word[1];
	m_trigWord[2] = word[2];
	m_trigWord[3] = word[3];
}
