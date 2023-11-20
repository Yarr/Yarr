#include "Rd53bAnalysis.h"

// std/stl
#include <iomanip>
#include <fstream>

// yarr
#include "Rd53bHistogrammer.h"
#include "AllAnalyses.h"
#include "StdHistogrammer.h" // knowledge of other analyses/etc
#include "StdTriggerAction.h"
#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53bAnalysis");
}

namespace {
    bool fescope_registered =
        StdDict::registerAnalysis("FrontEndScopeAnalysis",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new FrontEndScopeAnalysis());} );

    bool toa_analysis_registered = 
        StdDict::registerAnalysis("ToaAnalysis",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new ToaAnalysis());} );
}


//////////////////////////////////////////////////////////////////////////////
//
// FrontEndScopeAnalysis
//
//////////////////////////////////////////////////////////////////////////////
void FrontEndScopeAnalysis::loadConfig(const json &j) {

    if(j.contains("doPulseShapeMap")) {
        m_doPulseShapeMap = static_cast<bool>(j["doPulseShapeMap"]);;
    } else {
        m_doPulseShapeMap = false;
    }

    if(j.contains("pulseShapeBins")) {
        auto j_bounds = j["pulseShapeBins"];
        if(j_bounds.contains("xlo")) {
            m_pulseShape_xlo = j_bounds["xlo"];
        }
        if(j_bounds.contains("xhi")) {
            m_pulseShape_xhi = j_bounds["xhi"];
        }
        if(j_bounds.contains("nxbins")) {
            m_pulseShape_nxbins = j_bounds["nxbins"];
        }

        if(j_bounds.contains("ylo")) {
            m_pulseShape_ylo = j_bounds["ylo"];
        }
        if(j_bounds.contains("yhi")) {
            m_pulseShape_yhi = j_bounds["yhi"];
        }
        if(j_bounds.contains("nybins")) {
            m_pulseShape_nybins = j_bounds["nybins"];
        }
    }

    if(j.contains("excludeLRCols")) {
        m_exclude_LRCols = j["excludeLRCols"];
    }

}

void FrontEndScopeAnalysis::init(const ScanLoopInfo* s) {

    m_n_count = 1;
    m_injections = 0;
    m_has_threshold_loop = false;

    for (unsigned n = 0; n < s->size(); n++) {
        auto loop = s->getLoop(n);
        if (!(loop->isMaskLoop() || loop->isTriggerLoop() || loop->isDataLoop())) {
            m_loops.push_back(n);
            m_loopMax.push_back((unsigned)loop->getMax());
        } else {
            unsigned cnt = (loop->getMax() - loop->getMin()) / loop->getStep();
            if (cnt == 0) cnt = 1;
            m_n_count *= cnt;
        }
        if (loop->isTriggerLoop()) {
            auto trigger_loop = dynamic_cast<const StdTriggerAction*>(loop);
            if(trigger_loop == nullptr) {
                logger->error("FrontEndScopeAnalysis: loop declared as TriggerLoop, but does not have trigger count");
            } else {
                m_injections = trigger_loop->getTrigCnt();
            }
        } // triggerloop


        // loop over thresholds
        if (loop->isParameterLoop()) {
            logger->info("FrontEndScopeAnalysis: loop #{} is parameter loop", n);
            m_has_threshold_loop = true;
            m_threshold_min = loop->getMin();
            m_threshold_max = loop->getMax();
            m_threshold_step = loop->getStep();
            m_threshold_bins = (m_threshold_max - m_threshold_min) / m_threshold_step;
            logger->info("FrontEndScopeAnalysis: threshold min/max = {}/{}, step = {}", m_threshold_min, m_threshold_max, m_threshold_step);
        }
    } // n
}

