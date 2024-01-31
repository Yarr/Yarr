// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for ITkPixV2
// # Date: 07/2023
// ################################

#include "Itkpixv2MaskLoop.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Itkpixv2MaskLoop");
}

enum MaskType {StandardMask = 0, CrossTalkMask = 1, CrossTalkMaskv2 = 2, PToTMask = 3};

//Switch between different injection patters for cross-talk scan
enum MaskSize {CrossTalkMask8=0, CrossTalkMask4=1, 
    CrossTalkMask2ud=2, CrossTalkMask2rl=3, 
    CrossTalkMask1u=4, CrossTalkMask1d=5,
    CrossTalkMask1r=6, CrossTalkMask1l=7,
    CrossTalkMask1ur=8, CrossTalkMask1ul=9,
    CrossTalkMask1dr=10, CrossTalkMask1dl=11};

//Switch for including/excluding edges pixels
enum IncludedPixels {includeEdges=0 ,removeEdgesFullSensor=1,only00CornerForBumpBonding=2 };

//////////////////////////////////////////////////////////////////////////////
//The bump-bond in 25x100 um^2 sensors can be done in two configurations:
//RecSensorUpDown: 
//the bump-bond at col=0, row=0 is associated with the pixel at the corner
//RecSensorDownUp: 
//the bump-bond at col=0, row=1 is associated with the pixel at the corner
//////////////////////////////////////////////////////////////////////////////
enum SensorType { SquareSensor=0, RecSensorUpDown=1, RecSensorDownUp=2 };



Itkpixv2MaskLoop::Itkpixv2MaskLoop() : LoopActionBase(LOOP_STYLE_MASK) {
    min = 0;
    max = 64;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    m_maskType = StandardMask;
    m_applyEnMask = false;

    //-----Parameter related to cross-talk-------------------
    m_maskSize = CrossTalkMask4; //default is injection in the 4 neighboring pixels 
    m_sensorType = SquareSensor; //assume a 50x50 sensor
    m_includedPixels= includeEdges; //include all pixels along the edges

    //Change of address for the neighbouring pixels. Coordinates are in the form [col, row]
    //50x50: Same behaviour for even and odd columns
    //Even
    AllNeighboursCoordinates["50x50"][0]= {{ std::make_pair(0,1),std::make_pair(1,1),std::make_pair(1,0),std::make_pair(1,-1), std::make_pair(0,-1), std::make_pair(-1,-1),std::make_pair(-1,0),std::make_pair(-1,1) }};
    //Odd
    AllNeighboursCoordinates["50x50"][1]=AllNeighboursCoordinates["50x50"][0];

    //25x100, up down -RecSensorUpDown
    //Even
    AllNeighboursCoordinates["25x100_updown"][0] ={{std::make_pair(1,0),std::make_pair(3,0),std::make_pair(2,0),std::make_pair(3,-1),std::make_pair(1,-1),std::make_pair(-1,-1),std::make_pair(-2,0),std::make_pair(-1,0)}}; 
    //Down
    AllNeighboursCoordinates["25x100_updown"][1] ={{std::make_pair(-1,1),std::make_pair(1,1),std::make_pair(2,0),std::make_pair(1,0),std::make_pair(-1,0),std::make_pair(-3,0),std::make_pair(-2,0),std::make_pair(-3,1)}}; 

    //25x100, down up -RecSensorDownUp
    //Even
    AllNeighboursCoordinates["25x100_downup"][0] ={{std::make_pair(1,1),std::make_pair(3,1),std::make_pair(2,0),std::make_pair(3,0),std::make_pair(1,0),std::make_pair(-1,0),std::make_pair(-2,0),std::make_pair(-1,1)}};
    //Odd
    AllNeighboursCoordinates["25x100_downup"][0] ={{std::make_pair(-1,0),std::make_pair(1,0),std::make_pair(2,0),std::make_pair(1,-1),std::make_pair(-1,-1),std::make_pair(-3,-1),std::make_pair(-2,0),std::make_pair(-3,0)}};

    //How many and which pixels to consider for cross-talk (1: consider, 0: ignore)
    m_mask_size = {{
        {1,1,1,1,1,1,1,1},         //8
            {1,0,1,0,1,0,1,0},         //4
            {1,0,0,0,1,0,0,0},         //2up-down
            {0,0,1,0,0,0,1,0},         //2right-left
            {0,0,0,0,1,0,0,0},         //1up 
            {1,0,0,0,0,0,0,0},         //1down
            {0,0,1,0,0,0,0,0},	   //1right
            {0,0,0,0,0,0,1,0},         //1left    
            {0,0,0,1,0,0,0,0},         //1up-right    
            {0,0,0,0,0,1,0,0},         //1up-left    
            {0,1,0,0,0,0,0,0},         //1down-right    
            {0,0,0,0,0,0,0,1},         //1down-left    
    }}; 


}

