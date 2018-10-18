#ifndef RD53AMASKLOOP_H
#define RD53AMASKLOOP_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for RD53A
// # Date: 03/2018
// ################################

#include <iostream>
#include <vector>
#include <tuple>
#include <array>
#include <string>
#include <utility>


#include "FrontEnd.h"
#include "Rd53a.h"
#include "LoopActionBase.h"

class Rd53aMaskLoop : public LoopActionBase {
    public:
        Rd53aMaskLoop();

        void writeConfig(json &j);
        void loadConfig(json &j);
    private:
        unsigned m_cur;
	int m_maskType;
	int m_maskSize;
	int m_sensorType;
	int m_includedPixels;

        void init();
        void end();
        void execPart1();
        void execPart2();

	
	bool ApplyMask(int col, int row);
        
	std::map<FrontEnd*, std::array<uint16_t, Rd53a::n_DC*Rd53a::n_Row>> m_pixRegs;
	
	//Needed for cross-talk mask
	std::map< std:: string,  std::array< std::array<   std::pair<int, int> , 8 >, 2>    > AllNeighboursCoordinates;

	std::array< std::array<int, 8>, 8> m_mask_size;
		
	
	bool getNeighboursMap(int col, int row, int sensorType, int maskSize, std::vector<std::pair<int, int>> &neighbours);

	//int IdentifyCorner(int col, int row);
	//int IdentifyPixel(int col, int row);

	bool IgnorePixel(int col, int row);
	
};

#endif
