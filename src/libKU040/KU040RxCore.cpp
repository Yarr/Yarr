#include "KU040RxCore.h"
#include "KU040Registers.h"
#include <cstring>

KU040RxCore::KU040RxCore()
{
	m_enableMask = 0;
	m_skipRecsWithErrors = false;
}

KU040RxCore::~KU040RxCore()
{
	std::cout << "desctructor" << std::endl;
	// disable all channels
	for(int ch = 0; ch < 20; ch++)
	{
		std::cout << "Disabling RX channel " << ch << std::endl;
		m_com->Write(KU040_PIXEL_RX_CONTROL(ch), 6);				// leave all RX channels ON
	}
}

void KU040RxCore::setRxEnable(uint32_t val)
{
	// save mask locally
	m_enableMask = val;

	// loop through channels and enable them
	for(int ch = 0; ch < 20; ch++)
	{
		// clear the FIFO and reset the counters
		m_com->Write(KU040_PIXEL_RX_CONTROL(ch), 0x18);
		m_com->Write(KU040_PIXEL_RX_CONTROL(ch), 0x0);

		if(m_enableMask & (1 << ch))
		{
			// channel is enabled
			m_com->Write(KU040_PIXEL_RX_CONTROL(ch), 0x6);
		}
		else
		{
			// channel is disabled
			m_com->Write(KU040_PIXEL_RX_CONTROL(ch), 0x0);
		}
	}
}

void KU040RxCore::maskRxEnable(uint32_t val, uint32_t mask)
{
	uint32_t tmp = m_enableMask;
    tmp &= ~mask;
    val |= tmp;
    setRxEnable(val);
}

RawData* KU040RxCore::readData()
{
	std::vector<uint32_t> formatted_data;

	// loop through all enabled channels and fetch the FIFO data
	for(int ch = 0; ch < 20; ch++)
	{
		// skip disabled channels
		if((m_enableMask & (1 << ch)) == 0) continue;

		// get FIFO content length
		uint32_t fifo_words = m_com->Read(KU040_PIXEL_RX_FIFOCNT(ch));

		// process if data is available
		if(fifo_words > 0)
		{
			uint32_t *buf = new uint32_t[fifo_words];

			// read everything
			m_com->Read(KU040_PIXEL_RX_FIFO(ch), buf, fifo_words, true);

			// push it
			for(uint32_t i = 0; i < fifo_words; i++)
			{
				// push only valid records or all records if m_skipRecsWithError is false
				if(!m_skipRecsWithErrors || ((buf[i] & (1U<<25)) == 0)) 
				{
					formatted_data.push_back((ch << 26) | (0x0 << 24) | (buf[i] & 0xFFFFFF));
				}
				else
				{
					std::cout << "Skipping record 0x" << std::hex << buf[i] << std::dec << std::endl;
				}
			}

			delete[] buf;
		}
	}

	// return the data to caller
	if(formatted_data.size() == 0)
	{
		return NULL;
	}
	else
	{
		uint32_t *buf = new uint32_t[formatted_data.size()];
		std::copy(formatted_data.begin(), formatted_data.end(), buf);
		//std::cout << "returning " << formatted_data.size() << " records." << std::endl;
		return new RawData(0x0, buf, formatted_data.size());
	}
}

uint32_t KU040RxCore::getDataRate()
{
	return 0;
}

uint32_t KU040RxCore::getCurCount()
{
	return 0;
}

bool KU040RxCore::isBridgeEmpty()
{
	for(int ch = 0; ch < 20; ch++)
	{
		// skip disabled channels
		if((m_enableMask & (1 << ch)) == 0) continue;

		// channel is disabled
		if(m_com->Read(KU040_PIXEL_RX_STATUS(ch)) & 0x4)
		{
			return false;
		}
	}

	// if we reach this point no channel had data
	return true;
}

void KU040RxCore::setEmu(uint32_t mask, uint8_t hitcnt)
{
	// save mask
	m_emuMask = mask;

	// set emulator enable mask
	m_com->Write(KU040_PIXEL_DEBUG_EMU_ENABLE, mask);
	m_com->Write(KU040_PIXEL_DEBUG_EMU_HITDIST, 0x050003);
}

uint32_t KU040RxCore::getEmu()
{
	return m_emuMask;
}
