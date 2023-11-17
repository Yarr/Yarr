#include <cstdint>
#include <string>
#include <iomanip>

#include <filesystem>
namespace fs = std::filesystem;

#include "SpecCom.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "storage.hpp"
// YARR
#include "HwController.h"
#include "FrontEnd.h"
#include "AllChips.h"
#include "ScanHelper.h"
#include "Utils.h"


auto logger = logging::make_log("rd53bDataMergingCheck_2to1");

constexpr const char* COLOR_GREEN = "\033[32m";
constexpr const char* COLOR_RESET = "\033[0m";

void printHelp() {
       std::cout << "./bin/rd53bDataMergingCheck_2to1 [-h] [-r <hw_controller_file>] [-c <connectivity_file>] [-t <test_size>] [-s] [-n] [-v]\n\n"
              << "Options:\n"
              << "  -h                   Display this help message.\n"
              << "  -r <hw_controller_file>   Specify hardware controller JSON path.\n"
              << "  -c <connectivity_file>    Specify connectivity config JSON path.\n"
              << "  -t <test_size>            Specify the error counter test size. Default 1 x 10^6\n"
              << "  -s                   Skip chip configuration.\n"
              << "  -n                   Don't update the controller condfig with the best delay values\n" 
              << "  -v                   Print out and store raw error counter values.\n"; 
}

std::unique_ptr<FrontEnd> init_fe(std::unique_ptr<HwController>& hw, json &jconn, int fe_num) {
    std::string chip_type = jconn["chipType"];
    auto fe = StdDict::getFrontEnd(chip_type);
    auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
    auto chip_configs = jconn["chips"];
    if(fe_num >= chip_configs.size()) {
        std::stringstream e;
        e << "Invalid FE index (" << fe_num << ") for connectivity file with " << chip_configs.size() << " chips";
        throw std::runtime_error(e.str());
    }
    auto chip_config = chip_configs[fe_num];
    fe->init(&*hw, FrontEndConnectivity(chip_config["tx"], chip_config["rx"]));
    auto chip_register_file_path = chip_config["__config_path__"];
    fs::path pconfig{chip_register_file_path};
    if(!fs::exists(pconfig)) {
        std::cerr << "WARNING: Chip config \"" << chip_register_file_path << "\" not found" << std::endl;
        fe.reset();
        return fe;
    }
    auto chip_register_json = ScanHelper::openJsonFile(chip_register_file_path);
    cfg->loadConfig(chip_register_json);
    hw->setCmdEnable(cfg->getTxChannel());
    return fe;
}


