#include <iostream>
#include <chrono>
#include <unistd.h>
#include "SpecController.h"
#include "Rd53a.h"
#include <fstream>
#include <cmath>

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

void decode(RawData *data, std::array<uint16_t,8> *readcount) {
    if (data != NULL) {
        int frameNum = 0;
        unsigned zz = 0;
        unsigned stat = 0;
        unsigned addr1 = 0;
        unsigned val1 = 0;
        unsigned val1_1 = 0;
        unsigned val1_2 = 0;
        unsigned addr2 = 0;
        unsigned val2 = 0;

        for (unsigned i=0; i<data->words; i++) {
            if (data->buf[i] != 0xFFFFFFFF) {
                // std::cout << "[RawBuf]" << std::bitset<32>(data->buf[i]) << std::endl;
                if(frameNum%2==0) {
                    zz = 0;
                    stat = 0;
                    addr1 = 0;
                    val1 = 0;
                    val1_1 = 0;
                    val1_2 = 0;
                    addr2 = 0;
                    val2 = 0;

                    zz = 0xFF & (data->buf[i] >> 24);

                    if(!(zz == 0x55 || zz == 0x99 || zz == 0xd2)) {
                        std::cout << "wrong Aurora code" << std::endl;
                        std::cout << "[zz]" << std::hex << zz << std::dec << std::endl;
                        return;
                    }

                    stat = 0xF & (data->buf[i] >> 20);
                    addr1 = 0x3FF & (data->buf[i] >> 10);
                    val1_1 = (0x3FF & (data->buf[i] >> 0)) << 6;
                }
                else {

                    val1_2 = 0x3F & (data->buf[i] >> 26);
                    val1 = val1_1 + val1_2;

                    addr2 = 0x3FF & (data->buf[i] >> 16);
                    val2 = 0xFFFF & (data->buf[i] >> 0);

                    // display results
                    // std::cout << "[zz]" << std::hex << zz << std::dec << std::endl;
                    if(stat != 0) {
                        std::cout << "not Ready status!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                        std::cout << "[stat]" << stat << std::endl;
                    }
                    if(zz == 0x99 || zz == 0xd2) {
                        std::cout << " [addr1]" << addr1 << std::endl;

                        if(110 <= addr1 && addr1 <= 117) {
                            // std::cout << "[# of start/stop 1]" << ((val1 >> 12) & 0xF) << std::endl;

                            readcount->at(addr1-110) = ((val1 >> 0) & 0xFFF);
                            std::cout << "[count1]" << ((val1 >> 0) & 0xFFF) << std::endl;
                        }
                        else
                            std::cout << "[val1]" << val1 << std::endl;
                    }
                    if(zz == 0x55 || zz == 0xd2) {
                        std::cout << " [addr2]" << addr2 << std::endl;

                        if(110 <= addr2 && addr2 <= 117) {
                            // std::cout << "[# of start/stop 2]" << ((val2 >> 12) & 0xF) << std::endl;

                            readcount->at(addr2-110) = ((val2 >> 0) & 0xFFF);
                            std::cout << "[count2]" << ((val2 >> 0) & 0xFFF) << std::endl;
                        }
                        else
                            std::cout << "[val2]" << val2 << std::endl;

                    }

                }
            }
            frameNum++;
        }
    }
}

void printHelp();

