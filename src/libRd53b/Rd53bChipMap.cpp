#include "Rd53bChipMap.h"

#include <math.h>
#include <bitset>
#include <tuple>
#include <random>
#include <iostream> 
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */

Rd53bChipMap::~Rd53bChipMap(){
}

void Rd53bChipMap::reset(){
  //just reproduce the constructor functionality, so that we don't have to
  //create new instance for each generated event
  m_nfired_pixels = 0;
  m_is_filled = false;
  m_fired_pixels =  std::vector < std::vector < bool > >(m_cols, std::vector < bool > (m_rows, false) );
  m_tots =  std::vector < std::vector < int > >(m_cols, std::vector < int > (m_rows, 0) );
  m_nfired_pixels_in_region =  std::vector < std::vector < int > >(m_ccols, std::vector < int > (m_qrows, 0) );
  m_nfired_pixels_in_ccol =  std::vector<int>(m_ccols, 0);
}

void Rd53bChipMap::fillChipMap(int eta, int phi, int tot) {
  if(m_fired_pixels.at(eta).at(phi)) return;
  m_fired_pixels.at(eta).at(phi) = true;
  m_tots.at(eta).at(phi) = std::min(tot, 15);
  m_nfired_pixels++;
}

int Rd53bChipMap::getRegionIndex(int eta, int phi) {

  int myccol = getCcol(eta);
  int myqrow = getQrow(phi);
  
  return getRegion(myccol, myqrow);  
}

int Rd53bChipMap::getCcol(int eta) {
  return eta/m_cols_core;
}

int Rd53bChipMap::getQrow(int phi) {
  return phi/m_rows_quar;
}

int Rd53bChipMap::getRegion(int ccol, int qrow) {
  return (qrow*m_ccols+ccol);
}

int Rd53bChipMap::getTotalChannels() {
  return m_cols*m_rows; 
}

void Rd53bChipMap::fillRegions() {
  if (m_is_filled) return;  
  for (unsigned int eta = 0; eta < m_cols; eta++) {
    for (unsigned int phi = 0; phi < m_rows; phi++) {
      if (not m_fired_pixels.at(eta).at(phi)) continue;
      int myccol = getCcol(eta);
      int myqrow = getQrow(phi);
      m_nfired_pixels_in_region.at(myccol).at(myqrow)++;
      m_nfired_pixels_in_ccol.at(myccol)++;
    }
  }


  m_is_filled = true;    
}

std::string Rd53bChipMap::isHalfFired(int min_eta, int max_eta, int min_phi, int max_phi) {
  for (int eta = min_eta; eta<= max_eta; eta++) {
    for (int phi = min_phi; phi<= max_phi; phi++) {
      if (m_fired_pixels.at(eta).at(phi)) return "1";
    }
  }
  return "0";
}

std::string Rd53bChipMap::getBitTreeString(int myccol, int myqrow, bool do_compression) {
  std::string result = "";

  if (m_nfired_pixels_in_region.at(myccol).at(myqrow) == 0)
    return result;

  int min_eta = myccol*m_cols_core; 
  int max_eta = (myccol+1)*m_cols_core-1;

  int min_phi = myqrow*m_rows_quar; 
  int max_phi = (myqrow+1)*m_rows_quar-1;

  std::vector< std::tuple < int, int, int, int> > good_extremes = {std::make_tuple(min_eta,max_eta, min_phi, max_phi)};

  // First generate the first two bit pair
  std::string new_result = "";
  good_extremes = doSplit(good_extremes, new_result, do_compression, false);
  result+=new_result;
  // the bit tree works in 3 steps for each row, each time split right/left
  for(unsigned i = 0; i < good_extremes.size(); i++){
    std::vector< std::tuple < int, int, int, int> > row_extremes;
    row_extremes.push_back(good_extremes[i]);
    
    for(int loop = 0; loop < 3; loop++){
      std::string new_result = "";
      row_extremes = doSplit(row_extremes, new_result, do_compression, true);   
      result+=new_result;
    }
  }

  return result;
}

std::string Rd53bChipMap::getToTBitsString(int myccol, int myqrow, std::vector<int>& tots) {
  std::string result = "";

  if (m_nfired_pixels_in_region.at(myccol).at(myqrow) == 0)
    return result;

  int min_eta = myccol*m_cols_core; 
  int max_eta = (myccol+1)*m_cols_core-1;

  int min_phi = myqrow*m_rows_quar; 
  int max_phi = (myqrow+1)*m_rows_quar-1;

  for (int phi=min_phi; phi<=max_phi; phi++) {
    for (int eta=min_eta; eta<=max_eta; eta++) {
      tots.push_back(m_tots.at(eta).at(phi));
      if (m_tots.at(eta).at(phi)==0) continue;
      std::bitset<4> tot;
      tot |= (m_tots.at(eta).at(phi) - 1);
      result += tot.to_string();
    }      
  }    

  return result;
}


