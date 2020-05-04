// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Analysis Base class
// # Comment: 
// ################################

#include "Fei4Analysis.h"

#include "AllAnalyses.h"

#include "logging.h"

namespace {
    auto alog = logging::make_log("Fei4Analysis");
}

namespace {
    bool oa_registered =
      StdDict::registerAnalysis("OccupancyAnalysis",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new OccupancyAnalysis());});

    bool l1_registered =
      StdDict::registerAnalysis("L1Analysis",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new L1Analysis());});

    bool tot_registered =
      StdDict::registerAnalysis("TotAnalysis",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new TotAnalysis());});

    bool no_registered =
      StdDict::registerAnalysis("NoiseAnalysis",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new NoiseAnalysis());});

    bool no_tune_registered =
      StdDict::registerAnalysis("NoiseTuning",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new NoiseTuning());});

    bool sf_registered =
      StdDict::registerAnalysis("ScurveFitter",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new ScurveFitter());});

    bool gbl_thr_registered =
      StdDict::registerAnalysis("OccGlobalThresholdTune",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new OccGlobalThresholdTune());});

    bool pix_thr_registered =
      StdDict::registerAnalysis("OccPixelThresholdTune",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new OccPixelThresholdTune());});

    bool del_registered =
      StdDict::registerAnalysis("DelayAnalysis",
                                []() { return std::unique_ptr<AnalysisAlgorithm>(new DelayAnalysis());});
}

