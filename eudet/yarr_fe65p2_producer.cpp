#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/ExampleHardware.hh"
#include <iostream>
#include <ostream>
#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <sstream>
#include <signal.h>

#include "SpecController.h"
#include "TxCore.h"
#include "RxCore.h"
#include "Bookkeeper.h"
#include "Fe65p2.h"
#include "ScanBase.h"
#include "Fe65p2DataProcessor.h"
#include "Fei4Histogrammer.h"
#include "Fei4Analysis.h"
#include "Fei4Scans.h"
#include "Fe65p2Scans.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, int32_t, uint32_t, float>;

#ifdef _DEBUG
#define DEBUG_OUT(x)  std::cout<<x<<std::endl
#else
#define DEBUG_OUT(x)
#endif
// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "YarrFei4";

class YarrFe65p2Producer;

class EudetArchiver : public HistogramAlgorithm {
    public:
        EudetArchiver(YarrFe65p2Producer *prod, int run) : HistogramAlgorithm() {
            r = NULL;
            m_prod = prod;
            m_run = run;
            m_eventCount = 0;
            eudaqEvent = NULL;
            m_curTag = -1;
        }
        ~EudetArchiver();
        

        void processEvent(Fei4Data *data);
        void create(LoopStatus &stat) {}
    private:
        YarrFe65p2Producer *m_prod;
        eudaq::RawDataEvent *eudaqEvent;
        int m_run;
        int m_eventCount;
        int m_curTag;
        int m_curBlock;
};


// Declare a new class that inherits from eudaq::Producer
class YarrFe65p2Producer : public eudaq::Producer {
    public:
        enum status_enum{
            unconfigured,
            configured,
            starting,
            started,
            stopping,
            stopped,
            doTerminat

        };
        // The constructor must call the eudaq::Producer constructor with the name
        // and the runcontrol connection string, and initialize any member variables.
        YarrFe65p2Producer(const int &prodid, const int &specid, const std::string & name, const std::string & runcontrol)
            : eudaq::Producer(("YarrFe65p2Producer_"+std::to_string(prodid)), runcontrol),
            m_run(0), m_eventCount(0) {
                spec = dynamic_cast<HwController*>(new SpecController());
                dynamic_cast<SpecController*>(spec)->init(specid);
                tx = dynamic_cast<TxCore*>(spec);
                rx = dynamic_cast<RxCore*>(spec);
                bookie = new Bookkeeper(tx, rx);
                m_daqId = prodid;
                m_dutName = name;
                m_trigMultiplier = 5;
                m_trigLatency = 200; 
                scanDone = false;
                processorDone = false;
                
                // Setup FE
                myFe = new Fe65p2(tx);
                std::cout << "DUT pointer " << myFe << std::endl; 
                bookie->addFe((FrontEnd*)(myFe), 0, 0);
                myFe->setName(m_dutName);
                myFe->setActive(1);
                
                s = new Fe65p2ExtTrigger(bookie);
                s->init();
            }

        ~YarrFe65p2Producer() {
            delete s;
            delete bookie;
            delete spec;
        }

        // This gets called whenever the DAQ is configured
        virtual void OnConfigure(const eudaq::Configuration & config) {
            SetStatus(eudaq::Status::LVL_BUSY, "Configuring");
            std::cout << "####################" << std::endl;
            std::cout << "### Configuring: ### " << config.Name() << std::endl;
            std::cout << "####################" << std::endl;
            config.Print();
            // Do any configuration of the hardware here
            // Configuration file values are accessible as config.Get(name, default)
            m_daqId = config.Get("DaqId", 0);
            m_dutName = config.Get("DutName", "LBLFE65-16");
            m_trigMultiplier= config.Get("TriggerMultiplier", 5);
            m_trigLatency = config.Get("TriggerLatency", 200);

            // Configure hardware 
            std::cout << "Setting up triggerlogic ..." << std::endl;
            tx->resetTriggerLogic();
            tx->setTriggerLogicMask(0x0100); // Eudet TLU
            tx->setTriggerLogicMode(MODE_EUDET_TAG);
               
            // Load config
            std::cout << "Loading config ... " << std::endl;
            json icfg;
            std::fstream icfg_file(("configs/" + m_dutName + ".json").c_str(), std::ios::in);
            if (icfg_file) {
                icfg_file >> icfg;
                bookie->g_fe65p2->fromFileJson(icfg);
                myFe->fromFileJson(icfg);
                std::cout << "Config for FE " << m_dutName << " loaded!" << std::endl;
            } else {
                std::cerr << "~~ERROR~~ Config for FE " << m_dutName << " not found! Aborting!" << std::endl;
                SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
                throw eudaq::Exception(("Config for FE " + m_dutName + " not found! Aborting!").c_str());
            }
            icfg_file.close();

            // Configure chip
            std::cout << "Configuring FE " << m_dutName << " ... " << std::endl;
            tx->setCmdEnable(0x1); // Enable first channel;
            myFe->configure();
            while(!tx->isCmdEmpty());
            std::cout << "... success!" << std::endl;
            
            // Enable Rx
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            rx->setRxEnable(0x1);


            // At the end, set the status that will be displayed in the Run Control.
            SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
            m_stat = configured;
            std::cout << "...Configured" << std::endl;
        }

