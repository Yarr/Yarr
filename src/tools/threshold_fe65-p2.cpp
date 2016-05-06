#include <SpecController.h>
#include <TxCore.h>
#include <RxCore.h>
#include <RawData.h>
#include <Histo2d.h>
#include <Histo1d.h>
#include <SerialCom.h>
#include "lmcurve.h"

#include <iostream>
#include <stdint.h>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <fstream>
#include <iomanip>

struct field {
    //GlobalConfOutSr
    uint16_t TestHitCnfg : 16;
    uint16_t SignLdCnfg : 16;
    uint16_t InjEnLdCnfg : 16;
    uint16_t TDacLdCnfg : 16;
    uint16_t PixConfLdCnfg : 16;
    uint16_t SPARE_0 : 16;

    //GlobalConfOut
    uint16_t OneSrConf : 16;
    uint16_t HitDiscCnfg : 16;
    uint16_t LatencyCnfg : 16;
    uint16_t ColEnCnfg : 16;
    uint16_t ColSrEnCnfg : 16;
    uint16_t ColSrOutCnfg : 16;
    uint16_t SPARE : 16;
    uint16_t PrmpVbpDacConf : 16;
    uint16_t vthin1DacConf : 16;
    uint16_t vthin2DacConf : 16;
    uint16_t vffDacConf : 16;
    uint16_t VctrCF0DacConf : 16;
    uint16_t VctrCF1DacConf : 16;
    uint16_t PrmpVbnFolDacConf : 16;
    uint16_t vbnLccDacConf : 16;
    uint16_t compVbnDacConf : 16;
    uint16_t preCompVbnDacConf : 16;
};

// Errorfunction
// par[0] = Mean
// par[1] = Sigma
// par[2] = Normlization
#define SQRT2 1.414213562
#define ELECTRON_CHARGE 1.602e-19
namespace Fe65p2 {
    double scurveFct(double x, const double *par) {
        return 0.5*(2-erfc((x-par[0])/(par[1]*SQRT2)))*par[2];
    }

    double toCharge(double v) {return (1.18e-15)*v/ELECTRON_CHARGE;}
    double toV(double charge) {return (charge*ELECTRON_CHARGE)/1.18e-15;}
}

union global_reg {
    uint16_t array[23];
    struct field reg;
};  


void config_global(TxCore *tx, uint16_t *cfg) {
    // std::cout << " ... writing registers" << std::endl;
    for (unsigned i=0; i<23; i++) {
        uint32_t cmd = 0x80000000;
        cmd |= (0xefff & i) << 16;
        cmd |= (0xffff & cfg[i]);
        tx->writeFifo(0x0);
        tx->writeFifo(cmd);
    }

    //std::cout << " ... shifting into sreg and loading" << std::endl;
    // Send config	
    tx->writeFifo(0x0);
    tx->writeFifo(0x80310001);

    usleep(5000);
}

void shift_pixel(TxCore *tx, uint16_t mask) {
    //std::cout << " ... writing pixel mask" << std::endl;
    for (unsigned i=0; i<16; i++) {
        tx->writeFifo(0x0);
        tx->writeFifo(0x80000000 + ((0x20 + i) << 16) + mask);
    }

    //std::cout << " ... shifting into pixel sreg" << std::endl;
    // Send pixel config	
    tx->writeFifo(0x0);
    tx->writeFifo(0x80310004);

    usleep(50000);
}

void load_config(TxCore *tx) {
    //std::cout << " .. pusling ld_cnfg" << std::endl;
    // Send pixel config	
    tx->writeFifo(0x0);
    tx->writeFifo(0x80310008);
}

void shiftByOne(TxCore *tx) {
    //std::cout << " .. pusling ld_cnfg" << std::endl;
    // Send pixel config	
    tx->writeFifo(0x0);
    tx->writeFifo(0x80310010);
}

void shift_pixel(TxCore *tx, uint16_t *reg) {
    for (unsigned i=0; i<16; i++) {
        tx->writeFifo(0x0);
        tx->writeFifo(0x80000000 + ((0x20 + i) << 16) + reg[i]);
    }

    // Send pixel config	
    tx->writeFifo(0x0);
    tx->writeFifo(0x80310004);

    usleep(5000);
}

