// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Analysis Base class
// # Comment: 
// ################################

#include "Fei4Analysis.h"

Fei4Analysis::Fei4Analysis() {

}

Fei4Analysis::Fei4Analysis(Bookkeeper *b, unsigned ch) {
    bookie = b;
    channel = ch;
}

Fei4Analysis::~Fei4Analysis() {
    for (unsigned i=0; i<algorithms.size(); i++) {
        delete algorithms[i];
    }
}

void Fei4Analysis::init() {
    for (unsigned i=0; i<algorithms.size(); i++) {
        algorithms[i]->connect(output);
        algorithms[i]->init(scan);
    }
}

void Fei4Analysis::process() {
    while(!input->empty()) {
        HistogramBase *h = input->popData();
        if (h != NULL) {
            for (unsigned i=0; i<algorithms.size(); i++) {
                algorithms[i]->processHistogram(h);
            }
            delete h;
        }
    }
}

void Fei4Analysis::end() {
    for (unsigned i=0; i<algorithms.size(); i++) {
        algorithms[i]->end();
    }
}

void Fei4Analysis::addAlgorithm(AnalysisAlgorithm *a) {
    algorithms.push_back(a);
    a->setBookkeeper(bookie);
    a->setChannel(channel);
}

void Fei4Analysis::addAlgorithm(AnalysisAlgorithm *a, unsigned ch) {
    algorithms.push_back(a);
}

void Fei4Analysis::plot(std::string basename, std::string dir) {
    if (output->empty())
        return;
    for (std::deque<HistogramBase*>::iterator it = output->begin(); it != output->end(); ++it) {
        std::cout << "Plotting : " << (*it)->getName() << std::endl;
        (*it)->plot(basename, dir);
    }
}

void Fei4Analysis::toFile(std::string basename, std::string dir) {
    if (output->empty())
        return;
    for (std::deque<HistogramBase*>::iterator it = output->begin(); it != output->end(); ++it) {
        std::cout << "Saving : " << (*it)->getName() << std::endl;
        (*it)->toFile(basename, dir, true);
    }
}


