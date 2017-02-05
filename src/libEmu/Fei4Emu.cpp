/*
 * Author: K. Potamianos <karolos.potamianos@cern.ch>
 * Date: 2016-VI-25
 * Description: this is a port of the FE-I4 emulator for IBLROD (2014-VI-12)
 * Updates: addition of command decoder by N. Whallon <alokin@uw.edu>
 */

#include "Fei4Emu.h"

// should use a better function than this (box muller method - stolen from the internet)
double rand_normal(double mean, double sigma, bool can_be_negative) {
    static double n2 = 0.0;
    static int n2_cached = 0;
    if (!n2_cached) {
        double x, y, r;
        do {
            x = 2.0 * rand() / RAND_MAX - 1;
            y = 2.0 * rand() / RAND_MAX - 1;

            r = x * x + y * y;
        } while (r == 0.0 || r > 1.0);
        double d = sqrt(-2.0 * log(r) / r);
        double n1 = x * d;
        n2 = y * d;
        double result = n1 * sigma + mean;
        n2_cached = 1;
        if (!can_be_negative && result < 0) {
            rand_normal(mean, sigma, can_be_negative);
        }
        return result;
    } else {
        n2_cached = 0;
        if (!can_be_negative && n2 * sigma + mean < 0) {
            rand_normal(mean, sigma, can_be_negative);
        }
        return n2 * sigma + mean;
    }
}

Fei4Emu::Fei4Emu() {
    srand(time(NULL));

    m_feId = 0x00;
    m_l1IdCnt = 0x00;
    m_bcIdCnt = 0x00;

    m_feGeo = { 336, 80 };

    m_feCfg = std::make_shared<Fei4Cfg>();
    m_txShm = std::make_shared<EmuShm>(1337, 256, 0);
    m_rxShm = std::make_shared<EmuShm>(1338, 256, 0);

    this->initializePixelModels();
}

Fei4Emu::Fei4Emu(std::string output_model_cfg): Fei4Emu() {
    m_output_model_cfg = output_model_cfg;
}

Fei4Emu::Fei4Emu(std::string output_model_cfg, std::string input_model_cfg): Fei4Emu() {
    this->initializePixelModelsFromFile(input_model_cfg);
    m_output_model_cfg = output_model_cfg;
}

Fei4Emu::~Fei4Emu() {
}

void Fei4Emu::initializePixelModels() {
    // initialize the pixel models
    for (unsigned col = 1; col <= m_feCfg->n_Col; col++) {
        for (unsigned row = 1; row <= m_feCfg->n_Row; row++) {
            m_pixelModels[col - 1][row - 1].Vthin_mean = 22;
            m_pixelModels[col - 1][row - 1].Vthin_sigma = 1;
            m_pixelModels[col - 1][row - 1].Vthin_gauss = rand_normal(m_pixelModels[col - 1][row - 1].Vthin_mean, m_pixelModels[col - 1][row - 1].Vthin_sigma, 0);

            m_pixelModels[col - 1][row - 1].TDACVbp_mean = 1;
            m_pixelModels[col - 1][row - 1].TDACVbp_sigma = 0;
            m_pixelModels[col - 1][row - 1].TDACVbp_gauss = rand_normal(m_pixelModels[col - 1][row - 1].TDACVbp_mean, m_pixelModels[col - 1][row - 1].TDACVbp_sigma, 0);

            m_pixelModels[col - 1][row - 1].noise_sigma_mean = 150;
            m_pixelModels[col - 1][row - 1].noise_sigma_sigma = 15;
            m_pixelModels[col - 1][row - 1].noise_sigma_gauss = rand_normal(m_pixelModels[col - 1][row - 1].noise_sigma_mean, m_pixelModels[col - 1][row - 1].noise_sigma_sigma, 0);
        }
    }
}