void getData(RxCore *rx, Histo2d *h) {
    unsigned count = 0;
    RawDataContainer rdc;
    RawData *newData = NULL;
    do {
        usleep(500);
        newData = rx->readData();
        if (newData != NULL) {
            rdc.add(newData);
            count += newData->words;
        }
    } while(newData != NULL);

    //std::cout << "Received: " << count << " words!" << std::endl;
    for (unsigned i=0; i<rdc.size(); i++) {
        for (unsigned j=0; j<rdc.words[i]; j++) {
            uint32_t data = rdc.buf[i][j];
            data = data & 0x00FFFFFF;
            if ((data & 0x00800000) == 0x00800000) {
                //std::cout << "BCID = " << (data & 0x007FFFFF) << std::endl;
            } else {
                unsigned col  = (data & 0x1e0000) >> 17;
                unsigned row  = (data & 0x01F800) >> 11;
                unsigned rowp = (data & 0x000400) >> 10;
                unsigned tot0 = (data & 0x0000F0) >> 4;
                unsigned tot1 = (data & 0x00000F) >> 0;

                if ((tot0 != 15 || tot1 != 15) && (tot0 > 0 || tot1 > 0)) {
                    unsigned real_col = 0;
                    unsigned real_row = 0;
                    if (rowp == 1) {
                        real_col = (col*4) + ((row/32)*2) + 1;
                    } else {
                        real_col = (col*4) + ((row/32)*2) + 2;
                    }

                    if (row < 32) {
                        if (tot1 != 15) {
                            real_row = (row+1)*2;
                        } else {
                            real_row = (row+1)*2 - 1;
                        }
                    } else {
                        if (tot1 != 15) {
                            real_row = 64 - (row-32)*2;
                        } else {
                            real_row = 64 - (row-32)*2 - 1;
                        }
                    }
                    h->fill(real_col, real_row);

                    //std::cout << "Col = " << col << "; Row = " << row << "; RowP = " << rowp << "; ToT0 = " << tot0 << "; ToT1 = " << tot1 << "; Real Col = " << real_col << "; Real Row =" << real_row << std::endl;
                }

            }
        }
    }

}