void Itkpixv2MaskLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = min;


    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        Itkpixv2 *itkpixv2 = dynamic_cast<Itkpixv2*>(fe);
        g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        // Save current version of the pix regs to transferred back into the config at the end
        m_pixRegs[fe] = itkpixv2->pixRegs;
        // Turn off all pixels to start with
        for (unsigned col=0; col<Itkpixv2::n_Col; col++) {
            for (unsigned row=0; row<Itkpixv2::n_Row; row++) {
                itkpixv2->setEn(col, row, 0); // TODO make configurable
                itkpixv2->setInjEn(col, row, 0);
                itkpixv2->setHitbus(col, row, 0);
            }
        }
        itkpixv2->configurePixels();
        while(!g_tx->isCmdEmpty()) {}
    }
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Itkpixv2MaskLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");

    unsigned counter = 0;
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        std::vector<std::pair<unsigned, unsigned>> modPixels;

        Itkpixv2 *itkpixv2 = dynamic_cast<Itkpixv2*>(fe);

        for(unsigned col=0; col<Itkpixv2::n_Col; col++) {
            for(unsigned row=0; row<Itkpixv2::n_Row; row++) {
                //Disable pixels of last mask stage if they were injected to 
                if (itkpixv2->getInjEn(col, row) == 1) {
                    //logger->info("Disabling {};{}", col, row);
                    itkpixv2->setEn(col, row, 0); // TODO make configurable
                    itkpixv2->setInjEn(col, row, 0);
                    itkpixv2->setHitbus(col, row, 0);
                    modPixels.push_back(std::make_pair(col, row));
                }
            }
        }

        for(unsigned col=0; col<Itkpixv2::n_Col; col++) {
            for(unsigned row=0; row<Itkpixv2::n_Row; row++) {
                //Disable pixels of last mask stage if they were injected to 
                // Enable pixels of current mask stage
                if (applyMask(col,row)){
                    // If the pixel is disabled, skip it
                    if(m_applyEnMask && !Itkpixv2::getPixelBit(m_pixRegs[fe], col, row, 0)) continue;

                    //---------------------------------------------------------------------------------
                    // std cross-talk scan, inj in surrounding pixels and read out the central one
                    //---------------------------------------------------------------------------------		    		  
                    if (m_maskType == CrossTalkMask  ){	      
                        std::vector<std::pair<int, int>> neighbours;		 
                        getNeighboursMap(col,row,m_sensorType, m_maskSize, neighbours);
                        //Read-only central pixel
                        //logger->info("Enabling {} {}", col, row);
                        itkpixv2->setEn(col, row, 1);
                        itkpixv2->setInjEn(col, row, 0);		
                        itkpixv2->setHitbus(col, row, 1);
                        modPixels.push_back(std::make_pair(col, row));
                        counter++;
                        //Inject only neighbours
                        for (auto n: neighbours){ 
                            //logger->info("Inject in {} {}", n.first, n.second);
                            itkpixv2->setEn(n.first, n.second, 0);
                            itkpixv2->setInjEn(n.first, n.second,1);
                            itkpixv2->setHitbus(n.first, n.second, 0);
                            modPixels.push_back(std::make_pair(n.first, n.second));
                        }
                    }
                    //---------------------------------------------------------------------------------
                    // alternative cross-talk scan, inj central pixel, read out the surrounding ones
                    //--------------------------------------------------------------------------------		  
                    else if (m_maskType == CrossTalkMaskv2 ){	      
                        std::vector<std::pair<int, int>> neighbours;		 
                        getNeighboursMap(col,row, m_sensorType,m_maskSize,neighbours);
                        //Inject-only central pixel
                        itkpixv2->setEn(col, row, 0);
                        itkpixv2->setInjEn(col, row, 1);	
                        itkpixv2->setHitbus(col, row, 0);	
                        modPixels.push_back(std::make_pair(col, row));
                        counter++;
                        //Read out neighbours
                        for (auto n: neighbours){ 		  	       
                            itkpixv2->setInjEn(n.first, n.second, 0);
                            itkpixv2->setEn(n.first, n.second, 1);
                            itkpixv2->setHitbus(col, row, 1);	
                            modPixels.push_back(std::make_pair(n.first, n.second));
                        }
                    }
                    else{
                        //------------------------------------------------------------------------
                        //standard map, inj and read out the same pixel
                        //------------------------------------------------------------------------
                        //logger->info("Enabling {};{}", col, row);
                        itkpixv2->setEn(col, row, (m_maskType == PToTMask) ? 0 : 1); // TODO Make configurable
                        itkpixv2->setInjEn(col, row, 1);
                        itkpixv2->setHitbus(col, row, 1);
                        modPixels.push_back(std::make_pair(col, row));
                        counter++;		    
                    }
                }
            }
        }
        //logger->info(" ---> Active pixels ");
        //for (auto n: modPixels)
        //  logger->info("{} {}",n.first,n.second);
        itkpixv2->configurePixels(modPixels);
        while(!g_tx->isCmdEmpty()) {}
    }

    g_tx->setCmdEnable(keeper->getTxMask());
    g_stat->set(this, m_cur);
    logger->info(" ---> Mask Stage {} (Activated {} pixels)", m_cur, counter);
}

