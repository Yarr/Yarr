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

using json = nlohmann::json;

#ifdef _DEBUG
#define DEBUG_OUT(x)  std::cout<<x<<std::endl
#else
#define DEBUG_OUT(x)
#endif
// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "YarrFei4";


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
        YarrFe65p2Producer(const std::string & name, const std::string & runcontrol)
            : eudaq::Producer(name, runcontrol),
            m_run(0), m_eventCount(0) {
                spec = new SpecController(0);    
                tx = new TxCore(spec);
                rx = new RxCore(spec);
                bookie = new Bookkeeper(tx, rx);
                m_daqId = 0;
                m_dutName = "Unknown";
                m_trigMultiplier = 5;
                m_trigLatency = 200; 
                scanDone = false;
                processorDone = false;
            }

        ~YarrFe65p2Producer() {
            delete bookie;
            delete tx;
            delete rx;
            delete spec;
        }

        // This gets called whenever the DAQ is configured
        virtual void OnConfigure(const eudaq::Configuration & config) {
            std::cout << "Configuring: " << config.Name() << std::endl;
            config.Print();
            // Do any configuration of the hardware here
            // Configuration file values are accessible as config.Get(name, default)
            m_daqId = config.Get("DaqId", 0);
            m_dutName = config.Get("DutName", "unknown");
            m_trigMultiplier= config.Get("TriggerMultiplier", 5);
            m_trigLatency = config.Get("TriggerLatency", 200);

            // Configure hardware 
            tx->setTriggerLogicMask(0x0100); // Eudet TLU
            tx->setTriggerLogicMode(MODE_EUDET_TAG);
            
            // Setup FE
            bookie->addFe(new Fe65p2(tx), 0, 0);
            Fe65p2 *dut = dynamic_cast<Fe65p2*>(bookie->getLastFe());
            dut->setName(m_dutName);
            dut->setActive(1);

            // Load config
            json icfg;
            std::fstream icfg_file((m_dutName + ".json").c_str(), std::ios::in);
            if (icfg_file) {
                icfg_file >> icfg;
                bookie->g_fe65p2->fromFileJson(icfg);
                dynamic_cast<FrontEndCfg*>(dut)->fromFileJson(icfg);
                std::cout << "Config for FE " << m_dutName << " loaded!" << std::endl;
            } else {
                std::cerr << "~~ERROR~~ Config for FE " << m_dutName << " not found! Aborting!" << std::endl;
                throw eudaq::Exception(("Config for FE " + m_dutName + " not found! Aborting!").c_str());
            }
            icfg_file.close();

            // Configure chip
            std::cout << "Configuring FE " << m_dutName << " ... " << std::endl;
            tx->setCmdEnable(0x1); // Enable first channel;
            dut->configure();
            while(!tx->isCmdEmpty());
            std::cout << "... success!" << std::endl;

            // Enable Rx
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            rx->setRxEnable(0x1);

            s = new Fe65p2ExtTrigger(bookie);

            // At the end, set the status that will be displayed in the Run Control.
            SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
            m_stat = configured;
            std::cout << "...Configured" << std::endl;
        }

        // This gets called whenever a new run is started
        // It receives the new run number as a parameter
        virtual void OnStartRun(unsigned param) {
            std::cout << "Start Run: " << m_run << std::endl;
            m_stat = starting;
            m_run = param;
            m_eventCount = 0;


            // It must send a BORE to the Data Collector
            eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
            // You can set tags on the BORE that will be saved in the data file
            // and can be used later to help decoding
            bore.SetTag("DaqID", m_daqId);
            bore.SetTag("DutName", m_dutName);
            bore.SetTag("DutType", "Fe65p2"); // TODO hardcoded for now
            bore.SetTag("TrigMultiplier", m_trigMultiplier);
            bore.SetTag("TrigLatency", m_trigLatency);
            bore.SetTimeStampToNow();
            // Send the event to the Data Collector
            SendEvent(bore);

            scanDone = false;
            processorDone = false;

            Fe65p2 *dut = dynamic_cast<Fe65p2*>(bookie->getLastFe());
            
            // Init histogrammer
            std::cout << "Initiliasing histogrammer." << std::endl;
            dut->histogrammer->setMapSize(64, 64);
            dut->histogrammer->connect(dut->clipDataFei4, dut->clipHisto);
            dut->histogrammer->addHistogrammer(new OccupancyMap());
            dut->histogrammer->addHistogrammer(new TotMap());
            dut->histogrammer->addHistogrammer(new Tot2Map());
            dut->histogrammer->addHistogrammer(new L1Dist());
            dut->histogrammer->addHistogrammer(new HitsPerEvent());
            dut->histogrammer->addHistogrammer(new TotDist());
            // This is where the raw data is saved
            dut->histogrammer->addHistogrammer(new DataArchiver((m_dutName + "_" + this->toString(m_run, 6) + ".raw")));
            
            dataFile.open((m_dutName + "_" + this->toString(m_run, 6) + ".raw").c_str(), std::fstream::in | std::fstream::binary);

            // Init analysis
            std::cout << "Initiliasing analysis." << std::endl;
            dut->ana = new Fei4Analysis(bookie, dut->getRxChannel());
            dut->ana->setMapSize(64, 64);
            dut->ana->connect(s, dut->clipHisto, dut->clipResult);
            dut->ana->addAlgorithm(new L1Analysis());
            dut->ana->addAlgorithm(new TotDistPlotter());
            dut->ana->addAlgorithm(new OccupancyAnalysis());

            std::cout << "Initiliasing scan loop." << std::endl;
            s->init();
            s->preScan();
            // Startprocessor threads
            unsigned int numThreads = std::thread::hardware_concurrency();
            std::cout << "-> Starting " << numThreads << " processor Threads:" << std::endl; 
            for (unsigned i=0; i<numThreads; i++) {
                procThreads.push_back(std::thread(&YarrFe65p2Producer::process, this, bookie));
                std::cout << "  -> Processor thread #" << i << " started!" << std::endl;
            }
            
            std::cout << "-> Starting histogrammer and analysis threads:" << std::endl;
            anaThreads.push_back(std::thread(&YarrFe65p2Producer::analysis, this, dut->histogrammer, dut->ana));
            std::cout << "  -> Analysis thread of Fe " << dut->getName() << std::endl;
            
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
            std::cout << "Stopping Run" << std::endl;
            m_stat = stopping;
            
            Fe65p2 *dut = dynamic_cast<Fe65p2*>(bookie->getLastFe());
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
            
            delete dut->histogrammer; // This will close the file
            
            std::cout << "Waiting for all data to be sent ..." << std::endl;
            // wait until all events have been read out from the hardware
            while (m_stat == stopping) {
                eudaq::mSleep(20);
            }

            // Cleanup
            runThread.clear();
            procThreads.clear();
            anaThreads.clear();

            // Plot stuff
            dut->ana->plot(m_dutName + "_" + this->toString(m_run, 6));
            delete dut->ana;
            
            // Save config
            std::fstream ocfg_file((m_dutName + "_" + this->toString(m_run, 6) + ".json").c_str(), std::ios::out);
            json cfg;
            dut->toFileJson(cfg);
            ocfg_file << std::setw(4) << cfg;
            ocfg_file.close();

            //Empty containers
            dut->clipDataFei4->clear();
            dut->clipHisto->clear();
            dut->clipResult->clear();

            // Send an EORE after all the real events have been sent
            // You can also set tags on it (as with the BORE) if necessary
            auto EOREvent = eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_eventCount);
            EOREvent.SetTag("EventCount", m_eventCount);
            EOREvent.SetTimeStampToNow();
            SendEvent(EOREvent);
            m_stat = configured;
            std::cout << "Stopped \n "<<m_eventCount<<" Events send"<< std::endl;
        }

        // This gets called when the Run Control is terminating,
        // we should also exit.
        virtual void OnTerminate() {
            std::cout << "Terminating..." << std::endl;
            m_stat = doTerminat;
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

                if (!makeAndSendEvents())
                {
                    eudaq::mSleep(20);
                }

            }
            DEBUG_OUT("</starting>");
        }

        void state_stopping(){
            DEBUG_OUT("<stopping>");
            while (m_stat == stopping){
                if (!makeAndSendEvents())
                {

                    m_stat = stopped;
                }
            }
            DEBUG_OUT("</stopping>");
        }

        bool makeAndSendEvents(){
            // If event
            ++m_eventCount;
            eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_eventCount);
            //ev.SetTag("EventNumber", m_eventCount);

            if (dataFile.eof())
               return false;
            
            Fei4Event event;
            event.fromFileBinary(dataFile);

            char *buffer = new char[sizeof(uint32_t) + (3*sizeof(uint16_t)) + (event.nHits*sizeof(Fei4Hit))];
            unsigned it = 0;
            *(uint32_t*)&buffer[it] = event.tag; it+= sizeof(uint32_t);
            *(uint16_t*)&buffer[it] = event.l1id; it+= sizeof(uint16_t);
            *(uint16_t*)&buffer[it] = event.bcid; it+= sizeof(uint16_t);
            *(uint16_t*)&buffer[it] = event.nHits; it+= sizeof(uint16_t);
            for (unsigned i=0; i<event.nHits; i++) {
                *(Fei4Hit*)&buffer[it] = event.hits[i]; it+= sizeof(Fei4Hit);
            }

            // Fill event with data
            ev.SetTimeStampToNow();
            // Send the event to the Data Collector 
            if (m_eventCount % 10 == 0)
            {
                std::cout << "sending Event: " << m_eventCount << std::endl;
            }

            SendEvent(ev);
            return true;
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

    private:
        // YARR objects
        SpecController *spec;
        TxCore *tx;
        RxCore *rx;
        Bookkeeper *bookie;
        ScanBase *s;
        std::fstream dataFile;

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
    eudaq::Option<std::string> name(op, "n", "name", "Example", "string",
            "The name of this Producer");

    try {
        // This will look through the command-line arguments and set the options
        op.Parse(argv);
        // Set the Log level for displaying messages based on command-line
        EUDAQ_LOG_LEVEL(level.Value());
        std::cout << "Example Producer name = \"" << name.Value() << "\" connected to " << rctrl.Value() << std::endl;
        if (name.IsSet())
        {
            eudaq::mSleep(1000);
        }
        eudaq::mSleep(3000);
        // Create a producer
        YarrFe65p2Producer producer(name.Value(), rctrl.Value());
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