        // This gets called whenever a new run is started
        // It receives the new run number as a parameter
        virtual void OnStartRun(unsigned param) {
            m_stat = starting;
            m_run = param;
            m_eventCount = 0;
            SetStatus(eudaq::Status::LVL_BUSY, "Starting");
            std::cout << "##################" << std::endl;
            std::cout << "### Start Run: ### " << m_run << std::endl;
            std::cout << "##################" << std::endl;

            tx->resetTriggerLogic();

            // It must send a BORE to the Data Collector
            eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
            // You can set tags on the BORE that will be saved in the data file
            // and can be used later to help decoding
            bore.SetTag("DAQID", m_daqId);
            bore.SetTag("DUTNAME", m_dutName);
            bore.SetTag("DUTTYPE", "Fe65p2"); // TODO hardcoded for now
            bore.SetTag("TRIGMULTIPLIER", m_trigMultiplier);
            bore.SetTag("TRIGLATENCY", m_trigLatency);
            bore.SetTimeStampToNow();
            // Send the event to the Data Collector
            SendEvent(bore);

            scanDone = false;
            processorDone = false;

            if (myFe->clipDataFei4 == NULL)
                std::cout << "something wrong" << std::endl;

            // Init histogrammer
            std::cout << "Initiliasing histogrammer." << std::endl;
            myFe->histogrammer = new Fei4Histogrammer();
            myFe->histogrammer->connect(myFe->clipDataFei4, myFe->clipHisto);
            myFe->histogrammer->addHistogrammer(new OccupancyMap());
            myFe->histogrammer->addHistogrammer(new TotMap());
            //myFe->histogrammer->addHistogrammer(new Tot2Map());
            myFe->histogrammer->addHistogrammer(new L1Dist());
            //myFe->histogrammer->addHistogrammer(new HitsPerEvent());
            myFe->histogrammer->addHistogrammer(new TotDist());
            // This is where the raw data is saved
            myFe->histogrammer->addHistogrammer(new DataArchiver(("data/" + m_dutName + "_" + this->toString(m_run, 6) + ".raw")));
            myFe->histogrammer->addHistogrammer(new EudetArchiver(this, m_run));
            myFe->histogrammer->setMapSize(64, 64);
            
            // Init analysis
            std::cout << "Initiliasing analysis." << std::endl;
            myFe->ana = new Fei4Analysis(bookie, myFe->getRxChannel());
            myFe->ana->connect(s, myFe->clipHisto, myFe->clipResult);
            myFe->ana->addAlgorithm(new L1Analysis());
            myFe->ana->addAlgorithm(new TotDistPlotter());
            myFe->ana->addAlgorithm(new OccupancyAnalysis());
            myFe->ana->setMapSize(64, 64);

            std::cout << "Initiliasing scan loop." << std::endl;
            s->preScan();
            // Startprocessor threads
            //unsigned int numThreads = std::thread::hardware_concurrency();
            unsigned int numThreads = 1;
            std::cout << "-> Starting " << numThreads << " processor Threads:" << std::endl; 
            for (unsigned i=0; i<numThreads; i++) {
                procThreads.push_back(std::thread(&YarrFe65p2Producer::process, this, bookie));
                std::cout << "  -> Processor thread #" << i << " started!" << std::endl;
            }
            
            std::cout << "-> Starting histogrammer and analysis threads:" << std::endl;
            anaThreads.push_back(std::thread(&YarrFe65p2Producer::analysis, this, myFe->histogrammer, myFe->ana));
            std::cout << "  -> Analysis thread started" << std::endl;
            
            // Going into run mode
            std::cout << "Going into runmode!" << std::endl;
            runThread.push_back(std::thread(&ScanBase::run, s));

            // At the end, set the status that will be displayed in the Run Control.
            m_stat = started;
            SetStatus(eudaq::Status::LVL_OK, "Running");
            std::cout << "... Started"<< std::endl;
        }

