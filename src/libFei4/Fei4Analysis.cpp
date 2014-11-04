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

Fei4Analysis::~Fei4Analysis() {

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

void Fei4Analysis::addAlgorithm(AnalysisAlgorithm *a) {
    algorithms.push_back(a);
}

void Fei4Analysis::plot(std::string basename) {
    if (output->empty())
        return;
    for (std::deque<HistogramBase*>::iterator it = output->begin(); it != output->end(); ++it) {
        std::cout << "Plotting : " << (*it)->getName() << std::endl;
        (*it)->plot(basename);
    }
}
    

void OccupancyAnalysis::init(ScanBase *s) {
    n_count = 1;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
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
    for (unsigned n=0; n<loops.size(); n++) {
        ident += h->getStat().get(loops[n])+offset;
        offset += loopMax[n];
        name += "-" + std::to_string(h->getStat().get(loops[n]));
    }
    
    // Check if Histogram exists
    if (occMaps[ident] == NULL) {
        Histo2d *h = new Histo2d(name, 80, 0.5, 80.5, 336, 0.5, 336.5, typeid(this));
        h->setXaxisTitle("Column");
        h->setYaxisTitle("Row");
        h->setZaxisTitle("Hits");
        occMaps[ident] = h;
        innerCnt[ident] = 0;
    }

    // Add up Histograms
    occMaps[ident]->add(*(Histo2d*)h);
    innerCnt[ident]++;

    // Got all data, finish up Analysis
    if (innerCnt[ident] == n_count) {
        std::cout << "Collected all inner loops for " << occMaps[ident]->getName() << "(" << ident << ")" << std::endl;
        output->pushData(occMaps[ident]);
    }
}

void ScurveFitter::init(ScanBase *s) {
    std::shared_ptr<LoopActionBase> tmpVcalLoop(Fei4ParameterLoopBuilder(&Fei4::PlsrDAC));
    scan = s;
    n_count = 26880;
    vcalLoop = 0;
    std::cout << "Init ScurveFitter " << std::endl;
    std::cout << "Looking for " << (tmpVcalLoop->type()).name() << std::endl;
    for (unsigned n=0; n<s->size(); n++) {
        std::shared_ptr<LoopActionBase> l = s->getLoop(n);
        std::cout << "Got " << n << " " << l->type().name() << std::endl;
        if ((l->type() != typeid(Fei4TriggerLoop*) &&
                    l->type() != typeid(Fei4MaskLoop*) &&
                    l->type() != typeid(StdDataLoop*) &&
                    l->type() != typeid(Fei4DcLoop*)) &&
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
        if (l->type() == tmpVcalLoop->type()) {
            vcalLoop = n;
            vcalMax = l->getMax();
            vcalMin = l->getMin();
            vcalStep = l->getStep();
            std::cout << "Found Vcal Loop from " << vcalMin << " to " << vcalMax << " in " << l->getStep() << std::endl;
            vcalBins = (vcalMax-vcalMin)/vcalStep;
        }
    }
}

void ScurveFitter::processHistogram(HistogramBase *t) {
    // Check if right Histogram
    if (t->getType() != typeid(OccupancyMap*))
        return;

    Histo2d *h = (Histo2d*) t;
    for (unsigned bin=0; bin<26880; bin++) {
        if (h->getBin(bin) != 0) {
            // Select correct output container
            unsigned ident = bin;
            unsigned offset = 26880;
            unsigned vcal = h->getStat().get(vcalLoop);
            // Determine identifier
            std::string name = "Scurve";
            name += "-" + std::to_string(bin);
            // Check for other loops
            for (unsigned n=0; n<loops.size(); n++) {
                ident += h->getStat().get(loops[n])+offset;
                offset += loopMax[n];
                name += "-" + std::to_string(h->getStat().get(loops[n]));
            }
            
            // Check if Histogram exists
            if (histos[ident] == NULL) {
                Histo1d *h = new Histo1d(name, vcalBins+1, vcalMin-((double)vcalStep/2.0), vcalMax+((double)vcalStep/2.0), typeid(this));
                h->setXaxisTitle("Vcal");
                h->setYaxisTitle("Occupancy");
                histos[ident] = h;
                innerCnt[ident] = 0;
            }

            // Add up Histograms
            histos[ident]->fill(vcal, h->getBin(bin));
            innerCnt[ident]++;

            // Got all data, finish up Analysis
            if (innerCnt[ident] == vcalBins) {
                if (bin%1500==0) {
                    output->pushData(histos[ident]);
                    std::cout << "Collected all inner loops for " << histos[ident]->getName() << "(" << ident << ")" << std::endl;
                } else {
                    delete histos[ident];
                    histos[ident] = NULL;
                }
            }
        }
    }
}

void ScurveFitter::end() {

}
