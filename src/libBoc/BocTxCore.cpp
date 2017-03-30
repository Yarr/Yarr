// #################################
// # Author: Marius Wensing
// # Email: marius.wensing at cern.ch
// # Project: Yarr
// # Description: BOC FW library
// # Comment: Transmitter Core
// ################################

#include "BocTxCore.h"

BocTxCore::BocTxCore()
{
	// we are currently not sending
	m_isSending = false;
}

BocTxCore::~BocTxCore()
{
	// stop triggering
	if(m_trigThread != nullptr)
	{
		// disable triggering
		m_trigThreadRunning = false;
		m_trigThread->join();
		delete m_trigThread;
		m_trigThread = nullptr;
	}
}

void BocTxCore::setCmdEnable(uint32_t value)
{
	// save mask internally
	m_enableMask = value;

	// update BOC configuration
	m_com->writeSingle(BMFN_OFFSET + BMF_TXBROADCAST0, (m_enableMask >>  0) & 0xFF);
	m_com->writeSingle(BMFN_OFFSET + BMF_TXBROADCAST1, (m_enableMask >>  8) & 0xFF);
	m_com->writeSingle(BMFS_OFFSET + BMF_TXBROADCAST0, (m_enableMask >> 16) & 0xFF);
	m_com->writeSingle(BMFS_OFFSET + BMF_TXBROADCAST1, (m_enableMask >> 24) & 0xFF);

	// make sure the configuration is matching
	m_com->writeSingle(BMFN_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x41);
	m_com->writeSingle(BMFS_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x41);

}

uint32_t BocTxCore::getCmdEnable()
{
	return m_enableMask;
}

//void BocTxCore::maskCmdEnable(uint32_t value, uint32_t mask)
//{
//    uint32_t tmp = getCmdEnable();
//    tmp &= ~mask;
//    value |= tmp;
//    setCmdEnable
//    setCmdEnable(value);
//}

void BocTxCore::writeFifo(uint32_t value)
{
	// split
	cmdFifo.push((value >> 24) & 0xFF);
	cmdFifo.push((value >> 16) & 0xFF);
	cmdFifo.push((value >>  8) & 0xFF);
	cmdFifo.push((value >>  0) & 0xFF);

	// is it neccessary to send out a packet
	if(cmdFifo.size() > 128)
	{
		uint8_t buf[128];

		// fill up temporary buffer
		for(int i = 0; i < 128; i++)
		{
			buf[i] = cmdFifo.front();
			cmdFifo.pop();
		}

		// send data to the BMF north
		if(m_enableMask & 0x0000FFFF)
		{
			m_com->writeSingle(BMFN_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x41);
			m_com->writeNonInc(BMFN_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_FIFO_DATA, buf, 128);
		}

		// send data to the BMF north
		if(m_enableMask & 0xFFFF0000)
		{
			m_com->writeSingle(BMFS_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x41);
			m_com->writeNonInc(BMFS_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_FIFO_DATA, buf, 128);
		}
	}
}

void BocTxCore::releaseFifo()
{
	// if still data is in the FIFO fill it
	while(!cmdFifo.empty())
	{
		uint8_t buf[128];
		size_t buflen = 0;

		// fill up temporary buffer
		while((buflen < 128) && (!cmdFifo.empty()))
		{
			buf[buflen] = cmdFifo.front();
			cmdFifo.pop();
			buflen++;
		}

		// send data to the BMF north
		if(m_enableMask & 0x0000FFFF)
		{
			m_com->writeSingle(BMFN_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x41);
			m_com->writeNonInc(BMFN_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_FIFO_DATA, buf, buflen);
		}

		// send data to the BMF north
		if(m_enableMask & 0xFFFF0000)
		{
			m_com->writeSingle(BMFS_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x41);
			m_com->writeNonInc(BMFS_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_FIFO_DATA, buf, buflen);
		}
	}

	// send north
	if(m_enableMask & 0x0000FFFF)
	{
		m_com->writeSingle(BMFN_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x45);
	}

	// send south
	if(m_enableMask & 0xFFFF0000)
	{
		m_com->writeSingle(BMFS_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x45);
	}
}


bool BocTxCore::isCmdEmpty()
{
	// get first enabled channel
	int ch;
	for(ch = 0; ch < 32; ch++)
	{
		if(m_enableMask & (1 << ch)) break;
	}

	// get status register from enabled channel
	uint8_t status;
	if(ch >= 16)
	{
		status = m_com->readSingle(BMFS_OFFSET + BMF_TX_OFFSET + 32 * (ch%16) + BMF_TX_STATUS);
	}
	else
	{
		status = m_com->readSingle(BMFN_OFFSET + BMF_TX_OFFSET + 32 * (ch%16) + BMF_TX_STATUS);
	}

	return (status & 0x1);
}

