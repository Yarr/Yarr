// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aMaskLoop.h"

//enum PixelCategories  {LeftEdge, BottomEdge, RightEdge, UpperEdge, Corner, Middle};
//enum CornerCategories {UpperLeft, BottomLeft, BottomRight, UpperRight, NotCorner};

enum MaskType {StandardMask =0 , CrossTalkMask=1,CrossTalkMaskv2 =2};

//Elements of mask_size
enum MaskSize {CrossTalkMask8=0, CrossTalkMask4=1, 
	       CrossTalkMask2ud=2, CrossTalkMask2rl=3, 
	       CrossTalkMask1u=4, CrossTalkMask1d=5,
	       CrossTalkMask1r=6, CrossTalkMask1l=7};

enum SensorType { SquareSensor=0, RecSensorUpDown=1, RecSensorDownUp=2 };

Rd53aMaskLoop::Rd53aMaskLoop() : LoopActionBase() {
    min = 0;
    max = 32;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    verbose = false;
    m_maskType = StandardMask ; //the alternative is crosstalk or crosstalkv2  

    //-----Parameter related to cross-talk-------------------
    m_maskSize = CrossTalkMask4; 
    m_sensorType = SquareSensor; //assume a 50x50 sensor

    //Change of address for the neighbours:
    //50x50: Same behaviour for even and odd columns
    //Even
    AllNeighboursCoordinates["50x50"][0]= {{ std::make_pair(0,1),std::make_pair(1,1),std::make_pair(1,0),std::make_pair(1,-1), std::make_pair(0,-1), std::make_pair(-1,-1),std::make_pair(-1,0),std::make_pair(-1,1) }};
    //Odd
    AllNeighboursCoordinates["50x50"][1]={{ std::make_pair(0,1),std::make_pair(1,1),std::make_pair(1,0),std::make_pair(1,-1), std::make_pair(0,-1), std::make_pair(-1,-1),std::make_pair(-1,0),std::make_pair(-1,1) }}; 
    
    //25x100, up down -RecSensorUpDown
    //Even
    AllNeighboursCoordinates["25x100_updown"][0] ={{std::make_pair(1,0),std::make_pair(3,0),std::make_pair(2,0),std::make_pair(3,1),std::make_pair(1,-1),std::make_pair(-1,-1),std::make_pair(-2,0),std::make_pair(-1,0)}}; 
    //Down
    AllNeighboursCoordinates["25x100_updown"][1] ={{std::make_pair(-1,1),std::make_pair(1,1),std::make_pair(2,0),std::make_pair(1,0),std::make_pair(-1,0),std::make_pair(-3,0),std::make_pair(-2,0),std::make_pair(-3,1)}}; 

    //25x100, down up -RecSensorDownUp
    //Even
    AllNeighboursCoordinates["25x100_downup"][0] ={{std::make_pair(1,1),std::make_pair(3,1),std::make_pair(2,0),std::make_pair(3,0),std::make_pair(1,0),std::make_pair(-1,0),std::make_pair(-2,0),std::make_pair(-1,-1)}};
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
      }}; 

}

void Rd53aMaskLoop::init() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_done = false;
    m_cur = 0;
    for(FrontEnd *fe : keeper->feList) {
        // Make copy of pixRegs
        m_pixRegs[fe] = dynamic_cast<Rd53a*>(fe)->pixRegs;
        g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        for(unsigned col=0; col<Rd53a::n_Col; col++) {
            for(unsigned row=0; row<Rd53a::n_Row; row++) {
                dynamic_cast<Rd53a*>(fe)->setEn(col, row, 0);
                dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 0);
            }
        }
        // TODO make configrue for subset
        dynamic_cast<Rd53a*>(fe)->configurePixels();
        while(!g_tx->isCmdEmpty()) {}
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());
}

