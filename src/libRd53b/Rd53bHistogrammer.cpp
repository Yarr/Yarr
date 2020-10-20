#include "AllHistogrammers.h"
#include "logging.h"
#include <memory>

#include <iostream>
#include "Rd53bHistogrammer.h"

namespace {
    auto logger = logging::make_log("Rd53bHistogrammer");
}

namespace {
    bool toa_registered =
            StdDict::registerHistogrammer("ToaMap",
                    []() { return std::unique_ptr<HistogramAlgorithm>(new ToaMap()); });

    bool toa2_registered =
            StdDict::registerHistogrammer("Toa2Map",
                    []() { return std::unique_ptr<HistogramAlgorithm>(new Toa2Map()); });

    bool ptot_dist_registered = 
            StdDict::registerHistogrammer("PToTDist",
                    []() { return std::unique_ptr<HistogramAlgorithm>(new PToTDist()); });

    bool ptoa_dist_registered = 
            StdDict::registerHistogrammer("PToADist",
                    []() { return std::unique_ptr<HistogramAlgorithm>(new PToADist()); });

}

void ToaMap::processEvent(FrontEndData* data) {
    for (const FrontEndEvent& curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for(const FrontEndHit& curHit: curEvent.hits) {
                if(curHit.ptot > 0) {
                    double val = static_cast<double>(curHit.ptoa + (curEvent.tag * 16));
                    //double val = static_cast<double>(curHit.ptoa);
                    h->fill(curHit.col, curHit.row, val);
                }
            } // curHit
        } 
    } // curEvent
}

void Toa2Map::processEvent(FrontEndData* data) {
    for (const FrontEndEvent& curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for(const FrontEndHit& curHit: curEvent.hits) {
                if(curHit.ptot > 0) {
                    //double val = curHit.ptoa;// + curEvent.tag * 16;
                    double val = static_cast<double>(curHit.ptoa + (curEvent.tag * 16));
                    h->fill(curHit.col, curHit.row, val*val);
                }
            } // curHit
        } 
    } // curEvent
}

void PToTDist::processEvent(FrontEndData* data) {
    for(const FrontEndEvent& curEvent: data->events) {
        if(curEvent.nHits>0) {
            for(const FrontEndHit& curHit: curEvent.hits) {
                if(curHit.ptot > 0)
                    h->fill(curHit.ptot);
            }
        }
    } // curEvent
}

void PToADist::processEvent(FrontEndData* data) {
    for(const FrontEndEvent& curEvent: data->events) {
        if(curEvent.nHits>0) {
            for(const FrontEndHit& curHit: curEvent.hits) {
                if(curHit.ptot > 0) {
                    //double val = static_cast<double>(curHit.ptoa);// + curEvent.tag * 16);
                    double val = static_cast<double>(curHit.ptoa + (curEvent.tag * 16));
                    h->fill(val);
                }
            }
        }
    } // curEvent
}