int main(int argc, char *argv[]) {

    int duration = 0;
    int chunk = 1;
    int specid = 0;
    int iterations = 1;

    int c;
    while ((c = getopt(argc, argv, "hd:r:s:i:")) != -1) {
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'i':
                iterations = atoi(optarg);
                break;
            case 'd':
                duration = atoi(optarg);
                break;
            case 'r':
                chunk = atoi(optarg);
                break;
            case 's':
                specid = atoi(optarg);
                break;
            case '?':
                std::cerr << "-> Unknown parameter: " << (char)optopt << std::endl;
                return -1;
            default:
                std::cerr << "-> Error while parsing command line parameters!" << std::endl;
                return -1;
        }
    }

    std::cout << "-------------------------------------------------" << std::endl << std::endl;
    std::cout << "test ring oscillators." << std::endl;
    std::cout << "duration: " << duration << std::endl;
    std::cout << "# of chunks: " << chunk << std::endl;
    std::cout << std::endl << "-------------------------------------------------" << std::endl << std::endl << std::endl;


    SpecController spec;
    spec.init(specid);
    spec.setupMode();

    //Send IO config to active FMC
    spec.writeSingle(0x6<<14 | 0x0, 0x080000);
    spec.writeSingle(0x6<<14 | 0x1, 0xF);
    spec.setCmdEnable(0x1);
    spec.setRxEnable(0x0);

    unsigned chipId = 0;

    Rd53a fe(&spec);
    fe.setChipId(chipId);
    std::cout << ">>> Configuring chip with default config ..." << std::endl;
    fe.configure();
    std::cout << " ... done." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // TODO check link sync
    spec.setRxEnable(0x1);


    std::array<uint16_t,8> *readcount = new std::array<uint16_t,8>;
    std::array<double,8> sum;
    std::array<double,8> sum_squared;


    // initialization
    // enable
    //109
    fe.writeRegister(&Rd53a::RingOscEn, 0xFF);
    // counter reset
    fe.writeRegister(&Rd53a::RingOsc0, 0);
    fe.writeRegister(&Rd53a::RingOsc1, 0);
    fe.writeRegister(&Rd53a::RingOsc2, 0);
    fe.writeRegister(&Rd53a::RingOsc3, 0);
    fe.writeRegister(&Rd53a::RingOsc4, 0);
    fe.writeRegister(&Rd53a::RingOsc5, 0);
    fe.writeRegister(&Rd53a::RingOsc6, 0);
    fe.writeRegister(&Rd53a::RingOsc7, 0);

    // read initial values
    fe.readRegister(&Rd53a::RingOsc0);
    fe.readRegister(&Rd53a::RingOsc1);
    fe.readRegister(&Rd53a::RingOsc2);
    fe.readRegister(&Rd53a::RingOsc3);
    fe.readRegister(&Rd53a::RingOsc4);
    fe.readRegister(&Rd53a::RingOsc5);
    fe.readRegister(&Rd53a::RingOsc6);
    fe.readRegister(&Rd53a::RingOsc7);

    std::cout << "check if initial values are 0" << std::endl;
    RawData *data = NULL;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (data != NULL)
            delete data;
        data = spec.readData();
        decode(data, readcount);
    } while (data != NULL);
    std::cout << "---------------------------" << std::endl;


    // start
    fe.writeRegister(&Rd53a::GlobalPulseRt, 0x2000);

    for (int i=0; i<iterations; i++) {
        for(int r=0; r<chunk; r++) {
            std::cout << "pulse" << std::endl;
            fe.globalPulse(chipId, duration);
            usleep(1000);
        }

        // read values
        //110-117
        fe.readRegister(&Rd53a::RingOsc0);
        fe.readRegister(&Rd53a::RingOsc1);
        fe.readRegister(&Rd53a::RingOsc2);
        fe.readRegister(&Rd53a::RingOsc3);
        fe.readRegister(&Rd53a::RingOsc4);
        fe.readRegister(&Rd53a::RingOsc5);
        fe.readRegister(&Rd53a::RingOsc6);
        fe.readRegister(&Rd53a::RingOsc7);

        std::cout << "measured values" << std::endl;
        data = NULL;
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (data != NULL)
                delete data;
            data = spec.readData();
            decode(data, readcount);
        } while (data != NULL);
        std::cout << "---------------------------" << std::endl;


        for (unsigned j=0; j<8; j++) {
            sum[j] += (*readcount)[j];
            sum_squared[j] += pow((*readcount)[j],2);
        }
        // counter reset
        fe.writeRegister(&Rd53a::RingOsc0, 0);
        fe.writeRegister(&Rd53a::RingOsc1, 0);
        fe.writeRegister(&Rd53a::RingOsc2, 0);
        fe.writeRegister(&Rd53a::RingOsc3, 0);
        fe.writeRegister(&Rd53a::RingOsc4, 0);
        fe.writeRegister(&Rd53a::RingOsc5, 0);
        fe.writeRegister(&Rd53a::RingOsc6, 0);
        fe.writeRegister(&Rd53a::RingOsc7, 0);

        while(!spec.isCmdEmpty()) {}
    }


    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::ofstream ofs;
    ofs.open("ring_osc.log",std::ios::app);
    ofs << now_time << " ";
    ofs << duration << " ";
    ofs << chunk << " ";
    ofs << iterations << " ";
    for(unsigned j=0; j<8; j++) {
        sum[j] = sum[j]/(double)iterations;
        sum_squared[j] = sqrt((sum_squared[j] - iterations*sum[j]*sum[j])/(double)(iterations-1));
        ofs <<  sum[j] << " " << sum_squared[j] << " ";
    }
    ofs << std::endl;

    delete readcount;

    fe.writeRegister(&Rd53a::GlobalPulseRt, 0x0);
    spec.setRxEnable(0x0);
    return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -d <duration>: duration of Global Pulse" << std::endl;
    std::cout << " -r <chunk>: # of chunks" << std::endl;
    std::cout << " -s <specid>: SPEC ID" << std::endl;
}
