// #################################
// # Author: Marius Wensing
// # Email: marius.wensing at cern.ch
// # Project: Yarr
// # Description: KU040 FW library
// # Comment: Transmitter Core
// ################################

#include "KU040TxCore.h"
#include "KU040Registers.h"

KU040TxCore::KU040TxCore()
{
	// we are currently not sending
	m_isSending = false;
}

KU040TxCore::~KU040TxCore()
{
    setTrigEnable(0);
}

void KU040TxCore::DumpTxCounters()
{
	std::cout << "===== TX counters =======" << std::endl;
	std::cout << "channel\ttrig\tECR\tBCR\tCAL\tSLOW\tcorrupt" << std::endl;
	for(int i = 0; i < 20; i++)
	{
		uint32_t count_trig = m_com->Read(KU040_PIXLE_TX_CNT_TRIG(i));
		uint32_t count_ecr = m_com->Read(KU040_PIXLE_TX_CNT_ECR(i));
		uint32_t count_bcr = m_com->Read(KU040_PIXLE_TX_CNT_BCR(i));
		uint32_t count_cal = m_com->Read(KU040_PIXLE_TX_CNT_CAL(i));
		uint32_t count_slow = m_com->Read(KU040_PIXLE_TX_CNT_SLOW(i));
		uint32_t count_corrupt = m_com->Read(KU040_PIXLE_TX_CNT_CORRUPT(i));
		std::cout << i << "\t" << count_trig << "\t" << count_ecr << "\t" << count_bcr << "\t" << count_cal << "\t" << count_slow << "\t" << count_corrupt << std::endl;
	}
}

void KU040TxCore::setCmdEnable(uint32_t value)
{
	// save mask internally
	m_enableMask = value;

	// update TX configuration
	for(int i = 0; i < 20; i++)
	{
		// first reset the counters of the corresponding channel
		m_com->Write(KU040_PIXEL_TX_CONTROL(i), 0x8);
		m_com->Write(KU040_PIXEL_TX_CONTROL(i), 0x0);

		if(m_enableMask & (1 << i))
		{
			// enable the corresponding channel (with NRZ encoding)
			m_com->Write(KU040_PIXEL_TX_CONTROL(i), 0x1);
		}
		else
		{
			// disable the corresponding channel
			m_com->Write(KU040_PIXEL_TX_CONTROL(i), 0x0);
		}
	}
}

uint32_t KU040TxCore::getCmdEnable()
{
	return m_enableMask;
}

//void KU040TxCore::maskCmdEnable(uint32_t value, uint32_t mask)
//{
//    uint32_t tmp = getCmdEnable();
//    tmp &= ~mask;
//    value |= tmp;
//    setCmdEnable
//    setCmdEnable(value);
//}

void KU040TxCore::writeFifo(uint32_t value)
{
	cmdFifo.push(value);
}

void KU040TxCore::releaseFifo()
{
	size_t len = cmdFifo.size();
	uint32_t *buf = new uint32_t[len];

	// read all elements from the queue
	for(uint32_t i = 0; i < len; i++)
	{
		buf[i] = cmdFifo.front();
		cmdFifo.pop();
	}

	// send the queue to all enabled channels
	for(int i = 0; i < 20; i++)
	{
		if(m_enableMask & (1 << i))
		{
			m_com->Write(KU040_PIXEL_TX_FIFO(i), buf, len, true);
		}
	}

	delete[] buf;
}

bool KU040TxCore::isCmdEmpty()
{
	// check all enabled channels
	for(int i = 0; i < 20; i++)
	{
		if(m_enableMask & (1 << i))
		{
			if((m_com->Read(KU040_PIXEL_TX_STATUS(i)) & 0x2) == 0)
			{
				return false;
			}
		}
	}

	return true;
}

void KU040TxCore::setTrigEnable(uint32_t value)
{
	m_trigMask = value;

	if(value == 0)
	{
        // switch to internal serial port
		for(int i = 0; i < 20; i++)
		{
			if(m_enableMask & (1 << i))
			{
				// select internal trigger generator
				m_com->RMWbits(KU040_PIXEL_TX_CONTROL(i), 0xFFFFFFFBU, 0x0);
			}
		}
	}
	else
	{
        // abort and stop
		toggleTrigAbort();

        // select internal trigger generator for all selected channels
		for(int i = 0; i < 20; i++)
		{
			if(m_enableMask & (1 << i))
			{
				// select internal trigger generator
				m_com->RMWbits(KU040_PIXEL_TX_CONTROL(i), 0xFFFFFFFFU, 0x4);
			}
		}

        // start triggering
        if(m_trigCfg == INT_TIME)
        {
			m_com->Write(KU040_PIXEL_TRIGGEN_CONTROL, 0x5);
        }
        else
        {
			m_com->Write(KU040_PIXEL_TRIGGEN_CONTROL, 0x1);
		}
	}
}

uint32_t KU040TxCore::getTrigEnable()
{
	return m_trigMask;
}

void KU040TxCore::maskTrigEnable(uint32_t value, uint32_t mask)
{
	uint32_t tmp = getTrigEnable();
    tmp &= ~mask;
    value |= tmp;
    setTrigEnable(value);
}

void KU040TxCore::toggleTrigAbort()
{
    // abort and stop
    m_com->Write(KU040_PIXEL_TRIGGEN_CONTROL, 0x2);
}

bool KU040TxCore::isTrigDone()
{
	return m_com->Read(KU040_PIXEL_TRIGGEN_STATUS) & 0x1;
}

void KU040TxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg)
{
    m_trigCfg = cfg;
}

void KU040TxCore::setTrigFreq(double freq)
{
    // limit frequency to 200 kHz
    if(freq > 200e3)  freq = 200e3;

	uint32_t freq_int = (40e6 / freq) - 1;

	m_com->Write(KU040_PIXEL_TRIGGEN_FREQ, freq_int);
}

void KU040TxCore::setTrigCnt(uint32_t count)
{
	m_com->Write(KU040_PIXEL_TRIGGEN_COUNT, count);
}

void KU040TxCore::setTrigTime(double time)
{
	uint64_t time_int = (time * 40e6) - 1;

	uint32_t tmp[2];
	tmp[0] = time_int & 0xFFFFFFFFU;
	tmp[1] = time_int >> 32;

	m_com->Write(KU040_PIXEL_TRIGGEN_TIME1, tmp, 2, false);
}

void KU040TxCore::setTrigWordLength(uint32_t length)
{

}

void KU040TxCore::setTrigWord(uint32_t *word, uint32_t length)
{
	m_com->Write(KU040_PIXEL_TRIGGEN_WORD1, word, length, false);
}

void KU040TxCore::setTriggerLogicMask(uint32_t mask)
{

}

void KU040TxCore::setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode)
{

}

void KU040TxCore::resetTriggerLogic()
{
    toggleTrigAbort();
}

uint32_t KU040TxCore::getTrigInCount()
{
	return 0;
}