void Rd53aMaskLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    // Loop over FrontEnds
    for(FrontEnd *fe : keeper->feList) {
        g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        std::vector<std::pair<unsigned, unsigned>> modPixels;
	//Loop over all pixels - n_Col=400, n_Row=192 - from Rd53aPixelCfg.h
        for(unsigned col=0; col<Rd53a::n_Col; col++) {
            for(unsigned row=0; row<Rd53a::n_Row; row++) {
	      if (ApplyMask(col,row)){
		if (m_maskType == StandardMask){	       
		//------------------------------------------------------------------------
		//standard map, inj and read out the same pixel
		//------------------------------------------------------------------------
		  dynamic_cast<Rd53a*>(fe)->setEn(col, row, 1);
		  dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 1);
		  modPixels.push_back(std::make_pair(col, row));
		} 		
		if (m_maskType == CrossTalkMask  ){	      
		  //---------------------------------------------------------------------------------
		  // std cross-talk scan, inj in surrounding pixels and read out the central one
		  //---------------------------------------------------------------------------------		    
		  std::vector<std::pair<int, int>> neighbours;		 
		  getNeighboursMap(col,row,m_sensorType, m_maskSize, neighbours);
		  //Read-only central pixel
		  dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 0);		
		  dynamic_cast<Rd53a*>(fe)->setEn(col, row, 1);
		  modPixels.push_back(std::make_pair(col, row));
		  //Inject only neighbours
		  for (auto n: neighbours){ 
		    dynamic_cast<Rd53a*>(fe)->setInjEn(n.first, n.second, 1);
		    dynamic_cast<Rd53a*>(fe)->setEn(n.first, n.second, 0);
		    modPixels.push_back(std::make_pair(n.first, n.second));
		  }
		}
		else if (m_maskType == CrossTalkMaskv2 ){	      
		  //---------------------------------------------------------------------------------
		  // alternative cross-talk scan, inj central pixel, read out the surrounding ones
		  //---------------------------------------------------------------------------------
		  std::vector<std::pair<int, int>> neighbours;		 
		  getNeighboursMap(col,row, m_sensorType,m_maskSize,neighbours);
		  //Read-only central pixel
		  dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 1);		
		  dynamic_cast<Rd53a*>(fe)->setEn(col, row, 0);
		  modPixels.push_back(std::make_pair(col, row));
		  //Inject only neighbours
		    for (auto n: neighbours){ 		  	       
		      dynamic_cast<Rd53a*>(fe)->setInjEn(n.first, n.second, 0);
		      dynamic_cast<Rd53a*>(fe)->setEn(n.first, n.second, 1);
		      modPixels.push_back(std::make_pair(n.first, n.second));
		    }
		}
	      }//end ApplyMask
	      else {
		if (m_maskType == StandardMask){	       
		  //---------------------------------------------------------------------------------
		  // clean pixels for standardmask
		  //---------------------------------------------------------------------------------		  
		  if (dynamic_cast<Rd53a*>(fe)->getInjEn(col, row) == 1) {
		    dynamic_cast<Rd53a*>(fe)->setEn(col, row, 0);
		    dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 0);
		    modPixels.push_back(std::make_pair(col, row));
		  }		
                }
	      }	     
	    }//end row
	}//end column
	
	// TODO make configrue for subset
        // TODO set cmeEnable correctly
        dynamic_cast<Rd53a*>(fe)->configurePixels(modPixels);
        while(!g_tx->isCmdEmpty()) {}
    }
    // Reset CMD mask
    g_tx->setCmdEnable(keeper->getTxMask());
    g_stat->set(this, m_cur);
    std::cout << " ---> Mask Stage " << m_cur << std::endl;
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Rd53aMaskLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

    // Loop over FrontEnds to clean it up
    if (m_maskType == CrossTalkMask or m_maskType == CrossTalkMaskv2 ){
      for(FrontEnd *fe : keeper->feList) {
	g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
	std::vector<std::pair<unsigned, unsigned>> modPixels;
	for(unsigned col=0; col<Rd53a::n_Col; col++) {
	  for(unsigned row=0; row<Rd53a::n_Row; row++) {
	    if (ApplyMask(col,row)){
	      std::vector<std::pair<int, int>> neighbours;		 
	      //switch off central pixel
	      dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 0);		
	      dynamic_cast<Rd53a*>(fe)->setEn(col, row, 0);
	      modPixels.push_back(std::make_pair(col, row));
	      //switch off neighbours
	      getNeighboursMap(col,row, m_sensorType,m_maskSize, neighbours);
	      for (auto n: neighbours){ 		  	       
		dynamic_cast<Rd53a*>(fe)->setInjEn(n.first, n.second, 0);
		dynamic_cast<Rd53a*>(fe)->setEn(n.first, n.second, 0);
		modPixels.push_back(std::make_pair(n.first, n.second));
	      }
	    }
	  }
	}	
	// TODO make configrue for subset
	// TODO set cmeEnable correctly
	dynamic_cast<Rd53a*>(fe)->configurePixels(modPixels);
	while(!g_tx->isCmdEmpty()) {}	
      }
    }
    
    m_cur += step;
    if (!((int)m_cur < max)) m_done = true;
    // Nothing else to do here?
    
    
    
}

void Rd53aMaskLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    for(FrontEnd *fe : keeper->feList) {
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
    
}

void Rd53aMaskLoop::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
    if (!j["maskType"].empty())
      m_maskType = j["maskType"];
    if (!j["maskSize"].empty())
      m_maskSize = j["maskSize"];
    if (!j["sensorType"].empty())
      m_sensorType = j["sensorType"];

}



bool Rd53aMaskLoop::getNeighboursMap(int col, int row,int sensorType, int maskSize,  std::vector<std::pair<int, int>>  &neighboursindex){

  bool filled=true;

  std::string sensor;
  if (sensorType==SquareSensor)
    sensor="50x50";
  else if (sensorType==RecSensorUpDown)
    sensor="25x100_updown";
  else if (sensorType==RecSensorUpDown)
    sensor="25x100_updown";
  else
    std::cout<<"ERROR: sensor Type does not exist"<<std::endl;


  int i=0;
  for ( auto& p : AllNeighboursCoordinates[sensor][col%2]){
    if (m_mask_size[maskSize][i]==0) { i++; continue;}
    
    int shift_col=p.first;
    int shift_row=p.second;
    //Make sure that the pixel is within the sensor
    if ( row+shift_row<0  || col+shift_col<0 )  continue;
    if (static_cast<unsigned int>(col+shift_col) >(Rd53a::n_Col-1) || static_cast<unsigned int>(row+shift_row) >(Rd53a::n_Row-1)  )  continue;
    neighboursindex.push_back(std::make_pair(col+shift_col, row+shift_row));
    i++;    
  }
  
  return filled;
}

//Return true if the pixel should be considered for this scan step, or false if it should be ignored
bool Rd53aMaskLoop::ApplyMask(int col, int row){

  unsigned core_row = row/8;
  unsigned serial = (core_row*64)+((col+(core_row%8))%8)*8+row%8;
  //unsigned serial = (core_row*64)+(col%8)*8+row%8;
  if ((serial%max) == m_cur)
    return true;
  else
    return false;
  
}
