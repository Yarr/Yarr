#include "HistogramBase.h"

#include "Histo1d.h"
#include "Histo2d.h"
#include "Histo3d.h"

#include "logging.h"

namespace {

auto hlog = logging::make_log("HistogramBase");

} // End anon namespace

std::unique_ptr<HistogramBase> HistogramBase::fromJson(const json &j)
{
    if(!j.contains("Type")) {
        hlog->error("Loading histogram from json, no Type property");
        return nullptr;
    }

    std::string type = j["Type"];
    if(type=="Histo1d") {
        auto h = std::make_unique<Histo1d>(j["Name"],
                                           j["x"]["Bins"], j["x"]["Low"], j["x"]["High"]);
        h->fromJson(j);
        return h;
    } else if(type=="Histo2d") {
        auto h = std::make_unique<Histo2d>(j["Name"],
                                           j["x"]["Bins"], j["x"]["Low"], j["x"]["High"],
                                           j["y"]["Bins"], j["y"]["Low"], j["y"]["High"]);
        h->fromJson(j);
        return h;
    } else if(type=="Histo3d") {
        auto h = std::make_unique<Histo3d>(j["Name"],
                                           j["x"]["Bins"], j["x"]["Low"], j["x"]["High"],
                                           j["y"]["Bins"], j["y"]["Low"], j["y"]["High"],
                                           j["z"]["Bins"], j["z"]["Low"], j["z"]["High"]);
      	h->fromJson(j);
        return h;
    }

    hlog->error("Loading histogram from json, Type property not recognised");
    return nullptr;
}
