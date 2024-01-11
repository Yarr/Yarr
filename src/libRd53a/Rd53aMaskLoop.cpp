// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aMaskLoop.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53aMaskLoop");
}

//enum PixelCategories  {LeftEdge, BottomEdge, RightEdge, UpperEdge, Corner, Middle};
//enum CornerCategories {UpperLeft, BottomLeft, BottomRight, UpperRight, NotCorner};

enum MaskType {StandardMask =0 , CrossTalkMask=1,CrossTalkMaskv2 =2};

//Elements of mask_size
enum MaskSize {CrossTalkMask8=0, CrossTalkMask4=1, 
    CrossTalkMask2ud=2, CrossTalkMask2rl=3, 
    CrossTalkMask1u=4, CrossTalkMask1d=5,
    CrossTalkMask1r=6, CrossTalkMask1l=7,
    CrossTalkMask1ur=8, CrossTalkMask1ul=9,
    CrossTalkMask1dr=10, CrossTalkMask1dl=11};

//Switch for including/excluding edges pixels
enum IncludedPixels {includeEdges=0 ,removeEdgesFullSensor=1, removeEdgesSynFE=2, removeEdgesLinFE=3, removeEdgesDifFE=4,only00CornerForBumpBonding=5 };

//////////////////////////////////////////////////////////////////////////////
//The bump-bond in 25x100 micro-m^2 sensors can be done in two configurations:
//RecSensorUpDown: 
//the bump-bond at col=0, row=0 is associated with the pixel at the corner
//RecSensorDownUp: 
//the bump-bond at col=0, row=1 is associated with the pixel at the corner
//////////////////////////////////////////////////////////////////////////////
enum SensorType { SquareSensor=0, RecSensorUpDown=1, RecSensorDownUp=2 };


