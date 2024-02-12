#include "HistogramBase.h"

#include "Histo1d.h"
#include "Histo2d.h"
#include "Histo3d.h"

#include "logging.h"

namespace {

auto hlog = logging::make_log("HistogramBase");

LoopStatus statusFromJson(const json &j, const LoopStatus &input)
{
    std::vector<LoopStyle> styleVec;
    for (unsigned int i=0; i<input.styleSize(); i++) {
        styleVec.push_back((LoopStyle)input.getStyle(i));
    }
    std::vector<unsigned> statVec;
    for (unsigned int i=0; i<input.size(); i++) {
        statVec.push_back(j["loopStatus"][i]);
    }

    return {std::move(statVec), styleVec};
}

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
        // fromJson is not implemented
        hlog->error("Sorry, can't load Histo3d at the moment");
    }

    hlog->error("Loading histogram from json, Type property not recognised");
    return nullptr;
}

std::unique_ptr<HistogramBase> HistogramBase::fromJson(const json &j, const LoopStatus &ltemplate)
{
    if(!j.contains("loopStatus")) {
        hlog->error("Loading histogram from json, no loopStatus property to update location in scan loop");
    }

    auto lstatus = statusFromJson(j, ltemplate);

    if(!j.contains("Type")) {
        hlog->error("Loading histogram from json, no Type property");
        return nullptr;
    }

    std::string type = j["Type"];
    if(type=="Histo1d") {
        auto h = std::make_unique<Histo1d>(j["Name"],
                                           j["x"]["Bins"], j["x"]["Low"], j["x"]["High"], lstatus);
        h->fromJson(j);
        return h;
    } else if(type=="Histo2d") {
        auto h = std::make_unique<Histo2d>(j["Name"],
                                           j["x"]["Bins"], j["x"]["Low"], j["x"]["High"],
                                           j["y"]["Bins"], j["y"]["Low"], j["y"]["High"], lstatus);
        h->fromJson(j);
        return h;
    } else if(type=="Histo3d") {
        // fromJson is not implemented
        hlog->error("Sorry, can't load Histo3d at the moment");
    }

    hlog->error("Loading histogram from json, Type property not recognised");
    return nullptr;
}
