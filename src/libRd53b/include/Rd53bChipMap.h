#ifndef RD53B_CHIPMAP_H
#define RD53B_CHIPMAP_H

#include <string>
#include <vector>
#include <tuple>
#include <fstream>
#include <sstream>
#include <random>


class Rd53bChipMap {
  
public: 
  //// This is the chip map constructor:
  //   - cols: number of columns in the chip
  //   - rows: number of rows in the chip
  //   - cols_core: number of columns in a core
  //   - rows_core: number of rows in a core
  //   THE FOLLOWING PARAMETERS ARE USED SINCE FOR ITK STEP3 
  //   QUAD CHIP MODULES ARE SIMULATED AS BIG SINGLE CHIP MODULES
  //   - n_colsbtwchips: number of active columns between chips on a quad
  //   - n_rowsbtwchips: number of active rows between chips on a quad
  
  Rd53bChipMap(int cols, int rows, 
          int cols_core, int rows_quar):
          m_cols(cols), m_rows(rows),
          m_cols_core(cols_core), m_rows_quar(rows_quar)
          {
            // SETTING ALL QUANTITIES
            m_ccols = m_cols/m_cols_core;
            m_qrows = m_rows/m_rows_quar;
            m_nfired_pixels = 0;
            m_is_filled = false;
            m_fired_pixels =  std::vector < std::vector < bool > >(m_cols, std::vector < bool > (m_rows, false) );
            m_tots =  std::vector < std::vector < int > >(m_cols, std::vector < int > (m_rows, 0) );
            m_nfired_pixels_in_region =  std::vector < std::vector < int > >(m_ccols, std::vector < int > (m_qrows, 0) );
            m_nfired_pixels_in_ccol =  std::vector<int>(m_ccols, 0);
          };

  ~Rd53bChipMap();
  
  void fillChipMap(int eta, int phi, int tot);
  void fillRegions();
  bool isFilled() {return m_is_filled;}
  int getFiredPixels() {return m_nfired_pixels;}
  int getFiredPixelsInCcol(int ccol) {return m_nfired_pixels_in_ccol.at(ccol);}
  int getCcols() {return m_ccols;}
  int getQrows() {return m_qrows;}
  int getCcolsRegion() {return m_cols_core;}
  int getQrowsRegion() {return m_rows_quar;}  

  int getRegionIndex(int eta, int phi);
  int getCcol(int eta);
  int getQrow(int phi);
  int getRegion(int ccol, int qrow);
  int getTotalChannels();
  std::string getBitTreeString(int myccol, int myqrow, bool do_compression=true);
  std::string getToTBitsString(int myccol, int myqrow, std::vector<int>& tots);
  void readMapFile(std::string inputMapFileName);
  void generateRndmEvent(const double occupancy, const int clusterSize, const int clusterOrientation);
  std::string isHalfFired(int min_eta, int max_eta, int min_phi, int max_phi);
  std::string getPlainHitMap(int myccol, int myqrow);
  void reset();
  void setSeed(int seed = 0){generator.seed(seed);}
private:
  int m_cols;
  int m_rows;
  int m_cols_core;
  int m_rows_quar;
  int m_ccols;
  int m_qrows;
  int m_nfired_pixels;
  bool m_is_filled;
  std::mt19937 generator;


  std::vector<std::vector<int> > m_nfired_pixels_in_region;
  std::vector<int> m_nfired_pixels_in_ccol;
  std::vector<std::vector<bool> > m_fired_pixels;
  std::vector<std::vector<int> >  m_tots;
  
  std::vector< std::tuple < int, int, int, int> > doSplit(std::vector< std::tuple < int, int, int, int> > extremes, 
                                                          std::string& new_result, bool do_compression, bool do_rightLeft = true);
};


#endif // RD53B_CHIPMAP_H
