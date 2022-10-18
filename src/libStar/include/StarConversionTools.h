// #################################
// # Author: Olivier Arnaez & Elise Le Boulicaut
// # Email: Olivier Arnaez at cern.ch
// # Project: Yarr
// # Description: Conversion tools class
// ################################

#include <cmath>
#include <unistd.h>

class StarConversionTools {
    public:
  void loadDACtoVConversion(std::vector<double> &thrConverted);

  std::pair<double, double> convertDACtoV(const double & thrDAC, const double & err_thrDAC, const std::vector<double> & thrRef_mV);
};