void Fei4Emu::initializePixelModelsFromFile(std::string json_file_path) {
    std::ifstream file(json_file_path);
    nlohmann::json j;
    j << file;

    for (unsigned col = 1; col <= m_feCfg->n_Col; col++) {
        for (unsigned row = 1; row <= m_feCfg->n_Row; row++) {
            m_pixelModels[col - 1][row - 1].Vthin_mean = j["Vthin_mean_vector"][(col - 1) * m_feCfg->n_Row + (row - 1)];
            m_pixelModels[col - 1][row - 1].Vthin_sigma = j["Vthin_sigma_vector"][(col - 1) * m_feCfg->n_Row + (row - 1)];
            m_pixelModels[col - 1][row - 1].Vthin_gauss = j["Vthin_gauss_vector"][(col - 1) * m_feCfg->n_Row + (row - 1)];

            m_pixelModels[col - 1][row - 1].TDACVbp_mean = j["TDACVbp_mean_vector"][(col - 1) * m_feCfg->n_Row + (row - 1)];
            m_pixelModels[col - 1][row - 1].TDACVbp_sigma = j["TDACVbp_sigma_vector"][(col - 1) * m_feCfg->n_Row + (row - 1)];
            m_pixelModels[col - 1][row - 1].TDACVbp_gauss = j["TDACVbp_gauss_vector"][(col - 1) * m_feCfg->n_Row + (row - 1)];

            m_pixelModels[col - 1][row - 1].noise_sigma_mean = j["noise_sigma_mean_vector"][(col - 1) * m_feCfg->n_Row + (row - 1)];
            m_pixelModels[col - 1][row - 1].noise_sigma_sigma = j["noise_sigma_sigma_vector"][(col - 1) * m_feCfg->n_Row + (row - 1)];
            m_pixelModels[col - 1][row - 1].noise_sigma_gauss = j["noise_sigma_gauss_vector"][(col - 1) * m_feCfg->n_Row + (row - 1)];
        }
    }

    file.close();
}

void Fei4Emu::writePixelModelsToFile() {
    if (m_output_model_cfg.size() == 0)
    {
        fprintf(stderr, "ERROR - m_output_model_cfg was null, but the user requested to write out the pixel model configuration - this should not happen!\n");
        return;
    }

        std::ofstream file(m_output_model_cfg);
    nlohmann::json j;

    std::vector<float> Vthin_mean_vector; Vthin_mean_vector.reserve(m_feCfg->n_Col * m_feCfg->n_Row);
    std::vector<float> Vthin_sigma_vector; Vthin_sigma_vector.reserve(m_feCfg->n_Col * m_feCfg->n_Row);
    std::vector<float> Vthin_gauss_vector; Vthin_gauss_vector.reserve(m_feCfg->n_Col * m_feCfg->n_Row);

    std::vector<float> TDACVbp_mean_vector; TDACVbp_mean_vector.reserve(m_feCfg->n_Col * m_feCfg->n_Row);
    std::vector<float> TDACVbp_sigma_vector; TDACVbp_sigma_vector.reserve(m_feCfg->n_Col * m_feCfg->n_Row);
    std::vector<float> TDACVbp_gauss_vector; TDACVbp_gauss_vector.reserve(m_feCfg->n_Col * m_feCfg->n_Row);

    std::vector<float> noise_sigma_mean_vector; noise_sigma_mean_vector.reserve(m_feCfg->n_Col * m_feCfg->n_Row);
    std::vector<float> noise_sigma_sigma_vector; noise_sigma_sigma_vector.reserve(m_feCfg->n_Col * m_feCfg->n_Row);
    std::vector<float> noise_sigma_gauss_vector; noise_sigma_gauss_vector.reserve(m_feCfg->n_Col * m_feCfg->n_Row);

    for (unsigned col = 1; col <= m_feCfg->n_Col; col++) {
        for (unsigned row = 1; row <= m_feCfg->n_Row; row++) {
            Vthin_mean_vector.push_back(m_pixelModels[col - 1][row - 1].Vthin_mean);
            Vthin_sigma_vector.push_back(m_pixelModels[col - 1][row - 1].Vthin_sigma);
            Vthin_gauss_vector.push_back(m_pixelModels[col - 1][row - 1].Vthin_gauss);

            TDACVbp_mean_vector.push_back(m_pixelModels[col - 1][row - 1].TDACVbp_mean);
            TDACVbp_sigma_vector.push_back(m_pixelModels[col - 1][row - 1].TDACVbp_sigma);
            TDACVbp_gauss_vector.push_back(m_pixelModels[col - 1][row - 1].TDACVbp_gauss);

            noise_sigma_mean_vector.push_back(m_pixelModels[col - 1][row - 1].noise_sigma_mean);
            noise_sigma_sigma_vector.push_back(m_pixelModels[col - 1][row - 1].noise_sigma_sigma);
            noise_sigma_gauss_vector.push_back(m_pixelModels[col - 1][row - 1].noise_sigma_gauss);
        }
    }

    j["Vthin_mean_vector"] = Vthin_mean_vector;
    j["Vthin_sigma_vector"] = Vthin_sigma_vector;
    j["Vthin_gauss_vector"] = Vthin_gauss_vector;

    j["TDACVbp_mean_vector"] = TDACVbp_mean_vector;
    j["TDACVbp_sigma_vector"] = TDACVbp_sigma_vector;
    j["TDACVbp_gauss_vector"] = TDACVbp_gauss_vector;

    j["noise_sigma_mean_vector"] = noise_sigma_mean_vector;
    j["noise_sigma_sigma_vector"] = noise_sigma_sigma_vector;
    j["noise_sigma_gauss_vector"] = noise_sigma_gauss_vector;

    file << j;

    file.close();
}

