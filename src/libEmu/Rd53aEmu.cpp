#include "Rd53aEmu.h"

#include "Gauss.h"

Rd53aEmu::Rd53aEmu(RingBuffer * rx, RingBuffer * tx) {
    srand(time(NULL));

    m_feCfg = std::make_shared<Rd53aCfg>();
    m_txRingBuffer = tx;
    m_rxRingBuffer = rx;

    run = true;

    for (int col = 0; col < 136; col++) {
        for (int row = 0; row < 192; row++) {
            std::string linName = "linScurveCol" + std::to_string(col) + "Row" + std::to_string(row);
            linScurve[col][row] = new Histo1d(linName.c_str(), 256, -0.5, 4095.5, typeid(void));
            m_rd53aLinPixelModelObjects[col][row] = new Rd53aLinPixelModel(10, 2, 400, 100);

            std::string diffName = "diffScurveCol" + std::to_string(col) + "Row" + std::to_string(row);
            diffScurve[col][row] = new Histo1d(diffName.c_str(), 256, -0.5, 4095.5, typeid(void));
            m_rd53aDiffPixelModelObjects[col][row] = new Rd53aDiffPixelModel(10, 0, 10, 10);
        }
    }
    linThreshold = new Histo1d("linThreshold", 256, -0.5, 4095.5, typeid(void));
    diffThreshold = new Histo1d("diffThreshold", 256, -0.5, 4095.5, typeid(void));

    analogHits = new Histo2d("analogHits", 400, -0.5, 399.5, 192, -0.5, 191.5, typeid(void));
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

    while (run) {
        if (!m_txRingBuffer->isEmpty()) {

            // read the command header
            m_header = m_txRingBuffer->read32();
//          printf("Rd53aEmu got the header word: 0x%x\n", m_header);

            if (m_header == 0) { // ignore
            }
            else if (m_header == 0xFFFF) { // temporary command which says we must plot
                for (int col = 0; col < 136; col++) {
                    for (int row = 0; row < 192; row++) {
                        if (diffScurve[col][row]->getEntries() != 0 ) {
                            diffScurve[col][row]->scale(1.0/100.0); // hardcoded for now - the number of times we scan the same pixel
                            for (int bin = 0; bin < 256; bin++) {

                                if (diffScurve[col][row]->getBin(bin) > 0.5) {
                                    diffThreshold->fill(bin * 16);
                                    break;
                                }
                            }
                              if (col == 0 && row == 0) diffScurve[col][row]->plot("scurve", "");
//                            if (col == 0 && row == 0) diffScurve[col][row]->toFile("scurve", "", 0);
                        }

                        if (linScurve[col][row]->getEntries() != 0 ) {
                            linScurve[col][row]->scale(1.0/100.0); // hardcoded for now - the number of times we scan the same pixel
                            for (int bin = 0; bin < 256; bin++) {

                                if (linScurve[col][row]->getBin(bin) > 0.5) {
                                    linThreshold->fill(bin * 16);
                                    break;
                                }
                            }
                              if (col == 0 && row == 0) linScurve[col][row]->plot("scurve", "");
//                            if (col == 0 && row == 0) linScurve[col][row]->toFile("scurve", "", 0);
                        }
                    }
                }

                diffThreshold->plot("threshold", "");
                linThreshold->plot("threshold", "");
                analogHits->plot("analogHits", "");
                m_rxRingBuffer->write32(0xA); // test writing back
            }
            else if (m_header == 0x2B) { // Trigger_01
//              printf("got Trigger_01 command\n");

                // temporary counters to count hits
                int totalDigitalHits = 0;
                int diffAnalogHits = 0;
                int linAnalogHits = 0;
//              int syncAnalogHits = 0;
                for (unsigned dc = 0; dc < 200; dc++) {
                    // put these checks into a function maybe
                    // check pixels to see if the digital enable is set for "octo-columns" (columns of cores)
                    if (0   <= dc && dc < 64  && !((m_feCfg->EnCoreColSync.read()  >> ((dc - 0)   / 4)) & 0x1)) continue;
                    if (64  <= dc && dc < 128 && !((m_feCfg->EnCoreColLin1.read()  >> ((dc - 64)  / 4)) & 0x1)) continue;
                    if (128 <= dc && dc < 132 && !((m_feCfg->EnCoreColLin2.read()  >> ((dc - 128) / 4)) & 0x1)) continue;
                    if (132 <= dc && dc < 196 && !((m_feCfg->EnCoreColDiff1.read() >> ((dc - 132) / 4)) & 0x1)) continue;
                    if (196 <= dc && dc < 200 && !((m_feCfg->EnCoreColDiff2.read() >> ((dc - 196) / 4)) & 0x1)) continue;
                    // check pixels to see if double columns are enabled for injections
                    if (0   <= dc && dc < 16  && !((m_feCfg->CalColprSync1.read() >> (dc - 0)   & 0x1))) continue;
                    if (16  <= dc && dc < 32  && !((m_feCfg->CalColprSync2.read() >> (dc - 16)  & 0x1))) continue;
                    if (32  <= dc && dc < 48  && !((m_feCfg->CalColprSync3.read() >> (dc - 32)  & 0x1))) continue;
                    if (48  <= dc && dc < 64  && !((m_feCfg->CalColprSync4.read() >> (dc - 48)  & 0x1))) continue;
                    if (64  <= dc && dc < 80  && !((m_feCfg->CalColprLin1.read()  >> (dc - 64)  & 0x1))) continue;
                    if (80  <= dc && dc < 96  && !((m_feCfg->CalColprLin2.read()  >> (dc - 80)  & 0x1))) continue;
                    if (96  <= dc && dc < 112 && !((m_feCfg->CalColprLin3.read()  >> (dc - 96)  & 0x1))) continue;
                    if (112 <= dc && dc < 128 && !((m_feCfg->CalColprLin4.read()  >> (dc - 112) & 0x1))) continue;
                    if (128 <= dc && dc < 132 && !((m_feCfg->CalColprLin5.read()  >> (dc - 128) & 0x1))) continue;
                    if (132 <= dc && dc < 148 && !((m_feCfg->CalColprDiff1.read() >> (dc - 132) & 0x1))) continue;
                    if (148 <= dc && dc < 164 && !((m_feCfg->CalColprDiff2.read() >> (dc - 148) & 0x1))) continue;
                    if (164 <= dc && dc < 180 && !((m_feCfg->CalColprDiff3.read() >> (dc - 164) & 0x1))) continue;
                    if (180 <= dc && dc < 196 && !((m_feCfg->CalColprDiff4.read() >> (dc - 180) & 0x1))) continue;
                    if (196 <= dc && dc < 200 && !((m_feCfg->CalColprDiff5.read() >> (dc - 196) & 0x1))) continue;

                    for (unsigned row = 0; row < 192; row++) {
                        float capacitance_times_coulomb = 8000; // change this to the correct value later

                        float maximum_injection_voltage = 1.2;
//                      printf("m_feCfg->VcalHigh.read() = %d\n", m_feCfg->VcalHigh.read());
//                      printf("m_feCfg->VcalMed.read() = %d\n", m_feCfg->VcalMed.read());
//                      printf("m_feCfg->VcalHigh.read() - m_feCfg->VcalMed.read() = %d\n", m_feCfg->VcalHigh.read() - m_feCfg->VcalMed.read());
                        float injection_voltage = (m_feCfg->VcalHigh.read() - m_feCfg->VcalMed.read()) * maximum_injection_voltage / 4096.0;
                        float injection_charge = injection_voltage * capacitance_times_coulomb;
//                      printf("injection_voltage = %f\n", injection_voltage);

                        float noise_charge = Gauss::rand_normal(0, 50, 1); // generic, should remove

                        // sync front end
                        if (0 <= dc && dc < 64) {
                            // check the final pixel enable, and for now, just increment the number of hits - eventually, we should really just be writing hit data back to YARR
                            if (m_pixelRegisters[dc * 2][row] & 0x1) totalDigitalHits += 1;
                            if (m_pixelRegisters[dc * 2 + 1][row] & 0x1) totalDigitalHits += 1;
                        }
                        // linear front end
                        if (64 <= dc && dc < 132) {
                            for (int pix = 0; pix <= 1; pix++) {
                                noise_charge = m_rd53aLinPixelModelObjects[dc * 2 + pix - 128][row]->calculateNoise(); // overwrite the previous generic initialization
                                float lin_maximum_global_threshold_voltage = 1.2; // what should this actually be?
//                              printf("m_feCfg->VthresholdLin.read() = %d\n", m_feCfg->VthresholdLin.read());
                                float lin_global_threshold_with_smearing = m_rd53aLinPixelModelObjects[dc * 2 + pix - 128][row]->calculateThreshold(m_feCfg->VthresholdLin.read());
                                float lin_global_threshold_voltage = (lin_global_threshold_with_smearing) * lin_maximum_global_threshold_voltage / 1024.0;
                                float lin_global_threshold_charge = lin_global_threshold_voltage * capacitance_times_coulomb; // I imagine this might need a different capacitance

//                              printf("lin_global_threshold_voltage = %f\n", lin_global_threshold_voltage);
//                              printf("injection_charge = %f\n", injection_charge);
//                              printf("noise_charge = %f\n", noise_charge);
//                              printf("lin_global_threshold_charge = %f\n", lin_global_threshold_charge);

                                if (injection_charge + noise_charge - lin_global_threshold_charge > 0) {
                                    // check the final pixel enable, and for now, just increment the number of hits - eventually, we should really just be writing hit data back to YARR
                                    if (m_pixelRegisters[dc * 2 + pix][row] & 0x1) {
                                      linAnalogHits += 1;
                                      analogHits->fill(dc * 2 + pix, row);
                                      linScurve[dc * 2 + pix - 128][row]->fill((m_feCfg->VcalHigh.read() - m_feCfg->VcalMed.read()));
                                    }
                                }

                                if (m_pixelRegisters[dc * 2 + pix][row] & 0x1) totalDigitalHits += 1;
                            }
                        }
                        // differential front end
                        if (132 <= dc && dc < 200) {
                            for (int pix = 0; pix <= 1; pix++) {
                                noise_charge = m_rd53aDiffPixelModelObjects[dc * 2 + pix - 264][row]->calculateNoise(); // overwrite the previous generic initialization
                                float diff_maximum_global_threshold_voltage = 1.2; // what should this actually be?
//                              printf("m_feCfg->Vth1Diff.read() = %d\n", m_feCfg->Vth1Diff.read());
//                              printf("m_feCfg->Vth2Diff.read() = %d\n", m_feCfg->Vth2Diff.read());
                                float diff_global_threshold_with_smearing = m_rd53aDiffPixelModelObjects[dc * 2 + pix - 264][row]->calculateThreshold(m_feCfg->Vth1Diff.read(), m_feCfg->Vth2Diff.read());
                                float diff_global_threshold_voltage = (diff_global_threshold_with_smearing) * diff_maximum_global_threshold_voltage / 1024.0;
                                float diff_global_threshold_charge = diff_global_threshold_voltage * capacitance_times_coulomb; // I imagine this might need a different capacitance

//                              printf("diff_global_threshold_voltage = %f\n", diff_global_threshold_voltage);
//                              printf("injection_charge = %f\n", injection_charge);
//                              printf("noise_charge = %f\n", noise_charge);
//                              printf("diff_global_threshold_charge = %f\n", diff_global_threshold_charge);

                                if (injection_charge + noise_charge - diff_global_threshold_charge > 0) {
                                    // check the final pixel enable, and for now, just increment the number of hits - eventually, we should really just be writing hit data back to YARR
                                    if (m_pixelRegisters[dc * 2 + pix][row] & 0x1) {
                                      diffAnalogHits += 1;
                                      analogHits->fill(dc * 2 + pix, row);
                                      diffScurve[dc * 2 + pix - 264][row]->fill((m_feCfg->VcalHigh.read() - m_feCfg->VcalMed.read()));
                                    }
                                }

                                if (m_pixelRegisters[dc * 2 + pix][row] & 0x1) totalDigitalHits += 1;
                            }
                        }
                    }
                }
                // for now, print the total number of hits - eventually, we should really just be writing hit data back to YARR
//              printf("totalDigitalHits = %d\n", totalDigitalHits);
//              printf("diffAnalogHits = %d\n", diffAnalogHits);
//              printf("linAnalogHits = %d\n", linAnalogHits);
//              printf("syncAnalogHits = %d\n", syncAnalogHits);
//              printf("m_feCfg->VcalHigh.read() = %d\n", m_feCfg->VcalHigh.read());
            }
            else if (m_header == 0x6666) { // wrRegister
                m_id_address_some_data = m_txRingBuffer->read32();
//              printf("Rd53aEmu got id_address_some_data word: 0x%x\n", m_id_address_some_data);

                uint8_t byte1 = (m_id_address_some_data >> 16) >> 8;
                uint8_t byte2 = (m_id_address_some_data >> 16) & 0x00FF;
                uint8_t byte3 = (m_id_address_some_data & 0x0000FFFF) >> 8;
                uint8_t byte4 = (m_id_address_some_data & 0x0000FFFF) & 0x00FF;

//              printf("0x%x decoded into: 0x%x\n", byte1, eightToFive[byte1]);
//              printf("0x%x decoded into: 0x%x\n", byte2, eightToFive[byte2]);
//              printf("0x%x decoded into: 0x%x\n", byte3, eightToFive[byte3]);
//              printf("0x%x decoded into: 0x%x\n", byte4, eightToFive[byte4]);

                byte1 = eightToFive[byte1];
                byte2 = eightToFive[byte2];
                byte3 = eightToFive[byte3];
                byte4 = eightToFive[byte4];

                uint32_t address = (byte2 << 4) + (byte3 >> 1);
//              printf("address: 0x%x, %d\n", address, address);

                if (byte1 & 0x01) { // check the bit which determines whether big data or small data should be read
                    printf("big data expected\n");
                    // 50(?) more bits of data
                }
                else {
//                  printf("small data expected\n");
                    // 10(?) more bits of data
                    m_small_data = m_txRingBuffer->read32();
//                  printf("Rd53aEmu got the small_data word 0x%x\n", m_small_data);

                    uint8_t data_byte1 = (m_small_data >> 16) >> 8;
                    uint8_t data_byte2 = (m_small_data >> 16) & 0x00FF;
                    uint8_t data_byte3 = (m_small_data & 0x0000FFFF) >> 8;
                    uint8_t data_byte4 = (m_small_data & 0x0000FFFF) & 0x00FF;

//                  printf("0x%x decoded into: 0x%x\n", data_byte1, eightToFive[data_byte1]);
//                  printf("0x%x decoded into: 0x%x\n", data_byte2, eightToFive[data_byte2]);
//                  printf("0x%x decoded into: 0x%x\n", data_byte3, eightToFive[data_byte3]);
//                  printf("0x%x decoded into: 0x%x\n", data_byte4, eightToFive[data_byte4]);

                    data_byte1 = eightToFive[data_byte1];
                    data_byte2 = eightToFive[data_byte2];
                    data_byte3 = eightToFive[data_byte3];
                    data_byte4 = eightToFive[data_byte4];

                    uint32_t data = data_byte2 + (data_byte1 << 5) + (byte4 << 10) + ((byte3 & 0x1) << 15);
//                  printf("data: 0x%x, %d\n", data, data);

                    if (address == 0) { // configure pixels based on what's in the GR
//                      printf("being asked to configure pixels\n");
//                      printf("m_feCfg->PixMode.read() = 0x%x\n", m_feCfg->PixMode.read());
//                      printf("m_feCfg->BMask.read() = 0x%x\n", m_feCfg->BMask.read());
                        if (m_feCfg->PixMode.read() == 0x2) { // auto col = 1, auto row = 0, broadcast = 0
                            // configure all pixels in row m_feCfg->RegionRow.read() with value sent
                            for (unsigned dc = 0; dc < 200; dc++) {
                                m_pixelRegisters[dc * 2][m_feCfg->RegionRow.read()] = (uint8_t) (data & 0x00FF);
                                m_pixelRegisters[dc * 2 + 1][m_feCfg->RegionRow.read()] = (uint8_t) (data >> 8);
//                              printf("pixel %d %d 0x%x\n", dc * 2, m_feCfg->RegionRow.read(), data & 0x00FF);
                            }
                            // increment m_feCfg->RegionRow
                            if (m_feCfg->RegionRow.read() + 1 < 400) {
                                m_feCfg->RegionRow.write(m_feCfg->RegionRow.read() + 1);
                            }
                            else {
                                m_feCfg->RegionRow.write(0);
                            }
                        }
                    }
                    else { // configure the global register
                        m_feCfg->m_cfg[address] = data; // this is basically where we actually write to the global register
                    }
                }
            }
            else {
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
