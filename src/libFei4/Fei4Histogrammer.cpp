// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms Fei4 data
// # Comment: 
// ################################

#include "Fei4Histogrammer.h"

#include <iostream>

#include "AllHistogrammers.h"

#include "logging.h"

namespace {
    auto alog = logging::make_log("Fei4Histogrammer");
}

namespace {
    // Needs filename from somewhere...
    // bool da_registered =
    //   StdDict::registerHistogrammer("DataArchiver",
    //                             []() { return std::unique_ptr<HistogramAlgorithm>(new DataArchiver());});

    bool om_registered =
      StdDict::registerHistogrammer("OccupancyMap",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new OccupancyMap());});

    bool tot_registered =
      StdDict::registerHistogrammer("TotMap",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new TotMap());});

    bool tot2_registered =
      StdDict::registerHistogrammer("Tot2Map",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new Tot2Map());});

    bool tot_dist_registered =
      StdDict::registerHistogrammer("TotDist",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new TotDist());});

    bool tot3d_registered =
      StdDict::registerHistogrammer("Tot3d",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new Tot3d());});

    bool l1dist_registered =
      StdDict::registerHistogrammer("L1Dist",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new L1Dist());});

    bool l13d_registered =
      StdDict::registerHistogrammer("L13d",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new L13d());});

    bool hpe_registered =
      StdDict::registerHistogrammer("HitsPerEvent",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new HitsPerEvent());});
}

void DataArchiver::processEvent(Fei4Data *data) {
    for (const Fei4Event &curEvent: data->events) {
        // Save Event to File
        curEvent.toFileBinary(fileHandle);
    }
}


void OccupancyMap::processEvent(Fei4Data *data) {
    for (const Fei4Event &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const Fei4Hit &curHit: curEvent.hits) {
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row);
            }
        }
    }
}

void TotMap::processEvent(Fei4Data *data) {
    for (const Fei4Event &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const Fei4Hit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curHit.tot);
            }
        }
    }
}

void Tot2Map::processEvent(Fei4Data *data) {
    for (const Fei4Event &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const Fei4Hit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curHit.tot*curHit.tot);
            }
        }
    }
}

void TotDist::processEvent(Fei4Data *data) {
    for (const Fei4Event &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const Fei4Hit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.tot);
            }
        }
    }
}

void Tot3d::processEvent(Fei4Data *data) {
    for (const Fei4Event &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const Fei4Hit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curHit.tot);
            }
        }
    }
}

void L1Dist::processEvent(Fei4Data *data) {
    // Event Loop
    for (const Fei4Event &curEvent: data->events) {
        if(curEvent.l1id != l1id) {
            l1id = curEvent.l1id;
            if (curEvent.bcid - bcid_offset > 16) {
                bcid_offset = curEvent.bcid;
                //current_tag++;
            } else if ((curEvent.bcid+32768) - bcid_offset > 16 &&
                       static_cast<int>(curEvent.bcid) - static_cast<int>(bcid_offset) < 0) {
                bcid_offset = curEvent.bcid;
                //current_tag++;
            }
        }

        int delta_bcid = curEvent.bcid - bcid_offset;
        if (delta_bcid < 0)
            delta_bcid += 32768;
        h->fill(delta_bcid, curEvent.nHits);

        //TODO hack to generate proper tag, should come from FE/FW
        //curEvent.tag = current_tag;

    }
}

void L13d::processEvent(Fei4Data *data) {
    for (const Fei4Event &curEvent: data->events) {
        
        /*if(curEvent.l1id != l1id) {
            l1id = curEvent.l1id;
            if (curEvent.bcid - bcid_offset > 16) {
                bcid_offset = curEvent.bcid;
            } else if ((curEvent.bcid+32768) - bcid_offset > 16 &&
                       static_cast<int>(curEvent.bcid) - static_cast<int>(bcid_offset) < 0) {
                bcid_offset = curEvent.bcid;
            }
        }

        int delta_bcid = curEvent.bcid - bcid_offset;
        if (delta_bcid < 0)
            delta_bcid += 32768;
        */
        
        if (curEvent.nHits > 0) {
            for (const Fei4Hit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curEvent.l1id%16);
            }
        }
    }
}

void HitsPerEvent::processEvent(Fei4Data *data) {
    // Event Loop
    for (const Fei4Event &curEvent: data->events) {
        h->fill(curEvent.nHits);
    }
}
