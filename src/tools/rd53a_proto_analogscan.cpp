#include <iostream>
#include <chrono>
#include <unistd.h>
#include "SpecController.h"
#include "Rd53a.h"
#include "Histo2d.h"

#define EN_RX2 0x1
#define EN_RX1 0x2
#define EN_RX4 0x4
#define EN_RX3 0x8
#define EN_RX6 0x10
#define EN_RX5 0x20
#define EN_RX8 0x40
#define EN_RX7 0x80

#define EN_RX10 0x100
#define EN_RX9 0x200
#define EN_RX12 0x400
#define EN_RX11 0x800
#define EN_RX14 0x1000
#define EN_RX13 0x2000
#define EN_RX16 0x4000
#define EN_RX15 0x8000

#define EN_RX18 0x10000
#define EN_RX17 0x20000
#define EN_RX20 0x40000
#define EN_RX19 0x80000
#define EN_RX22 0x100000
#define EN_RX21 0x200000
#define EN_RX24 0x400000
#define EN_RX23 0x800000

Histo2d* decode(RawData *data, unsigned &hits) {
    Histo2d *h = new Histo2d("tmp", 400, -.5, 399.5, 192, -0.5, 191.5, typeid(void));
    if (data != NULL) {
        for (unsigned i=0; i<data->words; i++) {
            if (data->buf[i] != 0xFFFFFFFF) {
                if ((data->buf[i] >> 25) & 0x1) {
                    //unsigned l1id = 0x1F & (data->buf[i] >> 20);
                    //unsigned l1tag = 0x1F & (data->buf[i] >> 15);
                    //unsigned bcid = 0x7FFF & data->buf[i];
                    //std::cout << "[Header] : L1ID(" << l1id << ") L1Tag(" << l1tag << ") BCID(" <<  bcid << ")" << std::endl;
                } else {
                    unsigned core_col = 0x3F & (data->buf[i] >> 26);
                    unsigned core_row = 0x3F & (data->buf[i] >> 20);
                    unsigned region = 0xF & (data->buf[i] >> 16);
                    unsigned tot0 = 0xF & (data->buf[i] >> 0); //left most
                    unsigned tot1 = 0xF & (data->buf[i] >> 4);
                    unsigned tot2 = 0xF & (data->buf[i] >> 8);
                    unsigned tot3 = 0xF & (data->buf[i] >> 12);

                    unsigned pix_col = core_col*8+((region&0x1)*4);
                    unsigned pix_row = core_row*8+(0x7&(region>>1));
                    //std::cout << "[Data] : COL(" << core_col << ") ROW(" << core_row  << ") Region(" << region
                    //    << ") TOT(" << tot3 << "," << tot2 << "," << tot1 << "," << tot0 
                    //    << ") RAW(0x" << std::hex << data->buf[i] << std::dec << ")" << std::endl;
                    if (tot0 != 0xF) {
                        hits++;
                        h->fill(pix_col, pix_row);
                    }
                    if (tot1 != 0xF) {
                        hits++;
                        h->fill(pix_col+1, pix_row);
                    }
                    if (tot2 != 0xF) {
                        hits++;
                        h->fill(pix_col+2, pix_row);
                    }
                    if (tot3 != 0xF) {
                        hits++;
                        h->fill(pix_col+3, pix_row);
                    }
                }
            }
        }
    }
    return h;
}