void FrontEndScopeAnalysis::processHistogram(HistogramBase* h) {
    bool is_tot = (h->getName() == TotMap::outputName());
    bool is_toa = (h->getName() == ToaMap::outputName());
    bool is_occ = (h->getName() == OccupancyMap::outputName());
    if(!(is_tot || is_toa || is_occ)) return;

    //
    // Determine current identifier
    //
    unsigned id = 0;
    unsigned offset = 0;
    std::string occ_name = "OccMap";
    std::string ptot_name = "PToTMap";
    std::string ptoa_name = "PToAMap";
    for (unsigned n = 0; n < m_loops.size(); n++) {
        id += h->getStat().get(m_loops.at(n)) + offset;
        offset += m_loopMax.at(n);
        occ_name += "-" + std::to_string(h->getStat().get(m_loops.at(n)));
        ptot_name += "-" + std::to_string(h->getStat().get(m_loops.at(n)));
        ptoa_name += "-" + std::to_string(h->getStat().get(m_loops.at(n)));
    }


    //
    // Initialize histograms for this ID
    //
    if(h_occMaps[id] == NULL) {
        Histo2d* hist = new Histo2d(occ_name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5);
        hist->setXaxisTitle("Column");
        hist->setYaxisTitle("Row");
        hist->setZaxisTitle("Hits");
        h_occMaps[id].reset(hist);
        m_occ_count[id] = 0;
    }
    if(h_ptotMaps[id] == nullptr) {
        Histo2d* hist = new Histo2d(ptot_name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5);
        hist->setXaxisTitle("Column");
        hist->setYaxisTitle("Row");
        hist->setZaxisTitle("Sum PToT");
        h_ptotMaps[id].reset(hist);
        m_ptot_count[id] = 0;
    }
    if(h_ptoaMaps[id] == nullptr) {
        Histo2d* hist = new Histo2d(ptoa_name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5);
        hist->setXaxisTitle("Column");
        hist->setYaxisTitle("Row");
        hist->setZaxisTitle("Sum PToA");
        h_ptoaMaps[id].reset(hist);
        m_ptoa_count[id] = 0;
    }

    //
    // Merge histograms for this ID
    //
    if (h->getName() == OccupancyMap::outputName()) {
        h_occMaps[id]->add(*(Histo2d*)h);
        m_occ_count[id]++;
    } else if (h->getName() == TotMap::outputName()) {
        h_ptotMaps[id]->add(*(Histo2d*)h);
        m_ptot_count[id]++;
    } else if (h->getName() == ToaMap::outputName()) {
        h_ptoaMaps[id]->add(*(Histo2d*)h);
        m_ptoa_count[id]++;
    } else {
        return;
    }

    //
    // Have received the expected number of histograms, so we can start the analysis
    //
    bool all_data_received = (m_occ_count[id] == m_n_count) &&
                                (m_ptot_count[id] == m_n_count) &&
                                (m_ptoa_count[id] == m_n_count);
    if(all_data_received) {

        //
        //  set non-fully 100% occupancy bins to 0, this should be fine for the division
        //  later on for the mean PToT/PToA since Histo2d::divide just sets division by zero
        //  bins to 0
        //
        for(unsigned ii = 0; ii < h_occMaps[id]->size(); ii++) {
            if(h_occMaps[id]->getBin(ii) != m_injections) {
                h_occMaps[id]->setBin(ii, 0);
            }
        }

        ///////////////////////////////////////////////////////
        //
        //  PToT MAPS
        //
        ///////////////////////////////////////////////////////
        auto h_meanPToTMap = std::make_unique<Histo2d>("MeanPToTMap-"+std::to_string(id),nCol,0.5,nCol+0.5,nRow,0.5,nRow+0.5);
        h_meanPToTMap->setXaxisTitle("Column");
        h_meanPToTMap->setYaxisTitle("Row");
        h_meanPToTMap->setZaxisTitle("Mean PToT [1.5625ns]");
        h_meanPToTMap->add(*h_ptotMaps[id]);
        h_meanPToTMap->divide(*h_occMaps[id]);


        ///////////////////////////////////////////////////////
        //
        //  PToA MAPS
        //
        ///////////////////////////////////////////////////////
        auto h_meanPToAMap = std::make_unique<Histo2d>("MeanPToAMap-"+std::to_string(id),nCol,0.5,nCol+0.5,nRow,0.5,nRow+0.5);
        h_meanPToAMap->setXaxisTitle("Column");
        h_meanPToAMap->setYaxisTitle("Row");
        h_meanPToAMap->setZaxisTitle("Mean PToA [1.5625ns]");
        h_meanPToAMap->add(*h_ptoaMaps[id]);
        h_meanPToAMap->divide(*h_occMaps[id]);

        //
        // PULSE SHAPE MAP
        //
        if(m_has_threshold_loop && m_doPulseShapeMap && h_pulseShapeMap == NULL) {
            m_pulseShape_ylo = m_threshold_min;
            m_pulseShape_yhi = 1.15 * m_threshold_max;
            m_pulseShape_nybins = int( (m_pulseShape_yhi - m_pulseShape_ylo) / (m_threshold_step) );
            h_pulseShapeMap = std::make_unique<Histo2d>("ShapeMap-" + std::to_string(id), m_pulseShape_nxbins, m_pulseShape_xlo, m_pulseShape_xhi, m_pulseShape_nybins, m_pulseShape_ylo, m_pulseShape_yhi);
            h_pulseShapeMap->setXaxisTitle("Time [1.5625 ns]");
            h_pulseShapeMap->setYaxisTitle("Delta th [counts]");
        }

        // loop over all pixels
        if(m_has_threshold_loop && m_doPulseShapeMap && h_pulseShapeMap != NULL) {
            for(unsigned ii = 0; ii < h_meanPToAMap->size(); ii++) {

                // only fill for those thresholds where we have 100% occupancy
                if(!(h_occMaps[id]->getBin(ii) == m_injections)) continue;

                // Histo2D bins go up rows
                unsigned pix_row = static_cast<unsigned>(ii % 384);
                unsigned pix_col = static_cast<unsigned>(ii / 384);

                if(m_exclude_LRCols) {
                    if(pix_col == 0 || pix_col == 1) continue;
                    if(pix_col == 398 || pix_col == 399) continue;
                }

                double mean_ptoa = h_meanPToAMap->getBin(ii);
                double mean_ptot = h_meanPToTMap->getBin(ii);

                // rising edge
                h_pulseShapeMap->fill(mean_ptoa, static_cast<double>(id));
                // falling edge
                h_pulseShapeMap->fill(mean_ptoa + mean_ptot, static_cast<double>(id));
            }
        }

        //logger->info("Channel: {}, ID: {} -> Mean PToT = {} +- {}", channel, id, h_meanPToTDist->getMean(),  h_meanPToTDist->getStdDev());
        //logger->info("Channel: {}, ID: {} -> Mean PToA = {} +- {}", channel, id, h_meanPToADist->getMean(),  h_meanPToADist->getStdDev());
        output->pushData(std::move(h_meanPToTMap));
        output->pushData(std::move(h_meanPToAMap));

        //
        // clear the counts for this identifier
        //
        h_occMaps[id].reset();
        m_occ_count[id] = 0;

        h_ptotMaps[id].reset();
        m_ptot_count[id] = 0;

        h_ptoaMaps[id].reset();
        m_ptoa_count[id] = 0;
    } // all_data_received
}

