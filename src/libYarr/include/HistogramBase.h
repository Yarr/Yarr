#ifndef HISTOGRAMBASE_H
#define HISTOGRAMBASE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histogram Base Container
// # Comment: 
// ################################

#include <string>
#include <utility>
#include "LoopStatus.h"

#include "storage.hpp"

class HistogramBase {
public:
    HistogramBase(std::string arg_name, LoopStatus stat)
            : name(std::move(arg_name)), lStat(std::move(stat)) {}

    explicit HistogramBase(std::string arg_name)
            : name(std::move(arg_name)) {}

   virtual  ~HistogramBase() = default;

    /// Read histogram from json file, based on appropriate type
    static std::unique_ptr<HistogramBase> fromJson(const json &j);

    /// Read histogram from json file, with loop status
    /**
     * Use loop status template for loop types.
     * If loopStatus field present in json, use this to fill in position.
     */
    static std::unique_ptr<HistogramBase> fromJson(const json &j, const LoopStatus &ltemplate);

    const std::string &getName() const {
        return name;
    }
    const LoopStatus & getStat() const {return lStat;}
    virtual void toStream(std::ostream &out) const=0;
    virtual void toJson(json &j) const=0;
    virtual void toFile(const std::string &basename, const std::string &dir = "", bool header= true) const=0;
    virtual void plot(const std::string &basename, const std::string &dir = "") const=0;

    void setAxisTitle(std::string x, std::string y, std::string z) {
        xAxisTitle = std::move(x);
        yAxisTitle = std::move(y);
        zAxisTitle = std::move(z);
    }

    std::string getXaxisTitle() const {
        return xAxisTitle;
    }

    std::string getYaxisTitle() const {
        return yAxisTitle;
    }

    std::string getZaxisTitle() const {
        return zAxisTitle;
    }

    void setXaxisTitle(const std::string &arg_name) {
        xAxisTitle = arg_name;
    }

    void setYaxisTitle(const std::string &arg_name) {
        yAxisTitle = arg_name;
    }

    void setZaxisTitle(const std::string &arg_name) {
        zAxisTitle = arg_name;
    }
protected:
    std::string name;
    LoopStatus  lStat;
    std::string xAxisTitle {"x"};
    std::string yAxisTitle {"y"};
    std::string zAxisTitle {"z"};
};
#endif