int main(int argc, char *argv[]) {

    int specNum = 0;
    if (argc > 1)
        specNum = std::stoi(argv[1]);

    SpecController spec;
    spec.init(specNum);

    //Send IO config to active FMC
    spec.writeSingle(0x6<<14 | 0x0, 0x080000);
    spec.writeSingle(0x6<<14 | 0x1, 0xF);
    spec.setCmdEnable(0x1);
    spec.setRxEnable(0x0);

    Rd53a fe(&spec);
    fe.setChipId(0);
    std::cout << ">>> Configuring chip with default config ..." << std::endl;
    fe.configure();
    std::cout << " ... done." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // TODO check link sync
    spec.setRxEnable(0x1);

    std::cout << ">>> Enabling digital injection" << std::endl;
    fe.writeRegister(&Rd53a::InjEnDig, 0);
    fe.writeRegister(&Rd53a::InjAnaMode, 1);
    fe.writeRegister(&Rd53a::InjVcalHigh, 4000);
    fe.writeRegister(&Rd53a::InjVcalMed, 1000);
    fe.writeRegister(&Rd53a::LatencyConfig, 48);
    fe.writeRegister(&Rd53a::GlobalPulseRt, 0x4000);

    std::cout << ">>> Enabling some pixels" << std::endl;
    unsigned max_mask_stage = 32; // Must be divisible by 192
    unsigned max_col_stage = 20; //Must be divisble by 400

    Histo2d *h = new Histo2d("Occupancy", 400, -.5, 399.5, 192, -0.5, 191.5, typeid(void));
            
    uint32_t duration = 0x4;
    spec.setTrigFreq(1000);
    spec.setTrigCnt(50);
    spec.setTrigWordLength(16);
    std::array<uint32_t, 16> trigWord;
    trigWord.fill(0x69696969);
    trigWord[15] = 0x69696363;
    trigWord[14] = fe.genCal(8, 0, 0, 1, 0, 0); // Inject
    trigWord[8] = fe.genTrigger(0xF, 4, 0xF, 8); // Trigger
    trigWord[7] = fe.genTrigger(0xF, 4, 0xF, 8); // Trigger
    trigWord[2] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(duration)); // global pulse for sync FE
    trigWord[1] = 0x69696363;
    trigWord[0] = fe.genCal(8, 1, 0, 0, 0, 0); // Arm inject
    spec.setTrigWord(&trigWord[0], 16);
    spec.setTrigConfig(INT_COUNT);

    unsigned total_hits = 0;
    for (unsigned mask_stage=0; mask_stage<max_mask_stage; mask_stage++) {
        // Loop over columns
        // Configure pixels
        unsigned act_pix = 0;
        for (unsigned col=0; col<400; col++) {
            for (unsigned row=0; row<192; row++) {
                if (row%8==(mask_stage%8) && col%4==(mask_stage/8)) { // one (out of 32) pixel per core
                    fe.setEn(col, row, 1);
                    fe.setInjEn(col, row, 1);
                    //fe.setHitbus(col, row, 1);
                    act_pix++;
                } else {
                    fe.setEn(col, row, 0);
                    fe.setInjEn(col, row, 0);
                    //fe.setHitbus(col, row, 1);
                }
            }
        }
        fe.configurePixels();
        while(!spec.isCmdEmpty()) {}
        std::cout << "Enabled " << act_pix << " pixels" << std::endl;

        for (unsigned col_stage=0; col_stage<max_col_stage; col_stage+=4) {

            std::cout << "Mask = " << mask_stage << " , Col Loop = " << col_stage << std::endl;
            unsigned col_cnt = 0;
            for (unsigned col=0; col<200; col+=4) {
                // Enable cores for injection
                fe.disableCalCol(col);
                fe.disableCalCol(col+1);
                fe.disableCalCol(col+2);
                fe.disableCalCol(col+3);
                if ((col%max_col_stage) == col_stage) {
                    fe.enableCalCol(col+(mask_stage/16));
                    fe.enableCalCol(col+(mask_stage/16)+2);
                    col_cnt+=4;
                }
            }
            unsigned hits = 0;
            while(!spec.isCmdEmpty()) {}
            std::this_thread::sleep_for(std::chrono::microseconds(200));
            fe.globalPulse(8, 4);
            std::this_thread::sleep_for(std::chrono::microseconds(200));
            
            spec.setTrigEnable(1);

            while (!spec.isTrigDone()) {
                RawData *data = NULL;
                do {
                    std::this_thread::sleep_for(std::chrono::microseconds(200));
                    if (data != NULL)
                        delete data;
                    data = spec.readData();
                    Histo2d *tmp_h = decode(data, hits);
                    h->add(*tmp_h);
                    delete tmp_h;
                } while (data != NULL);

            }
            spec.setTrigEnable(0);
            std::cout << "Got " << hits << " hits" << std::endl;
            total_hits += hits;


        }
    }
    h->plot("rd53a_proto_analog");
    std::cout << "Saw " << total_hits << " hits!" << std::endl;


    spec.setRxEnable(0x0);
    return 0;
}