void Fei4Emu::executeLoop() {
    while (1)
    {
            uint32_t command;
        uint32_t type;
        uint32_t name;
        uint32_t chipid;
        uint32_t address;

        uint32_t value;
        uint32_t bitstream[21];

        command = m_txShm->read32();
        type = command >> 14;

        switch (type)
        {
            case 0x7400:
                handleTrigger();
                break;
            case 0x0168:
                name = command >> 10 & 0xF;
                chipid = command >> 6 & 0xF;
                address = command & 0x3F;

                switch (name)
                {
                    case 1:
//                        printf("recieved a RdRegister command\n");
                        break;
                    case 2:
//                        printf("recieved a WrRegister command\n");
                        value = m_txShm->read32();
                        value >>= 16;
                        handleWrRegister(chipid, address, value);
                        break;
                    case 4:
//                        printf("recieved a WrFrontEnd command\n");
                        for (int i = 0; i < 21; i++) {
                            bitstream[i] = m_txShm->read32();
                        }
                        handleWrFrontEnd(chipid, bitstream);
                        break;
                    case 8:
//                        printf("recieved a GlobalReset command\n");
                        break;
                    case 9:
//                        printf("recieved a GlobalPulse command\n");
                        handleGlobalPulse(chipid);
                        break;
                    case 10:
//                        printf("recieved a RunMode command\n");
                        handleRunMode(chipid, command);
                        break;
                }

                break;
            case 0:
                break;
            default:
                fprintf(stderr, "ERROR - unknown type recieved, %x\n", type);
                break;
        }

    }
}