        // This gets called whenever a run is stopped
        virtual void OnStopRun() {
            SetStatus(eudaq::Status::LVL_BUSY, "Stopping");
            std::cout << "####################" << std::endl;
            std::cout << "### Stopping Run ###" << std::endl;
            std::cout << "####################" << std::endl;
            m_stat = stopping;
             
            ((StdDataGatherer*)s->getLoop(1).get())->kill(); // Should be data gatherer
            eudaq::mSleep(20);
            
            // Wait for scan to exit
            std::cout << "Waiting for run thread ... " << std::endl;
            for (auto &n : runThread) n.join();
            s->postScan();
            scanDone = true;
            std::cout << "Waiting for proc threads ... " << std::endl;
            for (auto &n : procThreads) n.join();
            processorDone = true;
            std::cout << "Waiting for ana threads ... " << std::endl;
            for (auto &n : anaThreads) n.join();
            std::cout << "All done." << std::endl;
            
            m_stat = stopped;
            
            delete myFe->histogrammer; // This will close the file
            myFe->histogrammer = NULL;

            // Clean threads
            runThread.clear();
            procThreads.clear();
            anaThreads.clear();

            // Plot stuff
            myFe->ana->plot("data/" + m_dutName + "_" + this->toString(m_run, 6));
            delete myFe->ana;
            myFe->ana = NULL;
            
            // Save config
            std::cout << "Save backup config file." << std::endl;
            std::fstream ocfg_file(("data/" + m_dutName + "_" + this->toString(m_run, 6) + ".json").c_str(), std::ios::out);
            json cfg;
            myFe->toFileJson(cfg);
            ocfg_file << std::setw(4) << cfg;
            ocfg_file.close();

            //Empty containers
            std::cout << "Empty data containers." << std::endl;
            myFe->clipDataFei4->clear();
            myFe->clipHisto->clear();
            myFe->clipResult->clear();

            std::cout << "Sending EORE event" << std::endl;
            // Send an EORE after all the real events have been sent
            // You can also set tags on it (as with the BORE) if necessary
            auto EOREvent = eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_eventCount);
            EOREvent.SetTag("EVENTCOUNT", m_eventCount);
            EOREvent.SetTimeStampToNow();
            SendEvent(EOREvent);
            m_stat = configured;
            SetStatus(eudaq::Status::LVL_OK, "Stopped");
            std::cout << "Stopped \n "<<m_eventCount<<" Events send"<< std::endl;
        }

        // This gets called when the Run Control is terminating,
        // we should also exit.
        virtual void OnTerminate() {
            std::cout << "Terminating..." << std::endl;
            m_stat = doTerminat;
            rx->setRxEnable(0x0);
        }

        void state_waiting(){
            DEBUG_OUT("<waiting>");
            while (m_stat != started&&m_stat!=doTerminat)
            {
                eudaq::mSleep(20);
            }
            DEBUG_OUT("</waiting>");
        }

        void state_running(){
            DEBUG_OUT("<starting>");
            while (m_stat == started){


            }
            DEBUG_OUT("</starting>");
        }

        void state_stopping(){
            DEBUG_OUT("<stopping>");
            while (m_stat == stopping){
            }
            DEBUG_OUT("</stopping>");
        }

        // This is just an example, adapt it to your hardware
        void ReadoutLoop() {
            // Loop until Run Control tells us to terminate
            while (m_stat != doTerminat) {

                state_waiting();
                state_running();
                state_stopping();
            }
        }
        
        void incEventCount() {
            ++m_eventCount;
        }
    private:
        // YARR objects
        HwController *spec;
        TxCore *tx;
        RxCore *rx;
        Bookkeeper *bookie;
        ScanBase *s;
        Fe65p2 *myFe;

        // YARR producer config
        unsigned m_daqId; // To distinguish different systems
        std::string m_dutName; // Config assumed to be <m_dutName>.json
        unsigned m_trigMultiplier; // Overwrites FE config
        unsigned m_trigLatency;  // Overwrites FE config

        // General producer variables
        unsigned m_run; // run number
        unsigned m_eventCount; // event counter

        // Producer state
        status_enum m_stat = unconfigured;
        std::string toString(int value,int digitsCount)
        {
            std::ostringstream os;
            os<<std::setfill('0')<<std::setw(digitsCount)<<value;
            return os.str();
        }
        bool scanDone;
        bool processorDone;

        std::vector<std::thread> runThread;
        std::vector<std::thread> anaThreads;
        std::vector<std::thread> procThreads;

        void process(Bookkeeper *bookie) {
            // Set correct Hit Discriminator setting, for proper decoding
            Fe65p2DataProcessor proc;
            proc.connect(&bookie->rawData, &bookie->eventMap);
            proc.init();

            while(!scanDone) {
                // TODO some better wakeup signal?
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                proc.process();
            }
            proc.process();
        }

        void analysis(Fei4Histogrammer *h, Fei4Analysis *a) {
            h->init();
            a->init();

            while(!processorDone) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                h->process();
                a->process();
            }
            h->process();
            a->process();

            a->end();
        }
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
    // You can use the OptionParser to get command-line arguments
    // then they will automatically be described in the help (-h) option
    eudaq::OptionParser op("EUDAQ Example Producer", "1.0",
            "Just an example, modify it to suit your own needs");
    eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
            "tcp://localhost:44000", "address",
            "The address of the RunControl.");
    eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
            "The minimum level for displaying log messages locally");
    eudaq::Option<std::string> name(op, "n", "name", "fe65p2", "string",
            "Module name");
    eudaq::Option<int> specid(op, "i", "specid", 0, "int",
            "SPEC Id (/dev/specX)");
    eudaq::Option<int> prodid(op, "p", "prodid", 0, "int",
            "Producer Id");
    try {
        // This will look through the command-line arguments and set the options
        op.Parse(argv);
        // Set the Log level for displaying messages based on command-line
        EUDAQ_LOG_LEVEL(level.Value());

        std::cout << "     #############################" << std::endl;
        std::cout << "     ### YARR FE65-P2 Producer ###" << std::endl;
        std::cout << "     #############################" << std::endl;
        std::cout << "-> Producer connected to " << rctrl.Value() << std::endl;
        std::cout << "-> Module: " << name.Value() << std::endl;   
        eudaq::mSleep(3000);
        // Create a producer
        YarrFe65p2Producer producer(specid.Value(), prodid.Value(), name.Value(), rctrl.Value());
        // And set it running...
        producer.ReadoutLoop();
        // When the readout loop terminates, it is time to go
        std::cout << "Quitting" << std::endl;
    }
    catch (...) {
        // This does some basic error handling of common exceptions
        return op.HandleMainException();
    }
    return 0;
}

