#include "BocRxCore.h"
#include <cstring>

BocRxCore::BocRxCore()
{
	m_enableMask = 0;

	for(int ch = 0; ch < 32; ch++)
	{
		m_formState[ch] = 0;
	}
}

BocRxCore::~BocRxCore()
{
	// disable all channels
	for(int ch = 0; ch < 32; ch++)
	{
		// channel is disabled
		if(ch >= 16)
		{
			m_com->writeSingle(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL, 0x0);
		}
		else
		{
			m_com->writeSingle(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL, 0x0);
		}
	}
}

void BocRxCore::setRxEnable(uint32_t val)
{
	// save mask locally
	m_enableMask = val;

	// loop through channels and enable them
	for(int ch = 0; ch < 32; ch++)
	{
		if(m_enableMask & (1 << ch))
		{
			// channel is enabled
			if(ch >= 16)
			{
				uint8_t tmp = m_com->readSingle(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL);
				tmp = (tmp & 0xE0) | 0x5;
				m_com->writeSingle(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL, tmp);
				m_com->writeSingle(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_DCNT_LOW, 0x0);	// reset FIFO
			}
			else
			{
				uint8_t tmp = m_com->readSingle(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL);
				tmp = (tmp & 0xE0) | 0x5;
				m_com->writeSingle(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL, tmp);
				m_com->writeSingle(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_DCNT_LOW, 0x0);	// reset FIFO
			}			
		}
		else
		{
			// channel is disabled
			if(ch >= 16)
			{
				uint8_t tmp = m_com->readSingle(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL);
				tmp = (tmp & 0xE0);
				m_com->writeSingle(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL, tmp);

			}
			else
			{
				uint8_t tmp = m_com->readSingle(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL);
				tmp = (tmp & 0xE0);
				m_com->writeSingle(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL, tmp);
			}
		}
	}
}

void BocRxCore::maskRxEnable(uint32_t val, uint32_t mask)
{
	uint32_t tmp = m_enableMask;
    tmp &= ~mask;
    val |= tmp;
    setRxEnable(val);
}

