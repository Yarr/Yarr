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
            m_command = m_txRingBuffer->read32();
            printf("Rd53aEmu got the word: %x\n", m_command);

            if (m_command == 0x6666)
            {
              m_command = m_txRingBuffer->read32();
              printf("Rd53aEmu got the word: %x\n", m_command);
              uint16_t m_command_l = m_command >> 16;
              uint16_t m_command_r = m_command & 0x0000FFFF;

              uint16_t m_command_ll = m_command_l >> 8;
              uint16_t m_command_lr = m_command_l & 0x00FF;

              uint16_t m_command_rl = m_command_r >> 8;
              uint16_t m_command_rr = m_command_r & 0x00FF;
              printf("%x decoded into: %x\n", m_command_ll, eightToFive[m_command_ll]);
              printf("%x decoded into: %x\n", m_command_lr, eightToFive[m_command_lr]);
              printf("%x decoded into: %x\n", m_command_rl, eightToFive[m_command_rl]);
              printf("%x decoded into: %x\n", m_command_rr, eightToFive[m_command_rr]);

            m_command = m_txRingBuffer->read32();
            printf("Rd53aEmu got the word: %x\n", m_command);
            m_command = m_txRingBuffer->read32();
            printf("Rd53aEmu got the word: %x\n", m_command);

            }
        }
    }
}

void Rd53aEmu::pushOutput(uint32_t value) {
    if (m_rxRingBuffer) {
        m_rxRingBuffer->write32(value);
    }
}
