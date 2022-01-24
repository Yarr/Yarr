// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histograms event data
// ################################

#include "StdHistogrammer.h"

#include "AllHistogrammers.h"
#include "Histo1d.h"
#include "Histo2d.h"
#include "Histo3d.h"

#include "logging.h"

namespace {
    auto alog = logging::make_log("StdHistogrammer");
}

namespace {
    bool da_registered =
      StdDict::registerHistogrammer("DataArchiver",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new DataArchiver());});

    bool om_registered =
      StdDict::registerHistogrammer("OccupancyMap",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new OccupancyMap());});

    std::function<std::unique_ptr<HistogramAlgorithm>(json*)> fHFD( [](json* cfg) { return std::unique_ptr<HistogramAlgorithm>(new HistoFromDisk(cfg));} );
    bool hfd_registered =
      StdDict::registerHistogrammer("HistoFromDisk", fHFD);

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
    
    bool tag_registered =
      StdDict::registerHistogrammer("TagMap",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new TagMap());});

    bool tagdist_registered =
      StdDict::registerHistogrammer("TagDist",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new TagDist());});

    bool l13d_registered =
      StdDict::registerHistogrammer("L13d",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new L13d());});

    bool hpe_registered =
      StdDict::registerHistogrammer("HitsPerEvent",
                                []() { return std::unique_ptr<HistogramAlgorithm>(new HitsPerEvent());});
}

bool DataArchiver::open(std::string filename) {
    fileHandle.open(filename.c_str(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
    return fileHandle.good();
}

void DataArchiver::processEvent(FrontEndData *data) {
    if(fileHandle.is_open()) {
        for (const FrontEndEvent &curEvent: data->events) {
            curEvent.toFileBinary(fileHandle);
        }
    }
}

void OccupancyMap::create(const LoopStatus &stat) {
    h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("Hits");
    r.reset(h);
}

void OccupancyMap::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row);
            }
        }
    }
}

void HistoFromDisk::create(const LoopStatus &stat) {
    //Let's open the input file from HistoFromDisk
    std::ifstream file(m_config["inputFileName"], std::fstream::in);
    json inputFile(json::parse(file));
    file.close();

    //Let's deal with the loopStatus in order to use this input histogram in the right place of the scan hierarchy
    std::string strLoopStatus = " styles=";
    for (unsigned int i=0; i<stat.styleSize();i++)
	    strLoopStatus += stat.getStyle(i) + ", ";
    strLoopStatus += ", stats=";
    for (unsigned int i=0; i<stat.size();i++)
            strLoopStatus += stat.get(i) + ", ";
    bool matches=true;
    for (unsigned int i=0; matches && i<stat.size();i++)
            if (inputFile["loopStatus"][i] != stat.get(i)) 
		matches = false;
    if (!matches) {
	alog->debug("HistoFromDisk skipping file {} that does not correspond to LoopStatus {}", std::string(m_config["inputFileName"]), strLoopStatus.c_str());
	h = NULL;
   } else { //If we have the right HistoFromDisk at hand...
    	alog->info("HistoFromDisk opening file {} that corresponds to LoopStatus {}", std::string(m_config["inputFileName"]), strLoopStatus.c_str());
	std::vector<LoopStyle> styleVec;
    	for (unsigned int i=0; i<stat.styleSize();i++) 
	    styleVec.push_back((LoopStyle)stat.getStyle(i));
    	std::vector<unsigned> statVec;
    	for (unsigned int i=0; i<stat.size();i++) 
	    statVec.push_back(inputFile["loopStatus"][i]);
    	const LoopStatus newLoopStatus(std::move(statVec), styleVec);

    	if (inputFile["Type"]=="Histo1d") {
      		h = new Histo1d(inputFile["Name"], 
		  		inputFile["x"]["Bins"], inputFile["x"]["Low"], inputFile["x"]["High"], newLoopStatus); 
      		((Histo1d*)h)->fromFile(m_config["inputFileName"]);
    	} else if (inputFile["Type"]=="Histo2d") {
      		h = new Histo2d(inputFile["Name"], 
		 		inputFile["x"]["Bins"], inputFile["x"]["Low"], inputFile["x"]["High"],
		  		inputFile["y"]["Bins"], inputFile["y"]["Low"], inputFile["y"]["High"], newLoopStatus);
      		((Histo2d*)h)->fromFile(m_config["inputFileName"]);
    	} else if (inputFile["Type"]=="Histo3d") {
      		h = new Histo3d(inputFile["Name"], 
		  		inputFile["x"]["Bins"], inputFile["x"]["Low"], inputFile["x"]["High"],
		  		inputFile["y"]["Bins"], inputFile["y"]["Low"], inputFile["y"]["High"],
		  		inputFile["z"]["Bins"], inputFile["z"]["Low"], inputFile["z"]["High"], newLoopStatus);
      	((Histo3d*)h)->fromFile(m_config["inputFileName"]);
    	}
    } 
   
    //Don't forget this line
    r.reset(h);
}