void BocTxCore::setTrigEnable(uint32_t value)
{
	m_trigMask = value;

	if(value == 0)
	{
		if(m_trigThread != nullptr)
		{
			// disable triggering
			m_trigThreadRunning = false;
			m_trigThread->join();
			delete m_trigThread;
			m_trigThread = nullptr;
		}
	}
	else
	{
		m_trigThreadRunning = true;
		m_trigThread = new std::thread(&BocTxCore::trigThreadProc, this);
	}
}

uint32_t BocTxCore::getTrigEnable()
{
	return m_trigMask;
}

void BocTxCore::maskTrigEnable(uint32_t value, uint32_t mask)
{
	uint32_t tmp = getTrigEnable();
    tmp &= ~mask;
    value |= tmp;
    setTrigEnable(value);
}

void BocTxCore::toggleTrigAbort()
{
	if(m_trigThread != nullptr)
	{
		m_trigThreadRunning = false;
		m_trigThread->join();
		delete m_trigThread;
		m_trigThread = nullptr;
	}
}

bool BocTxCore::isTrigDone()
{
	return !m_trigThreadRunning;
}

void BocTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg)
{

}

void BocTxCore::setTrigFreq(double freq)
{
	m_trigFreq = freq;
}

void BocTxCore::setTrigCnt(uint32_t count)
{
	m_trigCount = count;
}

void BocTxCore::setTrigTime(double time)
{
	m_trigTime = time;
}

void BocTxCore::setTrigWordLength(uint32_t length)
{

}

void BocTxCore::setTrigWord(uint32_t *word)
{
	m_trigWord[3] = word[3];
	m_trigWord[2] = word[2];
	m_trigWord[1] = word[1];
	m_trigWord[0] = word[0];
}

void BocTxCore::setTriggerLogicMask(uint32_t mask)
{

}

void BocTxCore::setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode)
{

}

void BocTxCore::resetTriggerLogic()
{

}

uint32_t BocTxCore::getTrigInCount()
{
	return 0;
}

void BocTxCore::trigThreadProc()
{
	// debug output
	std::cout << "Executing trigger thread..." << std::endl;
	std::cout << "Parameters: " << std::endl;
	std::cout << "    trigWord[3] = 0x" << std::hex << m_trigWord[3] << std::dec << std::endl;
	std::cout << "    trigWord[2] = 0x" << std::hex << m_trigWord[2] << std::dec << std::endl;
	std::cout << "    trigWord[1] = 0x" << std::hex << m_trigWord[1] << std::dec << std::endl;
	std::cout << "    trigWord[0] = 0x" << std::hex << m_trigWord[0] << std::dec << std::endl;
	std::cout << "    trigCount = " << m_trigCount << std::endl;

	uint32_t trigNum = 0;
	double trigTime = 0;

	// set up trigger mask
	m_com->writeSingle(BMFN_OFFSET + BMF_TXBROADCAST0, (m_trigMask >>  0) & 0xFF);
	m_com->writeSingle(BMFN_OFFSET + BMF_TXBROADCAST1, (m_trigMask >>  8) & 0xFF);
	m_com->writeSingle(BMFS_OFFSET + BMF_TXBROADCAST0, (m_trigMask >> 16) & 0xFF);
	m_com->writeSingle(BMFS_OFFSET + BMF_TXBROADCAST1, (m_trigMask >> 24) & 0xFF);

	// we can abort triggering by setting trigThreadRunning to false
	while(m_trigThreadRunning)
	{
		// put trigger word into the FIFO
		writeFifo(m_trigWord[3]);
		writeFifo(m_trigWord[2]);
		writeFifo(m_trigWord[1]);
		writeFifo(m_trigWord[0]);

		// send out the trigger
		releaseFifo();

		// count triggers sent out
		trigNum++;

		// output trigger number
		std::cout << "Sending trigger #" << trigNum << std::endl;

		// did we reach trigger count
		if(trigNum == m_trigCount)
			m_trigThreadRunning = false;

		// sleep a bit
		std::this_thread::sleep_for(std::chrono::microseconds((int)(1000000.0 / m_trigFreq)));
		trigTime += 1.0 / m_trigFreq;

		// stop triggering after time
		if(trigTime > m_trigTime)
			m_trigThreadRunning = false;
	}

	// debug
	std::cout << "finished trigger thread and cleaning up" << std::endl;

	// restore mask to cmd mask
	m_com->writeSingle(BMFN_OFFSET + BMF_TXBROADCAST0, (m_enableMask >>  0) & 0xFF);
	m_com->writeSingle(BMFN_OFFSET + BMF_TXBROADCAST1, (m_enableMask >>  8) & 0xFF);
	m_com->writeSingle(BMFS_OFFSET + BMF_TXBROADCAST0, (m_enableMask >> 16) & 0xFF);
	m_com->writeSingle(BMFS_OFFSET + BMF_TXBROADCAST1, (m_enableMask >> 24) & 0xFF);
}
