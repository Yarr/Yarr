/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

#include "Itkpixv2EmuCommandExe.h"
#include "logging.h"
#include <random>
#include <mutex>
#include <chrono>

namespace {
    auto rlog = logging::make_log("Itkpixv2EmuCommandExe");
}


Itkpixv2EmuCommandExe::Itkpixv2EmuCommandExe(EmuCom* rx, std::shared_ptr<Itkpixv2Cfg>& cfg){

    //attach the output pipeline
    m_rx = rx;

    //link the registers
    m_cfg = cfg;

    //initialize the encoder
    m_encoder = std::make_shared<Itkpixv2Encoder>();
    m_encoder->setEventsPerStream(1);

    //try to pre-allocate some memory to the output buffer
    m_encoder->getWords().reserve(1000);

}

void Itkpixv2EmuCommandExe::exe(const Itkpixv2EmuUtils::Cmd cmd){

    (this->*commandMap[cmd.header])(cmd);

}

void Itkpixv2EmuCommandExe::doSync(const Itkpixv2EmuUtils::Cmd& cmd){

}

void Itkpixv2EmuCommandExe::doPLLlock(const Itkpixv2EmuUtils::Cmd& cmd){

}

void Itkpixv2EmuCommandExe::doClear(const Itkpixv2EmuUtils::Cmd& cmd){

    rlog->warn("Clear command received - potential reset not implemented!");

}

void Itkpixv2EmuCommandExe::doGlobalPulse(const Itkpixv2EmuUtils::Cmd& cmd){
    
    rlog->warn("GlobalPulse command received - potential reset not implemented!");

}

void Itkpixv2EmuCommandExe::doCal(const Itkpixv2EmuUtils::Cmd& cmd){
    //First, check if we're doing digital or analog injection,
    //this is controlled by InjDigEn (global register 54,
    //1 for digital, 0 for analog).
    //The idea is to have the signals in all enabled pixels
    //generated with this function, then we'll trigger.

    //start by zeroing the current ToTs
    m_tots.reset();

    //handle noise
    if (!m_cfg->InjDigEn.read()) m_noiseDist = std::normal_distribution<float>(0., 50.);

    for (uint32_t pixel : m_activePixels){
        //The core-col calibration enable needs to be checked here
        //What is the ccol of the current pixel?
        uint16_t ccol = (pixel / 384) / 8;

        //What is the current ccol cal masking?
        uint64_t ccolCalEnabled = 0x0ULL;
        ccolCalEnabled |= m_cfg->EnCoreColCal0.read();
        ccolCalEnabled |= ((uint64_t)m_cfg->EnCoreColCal1.read() << 16);
        ccolCalEnabled |= ((uint64_t)m_cfg->EnCoreColCal2.read() << 32);
        ccolCalEnabled |= ((uint64_t)m_cfg->EnCoreColCal3.read() << 48);

        //if this ccol is masked, continue
        if (!((0x1ULL << ccol) & ccolCalEnabled)) continue;

        switch (m_cfg->InjDigEn.read()){
            //analog injection
            case 0:{
                rlog->trace("Analog injection happening...");
                //What's the injected charge?
                //This calibration works
                float injCharge = m_cfg->toCharge(m_cfg->InjVcalDiff.read());
                //rlog->info("Injected charge = {}", injCharge);
                
                //What is the global threshold? Probably quite overkill - implement the
                //left edge, middle and right edge treatment
                //The current pixel is most likely in the center.
                int globalDAC = m_cfg->DiffTh1M.read() - m_cfg->DiffTh2.read();
                if      (pixel / 384 <=1)    globalDAC = m_cfg->DiffTh1L.read() - m_cfg->DiffTh2.read();
                else if (pixel / 384 >= 398) globalDAC = m_cfg->DiffTh1R.read() - m_cfg->DiffTh2.read();
                //Include the random effect of non-shperical-cow pixels
                float globalDACCharge = m_thresholds(pixel) * Itkpixv2EmuUtils::globalDACToCharge(globalDAC);
                
                //What is the current pixel's TDAC?
                int TDAC = m_cfg->getTDAC(pixel / 384, pixel % 384);
                float TDACCharge = Itkpixv2EmuUtils::TDACToCharge(TDAC);
                //rlog->info("TDAC is {}", TDAC);

                //Add some noise on top  and translate the charge over threshold to ToT
                float noiseCharge = m_noiseDist(m_rng);//Itkpixv2EmuUtils::noiseCharge(m_rng);
                float tot =  Itkpixv2EmuUtils::chargeToToT(injCharge + noiseCharge - (globalDACCharge + TDACCharge));
                m_tots(pixel) = tot > 0 ? tot + 1 : 0;

                break;
            }
            case 1:{
                rlog->trace("Digital injection happening...");
                //Let's just put the middle value everywhere now...
                m_tots(pixel) = 9;
                break;
            }
        }

    }
    m_hitMapFilled = true;

}