// functions for handling the recieved commands
void Fei4Emu::handleGlobalPulse(uint32_t chipid) {
    // ignore if we get a ReadErrorReq
    if (m_feCfg->getValue(&Fei4Cfg::ReadErrorReq) == 1) {
    }

    // eventually, I should change the FE that I use based on the chipid

    // check if I need to shift the Shift Register by one
    if (m_feCfg->getValue(&Fei4Cfg::S0) == 0 && m_feCfg->getValue(&Fei4Cfg::S1) == 0 && m_feCfg->getValue(&Fei4Cfg::HitLD) == 0 && m_feCfg->getValue(&Fei4Cfg::SR_Clock) == 1) {
        // use Fei4Cfg::Colpr_Mode to determine which dc to loop over
        unsigned dc_step = 40;
        switch (m_feCfg->getValue(&Fei4Cfg::Colpr_Mode)) {
            case 0:
                dc_step = 40;
                break;
            case 1:
                dc_step = 4;
                break;
            case 2:
                dc_step = 8;
                break;
            case 3:
                dc_step = 1;
                break;
        }

        // loop through the 40 double columns
        for (unsigned i = 0; i < 40 / dc_step; i++) {
            unsigned dc = m_feCfg->getValue(&Fei4Cfg::Colpr_Addr) + dc_step * i % 40;

            // use these to deal with overflow bits
            uint32_t current_last_bit = 0;
            uint32_t previous_last_bit = 0;

            // shift all bits left by 1, keeping track of the overflow bits
            for (int j = 0; j < 21; j++) {
                current_last_bit = m_shiftRegisterBuffer[dc][j] & 0x80000000;
                if (current_last_bit) {
                    current_last_bit = 0x00000001;
                }
                m_shiftRegisterBuffer[dc][j] <<= 1;
                m_shiftRegisterBuffer[dc][j] += previous_last_bit;
                previous_last_bit = current_last_bit;
            }
        }
    }

    // check if we should write to the shift registers from the pixel registers
    if (m_feCfg->getValue(&Fei4Cfg::S0) == 1 && m_feCfg->getValue(&Fei4Cfg::S1) == 1 && m_feCfg->getValue(&Fei4Cfg::HitLD) == 0 && m_feCfg->getValue(&Fei4Cfg::SR_Clock) == 1) {
        // use Fei4Cfg::Colpr_Mode to determine which dc to loop over
        unsigned dc_step = 40;
        switch (m_feCfg->getValue(&Fei4Cfg::Colpr_Mode)) {
            case 0:
                dc_step = 40;
                break;
            case 1:
                dc_step = 4;
                break;
            case 2:
                dc_step = 8;
                break;
            case 3:
                dc_step = 1;
                break;
        }

        // loop through the 40 double columns
        for (unsigned i = 0; i < 40 / dc_step; i++) {
            unsigned dc = m_feCfg->getValue(&Fei4Cfg::Colpr_Addr) + dc_step * i % 40;

            DoubleColumnBitOps* bitReg[] = { &m_feCfg->En(dc), &m_feCfg->TDAC(dc)[0], &m_feCfg->TDAC(dc)[1], &m_feCfg->TDAC(dc)[2], &m_feCfg->TDAC(dc)[3], &m_feCfg->TDAC(dc)[4], &m_feCfg->LCap(dc), &m_feCfg->SCap(dc), &m_feCfg->Hitbus(dc), &m_feCfg->FDAC(dc)[0], &m_feCfg->FDAC(dc)[1], &m_feCfg->FDAC(dc)[2], &m_feCfg->FDAC(dc)[3] };

            // loop through the 13 double column bits
            for (int j = 0; j < 13; j++) {
                // if a double column bit is 1, write the contents of the corresponding pixel register to the Shift Register
                if (m_feCfg->getValue(&Fei4Cfg::Pixel_latch_strobe) & (unsigned) pow(2, j)) {
                    memcpy(&m_shiftRegisterBuffer[dc][0], bitReg[j]->getStream(), 84);
                }
            }
        }
    }

    // check if we should write to the pixel registers from the shift registers
    if (m_feCfg->getValue(&Fei4Cfg::S0) == 0 && m_feCfg->getValue(&Fei4Cfg::S1) == 0 && m_feCfg->getValue(&Fei4Cfg::HitLD) == 0 && m_feCfg->getValue(&Fei4Cfg::Latch_Enable) == 1) {
        // use Fei4Cfg::Colpr_Mode to determine which dc to loop over
        unsigned dc_step = 40;
        switch (m_feCfg->getValue(&Fei4Cfg::Colpr_Mode)) {
            case 0:
                dc_step = 40;
                break;
            case 1:
                dc_step = 4;
                break;
            case 2:
                dc_step = 8;
                break;
            case 3:
                dc_step = 1;
                break;
        }

        // loop through the 40 double columns
        for (unsigned i = 0; i < 40 / dc_step; i++) {
            unsigned dc = m_feCfg->getValue(&Fei4Cfg::Colpr_Addr) + dc_step * i % 40;

            DoubleColumnBitOps* bitReg[] = { &m_feCfg->En(dc), &m_feCfg->TDAC(dc)[0], &m_feCfg->TDAC(dc)[1], &m_feCfg->TDAC(dc)[2], &m_feCfg->TDAC(dc)[3], &m_feCfg->TDAC(dc)[4], &m_feCfg->LCap(dc), &m_feCfg->SCap(dc), &m_feCfg->Hitbus(dc), &m_feCfg->FDAC(dc)[0], &m_feCfg->FDAC(dc)[1], &m_feCfg->FDAC(dc)[2], &m_feCfg->FDAC(dc)[3] };

            // loop through the 13 double column bits
            for (int j = 0; j < 13; j++) {
                // if a double column bit is 1, write the contents of the Shift Register to the corresponding pixel register
                if (m_feCfg->getValue(&Fei4Cfg::Pixel_latch_strobe) & (unsigned) pow(2, j)) {
                    bitReg[j]->set(&m_shiftRegisterBuffer[dc][0]);
                }
            }
        }
    }
}

void Fei4Emu::handleRunMode(uint32_t chipid, int command) {
    // eventually, I should change the FE that I use based on the chipId

    // decode the modeBits from the command word
    m_modeBits = command & 0x3F;
}