void FrontEndScopeAnalysis::end() {
    if(h_pulseShapeMap != NULL) {
        output->pushData(std::move(h_pulseShapeMap));
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Precision Time Distributions
//
///////////////////////////////////////////////////////////////////////////////

void ToaAnalysis::loadConfig(const json &j) {

    // check for valid ToA histogram bin configuration
    if (j.contains("toa_bins")) {
        auto j_bins = j["toa_bins"];
        if (j_bins.contains("n_bins") && j_bins.contains("x_lo") && j_bins.contains("x_hi")) {
            toa_bins_n = static_cast<unsigned>(j_bins["n_bins"]);
            toa_bins_x_lo = static_cast<float>(j_bins["x_lo"]);
            toa_bins_x_hi = static_cast<float>(j_bins["x_hi"]);
        }
    }

    // ToA unit
    if (j.contains("toa_unit")) {
        toa_unit = static_cast<std::string>(j["toa_unit"]);
    }

    // check for valid ToA sigma histogram bin configuration
    if (j.contains("toa_sigma_bins")) {
        auto j_bins = j["toa_sigma_bins"];
        if (j_bins.contains("n_bins") && j_bins.contains("x_lo") && j_bins.contains("x_hi")) {
            toa_sigma_bins_n = static_cast<unsigned>(j_bins["n_bins"]);
            toa_sigma_bins_x_lo = static_cast<float>(j_bins["x_lo"]);
            toa_sigma_bins_x_hi = static_cast<float>(j_bins["x_hi"]);
        }
    }

}

void ToaAnalysis::init(const ScanLoopInfo *s) {

    m_n_count = 1;
    m_injections = 0;

    for (unsigned n=0; n<s->size(); n++) {
        auto loop = s->getLoop(n);
        if (!(loop->isMaskLoop() || loop->isTriggerLoop() || loop->isDataLoop())) {
            m_loops.push_back(n);
            m_loopMax.push_back((unsigned)loop->getMax());
        } else {
            unsigned cnt = (loop->getMax() - loop->getMin()) / loop->getStep();
            if (cnt == 0) cnt = 1;
            m_n_count *= cnt;
        }
        if (loop->isTriggerLoop()) {
            auto trigger_loop = dynamic_cast<const StdTriggerAction *>(loop);
            if(trigger_loop == nullptr) {
                logger->error("ToaAnalysis: loop declared as trigger does not have a count");
            } else {
                m_injections = trigger_loop->getTrigCnt();
            }
        }

        if (loop->isParameterLoop()) {
            if(m_hasVcalLoop) {
                logger->warn("ToaAnalysis: multiple ParameterLoops encountered -- will not create ToA vs. Charge map!");
                m_hasVcalLoop = false;
            } else {
                m_hasVcalLoop = true;
                m_vcalMax = loop->getMax();
                m_vcalMin = loop->getMin();
                m_vcalStep = loop->getStep();
                m_vcalNBins = (m_vcalMax - m_vcalMin) / m_vcalStep;
            }
        }
    }
}

void ToaAnalysis::processHistogram(HistogramBase *h) {
    // Check if right Histogram
    bool is_occ = (h->getName() == OccupancyMap::outputName());
    bool is_ptoaMap = (h->getName() == ToaMap::outputName());
    bool is_ptoa2 = (h->getName() == Toa2Map::outputName());
    bool is_relevant = is_ptoaMap || is_occ || is_ptoa2;
    if(!is_relevant) return;

    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;

    // Determine identifier
    std::string occ_name = "OccMap";
    std::string ptoaMap_name = "ToAMap";
    std::string ptoa2Map_name = "ToA2Map";
    for (unsigned n=0; n<m_loops.size(); n++) {
        ident += h->getStat().get(m_loops[n])+offset;
        offset += m_loopMax[n];
        std::string stat = std::to_string(h->getStat().get(m_loops[n]));
        ptoaMap_name += "-" + stat; 
        ptoa2Map_name += "-" + stat; 
        occ_name += "-" + stat;
    }

    //
    // initialize histograms
    //
    // occupancy map
    if(h_occMaps[ident] == NULL) {
        Histo2d* hh = new Histo2d(occ_name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5);
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("Hits");
        h_occMaps[ident].reset(hh);
        occ_count[ident] = 0;
    }

    // PToA map
    if(h_ptoaMaps[ident] == NULL) {
        Histo2d* hh = new Histo2d(ptoaMap_name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5);
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("Hits");
        h_ptoaMaps[ident].reset(hh);
        ptoa_count[ident] = 0;
    }

    // PToA2 map
    if(h_ptoa2Maps[ident] == NULL) {
        Histo2d* hh = new Histo2d(ptoa2Map_name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5);
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("Hits");
        h_ptoa2Maps[ident].reset(hh);
        ptoa2_count[ident] = 0;
    }

    // ToA vs charge
    if(m_hasVcalLoop && h_chargeVsToaMap == NULL) {
        auto cfg = feCfg;
        double chargeMin = cfg->toCharge(m_vcalMin);
        double chargeMax = cfg->toCharge(m_vcalMax);
        double chargeStep = cfg->toCharge(m_vcalStep);

        Histo2d* hh = new Histo2d("ChargeVsToaMap", m_vcalNBins+1, chargeMin - chargeStep/2, chargeMax + chargeStep/2, toa_bins_n, toa_bins_x_lo + 0.5, toa_bins_x_hi + 0.5);
        hh->setXaxisTitle("Injected Charge [e]");
        hh->setYaxisTitle("ToA [" + toa_unit + "]");
        hh->setZaxisTitle("Pixels");
        h_chargeVsToaMap.reset(hh);
    }

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    //
    // MERGE HISTOGRAMS
    //
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    if(is_occ) {
        h_occMaps[ident]->add(*(Histo2d*)h);
        occ_count[ident]++;
    } else if (is_ptoaMap) {
        h_ptoaMaps[ident]->add(*(Histo2d*)h);
        ptoa_count[ident]++;
    } else if (is_ptoa2) {
        h_ptoa2Maps[ident]->add(*(Histo2d*)h);
        ptoa2_count[ident]++;
    } else { return; };

    bool got_all_data = (occ_count[ident] == m_n_count &&
                            ptoa_count[ident] == m_n_count &&
                            ptoa2_count[ident] == m_n_count);

    if(got_all_data) {

        ///////////////////////////////////////////////
        //
        // PToA stuff
        //
        ///////////////////////////////////////////////

        // mean PToA MAP
        std::unique_ptr<Histo2d> meanPToAMap(new Histo2d("MeanToAMap-"+std::to_string(ident), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5));
        meanPToAMap->setXaxisTitle("Column");
        meanPToAMap->setYaxisTitle("Row");
        meanPToAMap->setZaxisTitle("Mean ToA [1.5625 ns]");

        // sum PToA MAP
        std::unique_ptr<Histo2d> sumPToAMap(new Histo2d("SumToAMap-"+std::to_string(ident), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5));
        sumPToAMap->setXaxisTitle("Column");
        sumPToAMap->setYaxisTitle("Row");
        sumPToAMap->setZaxisTitle("Sum ToA [1.5625 ns]");

        // sum PToA^2 MAP
        std::unique_ptr<Histo2d> sumPToA2Map(new Histo2d("SumToA2Map-"+std::to_string(ident), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5));
        sumPToA2Map->setXaxisTitle("Column");
        sumPToA2Map->setYaxisTitle("Row");
        sumPToA2Map->setZaxisTitle("Sum ToA^2 [1.5625 ns]^2");

        // sigma PToA MAP
        std::unique_ptr<Histo2d> sigmaPToAMap(new Histo2d("sigmaToAMap-"+std::to_string(ident), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5));
        sigmaPToAMap->setXaxisTitle("Column");
        sigmaPToAMap->setYaxisTitle("Row");
        sigmaPToAMap->setZaxisTitle("Per-pixel sigma ToA [1.5625 ns]");

        // mean PToA DIST
        std::unique_ptr<Histo1d> meanPToADist(new Histo1d("MeanToADist-"+std::to_string(ident), toa_bins_n, toa_bins_x_lo + 0.5, toa_bins_x_hi));
        meanPToADist->setXaxisTitle("Mean ToA [1.5625 ns]");
        meanPToADist->setYaxisTitle("Number of Pixels");

        // sigma PToA DIST
        std::unique_ptr<Histo1d> sigmaPToADist(new Histo1d("SigmaToADist-"+std::to_string(ident), 20, 0, 20));
        sigmaPToADist->setXaxisTitle("Per-pixel sigma ToA [1.5625 ns]");
        sigmaPToADist->setYaxisTitle("Number of Pixels");

        // Finely binned PToA DIST
        std::unique_ptr<Histo1d> fineMeanPToADist(new Histo1d("MeanToADistFine-"+std::to_string(ident), toa_bins_n, toa_bins_x_lo + 0.5, toa_bins_x_hi + 0.5));

        // fill
        meanPToAMap->add(*h_ptoaMaps[ident]);
        meanPToAMap->divide(*h_occMaps[ident]);
        sumPToAMap->add(*h_ptoaMaps[ident]);
        sumPToA2Map->add(*h_ptoa2Maps[ident]);
        for(unsigned ii = 0; ii < meanPToAMap->size(); ii++) {

            if(!(h_occMaps[ident]->getBin(ii) > 0)) continue; // only consider filled bins

            double sumOfPToA_squared = sumPToAMap->getBin(ii)*sumPToAMap->getBin(ii);
            double sigma = sqrt(fabs((sumPToA2Map->getBin(ii) - (sumOfPToA_squared/m_injections))/(m_injections-1)));
            sigmaPToAMap->setBin(ii, sigma);
            meanPToADist->fill(meanPToAMap->getBin(ii));
            sigmaPToADist->fill(sigma);
            fineMeanPToADist->fill(meanPToAMap->getBin(ii));
        } // ii

        logger->info("\033[1;33mId:{} ScanID:{} ToA Mean = {} +- {}\033[0m", id, ident, meanPToADist->getMean(), meanPToADist->getStdDev());

        // ToA vs charge
        if (m_hasVcalLoop) {
            auto cfg = feCfg;
            double chargeAtCurrentStep = cfg->toCharge(ident);
            double bin_width = ((toa_bins_x_hi+0.5) - (toa_bins_x_lo+0.5)) / (toa_bins_n);
            for (unsigned ii = 0; ii < fineMeanPToADist->size(); ii++) {
                double toa_val = (ii+1) * bin_width + (toa_bins_x_lo+0.5);
                h_chargeVsToaMap->fill(chargeAtCurrentStep, toa_val, fineMeanPToADist->getBin(ii));
            } // ii
        } // hasVcalLoop

        output->pushData(std::move(meanPToAMap));
        output->pushData(std::move(meanPToADist));
        output->pushData(std::move(sigmaPToAMap));
        output->pushData(std::move(sigmaPToADist));

        occ_count[ident] = 0;
        ptoa_count[ident] = 0;
        ptoa2_count[ident] = 0;

    } // got_all_data
}

void ToaAnalysis::end() {
    if (m_hasVcalLoop && h_chargeVsToaMap != NULL) {
        output->pushData(std::move(h_chargeVsToaMap));
    }
}