void OccupancyAnalysis::init(ScanBase *s) {
    createMask=true;
    n_count = 1;
    injections = 0;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
                    l->type() != typeid(Rd53aMaskLoop*) &&
                    l->type() != typeid(Rd53aTriggerLoop*) &&
                    l->type() != typeid(Rd53aCoreColLoop*) &&
                    l->type() != typeid(Fe65p2MaskLoop*) &&
                    l->type() != typeid(Fe65p2TriggerLoop*) &&
                    l->type() != typeid(Fe65p2QcLoop*) &&
                    l->type() != typeid(Fei4MaskLoop*) &&
                    l->type() != typeid(StdDataLoop*) &&
                    l->type() != typeid(Fei4DcLoop*))) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }
        if (l->type() == typeid(Fei4TriggerLoop*)) {
            Fei4TriggerLoop *trigLoop = (Fei4TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
        if (l->type() == typeid(Fe65p2TriggerLoop*)) {
            Fe65p2TriggerLoop *trigLoop = (Fe65p2TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
        if (l->type() == typeid(Rd53aTriggerLoop*)) {
            Rd53aTriggerLoop *trigLoop = (Rd53aTriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
    }
}

void OccupancyAnalysis::processHistogram(HistogramBase *h) {
    // Check if right Histogram
    if (h->getType() != typeid(OccupancyMap*))
        return;

    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;

    // Determine identifier
    std::string name = "OccupancyMap";
    std::string name2 = "EnMask";
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        name += "-" + std::to_string(h->getStat().get(loops[n]));
        name2 += "-" + std::to_string(h->getStat().get(loops[n]));
    }

    // Check if Histogram exists
    if (occMaps[ident] == nullptr) {
        std::unique_ptr<Histo2d> hh(new Histo2d(name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("Hits");
        occMaps[ident] = std::move(hh);
        innerCnt[ident] = 0;
    }

    // Add up Histograms
    occMaps[ident]->add(*(Histo2d*)h);
    innerCnt[ident]++;
    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        std::unique_ptr<Histo2d> mask(new Histo2d(name2, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
        mask->setXaxisTitle("Column");
        mask->setYaxisTitle("Rows");
        mask->setZaxisTitle("Enable");


        for (unsigned i=0; i<occMaps[ident]->size(); i++) {
            if (occMaps[ident]->getBin(i) == injections) {
                mask->setBin(i, 1);
            } else {
                if (make_mask&&createMask) {
                    bookie->getFe(channel)->maskPixel((i/nRow), (i%nRow));
                }
            }
        }
        output->pushData(std::move(mask)); // TODO push this mask to the specific configuration
        output->pushData(std::move(occMaps[ident]));


        //delete occMaps[ident];
        //occMaps[ident] = NULL;
    }
}
void OccupancyAnalysis::loadConfig(json &j){
    if (!j["createMask"].empty()){
        createMask=j["createMask"];
    }
}

void TotAnalysis::init(ScanBase *s) {
    std::shared_ptr<LoopActionBase> tmpVcalLoop(new Fei4ParameterLoop(&Fei4::PlsrDAC));
    std::shared_ptr<LoopActionBase> tmpVcalLoop2(new Fe65p2ParameterLoop(&Fe65p2::PlsrDac));
    std::shared_ptr<LoopActionBase> tmpVcalLoop3(new Rd53aParameterLoop());

    useScap = true;
    useLcap = true;
    n_count = 1;
    injections = 1;
    pixelFb = NULL;
    globalFb = NULL;
    hasVcalLoop = false;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
                    l->type() != typeid(Rd53aMaskLoop*) &&
                    l->type() != typeid(Rd53aTriggerLoop*) &&
                    l->type() != typeid(Rd53aCoreColLoop*) &&
                    l->type() != typeid(Fe65p2MaskLoop*) &&
                    l->type() != typeid(Fe65p2TriggerLoop*) &&
                    l->type() != typeid(Fe65p2QcLoop*) &&
                    l->type() != typeid(Fei4MaskLoop*) &&
                    l->type() != typeid(StdDataLoop*) &&
                    l->type() != typeid(Fei4DcLoop*))) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }

        if (l->type() == typeid(Fei4TriggerLoop*)) {
            Fei4TriggerLoop *trigLoop = (Fei4TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == typeid(Fe65p2TriggerLoop*)) {
            Fe65p2TriggerLoop *trigLoop = (Fe65p2TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == typeid(Rd53aTriggerLoop*)) {
            Rd53aTriggerLoop *trigLoop = (Rd53aTriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        std::shared_ptr<LoopActionBase> tmpPrmpFb(new Fei4GlobalFeedback(&Fei4::PrmpVbpf));
        if (l->type() == tmpPrmpFb->type()) {
            globalFb = dynamic_cast<GlobalFeedbackBase*>(l.get());  
        }

        if (l->type() == typeid(Rd53aGlobalFeedback*)) {
            globalFb = dynamic_cast<GlobalFeedbackBase*>(l.get());  
        }

        if (l->type() == typeid(Fei4PixelFeedback*)) {
            pixelFb = dynamic_cast<PixelFeedbackBase*>(l.get());  
        }

        // Vcal Loop
        if (l->type() == tmpVcalLoop->type() ||
                l->type() == tmpVcalLoop2->type() ||
                l->type() == tmpVcalLoop3->type()) {
            vcalMax = l->getMax();
            vcalMin = l->getMin();
            vcalStep = l->getStep();
            vcalBins = (vcalMax-vcalMin)/vcalStep;
            hasVcalLoop = true;
        }
    }
}

void TotAnalysis::processHistogram(HistogramBase *h) {
    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;
    // Determine identifier
    std::string name = "OccMap";
    std::string name2 = "TotMap";
    std::string name3 = "Tot2Map";
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        name += "-" + std::to_string(h->getStat().get(loops[n]));
        name2 += "-" + std::to_string(h->getStat().get(loops[n]));
        name3 += "-" + std::to_string(h->getStat().get(loops[n]));
    }

    // Check if Histogram exists
    if (occMaps[ident] == NULL) {
        Histo2d *hh = new Histo2d(name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("Hits");
        occMaps[ident].reset(hh);
        occInnerCnt[ident] = 0;
        hh = new Histo2d(name2, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("{/Symbol S}(ToT)");
        totMaps[ident].reset(hh);
        totInnerCnt[ident] = 0;
        hh = new Histo2d(name3, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("{/Symbol S}(ToT^2)");
        tot2Maps[ident].reset(hh);
        tot2InnerCnt[ident] = 0;
    }

    if (chargeVsTotMap == NULL && hasVcalLoop) {
        FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(bookie->getFe(channel));
        double chargeMin = feCfg->toCharge(vcalMin, useScap, useLcap);
        double chargeMax = feCfg->toCharge(vcalMax, useScap, useLcap);
        double chargeStep = feCfg->toCharge(vcalStep, useScap, useLcap);

        Histo2d *hh = new Histo2d("ChargeVsTotMap", vcalBins+1, chargeMin-chargeStep/2, chargeMax+chargeStep/2, 160, 0.05, 16.05, typeid(this));
        hh->setXaxisTitle("Injected Charge [e]");
        hh->setYaxisTitle("ToT");
        hh->setZaxisTitle("Pixels");
        chargeVsTotMap.reset(hh);
    }

    // Gather Histogram
    if (h->getType() == typeid(OccupancyMap*)) {
        occMaps[ident]->add(*(Histo2d*)h);
        occInnerCnt[ident]++;
    } else if (h->getType() == typeid(TotMap*)) {
        totMaps[ident]->add(*(Histo2d*)h);
        totInnerCnt[ident]++;
    } else if (h->getType() == typeid(Tot2Map*)) {
        tot2Maps[ident]->add(*(Histo2d*)h);
        tot2InnerCnt[ident]++;
    } else {
        return;
    }

    // Got all data, finish up Analysis
    if (occInnerCnt[ident] == n_count &&
            totInnerCnt[ident] == n_count &&
            tot2InnerCnt[ident] == n_count) {
        std::unique_ptr<Histo2d> meanTotMap(new Histo2d("MeanTotMap-"+std::to_string(ident), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
        meanTotMap->setXaxisTitle("Column");
        meanTotMap->setYaxisTitle("Row");
        meanTotMap->setZaxisTitle("Mean ToT [bc]");
        std::unique_ptr<Histo2d> sumTotMap(new Histo2d("SumTotMap-"+std::to_string(ident), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
        sumTotMap->setXaxisTitle("Column");
        sumTotMap->setYaxisTitle("Row");
        sumTotMap->setZaxisTitle("Mean ToT [bc]");
        std::unique_ptr<Histo2d> sumTot2Map(new Histo2d("MeanTot2Map-"+std::to_string(ident), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
        sumTot2Map->setXaxisTitle("Column");
        sumTot2Map->setYaxisTitle("Row");
        sumTot2Map->setZaxisTitle("Mean ToT^2 [bc^2]");
        std::unique_ptr<Histo2d> sigmaTotMap(new Histo2d("SigmaTotMap-"+std::to_string(ident), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
        sigmaTotMap->setXaxisTitle("Column");
        sigmaTotMap->setYaxisTitle("Row");
        sigmaTotMap->setZaxisTitle("Sigma ToT [bc]");
        std::unique_ptr<Histo1d> meanTotDist(new Histo1d("MeanTotDist-"+std::to_string(ident), 16, 0.5, 16.5, typeid(this)));
        meanTotDist->setXaxisTitle("Mean ToT [bc]");
        meanTotDist->setYaxisTitle("Number of Pixels");
        std::unique_ptr<Histo1d> sigmaTotDist(new Histo1d("SigmaTotDist-"+std::to_string(ident), 101, -0.05, 1.05, typeid(this)));
        sigmaTotDist->setXaxisTitle("Sigma ToT [bc]");
        sigmaTotDist->setYaxisTitle("Number of Pixels");
        std::unique_ptr<Histo1d> tempMeanTotDist(new Histo1d("MeanTotDistFine-"+std::to_string(ident), 160, 0.05, 16.05, typeid(this)));

        meanTotMap->add(*totMaps[ident]);
        meanTotMap->divide(*occMaps[ident]);
        sumTotMap->add(*totMaps[ident]);
        sumTot2Map->add(*tot2Maps[ident]);
        for(unsigned i=0; i<meanTotMap->size(); i++) {
            double sigma = sqrt(fabs((sumTot2Map->getBin(i) - ((sumTotMap->getBin(i)*sumTotMap->getBin(i))/injections))/(injections-1)));
            sigmaTotMap->setBin(i, sigma);
            meanTotDist->fill(meanTotMap->getBin(i));
            tempMeanTotDist->fill(meanTotMap->getBin(i));
            sigmaTotDist->fill(sigma);
        }
        if (hasVcalLoop) {
            // Tot vs charge map
            FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(bookie->getFe(channel));
            double currentCharge = feCfg->toCharge(ident, useScap, useLcap);
            for (unsigned i=0; i<tempMeanTotDist->size(); i++) {
                chargeVsTotMap->fill(currentCharge, (i+1)*0.1, tempMeanTotDist->getBin(i));
            }
        }
        alog->info("[{}] ToT Mean = {} +- {}", channel, meanTotDist->getMean(), meanTotDist->getStdDev());

        if (globalFb != NULL) {
            double mean = 0;
            double entries = 0;
            for (unsigned i=0; i<meanTotMap->size(); i++) {
                if (occMaps[ident]->getBin(i) == injections) {
                    mean += meanTotMap->getBin(i);
                    entries++;
                } 
            }
            if (entries > 0) {
                mean = mean/entries;
            }
            alog->info("Mean is: {}", mean);

            // TODO Get this from somewhere
            double targetTot = bookie->getTargetTot();
            int sign = 0;
            bool last = false;
            if (mean < (targetTot-0.1)) {
                sign = -1;
            } else if (mean > (targetTot+0.1)) {
                sign = +1;
            } else {
                sign = 0;
                last = true;
            }
            globalFb->feedbackBinary(channel, sign, last);
        }

        if (pixelFb != NULL) {
            double targetTot = bookie->getTargetTot();
            Histo2d *fbHisto = new Histo2d("feedback", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
            for (unsigned i=0; i<meanTotMap->size(); i++) {
                int sign = 0;
                double mean = meanTotMap->getBin(i);
                if (mean < (targetTot-0.05)) {
                    sign = -1;
                } else if (mean > (targetTot+0.05)) {
                    sign = +1;
                } else {
                    sign = 0;
                }
                fbHisto->setBin(i, sign);
            }

            pixelFb->feedback(channel, fbHisto);
        }

        output->pushData(std::move(meanTotMap));
        output->pushData(std::move(sigmaTotMap));
        output->pushData(std::move(meanTotDist));
        output->pushData(std::move(sigmaTotDist));
        occInnerCnt[ident] = 0;
        totInnerCnt[ident] = 0;
        tot2InnerCnt[ident] = 0;
    }
}

void TotAnalysis::end() {
    if (hasVcalLoop) {
        output->pushData(std::move(chargeVsTotMap));
    }
}

void ScurveFitter::init(ScanBase *s) {
    std::shared_ptr<LoopActionBase> tmpVcalLoop(new Fei4ParameterLoop(&Fei4::PlsrDAC));
    std::shared_ptr<LoopActionBase> tmpVcalLoop2(new Fe65p2ParameterLoop(&Fe65p2::PlsrDac));
    std::shared_ptr<LoopActionBase> tmpVcalLoop3(new Rd53aParameterLoop());
    fb = NULL;
    scan = s;
    n_count = 1;
    vcalLoop = 0;
    injections = 50;
    useScap = true;
    useLcap = true;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if (l->type() != typeid(Fei4TriggerLoop*) &&
                l->type() != typeid(Rd53aMaskLoop*) &&
                l->type() != typeid(Rd53aTriggerLoop*) &&
                l->type() != typeid(Rd53aCoreColLoop*) &&
                l->type() != typeid(Fe65p2TriggerLoop*) &&
                l->type() != typeid(Fei4MaskLoop*) &&
                l->type() != typeid(Fe65p2MaskLoop*) &&
                l->type() != typeid(StdDataLoop*) &&
                l->type() != typeid(Fei4DcLoop*) &&
                l->type() != typeid(Fe65p2QcLoop*) &&
                l->type() != tmpVcalLoop3->type() &&
                l->type() != tmpVcalLoop2->type() &&
                l->type() != tmpVcalLoop->type()) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (l->type() == tmpVcalLoop3->type() ||
                l->type() == tmpVcalLoop2->type() ||
                l->type() == tmpVcalLoop->type()) {
                cnt++; // Parameter loop interval is inclusive
            }
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }
        // Vcal Loop
        if (l->type() == tmpVcalLoop->type() ||
                l->type() == tmpVcalLoop2->type() ||
                l->type() == tmpVcalLoop3->type()) {
            vcalLoop = n;
            vcalMax = l->getMax();
            vcalMin = l->getMin();
            vcalStep = l->getStep();
            vcalBins = (vcalMax-vcalMin)/vcalStep;
        }

        if (l->type() == typeid(Fei4TriggerLoop*)) {
            Fei4TriggerLoop *trigLoop = (Fei4TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
        if (l->type() == typeid(Fe65p2TriggerLoop*)) {
            Fe65p2TriggerLoop *trigLoop = (Fe65p2TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
        if (l->type() == typeid(Rd53aTriggerLoop*)) {
            Rd53aTriggerLoop *trigLoop = (Rd53aTriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        // check injection capacitor for FEI-4
        if(l->type() == typeid(Fei4MaskLoop*)) {
            std::shared_ptr<Fei4MaskLoop> msk = std::dynamic_pointer_cast<Fei4MaskLoop>(l);
            useScap = msk->getScap();
            useLcap = msk->getLcap();
        }
        
        // Find potential pixel feedback
        if (l->type() == typeid(Fei4PixelFeedback*)) {
            fb = (PixelFeedbackBase*)((Fei4PixelFeedback*) l.get());  
        }
        if (l->type() == typeid(Fe65p2PixelFeedback*)) {
            fb = (PixelFeedbackBase*)((Fe65p2PixelFeedback*) l.get());  
        }
        if (l->type() == typeid(Rd53aPixelFeedback*)) {
            fb = (PixelFeedbackBase*)((Rd53aPixelFeedback*) l.get());  
        }

    }

    for (unsigned i=vcalMin; i<=vcalMax; i+=vcalStep) {
        x.push_back(i);
    }
    cnt = 0;
    n_failedfit =0;
    prevOuter = 0;
    thrTarget = bookie->getTargetCharge();
}

// Errorfunction
// par[0] = Mean
// par[1] = Sigma
// par[2] = Normlization
#define SQRT2 1.414213562
double scurveFct(double x, const double *par) {
    return 0.5*( 2-erfc( (x-par[0])/(par[1]*SQRT2) ) )*par[2];
}

void ScurveFitter::processHistogram(HistogramBase *h) {
    cnt++;
    // Check if right Histogram
    if (h->getType() != typeid(OccupancyMap*))
        return;

    Histo2d *hh = (Histo2d*) h;

    unsigned medIdent = 0;
    unsigned medOffset = 0;
    unsigned outerIdent = 0;
    unsigned outerOffset = 0;
    for (unsigned n=0; n<loops.size(); n++) {
        outerIdent += hh->getStat().get(loops[n])+outerOffset;
        medIdent += hh->getStat().get(loops[n])+medOffset;
        medOffset += loopMax[n];
        outerOffset += loopMax[n];
    }
    medCnt[medIdent]++;

    for(unsigned col=1; col<=nCol; col++) {
        for (unsigned row=1; row<=nRow; row++) {
            unsigned bin = hh->binNum(col, row);
            if (hh->getBin(bin) != 0) {
                // Select correct output containe
                unsigned ident = bin;
                unsigned offset = nCol*nRow;
                unsigned vcal = hh->getStat().get(vcalLoop);
                // Determine identifier
                std::string name = "Scurve";
                name += "-" + std::to_string(col) + "-" + std::to_string(row);
                // Check for other loops
                for (unsigned n=0; n<loops.size(); n++) {
                    ident += hh->getStat().get(loops[n])+offset;
                    offset += loopMax[n];
                    name += "-" + std::to_string(hh->getStat().get(loops[n]));
                }

                // Check if Histogram exists
                if (histos[ident] == NULL) {
                    Histo1d *hhh = new Histo1d(name, vcalBins+1, vcalMin-((double)vcalStep/2.0), vcalMax+((double)vcalStep/2.0), typeid(this));
                    hhh->setXaxisTitle("Vcal");
                    hhh->setYaxisTitle("Occupancy");
                    histos[ident].reset(hhh);
                    innerCnt[ident] = 0;
                }
                if (sCurve[outerIdent] == NULL) {
                    Histo2d *hhh = new Histo2d("sCurve-" + std::to_string(outerIdent), vcalBins+1, vcalMin-((double)vcalStep/2.0), vcalMax+((double)vcalStep/2.0), injections-1, 0.5, injections-0.5, typeid(this));
                    hhh->setXaxisTitle("Vcal");
                    hhh->setYaxisTitle("Occupancy");
                    hhh->setZaxisTitle("Number of pixels");
                    sCurve[outerIdent].reset(hhh);
                }

                // Add up Histograms
                histos[ident]->fill(vcal, hh->getBin(bin));
                sCurve[outerIdent]->fill(vcal, hh->getBin(bin));
                innerCnt[ident]++;

                // Got all data, finish up Analysis
                // TODO This requires the loop to run from low to high and a hit in the last bin
                if (vcal == vcalMax) {
                    // Scale histos
                    //histos[ident]->scale(1.0/(double)injections);
                    lm_status_struct status;
                    lm_control_struct control;
                    control = lm_control_float;
                    //control.verbosity = 3;
                    control.verbosity = 0;
                    const unsigned n_par = 3;
                    //double par[n_par] = {((vcalMax-vcalMin)/2.0)+vcalMin,  5 , (double) injections};
                    double par[n_par] = {((vcalMax-vcalMin)/2.0)+vcalMin,  0.05*(((vcalMax-vcalMin)/2.0)+vcalMin)  , (double) injections};
                    std::chrono::high_resolution_clock::time_point start;
                    std::chrono::high_resolution_clock::time_point end;
                    start = std::chrono::high_resolution_clock::now();
                    lmcurve(n_par, par, vcalBins, &x[0], histos[ident]->getData(), scurveFct, &control, &status);
                    end = std::chrono::high_resolution_clock::now();
                    std::chrono::microseconds fitTime = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
                    if (thrMap[outerIdent] == NULL) {
                        Histo2d *hh2 = new Histo2d("ThresholdMap-" + std::to_string(outerIdent), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
                        hh2->setXaxisTitle("Column");
                        hh2->setYaxisTitle("Row");
                        hh2->setZaxisTitle("Threshold [e]");
                        thrMap[outerIdent].reset(hh2);
                        hh2 = new Histo2d("NoiseMap-"+std::to_string(outerIdent), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
                        hh2->setXaxisTitle("Column");
                        hh2->setYaxisTitle("Row");
                        hh2->setZaxisTitle("Noise [e]");

                        sigMap[outerIdent].reset(hh2);

                        Histo1d *hh1 = new Histo1d("Chi2Dist-"+std::to_string(outerIdent), 51, -0.025, 2.525, typeid(this));
                        hh1->setXaxisTitle("Fit Chi/ndf");
                        hh1->setYaxisTitle("Number of Pixels");
                        chiDist[outerIdent].reset(hh1);

                        hh2 = new Histo2d("Chi2Map-"+std::to_string(outerIdent), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
                        hh2->setXaxisTitle("Column");
                        hh2->setYaxisTitle("Row");
                        hh2->setZaxisTitle("Chi2");
                        chi2Map[outerIdent].reset(hh2);     

                        hh2 = new Histo2d("StatusMap-"+std::to_string(outerIdent), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
                        hh2->setXaxisTitle("Column");
                        hh2->setYaxisTitle("Row");
                        hh2->setZaxisTitle("Fit Status");
                        statusMap[outerIdent].reset(hh2);

                        hh1 = new Histo1d("StatusDist-"+std::to_string(outerIdent), 11, -0.5, 10.5, typeid(this));
                        hh1->setXaxisTitle("Fit Status ");
                        hh1->setYaxisTitle("Number of Pixels");
                        statusDist[outerIdent].reset(hh1);

                        hh1 = new Histo1d("TimePerFitDist-"+std::to_string(outerIdent), 201, -1, 401, typeid(this));
                        hh1->setXaxisTitle("Fit Time [us]");
                        hh1->setYaxisTitle("Number of Pixels");
                        timeDist[outerIdent].reset(hh1);
                    }

                    double chi2= status.fnorm/(double)status.nfev;

                    if (par[0] > vcalMin && par[0] < vcalMax && par[1] > 0 && par[1] < (vcalMax-vcalMin) && par[1] >= 0 
                            && chi2 < 2.5 && chi2 > 1e-6) {
                        FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(bookie->getFe(channel));
                        thrMap[outerIdent]->setBin(bin, feCfg->toCharge(par[0], useScap, useLcap));
                        // Reudce effect of vcal offset on this, don't want to probe at low vcal
                        sigMap[outerIdent]->setBin(bin, feCfg->toCharge(par[0]+par[1], useScap, useLcap)-feCfg->toCharge(par[0], useScap, useLcap));
                        chiDist[outerIdent]->fill(status.fnorm/(double)status.nfev);
                        timeDist[outerIdent]->fill(fitTime.count());
                        chi2Map[outerIdent]->setBin(bin, chi2 );
                        statusMap[outerIdent]->setBin(bin, status.outcome);
                        statusDist[outerIdent]->fill(status.outcome);

                    } else {
                        n_failedfit++;
                           alog->debug("Failed fit Col({}) Row({}) Threshold({}) Chi2({}) Status({}) Entries({}) Mean({})", col, row, thrMap[outerIdent]->getBin(bin), chi2, status.outcome, histos[ident]->getEntries(), histos[ident]->getMean());
                    }
                    if (row == nRow/2 && col%10 == 0) {
                        output->pushData(std::move(histos[ident]));
                    }
                    histos[ident].reset(nullptr);
                }
            }
        }
    }

    // Finished full vcal loop, if feedback loop provide TDAC feedback
    // Requires odd number of TDAC steps, optimised for 3 iterations
    if (medCnt[medIdent] == n_count && fb != nullptr) {
        if (outerIdent == 0) {
            thrTarget = thrMap[outerIdent]->getMean();
        }

        if (step[outerIdent] == nullptr) {
            Histo2d *hh2 = new Histo2d("StepMap-" + std::to_string(outerIdent), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
            hh2->setXaxisTitle("Column");
            hh2->setYaxisTitle("Row");
            hh2->setZaxisTitle("TDAC change");
            step[outerIdent].reset(hh2);
        }
        
        if (deltaThr[outerIdent] == nullptr) {
            Histo2d *hh2 = new Histo2d("DeltaThreshold-" + std::to_string(outerIdent), nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
            hh2->setXaxisTitle("Column");
            hh2->setYaxisTitle("Row");
            hh2->setZaxisTitle("Delta Threshold [e]");
            deltaThr[outerIdent].reset(hh2);
        }

        for(unsigned col=1; col<=nCol; col++) {
            for (unsigned row=1; row<=nRow; row++) {
                unsigned bin = thrMap[outerIdent]->binNum(col, row);
                step[outerIdent]->setBin(bin, 0);
                if (thrMap[outerIdent]->getBin(bin) != 0) {
                    double curDeltaThr = thrTarget - thrMap[outerIdent]->getBin(bin);
                    if (outerIdent == 0) { // First time
                        if (curDeltaThr > 5) { // Increase Threshold
                            step[outerIdent]->setBin(bin, -1);
                        } else if (curDeltaThr < -5) { //Decrease Threshold
                            step[outerIdent]->setBin(bin, +1);
                        }
                    } else if (step[prevOuter]->getBin(bin) != 0) { // Second or higher loop
                        if (fabs(curDeltaThr) < fabs(deltaThr[prevOuter]->getBin(bin))) {
                            // Did the sign change?
                            if ( ((curDeltaThr < 0) ? -1 : (curDeltaThr > 0)) != ((deltaThr[prevOuter]->getBin(bin) < 0) ? -1 : (deltaThr[prevOuter]->getBin(bin) > 0)) ) {
                                step[outerIdent]->setBin(bin, 0); // Best setting
                            } else {
                                step[outerIdent]->setBin(bin, step[prevOuter]->getBin(bin)); // Move in the same direction
                            }
                        } else {
                            // Last setting was better
                            step[outerIdent]->setBin(bin, step[prevOuter]->getBin(bin)*-1);
                        }
                    }
                } 
                deltaThr[outerIdent]->setBin(bin, thrTarget - thrMap[outerIdent]->getBin(bin));
            }
        }
        prevOuter = outerIdent;
        alog->info("[{}] --> Sending feedback #{}", this->channel, outerIdent);
        fb->feedback(this->channel, step[outerIdent].get());
    }
}

void ScurveFitter::end() {

    alog->info("scurve end");
    if (fb != nullptr) {
        alog->info("[{}] Tuned to ==> {}", thrTarget, this->channel);
    }

    // TODO Loop over outerIdent
    for (unsigned i=0; i<thrMap.size(); i++) {
        if (thrMap[i] != NULL) {


            int bin_width, xlow, xhigh, bins;
            double thrMean = thrMap[i]->getMean();
            double thrRms = thrMap[i]->getStdDev();
            double sigMean = sigMap[i]->getMean();
            double sigRms = sigMap[i]->getStdDev();


            bin_width = 10;
            int rThrMean = (int)(thrMean) - (int)(thrMean)%bin_width;
            int rThrRms = (int)(thrRms) - (int)(thrRms)%bin_width;
            xlow = rThrMean-(rThrRms*5)-bin_width/2.0;
            if (xlow < 0) xlow = -1*bin_width/2.0;
            xhigh = rThrMean+(rThrRms*5)+bin_width/2.0;
            if ((xhigh-xlow)%bin_width != 0)
                xhigh += ((xhigh-xlow)%bin_width);
            bins = (xhigh-xlow)/bin_width;

            Histo1d *hh1 = new Histo1d("ThresholdDist-" + std::to_string(i), bins, xlow, xhigh, typeid(this));
            hh1->setXaxisTitle("Threshold [e]");
            hh1->setYaxisTitle("Number of Pixels");
            thrDist[i].reset(hh1);

            bin_width = 5;
            int rSigMean = (int)(sigMean) - (int)(sigMean)%bin_width;
            int rSigRms = (int)(sigRms) - (int)(sigRms)%bin_width;
            xlow = rSigMean-(rSigRms*5)-bin_width/2.0;
            if (xlow < 0) xlow = -1*bin_width/2.0;
            xhigh = rSigMean+(rSigRms*5)+bin_width/2.0;
            if ((xhigh-xlow)%bin_width != 0)
                xhigh += ((xhigh-xlow)%bin_width);
            bins = (xhigh-xlow)/bin_width;

            hh1 = new Histo1d("NoiseDist-" + std::to_string(i), bins, xlow, xhigh, typeid(this));
            hh1->setXaxisTitle("Noise [e]");
            hh1->setYaxisTitle("Number of Pixels");
            sigDist[i].reset(hh1);

            for(unsigned bin=0; bin<(nCol*nRow); bin++) {
                if (thrMap[i]->getBin(bin) != 0)
                    thrDist[i]->fill(thrMap[i]->getBin(bin));
                if (sigMap[i]->getBin(bin) != 0)
                    sigDist[i]->fill(sigMap[i]->getBin(bin));
            }

            // Before moving data to clipboard
            alog->info("\033[1;33m[{}][{}] Threshold Mean = {} +- {}\033[0m", channel, i, thrMap[i]->getMean(), thrMap[i]->getStdDev());
            alog->info("\033[1;33m[{}][{}] Noise Mean = {} +- {}\033[0m", channel, i, sigMap[i]->getMean(), sigMap[i]->getStdDev());
            alog->info("\033[1;33m[{}][{}] Number of failed fits = {}\033[0m", channel, i, n_failedfit);
            output->pushData(std::move(sCurve[i]));
            output->pushData(std::move(thrDist[i]));
            output->pushData(std::move(thrMap[i]));
            output->pushData(std::move(sigDist[i]));
            output->pushData(std::move(chi2Map[i]));
            output->pushData(std::move(statusMap[i]));
            output->pushData(std::move(statusDist[i]));
            output->pushData(std::move(step[i]));
            output->pushData(std::move(deltaThr[i]));
        }

        output->pushData(std::move(sigMap[i]));
        output->pushData(std::move(chiDist[i]));
        output->pushData(std::move(timeDist[i]));
    }


}

void OccGlobalThresholdTune::init(ScanBase *s) {
    std::shared_ptr<LoopActionBase> tmpVthinFb(new Fei4GlobalFeedback(&Fei4::Vthin_Fine));
    std::shared_ptr<LoopActionBase> tmpVthinFb2(new Fe65p2GlobalFeedback(&Fe65p2::Vthin1Dac));
    std::shared_ptr<LoopActionBase> tmpVthinFb3(new Rd53aGlobalFeedback());
    n_count = 1;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
                    l->type() != typeid(Rd53aMaskLoop*) &&
                    l->type() != typeid(Rd53aTriggerLoop*) &&
                    l->type() != typeid(Rd53aCoreColLoop*) &&
                    l->type() != typeid(Fe65p2TriggerLoop*) &&
                    l->type() != typeid(Fei4MaskLoop*) &&
                    l->type() != typeid(Fe65p2MaskLoop*) &&
                    l->type() != typeid(StdDataLoop*) &&
                    l->type() != typeid(Fe65p2QcLoop*) &&
                    l->type() != typeid(Fei4DcLoop*))) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }

        if (l->type() == typeid(Fei4TriggerLoop*)) {
            Fei4TriggerLoop *trigLoop = (Fei4TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == typeid(Fe65p2TriggerLoop*)) {
            Fe65p2TriggerLoop *trigLoop = (Fe65p2TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == typeid(Rd53aTriggerLoop*)) {
            Rd53aTriggerLoop *trigLoop = (Rd53aTriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == tmpVthinFb->type() 
                || l->type() == tmpVthinFb2->type()
                || l->type() == tmpVthinFb3->type()) {
            fb = dynamic_cast<GlobalFeedbackBase*>(l.get()); 
            lb = (LoopActionBase*) l.get(); 
        }
    }

}

void OccGlobalThresholdTune::processHistogram(HistogramBase *h) {
    // Check if right Histogram
    if (h->getType() != typeid(OccupancyMap*))
        return;

    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;

    // Determine identifier
    std::string name = "OccupancyMap";
    std::string name2 = "OccupancyDist";
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        name += "-" + std::to_string(h->getStat().get(loops[n]));
        name2 += "-" + std::to_string(h->getStat().get(loops[n]));
    }

    // Check if Histogram exists
    if (occMaps[ident] == NULL) {
        Histo2d *hh = new Histo2d(name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("Hits");
        occMaps[ident].reset(hh);
        //Histo1d *hhh = new Histo1d(name2, injections+1, -0.5, injections+0.5, typeid(this));
        // Ignore first and last bin to dismiss masked or not functioning pixels
        Histo1d *hhh = new Histo1d(name2, injections-1, 0.5, injections-0.5, typeid(this));
        hhh->setXaxisTitle("Occupancy");
        hhh->setYaxisTitle("Number of Pixels");
        occDists[ident].reset(hhh);
        innerCnt[ident] = 0;
    }

    // Add up Histograms
    occMaps[ident]->add(*(Histo2d*)h);
    innerCnt[ident]++;

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {

        for(unsigned i=0; i<occMaps[ident]->size(); i++)
            occDists[ident]->fill(occMaps[ident]->getBin(i));

        bool done = false;
        double sign = 0;
        if (lb->getStep() == 1) {
            done = true;
        }

        double meanOcc = occDists[ident]->getMean()/(double)injections;
        double entries = occDists[ident]->getEntries();
        alog->info("[{}] Mean Occupancy = {}", channel, meanOcc);

        if (entries < (nCol*nRow)*0.005) { // Want at least 1% of all pixels to fire
            sign = -1;
        } else if ((meanOcc > 0.51) && !done) {
            sign = +1;
        } else if ((meanOcc < 0.49) && !done) {
            sign = -1;
        } else {
            sign = 0;
            done = true;
        }

        fb->feedback(this->channel, sign, done);
        output->pushData(std::move(occMaps[ident]));
        output->pushData(std::move(occDists[ident]));
        innerCnt[ident] = 0;
        //delete occMaps[ident];
        occMaps[ident] = nullptr;
        //delete occDists[ident];
        occDists[ident] = nullptr;
    }

}

void OccPixelThresholdTune::init(ScanBase *s) {
    n_count = 1;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
                    l->type() != typeid(Rd53aMaskLoop*) &&
                    l->type() != typeid(Rd53aTriggerLoop*) &&
                    l->type() != typeid(Rd53aCoreColLoop*) &&
                    l->type() != typeid(Fe65p2TriggerLoop*) &&
                    l->type() != typeid(Fei4MaskLoop*) &&
                    l->type() != typeid(Fe65p2MaskLoop*) &&
                    l->type() != typeid(StdDataLoop*) &&
                    l->type() != typeid(Fe65p2QcLoop*) &&
                    l->type() != typeid(Fei4DcLoop*))) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }

        if (l->type() == typeid(Fei4TriggerLoop*)) {
            Fei4TriggerLoop *trigLoop = (Fei4TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == typeid(Fe65p2TriggerLoop*)) {
            Fe65p2TriggerLoop *trigLoop = (Fe65p2TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == typeid(Rd53aTriggerLoop*)) {
            Rd53aTriggerLoop *trigLoop = (Rd53aTriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == typeid(Fei4PixelFeedback*)) {
            fb = (PixelFeedbackBase*)((Fei4PixelFeedback*) l.get());  
        }
        if (l->type() == typeid(Fe65p2PixelFeedback*)) {
            fb = (PixelFeedbackBase*)((Fe65p2PixelFeedback*) l.get());  
        }
        if (l->type() == typeid(Rd53aPixelFeedback*)) {
            fb = (PixelFeedbackBase*)((Rd53aPixelFeedback*) l.get());  
        }
    }

}

void OccPixelThresholdTune::processHistogram(HistogramBase *h) {
    // Check if right Histogram
    if (h->getType() != typeid(OccupancyMap*))
        return;

    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;

    // Determine identifier
    std::string name = "OccupancyMap";
    std::string name2 = "OccupancyDist";
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        name += "-" + std::to_string(h->getStat().get(loops[n]));
        name2 += "-" + std::to_string(h->getStat().get(loops[n]));
    }

    // Check if Histogram exists
    if (occMaps[ident] == NULL) {
        Histo2d *hh = new Histo2d(name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("Hits");
        occMaps[ident].reset(hh);
    }

    // Add up Histograms
    occMaps[ident]->add(*(Histo2d*)h);
    innerCnt[ident]++;

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        double mean = 0;
        Histo2d *fbHisto = new Histo2d("feedback", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        std::unique_ptr<Histo1d> occDist(new Histo1d(name2, injections-1, 0.5, injections-0.5, typeid(this)));
        occDist->setXaxisTitle("Occupancy");
        occDist->setYaxisTitle("Number of Pixels");
        for (unsigned i=0; i<fbHisto->size(); i++) {
            double occ = occMaps[ident]->getBin(i);
            if ((occ/(double)injections) > 0.7) {
                fbHisto->setBin(i, -1);
            } else if ((occ/(double)injections) < 0.3) {
                fbHisto->setBin(i, +1);
            } else {
                fbHisto->setBin(i, 0);
            }
            mean += occMaps[ident]->getBin(i);
            occDist->fill(occMaps[ident]->getBin(i));
        }
        alog->info("[{}] Mean Occupancy = {}", channel, mean/(nCol*nRow*(double)injections));
        alog->info("[{}] RMS = {}", channel, occDist->getStdDev());

        fb->feedback(this->channel, fbHisto);
        output->pushData(std::move(occMaps[ident]));
        output->pushData(std::move(occDist));
        innerCnt[ident] = 0;
        //delete occMaps[ident];
        occMaps[ident] = NULL;
    }

}

// TODO exclude every loop
void L1Analysis::init(ScanBase *s) {
    n_count = 1;
    injections = 0;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
                    l->type() != typeid(Rd53aMaskLoop*) &&
                    l->type() != typeid(Rd53aTriggerLoop*) &&
                    l->type() != typeid(Rd53aCoreColLoop*) &&
                    l->type() != typeid(Fei4MaskLoop*) &&
                    l->type() != typeid(StdDataLoop*) &&
                    l->type() != typeid(Fei4DcLoop*)) &&
                l->type() != typeid(Fe65p2MaskLoop*) &&
                l->type() != typeid(Fe65p2QcLoop*) &&
                l->type() != typeid(Fe65p2TriggerLoop*)) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }
        if (l->type() == typeid(Fei4TriggerLoop*)) {
            Fei4TriggerLoop *trigLoop = (Fei4TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == typeid(Fe65p2TriggerLoop*)) {
            Fe65p2TriggerLoop *trigLoop = (Fe65p2TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }

        if (l->type() == typeid(Rd53aTriggerLoop*)) {
            Rd53aTriggerLoop *trigLoop = (Rd53aTriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
    }
}

void L1Analysis::processHistogram(HistogramBase *h) {
    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;

    // Determine identifier
    std::string name = "L1Dist";
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        name += "-" + std::to_string(h->getStat().get(loops[n]));
    }

    // Check if Histogram exists
    if (l1Histos[ident] == NULL) {
        Histo1d *hh = new Histo1d(name, 16, -0.5, 15.5, typeid(this));
        hh->setXaxisTitle("L1Id");
        hh->setYaxisTitle("Hits");
        l1Histos[ident].reset(hh);
        innerCnt[ident] = 0;
    }

    // Add up Histograms
    if (h->getType() == typeid(L1Dist*)) {
        l1Histos[ident]->add(*(Histo1d*)h);
        innerCnt[ident]++;
    } else {
        return;
    }

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        output->pushData(std::move(l1Histos[ident]));
        innerCnt[ident] = 0;
    }
}

void L1Analysis::end() {
}

void TotDistPlotter::init(ScanBase *s) {
    n_count = 1;
    injections = 0;
    std::shared_ptr<LoopActionBase> tmpVcalLoop(new Fei4ParameterLoop(&Fei4::PlsrDAC));
    std::shared_ptr<LoopActionBase> tmpVcalLoop2(new Fe65p2ParameterLoop(&Fe65p2::PlsrDac));
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
                    l->type() != typeid(Rd53aMaskLoop*) &&
                    l->type() != typeid(Rd53aTriggerLoop*) &&
                    l->type() != typeid(Rd53aCoreColLoop*) &&
                    l->type() != typeid(Fei4MaskLoop*) &&
                    l->type() != typeid(StdDataLoop*) &&
                    l->type() != typeid(Fei4DcLoop*)) &&
                l->type() != typeid(Fe65p2MaskLoop*) &&
                l->type() != typeid(Fe65p2QcLoop*) &&
                l->type() != typeid(Fe65p2TriggerLoop*) &&
                l->type() != tmpVcalLoop->type() &&
                l->type() != tmpVcalLoop2->type()) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }
        if (l->type() == typeid(Fei4TriggerLoop*)) {
            Fei4TriggerLoop *trigLoop = (Fei4TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
        if (l->type() == typeid(Fe65p2TriggerLoop*)) {
            Fe65p2TriggerLoop *trigLoop = (Fe65p2TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
    }
}

void TotDistPlotter::processHistogram(HistogramBase *h) {
    // Check if right Histogram
    if (h->getType() != typeid(TotDist*))
        return;

    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;

    // Determine identifier
    std::string name = "TotDist";
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        name += "-" + std::to_string(h->getStat().get(loops[n]));
    }

    // Check if Histogram exists
    if (tot[ident] == NULL) {
        Histo1d *hh = new Histo1d(name, 16, 0.5, 16.5, typeid(this));
        hh->setXaxisTitle("ToT [bc]");
        hh->setYaxisTitle("Hits");
        tot[ident].reset(hh);
        innerCnt[ident] = 0;
    }

    // Add up Histograms
    tot[ident]->add(*(Histo1d*)h);
    innerCnt[ident]++;

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        output->pushData(std::move(tot[ident]));
    }
}

void NoiseAnalysis::init(ScanBase *s) {
    // We assume the nosie scan only has one trigger and data loop
    occ.reset(new Histo2d("Occupancy", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
    occ->setXaxisTitle("Col");
    occ->setYaxisTitle("Row");
    occ->setZaxisTitle("Hits");
    n_trigger = 0;
}

void NoiseAnalysis::processHistogram(HistogramBase *h) {
    if (h->getType() == typeid(OccupancyMap*)) {
        occ->add(*(Histo2d*)h);
    } else if (h->getType() == typeid(HitsPerEvent*)) {
        n_trigger += ((Histo1d*)h)->getEntries();       
    }
}

void NoiseAnalysis::loadConfig(json &j){
    if (!j["createMask"].empty()){
        createMask=j["createMask"];
		//std::cout << "createMask = " << createMask << std::endl;
    }
}

void NoiseAnalysis::end() {
    std::unique_ptr<Histo2d> noiseOcc(new Histo2d("NoiseOccupancy", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
    noiseOcc->setXaxisTitle("Col");
    noiseOcc->setYaxisTitle("Row");
    noiseOcc->setZaxisTitle("Noise Occupancy hits/bc");

    std::unique_ptr<Histo2d> mask(new Histo2d("NoiseMask", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
    mask->setXaxisTitle("Col");
    mask->setYaxisTitle("Row");
    mask->setZaxisTitle("Mask");

    noiseOcc->add(&*occ);
    noiseOcc->scale(1.0/(double)n_trigger);
    alog->info("[{}] Received {} total trigger!", channel, n_trigger);
    double noiseThr = 1e-6; 
    for (unsigned i=0; i<noiseOcc->size(); i++) {
        if (noiseOcc->getBin(i) > noiseThr) {
            mask->setBin(i, 0);
            if (make_mask&&createMask) {
                bookie->getFe(channel)->maskPixel((i/nRow), (i%nRow));
            }
        } else {
            mask->setBin(i, 1);
        }
    }

    output->pushData(std::move(occ));
    output->pushData(std::move(noiseOcc));
    output->pushData(std::move(mask));
}

void NoiseTuning::init(ScanBase *s) {
    n_count = 1;
    pixelFb = NULL;
    globalFb = NULL;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
                    l->type() != typeid(Rd53aMaskLoop*) &&
                    l->type() != typeid(Rd53aTriggerLoop*) &&
                    l->type() != typeid(Rd53aCoreColLoop*) &&
                    l->type() != typeid(Fei4MaskLoop*) &&
                    l->type() != typeid(StdDataLoop*) &&
                    l->type() != typeid(Fei4DcLoop*)) &&
                l->type() != typeid(Fe65p2MaskLoop*) &&
                l->type() != typeid(Fe65p2QcLoop*) &&
                l->type() != typeid(Fe65p2TriggerLoop*) &&
                l->type() != typeid(StdRepeater*)) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }

        std::shared_ptr<LoopActionBase> tmpPrmpFb(new Fei4GlobalFeedback(&Fei4::PrmpVbpf));
        if (l->type() == tmpPrmpFb->type()) {
            globalFb = dynamic_cast<GlobalFeedbackBase*>(l.get());  
        }

        if (l->type() == typeid(Rd53aGlobalFeedback*)) {
            globalFb = dynamic_cast<GlobalFeedbackBase*>(l.get());  
        }

        if (l->type() == typeid(Fei4PixelFeedback*)) {
            pixelFb = dynamic_cast<PixelFeedbackBase*>(l.get());  
        }

        if (l->type() == typeid(Rd53aPixelFeedback*)) {
            pixelFb = dynamic_cast<PixelFeedbackBase*>(l.get());  
        }
    }
}

void NoiseTuning::processHistogram(HistogramBase *h) {
    if (!(h->getType() == typeid(OccupancyMap*)))
        return;

    // Select correct output container
    unsigned ident = 0;
    unsigned offset = 0;

    // Determine identifier
    std::string name = "OccMap";
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        name += "-" + std::to_string(h->getStat().get(loops[n]));
    }


    if (occMaps[ident] == NULL) {
        Histo2d *hh = new Histo2d(name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("Hits");
        innerCnt[ident] = 0;
        occMaps[ident].reset(hh);
    }

    //Easy make it pretty
    occMaps[ident]->add(&*(Histo2d*)h);
    innerCnt[ident]++;

    if (innerCnt[ident] == n_count) {
        SPDLOG_LOGGER_TRACE(alog, "");
        if (globalFb != NULL) { // Global Threshold Tuning
            SPDLOG_LOGGER_TRACE(alog, "");
            unsigned numOfHits = 0;
            for (unsigned i=0; i<occMaps[ident]->size(); i++) {
                if (occMaps[ident]->getBin(i) > 1) {
                    numOfHits++;
                }
            }
            alog->info("[{}] Number of pixels with hits: {}", channel, numOfHits);
            if (numOfHits < 10) { // TODO not hardcode this value
                globalFb->feedbackStep(channel, -1, false);
            } else {
                globalFb->feedbackStep(channel, 0, true);
            }
        }

        if (pixelFb != NULL) { // Pixel Threshold Tuning
            Histo2d *fbHisto = new Histo2d("feedback", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
            SPDLOG_LOGGER_TRACE(alog, "");
            unsigned pixelWoHits = 0;
            for (unsigned i=0; i<occMaps[ident]->size(); i++) {
                if (occMaps[ident]->getBin(i) < 2) { //TODO un-hardcode this
                    fbHisto->setBin(i, -1);
                    pixelWoHits++;
                } else {
                    fbHisto->setBin(i, 0);
                }
            }
            alog->info("[{}] Number of pixels with hits: {}", channel, pixelWoHits);

            pixelFb->feedbackStep(channel, fbHisto);
        }
        output->pushData(std::move(occMaps[ident]));
        occMaps[ident] = NULL;
    }
}

void NoiseTuning::end() {
}

void DelayAnalysis::init(ScanBase *s) {
    std::shared_ptr<LoopActionBase> tmpVcalLoop(new Rd53aParameterLoop());
    scan = s;
    n_count = nCol*nRow;
    injections = 50;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if (l->type() != typeid(Fei4TriggerLoop*) &&
                l->type() != typeid(Rd53aMaskLoop*) &&
                l->type() != typeid(Rd53aTriggerLoop*) &&
                l->type() != typeid(Rd53aCoreColLoop*) &&
                l->type() != typeid(Fe65p2TriggerLoop*) &&
                l->type() != typeid(Fei4MaskLoop*) &&
                l->type() != typeid(Fe65p2MaskLoop*) &&
                l->type() != typeid(StdDataLoop*) &&
                l->type() != typeid(Fei4DcLoop*) &&
                l->type() != typeid(Fe65p2QcLoop*) &&
                l->type() != tmpVcalLoop->type()) {
            loops.push_back(n);
            loopMax.push_back((unsigned)l->getMax());
        } else {
            unsigned cnt = (l->getMax() - l->getMin())/l->getStep();
            if (cnt == 0)
                cnt = 1;
            n_count = n_count*cnt;
        }

        // Vcal Loop
        if (l->type() == tmpVcalLoop->type() ) {
            delayLoop = n;
            delayMax = l->getMax();
            delayMin = l->getMin();
            delayStep = l->getStep();
        }

        if (l->type() == typeid(Fei4TriggerLoop*)) {
            Fei4TriggerLoop *trigLoop = (Fei4TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
        if (l->type() == typeid(Fe65p2TriggerLoop*)) {
            Fe65p2TriggerLoop *trigLoop = (Fe65p2TriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
        if (l->type() == typeid(Rd53aTriggerLoop*)) {
            Rd53aTriggerLoop *trigLoop = (Rd53aTriggerLoop*) l.get();
            injections = trigLoop->getTrigCnt();
        }
    }
    count = 0;
}

void DelayAnalysis::processHistogram(HistogramBase *h) {
    // Check if right Histogram
    if (h->getType() != typeid(L13d*))
        return;


    Histo3d *hh = (Histo3d*) h;
    for(unsigned l1=0; l1<16; l1++) { //TODO hardcoded l1
        for(unsigned col=1; col<=nCol; col++) {
            for (unsigned row=1; row<=nRow; row++) {
                long unsigned bin = hh->binNum(col, row, l1);
                if (hh->getBin(bin) != 0) {
                    //std::cout << col << " " << row << " " << l1 << " " << bin << std::endl;
                    // Select correct output containe
                    unsigned ident = (row-1)+((col-1)*(nRow));
                    unsigned delay = hh->getStat().get(delayLoop);
                    // Determine identifier
                    std::string name = "Delay";
                    name += "-" + std::to_string(col) + "-" + std::to_string(row);
                    // Check for other loops
                    /*
                       unsigned outerIdent = 0;
                       unsigned offset = nCol*nRow;
                       unsigned outerOffset = 0;
                       for (unsigned n=0; n<loops.size(); n++) {
                       ident += hh->getStat().get(loops[n])+offset;
                       outerIdent += hh->getStat().get(loops[n])+offset;
                       offset += loopMax[n];
                       outerOffset += loopMax[n];
                       name += "-" + std::to_string(hh->getStat().get(loops[n]));
                       }*/

                    // Check if Histogram exists
                    if (histos[ident] == NULL) {
                        Histo1d *hhh = new Histo1d(name, 256, -0.5, 255.5, typeid(this)); // TODO hardcoded
                        hhh->setXaxisTitle("Delay");
                        hhh->setYaxisTitle("Occupancy");
                        histos[ident].reset(hhh);
                        innerCnt[ident] = 0;
                        count++;
                    }

                    // Add up Histograms
                    histos[ident]->fill((16*l1)+delay, hh->getBin(bin));
                    innerCnt[ident]++;

                    // Got all data, finish up Analysis
                    if (delay == delayMax) { // TODO hardcoded
                        if (delayMap == nullptr) {
                            delayMap.reset(new Histo2d("DelayMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
                            delayMap->setXaxisTitle("Col");
                            delayMap->setYaxisTitle("Row");
                            delayMap->setZaxisTitle("Mean Delay");
                        }
                        if (rmsMap == nullptr) {
                            rmsMap.reset(new Histo2d("RmsMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this)));
                            rmsMap->setXaxisTitle("Col");
                            rmsMap->setYaxisTitle("Row");
                            rmsMap->setZaxisTitle("RMS");
                        }
                        if (histos[ident]->getMean() > 0 && histos[ident]->getMean() < 256) {
                            delayMap->setBin(ident, histos[ident]->getMean());
                            rmsMap->setBin(ident, histos[ident]->getStdDev());
                        }
                    }
                }
            }
        }
    }
}

void DelayAnalysis::end() {
    output->pushData(std::move(delayMap));
    output->pushData(std::move(rmsMap));
    for(unsigned bin=0; bin<(nRow*nCol); bin++) {
        if (histos[bin]) {
            output->pushData(std::move(histos[bin]));
            bin+=1000;
        }
    }
}