void Fei4Emu::handleWrRegister(uint32_t chipid, uint32_t address, uint32_t value) {
    // write value to address in the Global Register (of FE chipid - ignoring this part for now)
    m_feCfg->cfg[address] = value;
}

void Fei4Emu::handleWrFrontEnd(uint32_t chipid, uint32_t bitstream[21]) {
    // write the bitstream to our Shift Register buffer (eventually should take into consideration chipid)
    // use Fei4Cfg::Colpr_Mode to determine which dc to loop over
    unsigned dc_step = 40;
    switch (m_feCfg->getValue(&Fei4Cfg::Colpr_Mode)) {
        case 0:
            dc_step = 40;
            break;
        case 1:
            dc_step = 4;
            break;
        case 2:
            dc_step = 8;
            break;
        case 3:
            dc_step = 1;
            break;
    }

    // loop through the 40 double columns
    for (unsigned i = 0; i < 40 / dc_step; i++) {
        unsigned dc = m_feCfg->getValue(&Fei4Cfg::Colpr_Addr) + dc_step * i % 40;

        // must copy the bitstream to the shift register in this inverted way because of the order in which it is read in
        for (int j = 0; j < 21; j++) {
            m_shiftRegisterBuffer[dc][j] = bitstream[20 - j];
        }
    }
}

void Fei4Emu::handleTrigger() {
    this->addDataHeader(false);    // no error flags

    // use Fei4Cfg::Colpr_Mode to determine which dc to loop over
    unsigned dc_step = 40;
    switch (m_feCfg->getValue(&Fei4Cfg::Colpr_Mode)) {
        case 0:
            dc_step = 40;
            break;
        case 1:
            dc_step = 4;
            break;
        case 2:
            dc_step = 8;
            break;
        case 3:
            dc_step = 1;
            break;
    }

    // loop through the 40 double columns
    for (unsigned i = 0; i < 40 / dc_step; i++) {
        unsigned dc = m_feCfg->getValue(&Fei4Cfg::Colpr_Addr) + dc_step * i % 40;

        for (unsigned row = 1; row <= m_feCfg->n_Row; row++) {
            for (int c = 0; c <= 1; c++) {
                if (m_feCfg->getEn(dc * 2 + 1 + c, row)) {
                    float injection_charge = m_feCfg->toCharge(m_feCfg->getValue(&Fei4Cfg::PlsrDAC), m_feCfg->getSCap(dc * 2 + 1 + c, row), m_feCfg->getLCap(dc * 2 + 1 + c, row));    // the injection charge is well defined
                    float threshold_charge = calculateThreshold(dc * 2 + 1 + c, row);                                            // the threshold charge requires quite some modeling
                    float noise_charge = rand_normal(0, m_pixelModels[dc * 2 + 1 + c - 1][row - 1].noise_sigma_gauss, 1);                            // the noise charge requires simple modeling

                    uint32_t digital_tot = 0;
                    uint32_t analog_tot = 0;

                    if (m_feCfg->getValue(&Fei4Cfg::DigHitIn_Sel)) {               // check if we are doing a digital hit 
                        digital_tot = 10;
                    }
                    else if (injection_charge + noise_charge > threshold_charge) {       // check if we are doing an analog hit 
                        analog_tot = this->calculateToT(injection_charge + noise_charge - threshold_charge);
                    }

                    if (digital_tot && analog_tot) {
                        fprintf(stderr, "ERROR - doing both a digital and an analog hit, this should not happen!\n");
                    }

                    if (digital_tot >= analog_tot) {
                        this->addHit(dc * 2 + 1 + c, row, digital_tot, 0);
                    }
                    if (analog_tot > digital_tot) {
                        this->addHit(dc * 2 + 1 + c, row, analog_tot, 0);
                    }
                }
            }
        }
    }
}

void Fei4Emu::pushOutput(uint32_t value) {
    if (m_rxShm) {
        m_rxShm->write32(value);
    }
}

// functions for modeling pixel responses
float Fei4Emu::calculateThreshold(int col, int row) {
    float modelVthin = m_pixelModels[col - 1][row - 1].Vthin_gauss * m_feCfg->getValue(&Fei4Cfg::Vthin_Fine) + m_pixelModels[col][row - 1].Vthin_gauss * m_feCfg->getValue(&Fei4Cfg::Vthin_Coarse) * 128;
    float modelTDAC = 30.0 * m_feCfg->getTDAC(col, row);
    float threshold = modelVthin - modelTDAC;

    if (threshold < 0) {
        threshold = 0;
    }

    return threshold;
}