void Itkpixv2EmuCommandExe::doWrReg(const Itkpixv2EmuUtils::Cmd& cmd){
    //rlog->info("Active pixels: {}", m_activePixels.size());
    
    //Can be either to pixel portal (register 0) or a global register.
    //Technically, only 9 bits represent the address, the 10-th bit
    //keeps track of the multiple-write mode
    switch (cmd.address & 0x1FF){
        case 0 : {
            //rlog->info("Writing pixel register, address 0x{:x}", cmd.address);

            //Registers PixRegionRow and PixRegionCol decide which of the pixel pairs the portal portals to
            uint16_t& val = m_cfg->pixRegs[m_cfg->PixRegionCol.read()][m_cfg->PixRegionRow.read()];
            
            //The pixel register has the following structure:
            //left pixel (16 bits): [TDAC sign, TDAC (4 bits), HitBus, Injection Enable, Enable] + right pixel analogously
            //different treatment of single and multiple write,
            //distinguished by the 10-th bit (0x200)
            switch ((cmd.address & 0x200) >> 9){
                //if 0, perform a single-write style of all 16 bits
                case 0: {
                    val = (cmd.data & 0xFFFF);
                    //bookkeeping
                    //left pixel
                    uint32_t coordinate = (m_cfg->PixRegionCol.read() * 2) * 384 + m_cfg->PixRegionRow.read();
                    (val & 0x0100) ? (void)m_activePixels.insert(coordinate) : (void)m_activePixels.erase(coordinate);
                    //right pixel
                    coordinate += 384;
                    (val & 0x0001) ? (void)m_activePixels.insert(coordinate) : (void)m_activePixels.erase(coordinate);
                    break;
                }
                //if it's 1, perform multiple-write style of either
                //TDAC or enable bits
                case 1: {
                    //Depending on the value of the writing mode register,
                    //write as requested
                    switch (m_cfg->PixConfMode.read()){
                        case 0: {
                            //Write & bookkeep mask information, it comes in the 10 bits of payload as (p. 46 of the RD53C manual)
                            //last 10 bits of cmd.data = unused[9:8], right-pixel-mask[7:5], unused[4:3], left-pixel-mask[2:0].
                            //Keep in mind that the masking can be broadcasted to all core columns.
                            //If we're parallel-masking, we'll write all CCols (50) in one go, i. e. repeat the current pixel pair
                            //masking information with periodicity 4 double columns
                            int colsToWrite = m_cfg->PixBroadcast.read() ? 50 : 1;
                            for (int ccol = 0; ccol < colsToWrite; ccol++){
                                val = m_cfg->pixRegs[m_cfg->PixRegionCol.read() + ccol * 4][m_cfg->PixRegionRow.read()];
                                //left pixel:
                                val = (val & 0xF8FF) | ((cmd.data << 3) & 0x0700);
                                uint32_t coordinate = ((m_cfg->PixRegionCol.read() + ccol * 4) * 2) * 384 + m_cfg->PixRegionRow.read();
                                (val & 0x0100) ? (void)m_activePixels.insert(coordinate) : (void)m_activePixels.erase(coordinate);
                                //right pixel:
                                val = (val & 0xFFF8) | (cmd.data & 0x0007);
                                coordinate += 384;
                                (val & 0x0001) ? (void)m_activePixels.insert(coordinate) : (void)m_activePixels.erase(coordinate);
                            }
                            break;
                        }
                        case 1: {
                            //Write TDAC information
                            //last 10 bits of cmd.data = right-pixel-TDAC[9:5], left-pixel-TDAC[4:0]
                            //left pixel:
                            val = (val & 0x07FF) | ((cmd.data << 6) & 0xF800);
                            //right pixel:
                            val = (val & 0xFF07) | ((cmd.data << 3) & 0x00F8);
                            break;
                        }
                    }
                    break;
                }
            }
            //if AutoRow is enabled, increase the current row
            if (m_cfg->PixAutoRow.read()) m_cfg->PixRegionRow.write(m_cfg->PixRegionRow.read() + 1);
            break;
        }

        default:
            (*m_cfg)[cmd.address] = (cmd.data & 0xFFFF);
            break;
    }

}