void TotMap::create(const LoopStatus &stat) {
    h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("Total ToT");
    r.reset(h);
}

void TotMap::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curHit.tot & 0x7ff);
            }
        }
    }
}

void Tot2Map::create(const LoopStatus &stat) {
    h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("Total ToT2");
    r.reset(h);
}

void Tot2Map::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, (curHit.tot & 0x7ff) * (curHit.tot & 0x7ff));
            }
        }
    }
}

void TotDist::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), 16, 0.5, 16.5, stat);
    h->setXaxisTitle("ToT [bc]");
    h->setYaxisTitle("# of Hits");
    r.reset(h);
}

void TotDist::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.tot & 0x7ff);
            }
        }
    }
}

void Tot3d::create(const LoopStatus &stat) {
    h = new Histo3d("Tot3d", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, 16, 0.5, 16.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("ToT");
    r.reset(h);
}

void Tot3d::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curHit.tot & 0x7ff);
            }
        }
    }
}

void TagDist::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), 257, -0.5, 256.5, stat);
    h->setXaxisTitle("Tag");
    h->setYaxisTitle("Hits");
    r.reset(h);
}

void TagDist::processEvent(FrontEndData *data) {
    // Event Loop
    for (const FrontEndEvent &curEvent: data->events) {
        h->fill(curEvent.tag, curEvent.nHits);
    }
}

void TagMap::create(const LoopStatus &stat) {
    h = new Histo2d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("Tag");
    r.reset(h);
}

void TagMap::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        if (curEvent.nHits > 0) {
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curEvent.tag);
            }
        }
    }
}

void L1Dist::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), 16, -0.5, 15.5, stat);
    h->setXaxisTitle("L1A");
    h->setYaxisTitle("Hits");
    r.reset(h);
    l1id = 33;
    bcid_offset = 0;
}

void L1Dist::processEvent(FrontEndData *data) {
    // Event Loop
    for (const FrontEndEvent &curEvent: data->events) {
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

void L13d::create(const LoopStatus &stat) {
    h = new Histo3d(outputName(), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, 16, -0.5, 15.5, stat);
    h->setXaxisTitle("Column");
    h->setYaxisTitle("Row");
    h->setZaxisTitle("L1A");
    r.reset(h);
    l1id = 33;
    bcid_offset = 0;
}

void L13d::processEvent(FrontEndData *data) {
    for (const FrontEndEvent &curEvent: data->events) {
        
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
            for (const FrontEndHit &curHit: curEvent.hits) {   
                if(curHit.tot > 0)
                    h->fill(curHit.col, curHit.row, curEvent.l1id%16);
            }
        }
    }
}

void HitsPerEvent::create(const LoopStatus &stat) {
    h = new Histo1d(outputName(), 1000, -0.5, 999.5, stat);
    h->setXaxisTitle("Number of Hits");
    h->setYaxisTitle("Events");
    r.reset(h);
}

void HitsPerEvent::processEvent(FrontEndData *data) {
    // Event Loop
    for (const FrontEndEvent &curEvent: data->events) {
        h->fill(curEvent.nHits);
    }
}