uint32_t Fei4Emu::calculateToT(float charge) {
    return (uint32_t) (charge * 9.0 / 16000.0) + 1;
}

// Assuming that first command byte is 0x01 (e.g. triger command is 0x1d0)
// Assuming that function isn't called until data is available to be read
// todo: use some buffering with size information (and command availability)

void Fei4Emu::decodeCommand(uint8_t *cmdStream, std::size_t size) {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

  uint8_t *cmdPtr = cmdStream;
  while(cmdPtr < cmdStream+size) {
    while (cmdPtr[0] != 0x1) cmdPtr++; // Look for 0x1
    switch(cmdPtr[1] & 0xF0) {
      case 0xd0: processL1A(); break;
      case 0x60: {
        switch(cmdPtr[1] & 0x0F) {
          case 0x1: processBCR(); break;
          case 0x2: processECR(); break;
          case 0x4: processCAL(); break;
          case 0x8: processSLOW(cmdPtr); break;
        }
        }; break;
      default: {
        std::cerr << "Fei4Emu: cannot decode command" << std::endl;
        std::for_each(cmdPtr, cmdPtr+4, [](uint8_t cmdByte) { std::cerr << std::hex << static_cast<uint32_t>(cmdByte) << std::dec << " "; });
        }
    }
    cmdPtr += 2;
  }

}

///
/// Randomly adding hits
/// Todo: handle only enabled pixels
///

void Fei4Emu::addRandomHits(uint32_t nHits) {
  startFrame();
  addDataHeader(false);
  addServiceRecord(true); // True because we can insert SRs [14-16] here
  while(nHits-- > 0) {
    addDataRecord( rand() % m_feGeo.nCol, rand() % m_feGeo.nRow, rand() % 0xF, rand() % 0xF);
  }
  addServiceRecord(false);
  endFrame();
}

void Fei4Emu::addPhysicsHits() {
}

void Fei4Emu::addHit(uint16_t col, uint16_t row, uint8_t tot1, uint8_t tot2) {
  addDataRecord(col, row, getToTCode(tot1), getToTCode(tot2));
}

uint8_t Fei4Emu::getToTCode(uint8_t dec_tot) {
  const uint8_t totCodes[3][17] = { 
    { 0xF, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xD, 0xD },
    { 0xF, 0xE, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xD },
    { 0xF, 0xE, 0xE, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD }};

  uint8_t hitDiscCfg = m_feCfg->getValue(&Fei4Cfg::HitDiscCnfg);
  if(dec_tot >= 16) dec_tot = 16;
  return totCodes[hitDiscCfg][dec_tot];
}

void Fei4Emu::addServiceRecord(bool isInfoSR) {

}

void Fei4Emu::startFrame() {
  pushOutput( (m_feId << 24) | 0xfc );
}

void Fei4Emu::endFrame() {
  pushOutput( (m_feId << 24) | 0xbc );
}

void Fei4Emu::addDataHeader(bool hasErrorFlags) {
  pushOutput( (m_feId << 24) | (0xe9 << 16) | (((uint32_t)hasErrorFlags) << 15) | ((m_l1IdCnt&0x1F) << 10) | (m_bcIdCnt&0x3FF) );
}

void Fei4Emu::addDataRecord(uint16_t col, uint16_t row, uint8_t tot1, uint8_t tot2) {
  pushOutput( (m_feId << 24) | ((col&0x7F) << 17) | ((row&0x1FF) << 8) | ((tot1&0xF) << 4) | (tot2&0xF) );
}

void Fei4Emu::addAddressRecord(uint16_t address, bool isGR) {
  pushOutput( (0xea << 16) | (((uint32_t)isGR) << 15) | (address&0x7FFFF) );
}

void Fei4Emu::addValueRecord(uint16_t value) {
  pushOutput( (0xec << 16) | value );
}

void Fei4Emu::processL1A() {
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  addRandomHits(10);
}

void Fei4Emu::processBCR() {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

}

void Fei4Emu::processECR() {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

}

void Fei4Emu::processCAL() {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

}

void Fei4Emu::processSLOW(uint8_t *cmdPtr) {
  std::cout << __PRETTY_FUNCTION__ << std::endl;

}
