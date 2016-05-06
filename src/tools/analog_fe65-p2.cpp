#include <SpecController.h>
#include <TxCore.h>
#include <RxCore.h>
#include <RawData.h>
#include <Histo2d.h>

#include <iostream>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

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
    if(0){
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
   
    std::string trash;
    std::cout << "Waiting" << std::endl;
    std::cin >> trash;

    // Turn on clocks
    tx.writeFifo(0x0);
    tx.writeFifo(0x80300003);
    
    // Reset
    tx.writeFifo(0x0);
    tx.writeFifo(0x80300033);
    usleep(5000);
    
    //Clocks on & Analog Inj
    // Pix D conf = 0
    tx.writeFifo(0x0);
    tx.writeFifo(0x8030000B);
    usleep(5000);
       
    // Setup pulser DAC
    unsigned PlsrDac = 1000;
    uint32_t dacReg = ((0x7<<12) | (PlsrDac << 2)) & 0x0000FFFF;
    tx.writeFifo(0x0);
    tx.writeFifo(0x80330000 | dacReg);
    tx.writeFifo(0x0);
    tx.writeFifo(0x80310000 + (0x1 << 5));
    usleep(1000);
    
    rx.setRxEnable(0x1);
    std::cout << "Waiting" << std::endl;
    std::cin >> trash;
    // Inject & trigger
    
    tx.writeFifo(0x0);
    //tx.writeFifo(0x80320000 + (greg.reg.LatencyCnfg+60)); // 1MHz injection
    tx.writeFifo(0x80320000 + (greg.reg.LatencyCnfg+5));
   
    uint16_t mask[16];
    for (unsigned i=0; i<16; i++) {
        mask[i] = 0;
    }
    mask[0] = 0x1;
    
    Histo2d occ("Analog-Scan", 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(NULL));    
    occ.setAxisTitle("Col", "Row", "Hits");
    for (unsigned i=0; i<16; i++) {
        std::cout << "-> Mask Stage: " << i << std::endl; 
        // 1 quad cols at a time
        // shift by one at the end
        greg.reg.ColEnCnfg = 0xFFFF;
        greg.reg.ColSrEnCnfg = (0xFFFF);
        greg.reg.vthin1DacConf = 255; //255
        //greg.reg.preCompVbnDacConf = 0;
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
        shift_pixel(&tx,(uint16_t) (0x1<<i));
            usleep(5000);
        greg.reg.InjEnLdCnfg = 1;
        greg.reg.PixConfLdCnfg = 3;
        config_global(&tx, &greg.array[0]);// Write config
        
        greg.reg.InjEnLdCnfg = 0;
        greg.reg.PixConfLdCnfg = 0;
        config_global(&tx, &greg.array[0]);// Write config
            
        // Write 0s
        //shift_pixel(&tx, (uint16_t) 0x0000);
        usleep(5000);
        for (unsigned j=0; j<4; j++) {
            std::cout << "---> Quad Col: " << j << std::endl;
            // Shift by one at the end
            //greg.reg.ColSrEnCnfg = (0x1111<<j);
            greg.reg.ColEnCnfg = (0x1111<<j);
            greg.reg.vthin1DacConf = 50; //255
            //greg.reg.preCompVbnDacConf = 50;
            config_global(&tx, &greg.array[0]);// Write config
        
            Histo2d mask_step(std::string("Index_" + std::to_string(i)), 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(NULL));
            usleep(10000);
            for(unsigned k=0; k<50; k++) {
                // Number of injections
                // Inject
                tx.writeFifo(0x0);
                tx.writeFifo(0x80310002);
                usleep(500);
                getData(&rx, &mask_step);
            }
            occ.add(&mask_step);
            mask_step.plot("bla");
        }
    }
    rx.setRxEnable(0x0);
    std::cout << "Hits = " << occ.getNumOfEntries() << std::endl;
    occ.plot("occ");
    return 0;
}
