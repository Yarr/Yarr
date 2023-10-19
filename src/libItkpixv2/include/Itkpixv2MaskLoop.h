#ifndef ITKPIXV2MASKLOOP_H
#define ITKPIXV2MASKLOOP_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Mask Loop for ITkPixV2
// # Date: 07/2023
// ################################

#include <vector>
#include <array>
#include <map>

#include "LoopActionBase.h"
#include "Itkpixv2.h"
#include "FrontEnd.h"

class Itkpixv2MaskLoop: public LoopActionBase {
    public:
        Itkpixv2MaskLoop();

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    private:
        unsigned m_cur;
        std::map<FrontEnd*, std::array<std::array<uint16_t, Itkpixv2::n_Row>, Itkpixv2::n_DC> > m_pixRegs;
        int m_maskType;
        bool m_applyEnMask;
        int m_maskSize;
        int m_sensorType;
        int m_includedPixels;

        //Needed for cross-talk mask
        std::map< std:: string,  std::array< std::array<   std::pair<int, int> , 8 >, 2>    > AllNeighboursCoordinates;

        std::array< std::array<int, 8>, 12> m_mask_size;

        bool getNeighboursMap(int col, int row, int sensorType, int maskSize, std::vector<std::pair<int, int>> &neighbours);
        bool ignorePixel(int col, int row);

        
        void init() override;
        void end() override;
        void execPart1() override;
        void execPart2() override;

        bool applyMask(unsigned col, unsigned row);

};
#endif
