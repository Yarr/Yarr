#include "GraphErrors.h"

#include <algorithm>
#include <iomanip>

#include "logging.h"

namespace {
    auto gelog = logging::make_log("GraphErrors");
}

GraphErrors::GraphErrors(const std::string &arg_name, unsigned npoints)
  : HistogramBase(arg_name)
  , data_npoints(0)
{
  if (npoints > 0) {
    data_x.reserve(npoints);
    data_y.reserve(npoints);
    data_xerr.reserve(npoints);
    data_yerr.reserve(npoints);
  }
}

GraphErrors::GraphErrors(const std::string &arg_name, const LoopStatus &stat, unsigned npoints)
  : HistogramBase(arg_name, stat)
  , data_npoints(0)
{
  if (npoints > 0) {
    data_x.reserve(npoints);
    data_y.reserve(npoints);
    data_xerr.reserve(npoints);
    data_yerr.reserve(npoints);
  }
}

GraphErrors::GraphErrors(const std::string &arg_name, std::vector<double> x, std::vector<double> y, std::vector<double> xerr, std::vector<double> yerr)
  : HistogramBase(arg_name)
  , data_x{std::move(x)}
  , data_y{std::move(y)}
  , data_xerr{std::move(xerr)}
  , data_yerr{std::move(yerr)}
{
  data_npoints = std::min({data_x.size(), data_y.size()});

  if (data_x.size() != data_y.size()) {
    gelog->warn("Xs and Ys are not of the same size!");
  }
}

GraphErrors::GraphErrors(const std::string &arg_name, std::vector<double> x, std::vector<double> y, std::vector<double> xerr, std::vector<double> yerr, const LoopStatus &stat)
  : HistogramBase(arg_name, stat)
  , data_x{std::move(x)}
  , data_y{std::move(y)}
  , data_xerr{std::move(xerr)}
  , data_yerr{std::move(yerr)}
{
  data_npoints = std::min({data_x.size(), data_y.size()});

  if (data_x.size() != data_y.size()) {
    gelog->warn("Xs and Ys are not of the same size!");
  }
}

unsigned GraphErrors::size() const {
  return data_npoints;
}

void GraphErrors::addPoint(double xval, double yval, double xerr, double yerr) {
  data_x.push_back(xval);
  data_y.push_back(yval);
  if (xerr >= 0) data_xerr.push_back(xerr);
  if (yerr >= 0) data_yerr.push_back(yerr);

  data_npoints++;
}

void GraphErrors::toStream(std::ostream &out) const {
  // Only data
  for (unsigned i=0; i<data_npoints; i++) {
    out << data_x[i] << " " << data_y[i];

    if (this->hasXerrs())
      out << " " << data_xerr[i];
    else
      out << " " << -1;

    if (this->hasYerrs())
      out << " " << data_yerr[i];
    else
      out << " " << -1;

    out << std::endl;
  }
}

void GraphErrors::toJson(json &j) const {
  j["Type"] = "GraphErrors";
  j["Name"] = name;

  j["x"]["AxisTitle"] = xAxisTitle;
  j["y"]["AxisTitle"] = yAxisTitle;
  j["z"]["AxisTitle"] = zAxisTitle;

  j["Data"]["npoints"] = data_npoints;
  for (unsigned i=0; i<data_npoints; i++) {
    j["Data"]["x"][i] = data_x[i];
    j["Data"]["y"][i] = data_y[i];

    if (this->hasXerrs())
      j["Data"]["xerr"][i] = data_xerr[i];

    if (this->hasYerrs())
      j["Data"]["yerr"][i] = data_yerr[i];
  }
}

void GraphErrors::toFile(const std::string &prefix, const std::string &dir, bool jsonType) const {
  std::string filename = dir + prefix + "_" + name;
  if (jsonType) {
    filename += ".json";
  } else {
    filename += ".dat";
  }

  std::fstream file(filename, std::fstream::out | std::fstream::trunc);
  json j;
  if (jsonType) {
    toJson(j);
    file << std::setw(4) << j;
  } else {
    toStream(file);
  }
  file.close();
}