Rd53aMaskLoop::Rd53aMaskLoop() : LoopActionBase(LOOP_STYLE_MASK) {
    min = 0;
    max = 32;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    m_maskType = StandardMask ; //the alternative is crosstalk or crosstalkv2  

    //-----Parameter related to cross-talk-------------------
    m_maskSize = CrossTalkMask4; 
    m_sensorType = SquareSensor; //assume a 50x50 sensor
    m_includedPixels= includeEdges; //include all edges

    //Change of address for the neighbours:
    //50x50: Same behaviour for even and odd columns
    //Even
    AllNeighboursCoordinates["50x50"][0]= {{ std::make_pair(0,1),std::make_pair(1,1),std::make_pair(1,0),std::make_pair(1,-1), std::make_pair(0,-1), std::make_pair(-1,-1),std::make_pair(-1,0),std::make_pair(-1,1) }};
    //Odd
    AllNeighboursCoordinates["50x50"][1]={{ std::make_pair(0,1),std::make_pair(1,1),std::make_pair(1,0),std::make_pair(1,-1), std::make_pair(0,-1), std::make_pair(-1,-1),std::make_pair(-1,0),std::make_pair(-1,1) }}; 

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

void Rd53aMaskLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    m_cur = min;
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        auto rd53a = dynamic_cast<Rd53a*>(fe);
        // Make copy of pixRegs
        m_pixRegs[fe] = rd53a->pixRegs;
        g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        for(unsigned col=0; col<Rd53a::n_Col; col++) {
            for(unsigned row=0; row<Rd53a::n_Row; row++) {
                rd53a->setEn(col, row, 0);
                rd53a->setInjEn(col, row, 0);
            }
        }
        // TODO make configrue for subset
        rd53a->configurePixels();
        while(!g_tx->isCmdEmpty()) {}
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53aMaskLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");

    // Loop over FrontEnds
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        std::vector<std::pair<unsigned, unsigned>> modPixels;

        auto rd53a = dynamic_cast<Rd53a*>(fe);

        //Loop over all pixels - n_Col=400, n_Row=192 - from Rd53aPixelCfg.h
        for(unsigned col=0; col<Rd53a::n_Col; col++) {
            for(unsigned row=0; row<Rd53a::n_Row; row++) {
                if (applyMask(col,row)){
                    if (m_maskType == StandardMask){	       
                        //------------------------------------------------------------------------
                        //standard map, inj and read out the same pixel
                        //------------------------------------------------------------------------
                        rd53a->setEn(col, row, 1);
                        rd53a->setInjEn(col, row, 1);
                        modPixels.push_back(std::make_pair(col, row));
                    } 		
                    if (m_maskType == CrossTalkMask  ){	      
                        //---------------------------------------------------------------------------------
                        // std cross-talk scan, inj in surrounding pixels and read out the central one
                        //---------------------------------------------------------------------------------		    		  
                        std::vector<std::pair<int, int>> neighbours;		 
                        getNeighboursMap(col,row,m_sensorType, m_maskSize, neighbours);
                        //Read-only central pixel
                        rd53a->setInjEn(col, row, 0);		
                        rd53a->setEn(col, row, 1);
                        modPixels.push_back(std::make_pair(col, row));
                        //Inject only neighbours
                        for (auto n: neighbours){ 
                            rd53a->setInjEn(n.first, n.second,1);
                            rd53a->setEn(n.first, n.second, 0);
                            modPixels.push_back(std::make_pair(n.first, n.second));
                        }
                    }
                    else if (m_maskType == CrossTalkMaskv2 ){	      
                        //---------------------------------------------------------------------------------
                        // alternative cross-talk scan, inj central pixel, read out the surrounding ones
                        //--------------------------------------------------------------------------------		  
                        std::vector<std::pair<int, int>> neighbours;		 
                        getNeighboursMap(col,row, m_sensorType,m_maskSize,neighbours);
                        //Read-only central pixel
                        rd53a->setInjEn(col, row, 1);		
                        rd53a->setEn(col, row, 0);
                        modPixels.push_back(std::make_pair(col, row));
                        //Inject only neighbours
                        for (auto n: neighbours){ 		  	       
                            rd53a->setInjEn(n.first, n.second, 0);
                            rd53a->setEn(n.first, n.second, 1);
                            modPixels.push_back(std::make_pair(n.first, n.second));
                        }
                    }
                }//end applyMask
                else {
                    if (m_maskType == StandardMask){	       
                        //---------------------------------------------------------------------------------
                        // clean pixels for standardmask
                        //--------------------------------------------------------------------------------- 		  
                        if (rd53a->getInjEn(col, row) == 1) {
                            rd53a->setEn(col, row, 0);
                            rd53a->setInjEn(col, row, 0);
                            modPixels.push_back(std::make_pair(col, row));
                        }		
                    }
                }	     
            }//end row
        }//end column
        rd53a->configurePixels(modPixels);
        while(!g_tx->isCmdEmpty()) {}
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());
    g_stat->set(this, m_cur);
    logger->info(" ---> Mask Stage {}", m_cur);
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Rd53aMaskLoop::execPart2() {
    SPDLOG_LOGGER_TRACE(logger, "");

    // Loop over FrontEnds to clean it up
    if (m_maskType == CrossTalkMask or m_maskType == CrossTalkMaskv2 ){
        for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            auto fe = keeper->getFe(id);
            g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());

            auto rd53a = dynamic_cast<Rd53a*>(fe);
            std::vector<std::pair<unsigned, unsigned>> modPixels;
            for(unsigned col=0; col<Rd53a::n_Col; col++) {
                for(unsigned row=0; row<Rd53a::n_Row; row++) {
                    if (applyMask(col,row)){
                        std::vector<std::pair<int, int>> neighbours;		 
                        //switch off central pixel
                        rd53a->setInjEn(col, row, 0);		
                        rd53a->setEn(col, row, 0);
                        modPixels.push_back(std::make_pair(col, row));
                        //switch off neighbours
                        getNeighboursMap(col,row, m_sensorType,m_maskSize, neighbours);
                        for (auto n: neighbours){ 		  	       
                            rd53a->setInjEn(n.first, n.second, 0);
                            rd53a->setEn(n.first, n.second, 0);
                            modPixels.push_back(std::make_pair(n.first, n.second));
                        }
                    }
                }
            }	
            rd53a->configurePixels(modPixels);
            while(!g_tx->isCmdEmpty()) {}	
        }
    }

    m_cur += step;
    if (!((int)m_cur < max)) m_done = true;
    // Nothing else to do here?



}

