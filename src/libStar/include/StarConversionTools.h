// #################################
// # Author: Olivier Arnaez & Elise Le Boulicaut
// # Email: Olivier Arnaez at cern.ch
// # Project: Yarr
// # Description: Conversion tools class
// ################################

#include <string>
#include <vector>

class StarConversionTools {
public:
  StarConversionTools();
  StarConversionTools(const std::string& file_name);

  virtual ~StarConversionTools() = default;

  std::pair<double, double> convertDACtoV(double thrDAC, double err_thrDAC);

private:

  std::vector<double>  m_thrCal_mV;
};