RawData* BocRxCore::readData()
{
	std::vector<uint32_t> formatted_data;

	// loop through all enabled channels and fetch the FIFO data
	for(int ch = 0; ch < 32; ch++)
	{
		// skip disabled channels
		if((m_enableMask & (1 << ch)) == 0) continue;

		// skip empty channels
		if(ch >= 16)
		{
			// get number of words in RX FIFO
			uint16_t fifo_words;
			uint8_t tmp[2];
			m_com->readInc(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_DCNT_LOW, tmp, 2);
			fifo_words = (uint16_t) tmp[0];
			fifo_words |= (uint16_t) tmp[1] << 8;

			while(fifo_words > 0)
			{
				// we will transfer 128 words at a time
				uint16_t transaction_words = (fifo_words > 128) ? 128 : fifo_words;
				uint16_t buf[128];

				m_com->read16(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_DATA_HIGH,
							  BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_DATA_LOW,
							  buf,
							  transaction_words);

				for(int i = 0; i < transaction_words; i++)
				{
					m_rxData[ch].push(buf[i]);
				}

				// decrement fifo_words
				fifo_words -= transaction_words;
			}
		}
		else
		{
			// get number of words in RX FIFO
			uint16_t fifo_words;
			uint8_t	tmp[2];
                        m_com->readInc(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_DCNT_LOW, tmp, 2);
                        fifo_words = (uint16_t)	tmp[0];
                        fifo_words |= (uint16_t) tmp[1]	<< 8;

			while(fifo_words > 0)
			{
				// we will transfer 128 words at a time
				uint16_t transaction_words = (fifo_words > 128) ? 128 : fifo_words;
				uint16_t buf[128];

				m_com->read16(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_DATA_HIGH,
							  BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_DATA_LOW,
							  buf,
							  transaction_words);

				for(int i = 0; i < transaction_words; i++)
				{
					m_rxData[ch].push(buf[i]);
				}

				// decrement fifo_words
				fifo_words -= transaction_words;
			}
		}
	}

	// let's format the data
	for(int ch = 0; ch < 32; ch++)
	{
		// build records
		while(m_rxData[ch].size() > 0)
		{
			uint16_t data = m_rxData[ch].front();
			m_rxData[ch].pop();

			// synchronize on k-word
			if(data & 0x100)
			{
				if((data & 0xFF) == 0xFC)
					m_formState[ch] = 1;
				else
					m_formState[ch] = 0;
			}
			else
			{
				switch(m_formState[ch])
				{
					case 0:
						// we should not get here...
						std::cout << "Skipping data outside frame..." << std::endl;
						break;

					case 1:
						m_formRecord[ch] = (uint32_t)(data & 0xFF) << 16;
						m_formState[ch]++;
						break;

					case 2:
						m_formRecord[ch] |= (uint32_t)(data & 0xFF) <<  8;
						m_formState[ch]++;
						break;

					case 3:
						m_formRecord[ch] |= (uint32_t)(data & 0xFF) << 0;
						m_formState[ch] = 1;
						formatted_data.push_back((ch << 26) | (0x0 << 24) | m_formRecord[ch]);
						break;

				}
			}
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
		std::cout << "returning " << formatted_data.size() << " records." << std::endl;
		return new RawData(0x0, buf, formatted_data.size());
	}
}

uint32_t BocRxCore::getDataRate()
{
	return 0;
}

uint32_t BocRxCore::getCurCount()
{
	return 0;
}

bool BocRxCore::isBridgeEmpty()
{
	for(int ch = 0; ch < 32; ch++)
	{
		// skip disabled channels
		if((m_enableMask & (1 << ch)) == 0) continue;

		// channel is disabled
		if(ch >= 16)
		{
			if((m_com->readSingle(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_STATUS) & 0x20) == 0) {
				return false;
			}
		}
		else
		{
			if((m_com->readSingle(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_STATUS) & 0x20) == 0) {
				return false;
			}
		}
	}

	// if we reach this point no channel had data
	return true;
}

void BocRxCore::setEmu(uint32_t mask, uint8_t hitcnt)
{
	// save mask
	m_emuMask = mask;

	// loop through the channels to enable the emulator
	for(int ch = 0; ch < 32; ch++)
	{
		if(ch >= 16)
		{
			uint8_t tmp = m_com->readSingle(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL);
			tmp = (tmp & 0x1F);
			if(mask & (1 << ch))
			{
				std::cout << "Enabling FE-I4 emulator on channel " << ch << std::endl;
				tmp |= 0x20;
				m_com->writeSingle(BMFS_OFFSET + BMF_EMU_OFFSET + 8 * (ch%16) + BMF_EMU_CTRL, 0x5);
				m_com->writeSingle(BMFS_OFFSET + BMF_EMU_OFFSET + 8 * (ch%16) + BMF_EMU_HITCNT, hitcnt);
			}
			else
			{
				m_com->writeSingle(BMFS_OFFSET + BMF_EMU_OFFSET + 8 * (ch%16) + BMF_EMU_CTRL, 0x0);
			}
			m_com->writeSingle(BMFS_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL, tmp);
		}
		else
		{
			uint8_t tmp = m_com->readSingle(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL);
			tmp = (tmp & 0x1F);
			if(mask & (1 << ch))
			{
				std::cout << "Enabling FE-I4 emulator on channel " << ch << std::endl;
				tmp |= 0x20;
				m_com->writeSingle(BMFN_OFFSET + BMF_EMU_OFFSET + 8 * (ch%16) + BMF_EMU_CTRL, 0x5);
				m_com->writeSingle(BMFN_OFFSET + BMF_EMU_OFFSET + 8 * (ch%16) + BMF_EMU_HITCNT, hitcnt);
			}
			else
			{
				m_com->writeSingle(BMFN_OFFSET + BMF_EMU_OFFSET + 8 * (ch%16) + BMF_EMU_CTRL, 0x0);
			}
			m_com->writeSingle(BMFN_OFFSET + BMF_RX_OFFSET + 32 * (ch%16) + BMF_RX_CTRL, tmp);
		}
	}
}

uint32_t BocRxCore::getEmu()
{
	return m_emuMask;
}
