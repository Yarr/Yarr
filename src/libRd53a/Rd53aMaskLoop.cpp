// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aMaskLoop.h"

enum PixelCategories  {LeftEdge, BottomEdge, RightEdge, UpperEdge, Corner, Middle};
enum CornerCategories {UpperLeft, BottomLeft, BottomRight, UpperRight, NotCorner};

Rd53aMaskLoop::Rd53aMaskLoop() : LoopActionBase() {
    min = 0;
    max = 32;
    step = 1;
    m_cur = 0;
    loopType = typeid(this);
    m_done = false;
    verbose = false;
    m_scanType = "std"; //the alternative is crosstalk or crosstalkv2   

    //Change in address mapping the 8 pixels around any pixel. {Col,Row} 
    Mask8x8={{ std::make_pair(0,1),
	       std::make_pair(1,1),
	       std::make_pair(1,0),
	       std::make_pair(1,-1),
	       std::make_pair(0,-1),
	       std::make_pair(-1,-1),
	       std::make_pair(-1,0),
	       std::make_pair(-1,1) }};

    //Special cases: edges and corner
    mask_edges   = {{{0,1,2,3,4},{2,3,4,5,6},{4,5,6,7,0},{6,7,0,1,2}}};
    mask_corners = {{{0,1,2},{2,3,4},{4,5,6},{6,7,0}}};
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
	      //------------------------------------------------------------------------
	      //standard map, inj and read out the same pixel
	      //------------------------------------------------------------------------
	      if (m_scanType == "std"){		
	        // Loop in terms of digital cores
                //unsigned core_col = col/8;
                unsigned core_row = row/8;
                // Serialise core column
                unsigned serial = (core_row*64)+(col%8)*8+row%8;
                if ((serial%max) == m_cur) {
		  dynamic_cast<Rd53a*>(fe)->setEn(col, row, 1);
		  dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 1);
		  modPixels.push_back(std::make_pair(col, row));
                } else {
		  if (dynamic_cast<Rd53a*>(fe)->getInjEn(col, row) == 1) {
		    dynamic_cast<Rd53a*>(fe)->setEn(col, row, 0);
		    dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 0);
		    modPixels.push_back(std::make_pair(col, row));
		  }		
                }
	      }
	      
	      //---------------------------------------------------------------------------------
	      // cross-talk scan
	      //---------------------------------------------------------------------------------
	      else if (m_scanType == "crosstalk" or m_scanType == "crosstalkv2" ){	      
		//This need to be changed
		unsigned core_row = row/8;
		// Serialise core column
		unsigned serial = (core_row*64)+(col%8)*8+row%8;
		std::vector<std::pair<int, int>> neighbours;		 
		getNeighboursMap(col,row, neighbours);
		if ((serial%max) == m_cur) {
		  //---------------------------------------------------------------------------------
		  // std cross-talk scan, inj in surrounding pixels and read out the central one
		  //---------------------------------------------------------------------------------
		  if (m_scanType == "crosstalk"  ){	      
		    //This vector has a variable lenght: 8 for pixels in the middle, 5 for edges, and 3 for corners
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
		  //---------------------------------------------------------------------------------
		  // alternative cross-talk scan, inj central pixel, read out the surrounding ones
		  //---------------------------------------------------------------------------------
		  else if (m_scanType == "crosstalkv2"  ){	      
		    //This vector has a variable lenght: 8 for pixels in the middle, 5 for edges, and 3 for corners
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
		}
	      }//end cross talk
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

    // Loop over FrontEnds
    if (m_scanType == "crosstalk" or m_scanType == "crosstalkv2" ){
      for(FrontEnd *fe : keeper->feList) {
	g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
	std::vector<std::pair<unsigned, unsigned>> modPixels;
	//Loop over all pixels - n_Col=400, n_Row=192 - from Rd53aPixelCfg.h
	for(unsigned col=0; col<Rd53a::n_Col; col++) {
	  for(unsigned row=0; row<Rd53a::n_Row; row++) {
	    if (m_scanType == "crosstalk"){	      
	      //This need to be changed
	      unsigned core_row = row/8;
	      // Serialise core column
	      unsigned serial = (core_row*64)+(col%8)*8+row%8;
	      std::vector<std::pair<int, int>> neighbours;		 
	      getNeighboursMap(col,row, neighbours);
	      if ((serial%max) == m_cur) {
		//This vector has a variable lenght: 8 for pixels in the middle, 5 for edges, and 3 for corners
		//Read-only central pixel
		dynamic_cast<Rd53a*>(fe)->setInjEn(col, row, 0);		
		dynamic_cast<Rd53a*>(fe)->setEn(col, row, 0);
		modPixels.push_back(std::make_pair(col, row));
		  //Inject only neighbours
		for (auto n: neighbours){ 		  	       
		  dynamic_cast<Rd53a*>(fe)->setInjEn(n.first, n.second, 0);
		  dynamic_cast<Rd53a*>(fe)->setEn(n.first, n.second, 0);
		  modPixels.push_back(std::make_pair(n.first, n.second));
		}
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
    j["scanType"] = m_scanType;
    
}

void Rd53aMaskLoop::loadConfig(json &j) {
    if (!j["min"].empty())
        min = j["min"];
    if (!j["max"].empty())
        max = j["max"];
    if (!j["step"].empty())
        step = j["step"];
    if (!j["scanType"].empty())
      m_scanType = j["scanType"];
}



bool Rd53aMaskLoop::getNeighboursMap(int col, int row,  std::vector<std::pair<int, int>>  &neighboursindex){

  bool filled=true;

  //In case it's a corner pixel
  if (IdentifyPixel (col,row)==Corner){
    
    int whichCorner = IdentifyCorner(col,row);
    for (int i : mask_corners[whichCorner] ){
      int shift_col=Mask8x8[i].first;
      int shift_row=Mask8x8[i].second;
      neighboursindex.push_back(std::make_pair(col+shift_col, row+shift_row));      
    }
  }
  //In case it's a edge pixel
  else if (IdentifyPixel (col,row)!=Corner && IdentifyPixel (col,row)!=Middle ){
    int whichEdge = IdentifyPixel(col,row);
    for (int i : mask_edges[whichEdge] ){
      int shift_col=Mask8x8[i].first;
      int shift_row=Mask8x8[i].second;
      neighboursindex.push_back(std::make_pair(col+shift_col, row+shift_row));
    }
  }
  //In case it's in the middle
  else if (IdentifyPixel (col,row)==Middle){
    for ( auto& p : Mask8x8 ){
      int shift_col=p.first;
      int shift_row=p.second;
      neighboursindex.push_back(std::make_pair(col+shift_col, row+shift_row));
    }
  }
  else 
    filled=false;
  
  return filled;
}
int Rd53aMaskLoop::IdentifyCorner(int col, int row){

   if (col==0 && row==0)
     return UpperLeft;
   else if (col==0 && row==(Rd53a::n_Row-1))
     return BottomLeft;  
   else if (col==(Rd53a::n_Col-1) && row==(Rd53a::n_Row-1))
     return BottomRight;
   else if (col==(Rd53a::n_Col-1) && row==0)
     return UpperRight;  
   else 
     return NotCorner;

}
int Rd53aMaskLoop::IdentifyPixel(int col, int row){

  if (IdentifyCorner(col,row)!=NotCorner)
    return Corner;  
  else if (col==0)
    return LeftEdge;
  else if (row==(Rd53a::n_Row)-1)
    return BottomEdge;
  else if (col==(Rd53a::n_Col-1))
    return RightEdge;
  else if (row==0)
    return UpperEdge;
  else 
    return Middle;   
}
