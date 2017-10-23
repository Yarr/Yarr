#include "Rd53aEmu.h"

Rd53aEmu::Rd53aEmu(RingBuffer * rx, RingBuffer * tx) {

    m_feCfg = std::make_shared<Rd53aCfg>();
    m_txRingBuffer = tx;
    m_rxRingBuffer = rx;

    run = true;
}

Rd53aEmu::~Rd53aEmu() {
}

void Rd53aEmu::executeLoop() {
    std::cout << "Starting emulator loop" << std::endl;

    // this is silly - change to a cool, fast array soon
    std::map<int,int> eightToFive;
    eightToFive[0x6A] = 0;
    eightToFive[0x6C] = 1;
    eightToFive[0x71] = 2;
    eightToFive[0x72] = 3;
    eightToFive[0x74] = 4;
    eightToFive[0x8B] = 5;
    eightToFive[0x8D] = 6;
    eightToFive[0x8E] = 7;
    eightToFive[0x93] = 8;
    eightToFive[0x95] = 9;
    eightToFive[0x96] = 10;
    eightToFive[0x99] = 11;
    eightToFive[0x9A] = 12;
    eightToFive[0x9C] = 13;
    eightToFive[0xA3] = 14;
    eightToFive[0xA5] = 15;
    eightToFive[0xA6] = 16;
    eightToFive[0xA9] = 17;
    eightToFive[0xAA] = 18;
    eightToFive[0xAC] = 19;
    eightToFive[0xB1] = 20;
    eightToFive[0xB2] = 21;
    eightToFive[0xB4] = 22;
    eightToFive[0xC3] = 23;
    eightToFive[0xC5] = 24;
    eightToFive[0xC6] = 25;
    eightToFive[0xC9] = 26;
    eightToFive[0xCA] = 27;
    eightToFive[0xCC] = 28;
    eightToFive[0xD1] = 29;
    eightToFive[0xD2] = 30;
    eightToFive[0xD4] = 31;

    while (run)
    {
        if (!m_txRingBuffer->isEmpty()) {
            m_header = m_txRingBuffer->read32();
            printf("Rd53aEmu got the header word: 0x%x\n", m_header);

            if (m_header == 0)
            {
            }
            else if (m_header == 0x6666) // wrRegister
            {
              m_id_address_some_data = m_txRingBuffer->read32();
//              printf("Rd53aEmu got the id_address_some_data word: 0x%x\n", m_id_address_some_data);

              uint8_t byte1 = (m_id_address_some_data >> 16) >> 8;
              uint8_t byte2 = (m_id_address_some_data >> 16) & 0x00FF;
              uint8_t byte3 = (m_id_address_some_data & 0x0000FFFF) >> 8;
              uint8_t byte4 = (m_id_address_some_data & 0x0000FFFF) & 0x00FF;
/*
              printf("0x%x decoded into: 0x%x\n", byte1, eightToFive[byte1]);
              printf("0x%x decoded into: 0x%x\n", byte2, eightToFive[byte2]);
              printf("0x%x decoded into: 0x%x\n", byte3, eightToFive[byte3]);
              printf("0x%x decoded into: 0x%x\n", byte4, eightToFive[byte4]);
*/
              byte1 = eightToFive[byte1];
              byte2 = eightToFive[byte2];
              byte3 = eightToFive[byte3];
              byte4 = eightToFive[byte4];

              uint32_t address = (byte2 << 4) + (byte3 >> 1);
              printf("address: 0x%x, %d\n", address, address);

              if (byte1 & 0x01) // check the bit which determines whether big data or small data should be read
              {
                printf("big data expected\n");
                // 50(?) more bits of data
              }
              else
              {
                printf("small data expected\n");
                // 10(?) more bits of data
                m_small_data = m_txRingBuffer->read32();
//                printf("Rd53aEmu got the small_data word 0x%x\n", m_small_data);

                uint8_t data_byte1 = (m_small_data >> 16) >> 8;
                uint8_t data_byte2 = (m_small_data >> 16) & 0x00FF;
                uint8_t data_byte3 = (m_small_data & 0x0000FFFF) >> 8;
                uint8_t data_byte4 = (m_small_data & 0x0000FFFF) & 0x00FF;
/*
                printf("0x%x decoded into: 0x%x\n", data_byte1, eightToFive[data_byte1]);
                printf("0x%x decoded into: 0x%x\n", data_byte2, eightToFive[data_byte2]);
                printf("0x%x decoded into: 0x%x\n", data_byte3, eightToFive[data_byte3]);
                printf("0x%x decoded into: 0x%x\n", data_byte4, eightToFive[data_byte4]);
*/
                data_byte1 = eightToFive[data_byte1];
                data_byte2 = eightToFive[data_byte2];
                data_byte3 = eightToFive[data_byte3];
                data_byte4 = eightToFive[data_byte4];

                uint32_t data = data_byte2 + (data_byte1 << 5) + (byte4 << 10) + ((byte3 & 0x1) << 15);
                printf("data: 0x%x, %d\n", data, data);

                if (address == 0) // configure pixels based on what's in the GR
                {
                  printf("being asked to configure pixels\n");
                  if (m_feCfg->PixMode.read() == 0x2) // auto col = 0, auto row = 1, broadcast = 0
                  {
                    // configure all pixels in row m_feCfg->RegionRow.read() with value sent
                    for (unsigned dc = 0; dc < 200; dc++)
                    {
                      m_pixelRegisters[dc][m_feCfg->RegionRow.read()] = (uint8_t) (data & 0x00FF);
                      m_pixelRegisters[dc * 2 + 1][m_feCfg->RegionRow.read()] = (uint8_t) (data >> 8);
                    }
                    // increment m_feCfg->RegionRow
                    if (m_feCfg->RegionRow.read() + 1 < 400)
                    {
                      m_feCfg->RegionRow.write(m_feCfg->RegionRow.read() + 1);
                    }
                    else
                    {
                      m_feCfg->RegionRow.write(0);
                    }
                  }
                }
                else // configure the global reguster
                {
                  m_feCfg->m_cfg[address] = data; // this is basically where we actually write to the global register
                }
              }
            }
            else
            {
              printf("unrecognized header, skipping for now (will eventually elegantly crash)\n");
            }
        }
    }
}

void Rd53aEmu::pushOutput(uint32_t value) {
    if (m_rxRingBuffer) {
        m_rxRingBuffer->write32(value);
    }
}