int main(void) {
    SerialCom serial("/dev/ttyUSB0");
    serial.write("++auto 0\r\n");
    serial.write("++addr 5\r\n");

    SpecController mySpec(0);
    TxCore tx(&mySpec);
    RxCore rx(&mySpec);

    union global_reg greg;

    // Default config
    greg.reg.TestHitCnfg = 0;
    greg.reg.SignLdCnfg = 0;
    greg.reg.InjEnLdCnfg = 0;
    greg.reg.TDacLdCnfg = 0;
    greg.reg.PixConfLdCnfg = 0;
    greg.reg.SPARE_0 = 0;

    greg.reg.OneSrConf = 0;
    greg.reg.HitDiscCnfg = 0;
    greg.reg.LatencyCnfg = 30;
    greg.reg.ColEnCnfg = 0xFFFF;
    greg.reg.ColSrEnCnfg = 0xFFFF;
    greg.reg.ColSrOutCnfg = 15;
    greg.reg.SPARE = 0;
    greg.reg.PrmpVbpDacConf = 50; //36
    greg.reg.vthin1DacConf = 255; //255
    greg.reg.vthin2DacConf = 0; //0
    greg.reg.vffDacConf = 24;
    greg.reg.VctrCF0DacConf = 0;
    greg.reg.VctrCF1DacConf = 0;
    greg.reg.PrmpVbnFolDacConf = 50;
    greg.reg.vbnLccDacConf = 1;
    greg.reg.compVbnDacConf = 25;
    greg.reg.preCompVbnDacConf = 50;

    std::cout << "Size: " << sizeof(greg.reg) << std::endl;


    rx.setRxEnable(0x0);

    tx.setCmdEnable(0x1);
    // Disable clocks
    tx.writeFifo(0x0);
    tx.writeFifo(0x80300000);


    config_global(&tx, &greg.array[0]);// Write config

    // Load defaults
    if (1) {
        //Write 1s
        shift_pixel(&tx, (uint16_t) 0xFFFF);

        greg.reg.SignLdCnfg = 0;
        greg.reg.InjEnLdCnfg = 0;
        greg.reg.TDacLdCnfg = 15;
        greg.reg.PixConfLdCnfg = 0;
        config_global(&tx, &greg.array[0]);// Write config

        greg.reg.SignLdCnfg = 0;
        greg.reg.InjEnLdCnfg = 0;
        greg.reg.TDacLdCnfg = 0;
        greg.reg.PixConfLdCnfg = 0;
        config_global(&tx, &greg.array[0]);// Write config

        // Write 0s
        shift_pixel(&tx, (uint16_t) 0x0000);

        greg.reg.SignLdCnfg = 1;
        greg.reg.InjEnLdCnfg = 1;
        greg.reg.TDacLdCnfg = 0;
        greg.reg.PixConfLdCnfg = 3;
        config_global(&tx, &greg.array[0]);// Write config

        greg.reg.SignLdCnfg = 0;
        greg.reg.InjEnLdCnfg = 0;
        greg.reg.TDacLdCnfg = 0;
        greg.reg.PixConfLdCnfg = 0;
        config_global(&tx, &greg.array[0]);// Write config
    }

    // Enable some pixels
    if (0){
        shift_pixel(&tx, (uint16_t) 0xFFFF); // Every 16th
        greg.reg.PixConfLdCnfg = 3;
        config_global(&tx, &greg.array[0]);

        greg.reg.PixConfLdCnfg = 0;
        config_global(&tx, &greg.array[0]);// Write config

        shift_pixel(&tx,(uint16_t) 0x0);
    }

    shift_pixel(&tx, (uint16_t) 0x0); // Every 16th
    greg.reg.TestHitCnfg = 0;
    //greg.reg.ColEnCnfg = 0xFFFF;
    //greg.reg.ColSrEnCnfg = 0xFFFF;
    config_global(&tx, &greg.array[0]);// Write config
    //shift_pixel(&tx,(uint16_t) 0x0);


    // Turn on clocks
    tx.writeFifo(0x0);
    tx.writeFifo(0x80300003);

    // Reset
    tx.writeFifo(0x0);
    tx.writeFifo(0x80300033);
    usleep(5000);

    //Clocks on & Analog Inj
    tx.writeFifo(0x0);
    tx.writeFifo(0x8030000B);
    usleep(5000);

    rx.setRxEnable(0x1);
    std::string trash;
    std::cout << "Waiting" << std::endl;
    std::cin >> trash;
    // Inject & trigger

    tx.writeFifo(0x0);
    //tx.writeFifo(0x80320000 + (greg.reg.LatencyCnfg+60));
    tx.writeFifo(0x80320000 + (greg.reg.LatencyCnfg+4));
    

    std::fstream file("par_file.dat", std::ios::out);
    unsigned thr = 20;
    for (unsigned parameter = 5; parameter < 161; parameter+=5) {
        unsigned step = 0;
        double adjust = (thr>50)?((thr-50)/3*0.005):0;
        if (adjust > 0.6) adjust = 0;
        //double vcal_low = 0.025+adjust;
        //double vcal_high = 0.225+adjust;
        //double vcal_step = 0.001;
        unsigned vcal_low = 0;
        unsigned vcal_high = 350;
        unsigned vcal_step = 5;
        int vcal_bins = (vcal_high - vcal_low)/vcal_step;
        int injections = 50;
        std::cout << "Vcal Low: " << vcal_low <<", Vcal High: " << vcal_high << std::endl;

        Histo2d smap2d("smap", vcal_bins+1, vcal_low-(vcal_step*0.5), vcal_high+(vcal_step*0.5), injections-1, 0.5, injections-0.5, typeid(NULL));
        Histo1d smap1d("smap", vcal_bins+1, vcal_low-(vcal_step*0.5), vcal_high+(vcal_step*0.5), typeid(NULL));
        smap2d.setAxisTitle("Vcal", "Hits", "Pixels");
        smap1d.setAxisTitle("Vcal", "Hits");

        Histo1d *pixelOcc[64*64];
        for (unsigned bin=0; bin<(64*64); bin++)
            pixelOcc[bin] = new Histo1d(("pixelOcc-" + std::to_string(bin)), vcal_bins, vcal_low-(vcal_step*0.5), vcal_high-(vcal_step*0.5), typeid(NULL));

        double *x = new double[vcal_bins];
        unsigned counter = 0;
        for(unsigned vcal=vcal_low; vcal < vcal_high; vcal+=vcal_step) {
            x[counter] = vcal;
            counter ++;
            std::cout << "Vcal = " << vcal << std::endl;
            //std::stringstream stream;
            //stream << std::setprecision(4) << vcal;
            //std::string vcal_str = stream.str();
            //std::cout << "Vcal = " << vcal_str << std::endl;
            //serial.write((":VOLT " + vcal_str + "\r\n"));
            //serial.write((":VOLT " + std::to_string(vcal) + "\r\n"));
            //serial.write((":VOLT:OFFSET " + std::to_string(1.2-vcal) + "\r\n"));
            uint32_t dacReg = ((0x7<<12) | (vcal << 2)) & 0x0000FFFF;
            tx.writeFifo(0x0);
            tx.writeFifo(0x80330000 | dacReg);
            tx.writeFifo(0x0);
            tx.writeFifo(0x80310000 + (0x1 << 5));
            usleep(5000);
            Histo2d occ("Occupancy", 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(NULL));    
            occ.setAxisTitle("Col", "Row", "Hits");
            for (unsigned i=0; i<8; i++) {
                std::cout << "-> Mask Stage: " << i << std::endl; 
                // 1 quad cols at a time
                // shift by one at the end
                greg.reg.ColEnCnfg = 0xFFFF;
                greg.reg.ColSrEnCnfg = (0xFFFF);
                greg.reg.vthin1DacConf = 255; //255
                config_global(&tx, &greg.array[0]);// Write config

                /*
                   shift_pixel(&tx,(uint16_t) (0x0));
                   greg.reg.InjEnLdCnfg = 1;
                   greg.reg.PixConfLdCnfg = 3;
                   config_global(&tx, &greg.array[0]);// Write config

                   greg.reg.InjEnLdCnfg = 0;
                   greg.reg.PixConfLdCnfg = 0;
                   config_global(&tx, &greg.array[0]);// Write config
                   */
                shift_pixel(&tx,(uint16_t) (0x101<<i));
                greg.reg.InjEnLdCnfg = 1;
                greg.reg.PixConfLdCnfg = 3;
                config_global(&tx, &greg.array[0]);// Write config

                greg.reg.InjEnLdCnfg = 0;
                greg.reg.PixConfLdCnfg = 0;
                config_global(&tx, &greg.array[0]);// Write config

                // Write 0s
                //shift_pixel(&tx, (uint16_t) 0x0000);

                for (unsigned j=0; j<4; j++) {
                    std::cout << "---> Quad Col: " << j << std::endl;
                    // Shift by one at the end
                    greg.reg.ColSrEnCnfg = (0x1111<<j);
                    greg.reg.ColEnCnfg = (0x1111<<j);
                    greg.reg.vthin1DacConf = thr; //255
                    greg.reg.PrmpVbpDacConf = parameter; //36
                    config_global(&tx, &greg.array[0]);// Write config
                    usleep(5000);

                    Histo2d mask_step(std::string("Index_" + std::to_string(i)), 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(NULL));
                    for(unsigned k=0; k<injections; k++) {
                        // Number of injections
                        // Inject
                        tx.writeFifo(0x0);
                        tx.writeFifo(0x80310002);
                        usleep(50);
                        getData(&rx, &mask_step);
                    }
                    occ.add(&mask_step);
                    //mask_step.plot("bla");
                }
            }
            std::cout << "Hits = " << occ.getNumOfEntries() << std::endl;
            //occ.plot("occ_" + std::to_string(step));
            for(unsigned bin=0; bin<64*64; bin++) {
                pixelOcc[bin]->fill(vcal, occ.getBin(bin));
                smap2d.fill(vcal, occ.getBin(bin));
                smap1d.fill(vcal, occ.getBin(bin));
            }
            step++;
        }
        smap2d.plot(std::to_string(parameter));
        smap1d.plot(std::to_string(parameter));

        Histo1d thrDist("ThresholdDist", 151, -5, 3005, typeid(NULL));
        thrDist.setAxisTitle("Threshold [e]", "# of pixel");
        Histo1d sigDist("SigmaDist", 51, -1.5, 151.5, typeid(NULL));
        sigDist.setAxisTitle("Threshold Sigma [e]", "# of pixel");
        Histo2d thrMap("ThresholdMap", 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(NULL));
        thrMap.setAxisTitle("Col", "Row", "Threshold [e]");
        Histo2d sigMap("SigmaMap", 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(NULL));
        sigMap.setAxisTitle("Col", "Row", "Threshold Sigma [e]");

        Histo1d *thrDistV[8];
        Histo1d *sigDistV[8];
        Histo2d *thrMapV[8];
        Histo2d *sigMapV[8];
        for(unsigned i=0; i<8; i++) {
            thrDistV[i] = new Histo1d(("ThresholdDist-"+std::to_string(i)), 151, -5, 3005, typeid(NULL));
            thrDistV[i]->setAxisTitle("Threshold [e]", "# of pixel");
            sigDistV[i] = new Histo1d(("SigmaDist-"+std::to_string(i)), 51, -1.5, 151.5, typeid(NULL));
            sigDistV[i]->setAxisTitle("Threshold Sigma [e]", "# of pixel");
            thrMapV[i] = new Histo2d(("ThresholdMap-"+std::to_string(i)), 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(NULL));
            thrMapV[i]->setAxisTitle("Col", "Row", "Threshold [e]");
            sigMapV[i] = new Histo2d(("SigmaMap-"+std::to_string(i)), 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(NULL));
            sigMapV[i]->setAxisTitle("Col", "Row", "Threshold Sigma [e]");
        }

        std::cout << "Analyzing" << std::endl;
        for(unsigned bin=0; bin<(64*64); bin++) {
            const unsigned n_par = 3;
            double par[n_par] = {(double) (vcal_high-vcal_low)/2.0, 3, (double) injections};
            lm_status_struct status;
            lm_control_struct control;
            control = lm_control_float;
            control.verbosity = 0;
            lmcurve(n_par, par, vcal_bins, &x[0], pixelOcc[bin]->getData(), Fe65p2::scurveFct, &control, &status);
            //std::cout << (bin/64)+1 << " " << (bin%64)+1 << std::endl;
            //std::cout << "[0] " << par[0] << " = " << Fe65p2::toCharge(par[0]) << std::endl;
            //std::cout << "[1] " << par[1] << " = " << Fe65p2::toCharge(par[1]) << std::endl;
            //std::cout << "[2] " << par[2] << std::endl;
            //std::cout << lm_shortmsg[status.outcome] << std::endl; 
            if (status.outcome < 4 && par[2] > (injections*0.9) && par[2] < (injections*1.1) && par[0] > vcal_low && par[0] < vcal_high) {
                thrDist.fill(Fe65p2::toCharge((0.564e-3*par[0])+0.011e-3));
                thrMap.fill((bin/64)+1, (bin%64)+1, Fe65p2::toCharge((0.564e-3*par[0])+0.011e-3));
                sigDist.fill(Fe65p2::toCharge((0.564e-3*par[1])+0.011e-3));
                sigMap.fill((bin/64)+1, (bin%64)+1, Fe65p2::toCharge((0.564e-3*par[1])+0.011e-3));

                thrDistV[(bin/512)]->fill(Fe65p2::toCharge((0.564e-3*par[0])+0.011e-3));
                thrMapV[(bin/512)]->fill((bin/64)+1, (bin%64)+1, Fe65p2::toCharge((0.564e-3*par[0])+0.011e-3));
                sigDistV[(bin/512)]->fill(Fe65p2::toCharge((0.564e-3*par[1])+0.011e-3));
                sigMapV[(bin/512)]->fill((bin/64)+1, (bin%64)+1, Fe65p2::toCharge((0.564e-3*par[1])+0.011e-3));
            }
            //if(bin%200 == 0)
            //    pixelOcc[bin]->plot("pix");
            delete pixelOcc[bin];

        }
        delete[] x;
        
        file << parameter << " ";
        file << thrDist.getMean() << " " << thrDist.getStdDev() << " ";
        file << sigDist.getMean() << " " << sigDist.getStdDev() << " ";
        for(unsigned i=0; i<8; i++) {
            //thrDistV[i]->plot(std::to_string(parameter));
            //sigDistV[i]->plot(std::to_string(parameter));
            //thrMapV[i]->plot(std::to_string(parameter));
            //sigMapV[i]->plot(std::to_string(parameter));
            std::cout << i << " Threshold Entries: " << thrDistV[i]->getEntries() << std::endl;
            std::cout << i << " Sigma Entries: " << sigDistV[i]->getEntries() << std::endl;
            std::cout << i << " Threshold Mean: " << thrDistV[i]->getMean() << " +-" << thrDistV[i]->getStdDev() << std::endl;
            std::cout << i << " Sigma Mean: " << sigDistV[i]->getMean() << " +-" << sigDistV[i]->getStdDev() << std::endl;
            file << thrDistV[i]->getMean() << " " << thrDistV[i]->getStdDev() << " ";
            file << sigDistV[i]->getMean() << " " << sigDistV[i]->getStdDev() << " ";

            delete thrDistV[i];
            delete sigDistV[i];
            delete thrMapV[i];
            delete sigMapV[i];
        }
        file << std::endl;

        thrDist.plot(std::to_string(parameter));
        sigDist.plot(std::to_string(parameter));
        thrMap.plot(std::to_string(parameter));
        sigMap.plot(std::to_string(parameter));
        std::cout << "Threshold Entries: " << thrDist.getEntries() << " " << thrDist.getEntries()/4096.0 << std::endl;
        std::cout << "Sigma Entries: " << sigDist.getEntries() << " " << thrDist.getEntries()/4096.0  << std::endl;
        std::cout << "Threshold Mean: " << thrDist.getMean() << " +-" << thrDist.getStdDev() << std::endl;
        std::cout << "Sigma Mean: " << sigDist.getMean() << " +-" << sigDist.getStdDev() << std::endl;
    }
    rx.setRxEnable(0x0);
    file.close();
    return 0;
}