void Itkpixv2EmuCommandExe::doRdReg(const Itkpixv2EmuUtils::Cmd& cmd){

    //Do the simplest case for now - single chip, single RdReg command
    //Form the 64-bit block and fill it

    //Create an empty service block
    uint64_t serviceBlock = 0ULL;

    //add aurora identifier, chip ID and status
    uint64_t auroraKWord = 0x99;
    serviceBlock |= (auroraKWord << 56);

    uint64_t id = m_cfg->getChipId() & 0x3;
    serviceBlock |= (id << 54);

    uint64_t status = 0b0;
    serviceBlock |= (status << 52);

    //There are two cases what value will be read - either a pixel reg through pixel portal
    //or a global register. Different values of the address field apply in each case.
    uint64_t addressField = 0ULL;
    uint64_t value = 0ULL;
    switch (cmd.address){
        case 0:
            //if we read pixel register, the address is 1 + 9bit row address
            addressField = 0x200 | m_cfg->PixRegionRow.read();
            value |= m_cfg->pixRegs[m_cfg->PixRegionCol.read()][m_cfg->PixRegionRow.read()];
            //if AutoRow is enabled, increase the current row
            if (m_cfg->PixAutoRow.read()) m_cfg->PixRegionRow.write(m_cfg->PixRegionRow.read() + 1);

        default:
            //if we read global register, the address is just that
            addressField = cmd.address;
            value |= m_cfg->operator[](cmd.address);
    }

    //add 10 bits of address and 16 bits of register value
    serviceBlock |= (addressField << 42);
    serviceBlock |= (value << 26);

    m_rx->write32(serviceBlock >> 32);
    m_rx->write32(serviceBlock & 0xFFFFFFFF);
    
}

void Itkpixv2EmuCommandExe::doTrigger(const Itkpixv2EmuUtils::Cmd& cmd){
    
    //We will likely want to implement some timing here.
    //For now, let's just encode and send as the trigger arrives.
    //Perhaps we could have the ToT actually buffered in
    //a PixelLayout<std::array<uint16_t, 4>>, and each received
    //triger command would operate on the length-4 array?

    for (uint8_t bc = 0; bc < 4; bc++){
        //the trigger pattern is ordered as MSB = first bc of the window
        if (Itkpixv2EmuUtils::lutTriggerPattern[cmd.header] & (0x1 << bc)){
            //The trigger command has to encode and send the data. We also
            //need to give the stream the correct header. That one is composed
            //of the tag-dependent base (LUT-ed) and the actual bc (T000 -> 00,
            //0T00 -> 01, 00T0 -> 10, 000T -> 11)
            
            uint8_t extendedTag = Itkpixv2EmuUtils::lutTriggerTagBase[cmd.id] << 2 | (bc & 0x3);
        
            //Encode. If the hit map is not empty, use the encoder. If it is
            //empty, send EoS + trigger header, bypassing the encoder. This saves
            //some processing time, as the encoder is still somewhat expensive.
            if (m_hitMapFilled){
                //Encode the hit map
                m_encoder->addToStream(m_tots, extendedTag);
            
                //Push whatever is currently in the encoder output. Then clear it.
                //This will always work for 1-event streams. We need to be careful
                //about the longer streams. How long does the DAQ wait?
                rlog->trace("Writing {} words to the input", m_encoder->getWords().size());
                m_rx->write32(m_encoder->getWords());
                m_encoder->getWords().clear();
                //Clear the ToT hit map
                m_tots.reset();
                m_hitMapFilled = false;  

            }
            else {
                //Form the EoS + trigger tag and send the empty event
                uint32_t emptyEvent = (1 << 31) | (0x00000000 & extendedTag << 23);
                m_rx->write32({emptyEvent, 0x00000000});
            }

        }
    }

}

void Itkpixv2EmuCommandExe::initPixels(const int seed){

    //We need to initialize all pixels with slightly
    //Randomized threshold to reflect real chip behaviour
    //The PixelLayout called m_thresholds will hold
    //a deviation from 1, where 1 would be exactly the desired
    //set threshold. For the time being, setting the deviation
    //to 10 %.
    
    m_rng = std::mt19937(seed);
    std::normal_distribution gauss(1., 0.10);

    for (uint col = 0; col < 400; col++){
        for (uint row = 0; row < 384; row++){
            m_thresholds(col, row) = gauss(m_rng);
        }
    }

}