void Itkpixv2MaskLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");


    // Loop over FrontEnds to clean it up
    if (m_maskType == CrossTalkMask or m_maskType == CrossTalkMaskv2 ){

        for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            auto fe = keeper->getFe(id);
            g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());

            Itkpixv2* itkpixv2 = dynamic_cast<Itkpixv2*>(fe);
            std::vector<std::pair<unsigned, unsigned>> modPixels;
            for(unsigned col=0; col<Itkpixv2::n_Col; col++) {
                for(unsigned row=0; row<Itkpixv2::n_Row; row++) {
                    if (applyMask(col,row)){
                        std::vector<std::pair<int, int>> neighbours;		 
                        //switch off central pixel
                        itkpixv2->setInjEn(col, row, 0);		
                        itkpixv2->setEn(col, row, 0);
                        itkpixv2->setHitbus(col, row, 0);
                        modPixels.push_back(std::make_pair(col, row));
                        //switch off neighbours
                        getNeighboursMap(col,row, m_sensorType,m_maskSize, neighbours);
                        for (auto n: neighbours){ 		  	       
                            itkpixv2->setInjEn(n.first, n.second, 0);
                            itkpixv2->setEn(n.first, n.second, 0);
                            itkpixv2->setHitbus(n.first, n.second, 0);
                            modPixels.push_back(std::make_pair(n.first, n.second));
                        }
                    }
                }
            }	
            itkpixv2->configurePixels(modPixels);
            while(!g_tx->isCmdEmpty()) {}	
        }
    }

    m_cur += step;
    if (!((int)m_cur < max)) m_done = true;
}

void Itkpixv2MaskLoop::end() {
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        // Copy original registers back
        // TODO need to make sure analysis modifies the right config
        // TODO not thread safe, in case analysis modifies them to early
        dynamic_cast<Itkpixv2*>(fe)->pixRegs = m_pixRegs[fe];
    }
}