int main(int argc, char **argv) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);
    
    // Init spec
    logger->info("Init spec");
    int c;	
    int specNum = 0;
    int n_lanes= 16;
    std::string hw_controller_filename = "";
    std::string connectivity_filename = "";
    uint32_t test_size = 1000000;
    bool save_delay=true;
    bool skip_config=false;
    bool print_raw_value=false;
    uint32_t cdrclksel = 0;
    uint32_t serblckperiod = 50;

    while ((c = getopt(argc, argv, "hr:c:t:nsv")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
		case 'r':
            hw_controller_filename = optarg;
            break;
        case 'c' :
            connectivity_filename = optarg;
            break;
        case 't' :
            test_size = std::stoi(optarg);
            break;
        case 'n' :
            save_delay = false;
            break;    
        case 's' :
            skip_config = true;
            break;   
        case 'v' :
            print_raw_value = true;
            break;  
		default:
            logger->critical("Invalid command line parameter(s) given!");
            return -1;
	    }
    }

    fs::path hw_controller_path{hw_controller_filename};
    if(!fs::exists(hw_controller_path)) {
        std::cerr << "ERROR: Provided hw controller file (=" << hw_controller_filename << ") does not exist" << std::endl;
        return 1;
    }

    fs::path connectivity_path{connectivity_filename};
    if(!fs::exists(connectivity_path)) {
        std::cerr << "ERROR: Provided connectivity file (=" << connectivity_filename << ") does not exist" << std::endl;
        return 1;
    }

    // instantiate the hw controller
    std::unique_ptr<HwController> hw;
    json jcontroller;

    try {
        jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
        hw = ScanHelper::loadController(jcontroller);
    } catch (std::exception& e) {
        std::cerr << "ERROR: Unable to load controller from provided config, exception caught: " << e.what() << std::endl;
        return 1;
    }

    // Set up hardware config 
    specNum=jcontroller["ctrlCfg"]["cfg"]["specNum"];
    SpecCom mySpec(specNum);
    logger->info("Doing data merging test on Spec Card {}", specNum);

    hw->setupMode();
    hw->setTrigEnable(0);
    hw->disableRx(); // needed?

    auto jconn = ScanHelper::openJsonFile(connectivity_filename);



    std::string chipType = ScanHelper::loadChipConfigs(jconn, false, Utils::dirFromPath(connectivity_filename));
    auto chip_configs = jconn["chips"];
    size_t n_chips = chip_configs.size();

    int lane_1=0;
    int lane_2=0;

    int temp_lane=0;
    for (int j=0; j<n_chips; j++){
        temp_lane=jconn["chips"][j]["rx"];
        if (temp_lane%4==1){
            lane_1=temp_lane;
        }
    }

    for (int j=0; j<n_chips; j++){
        temp_lane=jconn["chips"][j]["rx"];
        if (temp_lane%4==3){
            lane_2=temp_lane;
        }
    }

    int delay_1=jcontroller["ctrlCfg"]["cfg"]["delay"][lane_1];
    int delay_2=jcontroller["ctrlCfg"]["cfg"]["delay"][lane_2];

    // Calculate various parameters for the scan 
    double clk_speed=37.5;
    double readout_speed=clk_speed*32*10e6/(cdrclksel+1);
    double time=0.0;
    time=1/readout_speed*66*test_size;
    double clkcycles = 1/(clk_speed*10e6);
    double count=time/clkcycles/(serblckperiod*2+1);
    int min=std::floor(count);
    int max=std::ceil(count);
    int wait = time*10000000;

	// Enable manual delay control
	mySpec.writeSingle(0x2 << 14 | 0x6, 0xffff); 

	mySpec.writeSingle(0x2 << 14 | 0x8, test_size); 
	mySpec.writeSingle(0x2 << 14 | 0x9, 0); 

    std::cout << std::fixed << std::setprecision(2);
    mySpec.writeSingle(0x2 << 14 | 0x4, lane_1); 
    mySpec.writeSingle(0x2 << 14 | 0x5, delay_1);
    mySpec.writeSingle(0x2 << 14 | 0x4, lane_2); 
    mySpec.writeSingle(0x2 << 14 | 0x5, delay_2);
        

    logger->info("Setting up configuration for all chips...");

    // Set up all chips 
    for (size_t ichip = 0; ichip < n_chips; ichip++) {
        if (chip_configs[ichip]["enable"] == 0)
            continue;
        fs::path chip_register_file_path{chip_configs[ichip]["__config_path__"]};


        auto fe = init_fe(hw, jconn, ichip);
        auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
        std::string current_chip_name = cfg->getName();
        auto jchip = ScanHelper::openJsonFile(chip_register_file_path);

        if(!fe) {
            std::cerr << "WARNING: Skipping chip at index " << ichip << " in connectivity file" << std::endl;
            continue;
        } else {
            if (!skip_config){
                auto jchip = ScanHelper::openJsonFile(chip_register_file_path);
                fe->configure();           
            } else {
                logger->info("Skipping configuration!");
            }

            // Wait for fifo to be empty
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            while(!hw->isCmdEmpty());

            if (!skip_config){
                // Wait for sync
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            // By default, disable service blocks and set VDDA/VDDD low
            fe->writeNamedRegister("ServiceBlockEn", 1);

            int chip_id=jchip["RD53B"]["Parameter"]["ChipId"];

            if (chip_id==12){
                fe->writeNamedRegister("AuroraActiveLanes", 3);
                fe->writeNamedRegister("CdrClkSel", 2);
                fe->writeNamedRegister("CmlBias0", 500);
                fe->writeNamedRegister("CmlBias1", 0);
                fe->writeNamedRegister("SerEnLane", 15);
                fe->writeNamedRegister("SerEnTap", 0);
                fe->writeNamedRegister("SerInvTap", 0);
                fe->writeNamedRegister("SerSelOut0", 3);
                fe->writeNamedRegister("SerSelOut1", 3);
                fe->writeNamedRegister("SerSelOut2", 3);
                fe->writeNamedRegister("SerSelOut3", 3);


                fe->writeNamedRegister("ServiceBlockEn", 1);

                fe->writeNamedRegister("DataMergeOutMux0", 0);
                fe->writeNamedRegister("DataMergeOutMux1", 1);
                fe->writeNamedRegister("DataMergeOutMux2", 2);
                fe->writeNamedRegister("DataMergeOutMux3", 3);

                fe->writeNamedRegister("EnChipId", 1);
            } else if (chip_id==13){
                fe->writeNamedRegister("DataMergeEnBond", 1);
                fe->writeNamedRegister("CdrClkSel", 0);
                fe->writeNamedRegister("CmlBias0", 800);
                fe->writeNamedRegister("CmlBias1", 400);
                fe->writeNamedRegister("SerEnLane", 15);
                fe->writeNamedRegister("SerEnTap", 1);
                fe->writeNamedRegister("SerInvTap", 1);
                fe->writeNamedRegister("SerSelOut0", 3);
                fe->writeNamedRegister("SerSelOut1", 3);
                fe->writeNamedRegister("SerSelOut2", 3);
                fe->writeNamedRegister("SerSelOut3", 3);

                fe->writeNamedRegister("DataMergeInMux0", 1);
                fe->writeNamedRegister("DataMergeInMux1", 0);
                fe->writeNamedRegister("DataMergeInMux2", 2);
                fe->writeNamedRegister("DataMergeInMux3", 3);

                fe->writeNamedRegister("DataMergeOutMux0", 0);
                fe->writeNamedRegister("DataMergeOutMux1", 1);
                fe->writeNamedRegister("DataMergeOutMux2", 2);
                fe->writeNamedRegister("DataMergeOutMux3", 3);

                fe->writeNamedRegister("EnChipId", 1);
            } else if (chip_id==14){
                fe->writeNamedRegister("AuroraActiveLanes", 3);
                fe->writeNamedRegister("CdrClkSel", 2);
                fe->writeNamedRegister("CmlBias0", 500);
                fe->writeNamedRegister("CmlBias1", 0);
                fe->writeNamedRegister("SerEnLane", 15);
                fe->writeNamedRegister("SerEnTap", 0);
                fe->writeNamedRegister("SerInvTap", 0);
                fe->writeNamedRegister("SerSelOut0", 3);
                fe->writeNamedRegister("SerSelOut1", 3);
                fe->writeNamedRegister("SerSelOut2", 3);
                fe->writeNamedRegister("SerSelOut3", 3);

                fe->writeNamedRegister("DataMergeOutMux0", 0);
                fe->writeNamedRegister("DataMergeOutMux1", 1);
                fe->writeNamedRegister("DataMergeOutMux2", 2);
                fe->writeNamedRegister("DataMergeOutMux3", 3);

                fe->writeNamedRegister("EnChipId", 1);
            } else if (chip_id==15){
                //Primary
                fe->writeNamedRegister("EnChipId", 1);
                fe->writeNamedRegister("DataMergeEnBond", 1);

                fe->writeNamedRegister("ServiceBlockEn", 1);

                fe->writeNamedRegister("DataMergeInMux0", 3);
                fe->writeNamedRegister("DataMergeInMux1", 2);
                fe->writeNamedRegister("DataMergeInMux2", 1);
                fe->writeNamedRegister("DataMergeInMux3", 0);

                fe->writeNamedRegister("SerSelOut0", 3);
                fe->writeNamedRegister("SerSelOut1", 3);
                fe->writeNamedRegister("SerSelOut2", 3);
                fe->writeNamedRegister("SerSelOut3", 3);
            } else {
                std::cout << "ERROR: Non-standard chip IDs read from config, please check your configs! Chip ID: " << chip_id << std::endl;
                return 1; 
            }

            while(!hw->isCmdEmpty()){;}

            std::this_thread::sleep_for(std::chrono::microseconds(200));

            hw->flushBuffer();

        }
    }

    logger->info("Running data merging test for 2-to-1 data merging...");
    std::string chip_function = "";

    // Actually do the measurement 
    for (size_t ichip = 0; ichip < n_chips; ichip++) {
        if (chip_configs[ichip]["enable"] == 0)
            continue;
        fs::path chip_register_file_path{chip_configs[ichip]["__config_path__"]};


        std::string s = "";
        auto fe = init_fe(hw, jconn, ichip);
        auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
        std::string current_chip_name = cfg->getName();
        auto jchip = ScanHelper::openJsonFile(chip_register_file_path);

        if(!fe) {
            std::cerr << "WARNING: Skipping chip at index " << ichip << " in connectivity file" << std::endl;
            continue;
        } else {
            cdrclksel = fe->getRegisterValue("CdrClkSel");
            serblckperiod = fe->getRegisterValue("ServiceBlockPeriod");

            // Wait for fifo to be empty
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            while(!hw->isCmdEmpty());

            int chip_id=jchip["RD53B"]["Parameter"]["ChipId"];

            fe->writeNamedRegister("ServiceBlockEn", 1);
            
            if (chip_id==12 || chip_id==14){
                fe->writeNamedRegister("SerSelOut0", 1);
                fe->writeNamedRegister("SerSelOut1", 1);
            } else {
                fe->writeNamedRegister("SerSelOut0", 1);
                fe->writeNamedRegister("SerSelOut1", 1);
                fe->writeNamedRegister("SerSelOut2", 1);
                fe->writeNamedRegister("SerSelOut3", 1);
            }

            while(!hw->isCmdEmpty()){;}

            std::this_thread::sleep_for(std::chrono::microseconds(200));

            hw->flushBuffer();
            std::this_thread::sleep_for(std::chrono::microseconds(10000));

            // Enable manual delay control
            mySpec.writeSingle(0x2 << 14 | 0x6, 0xffff); 

            mySpec.writeSingle(0x2 << 14 | 0x8, test_size); 
            mySpec.writeSingle(0x2 << 14 | 0x9, 0); 

            std::cout << std::fixed << std::setprecision(2);
            if (chip_id==12 || chip_id == 13){
                mySpec.writeSingle(0x2 << 14 | 0x4, lane_1); 
                mySpec.writeSingle(0x2 << 14 | 0x5, delay_1);      
            } else {
                mySpec.writeSingle(0x2 << 14 | 0x4, lane_2); 
                mySpec.writeSingle(0x2 << 14 | 0x5, delay_2);  
            }     
            // Reset and restart error counter
            mySpec.writeSingle(0x2 << 14 | 0xb, 1); 
            mySpec.writeSingle(0x2 << 14 | 0xb, 0); 
            
            std::this_thread::sleep_for(std::chrono::microseconds(wait));
            if (chip_id==12 || chip_id == 13){
                mySpec.writeSingle(0x2 << 14 | 0x4, lane_1); 
            } else {
                mySpec.writeSingle(0x2 << 14 | 0x4, lane_2); 
            }               
            uint32_t errors = 0;
            errors = mySpec.readSingle(0x2<<14 | 0xb);
            double error_count = 0;
            double value=0;
            double link_quality=0;
            if (((errors>>31)&0x1)) {
                error_count = (0x7FFFFFFF & errors);
                if (error_count>=min && error_count<=max+1 ){ 
                    value = 1;
                    link_quality=1;
                } else { 
                    value = 0; 
                    link_quality = std::log(1 / (std::abs(error_count - count) / count))/13.0;                
                }
            }

            if(chip_id==15 || chip_id ==13){
                chip_function="Primary";
            } else {
                chip_function="Secondary";               
            }
            if (print_raw_value){
                logger->info("Chip ID {0:d} ({1:s}) \t Error count: {2:.2f}", chip_id, chip_function, error_count);
            } else {
                logger->info("Chip ID {0:d} ({1:s}) \t Link quality: {2:.2f}", chip_id, chip_function, link_quality);
            }    
        }
        fe->writeNamedRegister("ServiceBlockEn", 0);
        fe->writeNamedRegister("SerSelOut0", 3);
        fe->writeNamedRegister("SerSelOut1", 3);
        fe->writeNamedRegister("SerSelOut2", 3);
        fe->writeNamedRegister("SerSelOut3", 3);

    }

    return 0;
}
