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
    setTrigEnable(0);
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
        // disable the trigger generator
        m_com->writeSingle(BMFN_OFFSET + BMF_TRIG_CTRL, 0);
        m_com->writeSingle(BMFS_OFFSET + BMF_TRIG_CTRL, 0);
	}
	else
	{
        // abort and stop
        m_com->writeSingle(BMFN_OFFSET + BMF_TRIG_CTRL, 0x2);
        m_com->writeSingle(BMFS_OFFSET + BMF_TRIG_CTRL, 0x2);

        // trigger configuration
        uint32_t cnt_int = m_trigCount;
        uint32_t freq_int = (40e6 / m_trigFreq) - 1;
        uint64_t time_int = (m_trigTime * 40e6) - 1;
        uint8_t configBytes[16];
        configBytes[0] = (time_int >> 56) & 0xFF;
        configBytes[1] = (time_int >> 48) & 0xFF;
        configBytes[2] = (time_int >> 40) & 0xFF;
        configBytes[3] = (time_int >> 32) & 0xFF;
        configBytes[4] = (time_int >> 24) & 0xFF;
        configBytes[5] = (time_int >> 16) & 0xFF;
        configBytes[6] = (time_int >>  8) & 0xFF;
        configBytes[7] = (time_int >>  0) & 0xFF;
        configBytes[8] = (freq_int >> 24) & 0xFF;
        configBytes[9] = (freq_int >> 16) & 0xFF;
        configBytes[10] = (freq_int >>  8) & 0xFF;
        configBytes[11] = (freq_int >>  0) & 0xFF;
        configBytes[12] = (cnt_int >> 24) & 0xFF;
        configBytes[13] = (cnt_int >> 16) & 0xFF;
        configBytes[14] = (cnt_int >>  8) & 0xFF;
        configBytes[15] = (cnt_int >>  0) & 0xFF;
        m_com->writeNonInc(BMFN_OFFSET + BMF_TRIG_CONFIG, configBytes, 16);
        m_com->writeNonInc(BMFS_OFFSET + BMF_TRIG_CONFIG, configBytes, 16);

        // first make sure we don't send anything from the FIFO
	    if(m_enableMask & 0x0000FFFF)
	    {
		    m_com->writeSingle(BMFN_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x41);
	    }
	    if(m_enableMask & 0xFFFF0000)
	    {
		    m_com->writeSingle(BMFS_OFFSET + BMF_TX_OFFSET + 32 * 16 + BMF_TX_CTRL, 0x41);
	    }

        // start triggering
        if(m_trigCfg == INT_TIME)
        {
            m_com->writeSingle(BMFN_OFFSET + BMF_TRIG_CTRL, 0x5);
            m_com->writeSingle(BMFS_OFFSET + BMF_TRIG_CTRL, 0x5);
        }
        else
        {
            m_com->writeSingle(BMFN_OFFSET + BMF_TRIG_CTRL, 0x1);
            m_com->writeSingle(BMFS_OFFSET + BMF_TRIG_CTRL, 0x1);
        }
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
    // abort and stop
    m_com->writeSingle(BMFN_OFFSET + BMF_TRIG_CTRL, 0x2);
    m_com->writeSingle(BMFS_OFFSET + BMF_TRIG_CTRL, 0x2);
}

bool BocTxCore::isTrigDone()
{
    uint8_t north_status = m_com->readSingle(BMFN_OFFSET + BMF_TRIG_STATUS);
    uint8_t south_status = m_com->readSingle(BMFS_OFFSET + BMF_TRIG_STATUS);

    return (north_status & 0x1) && (south_status & 0x1);
}

void BocTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg)
{
    m_trigCfg = cfg;
}

void BocTxCore::setTrigFreq(double freq)
{
    // limit frequency to 200 kHz
    if(freq > 200e3)  freq = 200e3;

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

void BocTxCore::setTrigWord(uint32_t *word, uint32_t length)
{

    // TODO fix me
    // convert trigger word to bytes
    uint8_t trigBytes[16];
    trigBytes[0] = (word[3] >> 24) & 0xFF;
    trigBytes[1] = (word[3] >> 16) & 0xFF;
    trigBytes[2] = (word[3] >>  8) & 0xFF;
    trigBytes[3] = (word[3] >>  0) & 0xFF;
    trigBytes[4] = (word[2] >> 24) & 0xFF;
    trigBytes[5] = (word[2] >> 16) & 0xFF;
    trigBytes[6] = (word[2] >>  8) & 0xFF;
    trigBytes[7] = (word[2] >>  0) & 0xFF;
    trigBytes[8] = (word[1] >> 24) & 0xFF;
    trigBytes[9] = (word[1] >> 16) & 0xFF;
    trigBytes[10] = (word[1] >>  8) & 0xFF;
    trigBytes[11] = (word[1] >>  0) & 0xFF;
    trigBytes[12] = (word[0] >> 24) & 0xFF;
    trigBytes[13] = (word[0] >> 16) & 0xFF;
    trigBytes[14] = (word[0] >>  8) & 0xFF;
    trigBytes[15] = (word[0] >>  0) & 0xFF;

    // write all trigger words to the BOC at once
    m_com->writeNonInc(BMFN_OFFSET + BMF_TRIG_WORD, trigBytes, 16);
    m_com->writeNonInc(BMFS_OFFSET + BMF_TRIG_WORD, trigBytes, 16);
}

void BocTxCore::setTriggerLogicMask(uint32_t mask)
{

}

void BocTxCore::setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode)
{

}

void BocTxCore::resetTriggerLogic()
{
    toggleTrigAbort();
}

uint32_t BocTxCore::getTrigInCount()
{
	return 0;
}