bool GraphErrors::fromFile(const std::string &filename) {
  std::ifstream file(filename, std::fstream::in);
  json j;
  try {
    if (!file) {
      throw std::runtime_error("could not open file");
    }
    try {
      j = json::parse(file);
    } catch (json::parse_error &e) {
      throw std::runtime_error(e.what());
    }
  } catch (std::runtime_error &e) {
    gelog->error("Error opening graph: {}", e.what());
    return false;
  }

  // Check for type
  if (!j.contains("Type")) {
    gelog->error("Tried loading GraphErrors from file {}, but file has no header", filename);
    return false;
  } else {
    if (j["Type"] != "GraphErrors") {
      gelog->error("Tried loading GraphErrors from file {}, but file has incorrect header: {}", filename, std::string(j["Type"]));
      return false;
    }

    name = j["Name"];
    xAxisTitle = j["x"]["AxisTitle"];
    yAxisTitle = j["y"]["AxisTitle"];
    zAxisTitle = j["z"]["AxisTitle"];

    data_npoints = j["Data"]["npoints"];

    data_x.clear();
    data_y.clear();
    data_xerr.clear();
    data_yerr.clear();
    data_x.reserve(data_npoints);
    data_y.reserve(data_npoints);
    if (j.contains({"Data","xerr"}))
      data_xerr.reserve(data_npoints);
    if (j.contains({"Data","yerr"}))
      data_yerr.reserve(data_npoints);

    for (unsigned i=0; i<data_npoints; i++) {
      data_x.push_back(j["Data"]["x"][i]);
      data_y.push_back(j["Data"]["y"][i]);
      if (j.contains({"Data","xerr"}))
        data_xerr.push_back(j["Data"]["xerr"][i]);
      if (j.contains({"Data","yerr"}))
        data_yerr.push_back(j["Data"]["yerr"][i]);
    }
  }
  file.close();
  return true;
}

void GraphErrors::plot(const std::string &prefix, const std::string &dir) const {
  gelog->info("Plotting {}", name);

  std::string output = dir + prefix + "_" + name;
  for (unsigned i=0; i<lStat.size(); i++)
    output += "_" + std::to_string(lStat.get(i));
  output += ".png";

  std::string input;
  input += "$'set terminal png size 1280, 1024;";
  input += "unset key;";
  input += "set title \"" + name + "\";";
  input += "set xlabel \"" + xAxisTitle + "\";";
  input += "set ylabel \"" + yAxisTitle + "\";";
  input += "set offsets graph 0.1, graph 0.1, graph 0.1, graph 0.1;";
  input += "plot \\'-\\' using 1:2";
  if (this->hasXerrs() and this->hasYerrs()) input += ":3:4 with xyerrorbars";
  else if (this->hasXerrs()) input += ":3 with xerrors";
  else if (this->hasYerrs()) input += ":4 with yerrors";
  input += " pointtype 7 lc rgb \"black\";";
  input += "'";

  std::string cmd = "gnuplot -e " + input + " > " + output + "\n";
  FILE *gnu = popen(cmd.c_str(), "w");
  std::stringstream ss;
  toStream(ss);
  fprintf(gnu, "%s", ss.str().c_str());
  pclose(gnu);
}

bool GraphErrors::hasXerrs() const {
  if (data_xerr.empty())
    return false;

  if (data_xerr.size() != data_x.size()) {
    gelog->warn("X errors are not of the same size as X! X errors will be ignored.");
    return false;
  }

  return true;
}

bool GraphErrors::hasYerrs() const {
  if (data_yerr.empty())
    return false;

  if (data_yerr.size() != data_y.size()) {
    gelog->warn("Y errors are not of the same size as Y! Y errors will be ignored.");
    return false;
  }

  return true;
}