bool Itkpixv2MaskLoop::applyMask(unsigned col, unsigned row) {

    //Do not run over edges pixels, if not explicity requested  
    if (ignorePixel(col, row)) return false;

    // This is the mask pattern
    unsigned core_row = row/8;
    unsigned serial;
    if (m_maskType == PToTMask) {
        serial = row * 2 + (col % 8)/4;
    } else {
        serial = (core_row*64)+((col+(core_row%8))%8)*8+row%8;
    }
    //unsigned serial = (col%8*Itkpixv2::n_Row)+row;
    if ((serial%max) == m_cur){
        return true;
    }
    return false;
}

void Itkpixv2MaskLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["maskType"] = m_maskType;
    j["applyEnMask"] = m_applyEnMask;
    j["maskSize"] = m_maskSize;
    j["sensorType"] = m_sensorType;
    j["includedPixels"] = m_includedPixels;
}

void Itkpixv2MaskLoop::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("step"))
        step = j["step"];
    if (j.contains("maskType"))
        m_maskType = j["maskType"];
    if (j.contains("applyEnMask"))
        m_applyEnMask = j["applyEnMask"];    
    if (j.contains("maskSize"))
        m_maskSize = j["maskSize"];
    if (j.contains("sensorType"))
        m_sensorType = j["sensorType"];
    if (j.contains("includedPixels"))
        m_includedPixels = j["includedPixels"];
}


bool Itkpixv2MaskLoop::getNeighboursMap(int col, int row,int sensorType, int maskSize,  std::vector<std::pair<int, int>>  &neighboursindex){

    bool filled=true;

    std::string sensor;
    if (sensorType==SquareSensor)
        sensor="50x50";
    else if (sensorType==RecSensorDownUp)
        sensor="25x100_downup";
    else if (sensorType==RecSensorUpDown)
        sensor="25x100_updown";
    else
        logger->error("Sensor Type does not exist");

    //if (row==8)
    // std::cout<<"-- Central pixel:"<< col<<" "<< row<<std::endl;

    int i=0;
    for ( auto& p : AllNeighboursCoordinates[sensor][col%2]){


        if (m_mask_size[maskSize][i]==0) { i++; continue;}

        int shift_col=p.first;
        int shift_row=p.second;
        //if (row==8)
        //  std::cout<<i<<"  ("<<shift_row<<" "<<shift_col<<") ";

        //Make sure that the pixel is within the sensor
        if ( row+shift_row<0  || col+shift_col<0 ) {i++;  continue;}
        if (static_cast<unsigned int>(col+shift_col) >(Itkpixv2::n_Col-1) || static_cast<unsigned int>(row+shift_row) >(Itkpixv2::n_Row-1)  ) {i++; continue;}
        if (static_cast<unsigned int>(col+shift_col) >(Itkpixv2::n_Col-1) || static_cast<unsigned int>(row+shift_row) >(Itkpixv2::n_Row-1)  ) {i++; continue;}
        neighboursindex.push_back(std::make_pair(col+shift_col, row+shift_row));
        i++;    
    }
    //std::cout<<std::endl;

    return filled;
}


bool Itkpixv2MaskLoop::ignorePixel(int col, int row){

    //if checking bump bonding connections for rectangular sensors, only use (0,0) pixel
    if ( m_includedPixels == only00CornerForBumpBonding){
        if (col==0 and row==0) return false;
        else return true;
    }

    //check if all pixels are requested:
    if (m_includedPixels == includeEdges) return false;

    //if not, remove the edges == TO BE CHECKED
    if (m_sensorType==SquareSensor){
        if ((col==0 or col==Itkpixv2::n_Col-1) or (row==0 or row==Itkpixv2::n_Row-1)) return true;    
    }
    if (m_sensorType!=SquareSensor){
        if (col==0 or col==1 or col==Itkpixv2::n_Col-1 or col==Itkpixv2::n_Col-2) return true;

        if (m_sensorType==RecSensorUpDown)
            if ( ( (col%2)==0 and row==0) or ( (col%2==1 and row==Itkpixv2::n_Row-1))) return true;

        if (m_sensorType==RecSensorDownUp)
            if ( ( (col%2)==1 and row==0) or ( (col%2==0 and row==Itkpixv2::n_Row-1))) return true;
    }

    return false;
}