std::vector< std::tuple < int, int, int, int> > Rd53bChipMap::doSplit(std::vector< std::tuple < int, int, int, int> > extremes, 
 std::string& new_result, bool do_compression, bool do_leftRight) {

  std::vector< std::tuple < int, int, int, int> > new_extremes;
  for (auto & extreme: extremes) {
    const int current_eta_min = std::get<0>(extreme);
    const int current_eta_max = std::get<1>(extreme);
    const int average_eta = (current_eta_max+current_eta_min+1)/2;

    const int current_phi_min = std::get<2>(extreme);
    const int current_phi_max = std::get<3>(extreme);
    const int average_phi = (current_phi_max+current_phi_min+1)/2;

    int new_eta_min = current_eta_min;
    int new_eta_max = average_eta-1;      
    int new_phi_min = current_phi_min;
    int new_phi_max = current_phi_max;

      // change the extremes if you split top-bottom instead of right-left
    if (not do_leftRight) {
      new_eta_max = current_eta_max;
      new_phi_max = average_phi-1;
    }        

    std::string temp_result = "";
    if (isHalfFired(new_eta_min, new_eta_max, new_phi_min, new_phi_max) == "1") {
      new_extremes.push_back(std::make_tuple(new_eta_min, new_eta_max, new_phi_min, new_phi_max));
      temp_result+="1";
    } else temp_result+="0";  

    new_eta_min = average_eta;
    new_eta_max = current_eta_max;      
    new_phi_min = current_phi_min;
    new_phi_max = current_phi_max;

      // change the extremes if you split top-bottom instead of right-left
    if (not do_leftRight) {
      new_eta_min = current_eta_min;
      new_phi_min = average_phi;
    }

    if (isHalfFired(new_eta_min, new_eta_max, new_phi_min, new_phi_max) == "1") {
      new_extremes.push_back(std::make_tuple(new_eta_min, new_eta_max, new_phi_min, new_phi_max));
      temp_result+="1";
    } else temp_result+="0";  

    if (do_compression and temp_result=="01") temp_result="0";

    new_result+=temp_result;        
  }
  return new_extremes;
}

void Rd53bChipMap::readMapFile(std::string inputMapFileName){
  std::ifstream fin(inputMapFileName);
  std::string line;
  bool startEvt = false;
  while(std::getline(fin, line)){
    if(line.find("Event") != std::string::npos) startEvt = true;
    if(startEvt){
      while(std::getline(fin, line)){
        std::stringstream s(line);
        int tag, eta, phi, ccol, qrow, tot;
        if(s>>tag>>eta>>phi>>ccol>>qrow>>tot){
          fillChipMap(eta, phi, tot);
        }
        else{
          startEvt = false;
          break;
        }
      }
    }
  }
  fin.close();
}

void Rd53bChipMap::generateRndmEvent(double occupancy, int clusterSize, int clusterOrientation){
  
  std::uniform_real_distribution<double> prob(0.0,1.0);
  std::uniform_int_distribution<> tot(1,15);
  for(int eta = 0; eta < m_cols; eta++){
    for(int phi = 0; phi < m_rows; phi++){
      if(prob(generator) < occupancy){
        fillChipMap(eta, phi, tot(generator));
        if (clusterSize > 1)
        {
          if (clusterOrientation == 0)
          { // Extend in eta
            for (int eta_add = eta + 1; eta_add < eta + clusterSize; eta_add++)
              fillChipMap((eta_add % m_cols), phi, tot(generator));
          }
          else
          { // Extend in phi
            for (int phi_add = phi + 1; phi_add < phi + clusterSize; phi_add++)
              fillChipMap(eta, (phi_add % m_rows), tot(generator));
          }
  }
      } 
    }
  }
}

std::string Rd53bChipMap::getPlainHitMap(int myccol, int myqrow) {
  std::string result = "";

  if (m_nfired_pixels_in_region.at(myccol).at(myqrow) == 0)
    return result;

  int min_eta = myccol*m_cols_core; 
  int max_eta = (myccol+1)*m_cols_core-1;

  int min_phi = myqrow*m_rows_quar; 
  int max_phi = (myqrow+1)*m_rows_quar-1;

  for(int phi = min_phi; phi <= max_phi; phi++){
    for(int eta = min_eta; eta <= max_eta; eta++){
      if(m_fired_pixels.at(eta).at(phi)) result += "1";
      else result += "0";
    }
  }
  return result;
}
