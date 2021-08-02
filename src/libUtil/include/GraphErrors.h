#ifndef GRAPHERRORS_H
#define GRAPHERRORS_H

#include "HistogramBase.h"

class GraphErrors : public HistogramBase {
public:
  GraphErrors(const std::string &arg_name, unsigned npoints=0);
  GraphErrors(const std::string &arg_name, const LoopStatus &stat, unsigned npoints=0);
  GraphErrors(const std::string &arg_name, std::vector<double> x, std::vector<double> y, std::vector<double> xerr, std::vector<double> yerr);
  GraphErrors(const std::string &arg_name, std::vector<double> x, std::vector<double> y, std::vector<double> xerr, std::vector<double> yerr, const LoopStatus &stat);
  ~GraphErrors() {}

  unsigned size() const;

  void addPoint(double xval, double yval, double xerr=-1, double yerr=-1);

  void toFile(const std::string &prefix, const std::string &dir="", bool jsonType=true) const override;
  bool fromFile(const std::string &filename);
  void plot(const std::string &filename, const std::string &dir="") const override;
  void toStream(std::ostream &out) const override;
  void toJson(json &j) const override;

private:
  unsigned data_npoints;

  std::vector<double> data_x;
  std::vector<double> data_xerr;
  std::vector<double> data_y;
  std::vector<double> data_yerr;

  bool hasXerrs() const;
  bool hasYerrs() const;
};

#endif