void Rd53aMaskLoop::end() {
    SPDLOG_LOGGER_TRACE(logger, "");

    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        // Copy original registers back
        // TODO need to make sure analysis modifies the right config
        // TODO not thread safe
        dynamic_cast<Rd53a*>(fe)->pixRegs = m_pixRegs[fe];
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());

    // Nothing to do here?
}

void Rd53aMaskLoop::writeConfig(json &j) {
    j["min"] = min;
    j["max"] = max;
    j["step"] = step;
    j["maskType"] = m_maskType;
    j["maskSize"] = m_maskSize;
    j["sensorType"] = m_sensorType;
    j["includedPixels"] = m_includedPixels;

}

void Rd53aMaskLoop::loadConfig(const json &j) {
    if (j.contains("min"))
        min = j["min"];
    if (j.contains("max"))
        max = j["max"];
    if (j.contains("step"))
        step = j["step"];
    if (j.contains("maskType"))
        m_maskType = j["maskType"];
    if (j.contains("maskSize"))
        m_maskSize = j["maskSize"];
    if (j.contains("sensorType"))
        m_sensorType = j["sensorType"];
    if (j.contains("includedPixels"))
        m_includedPixels = j["includedPixels"];

}



bool Rd53aMaskLoop::getNeighboursMap(int col, int row,int sensorType, int maskSize,  std::vector<std::pair<int, int>>  &neighboursindex){

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
        if (static_cast<unsigned int>(col+shift_col) >(Rd53a::n_Col-1) || static_cast<unsigned int>(row+shift_row) >(Rd53a::n_Row-1)  ) {i++; continue;}
        if (static_cast<unsigned int>(col+shift_col) >(Rd53a::n_Col-1) || static_cast<unsigned int>(row+shift_row) >(Rd53a::n_Row-1)  ) {i++; continue;}
        neighboursindex.push_back(std::make_pair(col+shift_col, row+shift_row));
        i++;    
    }
    //std::cout<<std::endl;

    return filled;
}

//Return true if the pixel should be considered for this scan step, or false if it should be ignored
bool Rd53aMaskLoop::applyMask(int col, int row){


    //Do not run over edges pixels, if not explicity requested
    if (ignorePixel(col, row)) return false;

    unsigned core_row = row/8;
    unsigned serial = (core_row*64)+((col+(core_row%8))%8)*8+row%8;
    if ((serial%max) == m_cur){
        return true;
    }
    else
        return false;

}


bool Rd53aMaskLoop::ignorePixel(int col, int row) const{

    //if checking bump bonding connections for rectangular sensors, only use (0,0) pixel
    if ( m_includedPixels == only00CornerForBumpBonding){
        if (col==0 and row==0) return false;
        else return true;
    }

    //check if all pixels are requested:
    if (m_includedPixels == includeEdges) return false;

    //if not, remove the edges depending on the FE type
    if (m_sensorType==SquareSensor){
        if ((col==0 or col==Rd53a::n_Col-1) or (row==0 or row==Rd53a::n_Row-1)) return true;

        if (m_includedPixels==removeEdgesLinFE and (col<=128 or col>=263)) return true;

        if (m_includedPixels==removeEdgesSynFE and col>=127) return true;

        if (m_includedPixels==removeEdgesDifFE and col<=264) return true;
    }
    //Missing implementation for rectangular sensor
    if (m_sensorType!=SquareSensor){
        if (col==0 or col==1 or col==Rd53a::n_Col-1 or col==Rd53a::n_Col-2) return true;

        if (m_sensorType==RecSensorUpDown)
            if ( ( (col%2)==0 and row==0) or ( (col%2==1 and row==Rd53a::n_Row-1))) return true;

        if (m_sensorType==RecSensorDownUp)
            if ( ( (col%2)==1 and row==0) or ( (col%2==0 and row==Rd53a::n_Row-1))) return true;


        if (m_includedPixels==removeEdgesLinFE and (col<=129 or col>=262)) return true;

        if (m_includedPixels==removeEdgesSynFE and col>=126) return true;

        if (m_includedPixels==removeEdgesDifFE and col<=265) return true;
    }

    return false;
}