EudetArchiver::~EudetArchiver() {
        if (eudaqEvent != NULL) {
            // Send last eventr 
            eudaqEvent->SetTimeStampToNow();
            m_prod->SendEvent(*eudaqEvent);
            delete eudaqEvent;
        }
}

void EudetArchiver::processEvent(Fei4Data *data) {

    for (std::list<Fei4Event>::iterator eventIt = (data->events).begin(); eventIt!=data->events.end(); ++eventIt) {   
        Fei4Event curEvent = *eventIt;
        if (eudaqEvent == NULL) { // First time
            eudaqEvent = new eudaq::RawDataEvent(EVENT_TYPE, m_run, m_eventCount);
            eudaqEvent->SetTag("EventNumber", m_eventCount);
            m_prod->incEventCount();
            ++m_eventCount;
            m_curTag = curEvent.tag;
            m_curBlock = 0;
        } else if (m_curTag != (int) curEvent.tag) { // New trigger tag
            // Send the event to the Data Collector 
            eudaqEvent->SetTimeStampToNow();
            m_prod->SendEvent(*eudaqEvent);
            delete eudaqEvent;
            // Create new event
            eudaqEvent = new eudaq::RawDataEvent(EVENT_TYPE, m_run, m_eventCount);
            eudaqEvent->SetTag("EventNumber", m_eventCount);
            m_prod->incEventCount();
            ++m_eventCount;
            m_curTag = curEvent.tag;
            m_curBlock = 0;
        }
        if (m_eventCount%1000 == 0)
            std::cout << "~~ Got event #: " << m_eventCount << std::endl;
       
        // Fill event with data
        unsigned it = 0;
        unsigned char *buffer = new unsigned char[sizeof(uint32_t) + (3*sizeof(uint16_t)) + (curEvent.nHits*sizeof(Fei4Hit))];

        *(uint32_t*)&buffer[it] = curEvent.tag; it+= sizeof(uint32_t);
        *(uint16_t*)&buffer[it] = curEvent.l1id; it+= sizeof(uint16_t);
        *(uint16_t*)&buffer[it] = curEvent.bcid; it+= sizeof(uint16_t);
        *(uint16_t*)&buffer[it] = curEvent.nHits; it+= sizeof(uint16_t);
        for (unsigned i=0; i<curEvent.nHits; i++) {
            *(Fei4Hit*)&buffer[it] = curEvent.hits[i]; it+= sizeof(Fei4Hit);
        }

        eudaqEvent->AddBlock(m_curBlock,  buffer,sizeof(uint32_t) + (3*sizeof(uint16_t)) + (curEvent.nHits*sizeof(Fei4Hit))); 
        delete[] buffer;
        
        /* Old way
        ev.AddBlock(0, (char*) &curEvent.tag, sizeof(uint32_t));
        ev.AddBlock(1, (char*) &curEvent.l1id, sizeof(uint16_t));
        ev.AddBlock(2, (char*) &curEvent.bcid, sizeof(uint16_t));
        ev.AddBlock(3, (char*) &curEvent.nHits, sizeof(uint16_t));
        if (curEvent.nHits > 0)
            ev.AddBlock(4, (char*) &curEvent.hits[0], curEvent.nHits*sizeof(Fei4Hit));
        */
    }
}