void OccupancyAnalysis::init(ScanBase *s) {
    n_count = 1;
    injections = 0;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
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
    if (occMaps[ident] == NULL) {
        Histo2d *hh = new Histo2d(name, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("Hits");
        occMaps[ident] = hh;
        innerCnt[ident] = 0;
    }

    // Add up Histograms
    occMaps[ident]->add(*(Histo2d*)h);
    innerCnt[ident]++;

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        Histo2d *mask = new Histo2d(name2, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        mask->setXaxisTitle("Column");
        mask->setYaxisTitle("Rows");
        mask->setZaxisTitle("Enable");


        for (unsigned i=0; i<occMaps[ident]->size(); i++) {
            if (occMaps[ident]->getBin(i) == injections) {
                mask->setBin(i, 1);
            } else {
                if (make_mask) {
                    if(dynamic_cast<Fei4*>(bookie->getFe(channel))) {;
                        Fei4 *fe = dynamic_cast<Fei4*>(bookie->getFe(channel));
                        fe->setEn((i/nRow)+1, (i%nRow)+1, 0);
                        fe->setHitbus((i/nRow)+1, (i%nRow)+1, 1);
                    }
                }
            }
        }
        output->pushData(mask); // TODO push this mask to the specific configuration
        output->pushData(occMaps[ident]);


        //delete occMaps[ident];
        //occMaps[ident] = NULL;
    }
}

void TotAnalysis::init(ScanBase *s) {
    n_count = 1;
    injections = 1;
    pixelFb = NULL;
    globalFb = NULL;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
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

        std::shared_ptr<LoopActionBase> tmpPrmpFb(Fei4GlobalFeedbackBuilder(&Fei4::PrmpVbpf));
        if (l->type() == tmpPrmpFb->type()) {
            globalFb = dynamic_cast<GlobalFeedbackBase*>(l.get());  
        }

        if (l->type() == typeid(Fei4PixelFeedback*)) {
            pixelFb = dynamic_cast<PixelFeedbackBase*>(l.get());  
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
        occMaps[ident] = hh;
        occInnerCnt[ident] = 0;
        hh = new Histo2d(name2, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("{/Symbol S}(ToT)");
        totMaps[ident] = hh;
        totInnerCnt[ident] = 0;
        hh = new Histo2d(name3, nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        hh->setXaxisTitle("Column");
        hh->setYaxisTitle("Row");
        hh->setZaxisTitle("{/Symbol S}(ToT^2)");
        tot2Maps[ident] = hh;
        tot2InnerCnt[ident] = 0;
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
        Histo2d *meanTotMap = new Histo2d("MeanTotMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        meanTotMap->setXaxisTitle("Column");
        meanTotMap->setYaxisTitle("Row");
        meanTotMap->setZaxisTitle("Mean ToT [bc]");
        Histo2d *sumTotMap = new Histo2d("SumTotMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        sumTotMap->setXaxisTitle("Column");
        sumTotMap->setYaxisTitle("Row");
        sumTotMap->setZaxisTitle("Mean ToT [bc]");
        Histo2d *sumTot2Map = new Histo2d("MeanTot2Map", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        sumTot2Map->setXaxisTitle("Column");
        sumTot2Map->setYaxisTitle("Row");
        sumTot2Map->setZaxisTitle("Mean ToT^2 [bc^2]");
        Histo2d *sigmaTotMap = new Histo2d("SigmaTotMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        sigmaTotMap->setXaxisTitle("Column");
        sigmaTotMap->setYaxisTitle("Row");
        sigmaTotMap->setZaxisTitle("Sigma ToT [bc]");
        Histo1d *meanTotDist = new Histo1d("MeanTotDist_"+std::to_string(ident), 161, -0.05, 16.05, typeid(this));
        meanTotDist->setXaxisTitle("Mean ToT [bc]");
        meanTotDist->setYaxisTitle("Number of Pixels");
        Histo1d *sigmaTotDist = new Histo1d("SigmaTotDist", 101, -0.05, 1.05, typeid(this));
        sigmaTotDist->setXaxisTitle("Sigma ToT [bc]");
        sigmaTotDist->setYaxisTitle("Number of Pixels");

        meanTotMap->add(*totMaps[ident]);
        meanTotMap->divide(*occMaps[ident]);
        sumTotMap->add(*totMaps[ident]);
        sumTot2Map->add(*tot2Maps[ident]);
        for(unsigned i=0; i<meanTotMap->size(); i++) {
            double sigma = sqrt(fabs((sumTot2Map->getBin(i) - ((sumTotMap->getBin(i)*sumTotMap->getBin(i))/injections))/(injections-1)));
            sigmaTotMap->setBin(i, sigma);
            meanTotDist->fill(meanTotMap->getBin(i));
            sigmaTotDist->fill(sigma);
        }

        std::cout << "[" << channel << "] ToT Mean = " << meanTotDist->getMean() << " +- " << meanTotDist->getStdDev() << std::endl;

        if (globalFb != NULL) {
            double mean = 0;
            for (unsigned i=0; i<meanTotMap->size(); i++)
                mean += meanTotMap->getBin(i);
            mean = mean/(double)meanTotMap->size();
            std::cout << "Mean is: " << mean << std::endl;

            // TODO Get this from somewhere
            double targetTot = 10.0;
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

        output->pushData(meanTotMap);
        output->pushData(sigmaTotMap);
        output->pushData(meanTotDist);
        output->pushData(sigmaTotDist);
        delete sumTot2Map;
        delete sumTotMap;

        delete occMaps[ident];
        delete totMaps[ident];
        delete tot2Maps[ident];
        occInnerCnt[ident] = 0;
        totInnerCnt[ident] = 0;
        tot2InnerCnt[ident] = 0;
    }
}

void ScurveFitter::init(ScanBase *s) {
    std::shared_ptr<LoopActionBase> tmpVcalLoop(Fei4ParameterLoopBuilder(&Fei4::PlsrDAC));
    std::shared_ptr<LoopActionBase> tmpVcalLoop2(new Fe65p2ParameterLoop(&Fe65p2::PlsrDac));
    scan = s;
    n_count = nCol*nRow;
    vcalLoop = 0;
    injections = 50;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if (l->type() != typeid(Fei4TriggerLoop*) &&
                l->type() != typeid(Fe65p2TriggerLoop*) &&
                l->type() != typeid(Fei4MaskLoop*) &&
                l->type() != typeid(Fe65p2MaskLoop*) &&
                l->type() != typeid(StdDataLoop*) &&
                l->type() != typeid(Fei4DcLoop*) &&
                l->type() != typeid(Fe65p2QcLoop*) &&
                l->type() != tmpVcalLoop2->type() &&
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
        if (l->type() == tmpVcalLoop->type() ||
                l->type() == tmpVcalLoop2->type()) {
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
    }

    for (unsigned i=vcalMin; i<=vcalMax; i+=vcalStep) {
        x.push_back(i);
    }
    cnt = 0;

}

// Errorfunction
// par[0] = Mean
// par[1] = Sigma
// par[2] = Normlization
#define SQRT2 1.414213562
double scurveFct(double x, const double *par) {
    return 0.5*(2-erfc((x-par[0])/(par[1]*SQRT2)))*par[2];
}

void ScurveFitter::processHistogram(HistogramBase *h) {
    cnt++;
    // Check if right Histogram
    if (h->getType() != typeid(OccupancyMap*))
        return;

    Histo2d *hh = (Histo2d*) h;
    for(unsigned col=1; col<=nCol; col++) {
        for (unsigned row=1; row<=nRow; row++) {
            unsigned bin = hh->binNum(col, row);
            if (hh->getBin(bin) != 0) {
                // Select correct output container
                unsigned ident = bin;
                unsigned outerIdent = 0;
                unsigned offset = nCol*nRow;
                unsigned outerOffset = 0;
                unsigned vcal = hh->getStat().get(vcalLoop);
                //std::cout << "VCAL = " << vcal << std::endl;
                // Determine identifier
                std::string name = "Scurve";
                name += "-" + std::to_string(bin);
                // Check for other loops
                for (unsigned n=0; n<loops.size(); n++) {
                    ident += hh->getStat().get(loops[n])+offset;
                    outerIdent += hh->getStat().get(loops[n])+offset;
                    offset += loopMax[n];
                    outerOffset += loopMax[n];
                    name += "-" + std::to_string(hh->getStat().get(loops[n]));
                }

                // Check if Histogram exists
                if (histos[ident] == NULL) {
                    Histo1d *hhh = new Histo1d(name, vcalBins+1, vcalMin-((double)vcalStep/2.0), vcalMax+((double)vcalStep/2.0), typeid(this));
                    hhh->setXaxisTitle("Vcal");
                    hhh->setYaxisTitle("Occupancy");
                    histos[ident] = hhh;
                    innerCnt[ident] = 0;
                }

                // Add up Histograms
                histos[ident]->fill(vcal, hh->getBin(bin));
                innerCnt[ident]++;

                // Got all data, finish up Analysis
                // TODO This requires the loop to run from low to high and a hit in the last bin
                if (vcal == vcalMax) {
                    // Scale histos
                    //histos[ident]->scale(1.0/(double)injections);

                    lm_status_struct status;
                    lm_control_struct control;
                    control = lm_control_float;
                    control.verbosity = 0;
                    const unsigned n_par = 3;
                    double par[n_par] = {50, 5, (double) injections};
                    std::chrono::high_resolution_clock::time_point start;
                    std::chrono::high_resolution_clock::time_point end;
                    start = std::chrono::high_resolution_clock::now();
                    lmcurve(n_par, par, vcalBins, &x[0], histos[ident]->getData(), scurveFct, &control, &status);
                    end = std::chrono::high_resolution_clock::now();
                    std::chrono::microseconds fitTime = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
                    if (thrMap[outerIdent] == NULL) {
                        Histo2d *hh2 = new Histo2d("ThresholdMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
                        hh2->setXaxisTitle("Column");
                        hh2->setYaxisTitle("Row");
                        hh2->setZaxisTitle("Threshold [e]");
                        thrMap[outerIdent] = hh2;
                        hh2 = new Histo2d("NoiseMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
                        hh2->setXaxisTitle("Column");
                        hh2->setYaxisTitle("Row");
                        hh2->setZaxisTitle("Noise [e]");
                        sigMap[outerIdent] = hh2;
                        
                        int bin_width = 10;
                        int xlow = bookie->getTargetThreshold()-1000-bin_width/2;
                        if (xlow < 0) xlow = -1*bin_width/2;
                        int xhigh = bookie->getTargetThreshold()+1000+bin_width/2;
                        if ((xhigh-xlow)%bin_width != 0)
                            xhigh += ((xhigh-xlow)%bin_width);
                        int bins = (xhigh-xlow)/bin_width;
                        
                        Histo1d *hh1 = new Histo1d("ThresholdDist", bins, xlow, xhigh, typeid(this));
                        //Histo1d *hh1 = new Histo1d("ThresholdDist", 201, bookie->getTargetThreshold()-1005, bookie->getTargetThreshold()+1005, typeid(this));
                        //Histo1d *hh1 = new Histo1d("ThresholdDist", 201, -2.5, 1002.5, typeid(this));
                        hh1->setXaxisTitle("Threshold [e]");
                        hh1->setYaxisTitle("Number of Pixels");
                        thrDist[outerIdent] = hh1;
                        hh1 = new Histo1d("NoiseDist", 76, -1, 151, typeid(this));
                        hh1->setXaxisTitle("Noise [e]");
                        hh1->setYaxisTitle("Number of Pixels");
                        sigDist[outerIdent] = hh1;
                        hh1 = new Histo1d("Chi2Dist", 51, -0.025, 2.525, typeid(this));
                        hh1->setXaxisTitle("Fit Chi/ndf");
                        hh1->setYaxisTitle("Number of Pixels");
                        chiDist[outerIdent] = hh1;
                        hh1 = new Histo1d("TimePerFitDist", 201, -1, 401, typeid(this));
                        hh1->setXaxisTitle("Fit Time [us]");
                        hh1->setYaxisTitle("Number of Pixels");
                        timeDist[outerIdent] = hh1;
                    }
                    if (par[0] > vcalMin && par[0] < vcalMax && par[1] > 0 && par[1] < (vcalMax-vcalMin)/16.0) {
                        FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(bookie->getFe(channel));
                        thrMap[outerIdent]->fill(col, row, feCfg->toCharge(par[0]));
                        thrDist[outerIdent]->fill(feCfg->toCharge(par[0]));
                        sigMap[outerIdent]->fill(col, row, feCfg->toCharge(par[1]));
                        sigDist[outerIdent]->fill(feCfg->toCharge(par[1]));
                        chiDist[outerIdent]->fill(status.fnorm/(double)status.nfev);
                        timeDist[outerIdent]->fill(fitTime.count());
                    }

                }
            }
        }
    }
}

void ScurveFitter::end() {
    if (thrDist[0] != NULL) {
        std::cout << "[" << channel << "] Threashold Mean = " << thrDist[0]->getMean() << " +- " << thrDist[0]->getStdDev() << std::endl;
        output->pushData(thrDist[0]);
        output->pushData(thrMap[0]);
        output->pushData(sigDist[0]);
        std::cout << "[" << channel << "] Noise Mean = " << sigDist[0]->getMean() << " +- " << sigDist[0]->getStdDev() << std::endl;
        output->pushData(sigMap[0]);
        output->pushData(chiDist[0]);
        output->pushData(timeDist[0]);
    }
    // Delete s-curve
    for(unsigned bin=0; bin<(nCol*nRow); bin++) {
        if (bin%((nCol*nRow)/10)==0) {
            std::cout << "Saving S-curve #" << bin << std::endl;
            output->pushData(histos[bin]);
        } else {
            delete histos[bin];
            histos[bin] = NULL;
        }
    }
}

void OccGlobalThresholdTune::init(ScanBase *s) {
    std::shared_ptr<LoopActionBase> tmpVthinFb(Fei4GlobalFeedbackBuilder(&Fei4::Vthin_Fine));
    std::shared_ptr<LoopActionBase> tmpVthinFb2(new Fe65p2GlobalFeedback(&Fe65p2::Vthin1Dac));
    n_count = 1;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
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

        if (l->type() == tmpVthinFb->type() || l->type() == tmpVthinFb2->type()) {
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
        occMaps[ident] = hh;
        Histo1d *hhh = new Histo1d(name2, injections+1, -0.5, injections+0.5, typeid(this));
        hhh->setXaxisTitle("Occupancy");
        hhh->setYaxisTitle("Number of Pixels");
        occDists[ident] = hhh;
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
        std::cout << "[" << channel << "]Mean Occupancy: " << meanOcc << std::endl;

        if ((meanOcc > 0.51) && !done) {
            sign = +1;
        } else if ((meanOcc < 0.49) && !done) {
            sign = -1;
        } else {
            sign = 0;
            done = true;
        }

        std::cout << "Calling feedback" << std::endl;
        fb->feedback(this->channel, sign, done);
        std::cout << "After feedback" << std::endl;
        output->pushData(occMaps[ident]);
        output->pushData(occDists[ident]);
        innerCnt[ident] = 0;
        //delete occMaps[ident];
        occMaps[ident] = NULL;
        //delete occDists[ident];
        occDists[ident] = NULL;
    }

}

void OccPixelThresholdTune::init(ScanBase *s) {
    n_count = 1;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
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

        if (l->type() == typeid(Fei4PixelFeedback*)) {
            fb = (PixelFeedbackBase*)((Fei4PixelFeedback*) l.get());  
        }
        if (l->type() == typeid(Fe65p2PixelFeedback*)) {
            fb = (PixelFeedbackBase*)((Fe65p2PixelFeedback*) l.get());  
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
        occMaps[ident] = hh;
    }

    // Add up Histograms
    occMaps[ident]->add(*(Histo2d*)h);
    innerCnt[ident]++;

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        double mean = 0;
        Histo2d *fbHisto = new Histo2d("feedback", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
        Histo1d *occDist = new Histo1d(name2, injections+1, -0.5, injections+0.5, typeid(this));
        occDist->setXaxisTitle("Occupancy");
        occDist->setYaxisTitle("Number of Pixels");
        for (unsigned i=0; i<fbHisto->size(); i++) {
            double occ = occMaps[ident]->getBin(i);
            if ((occ/(double)injections) > 0.65) {
                fbHisto->setBin(i, -1);
            } else if ((occ/(double)injections) < 0.35) {
                fbHisto->setBin(i, +1);
            } else {
                fbHisto->setBin(i, 0);
            }
            mean += occMaps[ident]->getBin(i);
            occDist->fill(occMaps[ident]->getBin(i));
        }
        std::cout << "[" << channel << "] Mean Occupancy: " << mean/(nCol*nRow*(double)injections) << std::endl;
        std::cout << "[" << channel << "] RMS: " << occDist->getStdDev() << std::endl;

        fb->feedback(this->channel, fbHisto);
        output->pushData(occMaps[ident]);
        output->pushData(occDist);
        innerCnt[ident] = 0;
        //delete occMaps[ident];
        occMaps[ident] = NULL;
    }

}

// TODO exclude every loop
void L1Analysis::init(ScanBase *s) {
    n_count = 1;
    injections = 0;
    std::shared_ptr<LoopActionBase> tmpVcalLoop(Fei4ParameterLoopBuilder(&Fei4::PlsrDAC));
    std::shared_ptr<LoopActionBase> tmpVcalLoop2(new Fe65p2ParameterLoop(&Fe65p2::PlsrDac));
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
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

void L1Analysis::processHistogram(HistogramBase *h) {
    // Check if right Histogram
    if (h->getType() != typeid(L1Dist*))
        return;

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
        l1Histos[ident] = hh;
        innerCnt[ident] = 0;
    }

    // Add up Histograms
    l1Histos[ident]->add(*(Histo1d*)h);
    innerCnt[ident]++;

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        output->pushData(l1Histos[ident]);
    }
}

void TotDistPlotter::init(ScanBase *s) {
    n_count = 1;
    injections = 0;
    std::shared_ptr<LoopActionBase> tmpVcalLoop(Fei4ParameterLoopBuilder(&Fei4::PlsrDAC));
    std::shared_ptr<LoopActionBase> tmpVcalLoop2(new Fe65p2ParameterLoop(&Fe65p2::PlsrDac));
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
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
        tot[ident] = hh;
        innerCnt[ident] = 0;
    }

    // Add up Histograms
    tot[ident]->add(*(Histo1d*)h);
    innerCnt[ident]++;

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        output->pushData(tot[ident]);
    }
}

void NoiseAnalysis::init(ScanBase *s) {
    // We assume the nosie scan only has one trigger and data loop
    occ = new Histo2d("Occupancy", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
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

void NoiseAnalysis::end() {
    Histo2d* noiseOcc = new Histo2d("NoiseOccupancy", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
    noiseOcc->setXaxisTitle("Col");
    noiseOcc->setYaxisTitle("Row");
    noiseOcc->setZaxisTitle("Noise Occupancy hits/bc");

    Histo2d* mask = new Histo2d("NoiseMask", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, typeid(this));
    mask->setXaxisTitle("Col");
    mask->setYaxisTitle("Row");
    mask->setZaxisTitle("Mask");

    noiseOcc->add(occ);
    noiseOcc->scale(1.0/(double)n_trigger);
    double noiseThr = 1e-6; 
    for (unsigned i=0; i<noiseOcc->size(); i++) {
        if (noiseOcc->getBin(i) > noiseThr) {
            mask->setBin(i, 0);
            if (make_mask) {
                if(dynamic_cast<Fei4*>(bookie->getFe(channel))) {;
                    Fei4 *fe = dynamic_cast<Fei4*>(bookie->getFe(channel));
                    fe->setEn((i/nRow)+1, (i%nRow)+1, 0);
                    fe->setHitbus((i/nRow)+1, (i%nRow)+1, 1);
                }
            }
        } else {
            mask->setBin(i, 1);
        }
    }

    output->pushData(occ);
    output->pushData(noiseOcc);
    output->pushData(mask);
}
