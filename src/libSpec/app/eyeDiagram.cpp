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
#include "Rd53b.h"
#include "ScanHelper.h"
#include "Utils.h"


auto logger = logging::make_log("eyeDiagram");

constexpr const char* COLOR_GREEN = "\033[32m";
constexpr const char* COLOR_RESET = "\033[0m";

void printHelp() {
    std::cout << "./bin/eyeDiagram -r Hardware controller JSON path" << std::endl;
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
    fe->init(&*hw, chip_config["tx"], chip_config["rx"]);
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
    int delay = 16;
    int n_lanes= 4;
    int port_offset = 0;
    std::string hw_controller_filename = "";
    std::string connectivity_filename = "";

    while ((c = getopt(argc, argv, "hr:c:")) != -1) {
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


    json jcontroller;
    jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
    specNum=jcontroller["ctrlCfg"]["cfg"]["specNum"];

    SpecCom mySpec(specNum);

    logger->info("Scanning link quality against delay on Spec Card {}", specNum);



    // instantiate the hw controller
    std::unique_ptr<HwController> hw;
    try {
        jcontroller = ScanHelper::openJsonFile(hw_controller_filename);
        hw = ScanHelper::loadController(jcontroller);
    } catch (std::exception& e) {
        std::cerr << "ERROR: Unable to load controller from provided config, exception caught: " << e.what() << std::endl;
        return 1;
    }
    hw->setupMode();
    hw->setTrigEnable(0);
    hw->disableRx(); // needed?

    auto jconn = ScanHelper::openJsonFile(connectivity_filename);

    std::string chipType = ScanHelper::loadChipConfigs(jconn, false, Utils::dirFromPath(connectivity_filename));

    auto chip_configs = jconn["chips"];
    size_t n_chips = chip_configs.size();

    for (size_t ichip = 0; ichip < n_chips; ichip++) {
        if (chip_configs[ichip]["enable"] == 0)
            continue;
        fs::path chip_register_file_path{chip_configs[ichip]["__config_path__"]};
        if(!fs::exists(chip_register_file_path)) {
            std::cerr << "WARNING: Chip config for chip at index " << ichip << " in connectivity file does not exist, skipping (" << chip_register_file_path << ")" << std::endl;
            continue;
        }
        
        auto fe = init_fe(hw, jconn, ichip);
        if(!fe) {
            std::cerr << "WARNING: Skipping chip at index " << ichip << " in connectivity file" << std::endl;
            continue;
        } else {
            fe->configure();
            // Wait for fifo to be empty
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            while(!hw->isCmdEmpty());
        }
    }


    std::ofstream file;
    file.open("results.txt");

	// Enable manual delay control
	mySpec.writeSingle(0x2 << 14 | 0x6, 0xffff); 

	// Write error counter stop value and mode
    uint32_t test_size = 10e6;
	mySpec.writeSingle(0x2 << 14 | 0x8, test_size); 
	mySpec.writeSingle(0x2 << 14 | 0x9, 0); 

    std::vector<std::vector<double> > resultVec;
    resultVec.resize(n_lanes);
    for (uint32_t j=0; j<n_lanes; j++)
        resultVec[j].resize(32);

    std::cout << std::fixed << std::setprecision(0);
    std::string s = "";
    for (uint32_t i = 0; i<32; i++) {
        s+=std::to_string(i)+" | ";
        std::cout << std::setw(5) << i << " | ";
        for (uint32_t j = 0 ; j<n_lanes; j++) {
            mySpec.writeSingle(0x2 << 14 | 0x4, j+port_offset); 
            mySpec.writeSingle(0x2 << 14 | 0x5, i); 
        }
    
        // Wait for sync
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Reset and restart error counter
        mySpec.writeSingle(0x2 << 14 | 0xb, 1); 
        mySpec.writeSingle(0x2 << 14 | 0xb, 0); 
        
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        for (uint32_t j = 0 ; j<n_lanes; j++) {
		    mySpec.writeSingle(0x2 << 14 | 0xa, j+port_offset); 
		    uint32_t errors = 0;
		    errors = mySpec.readSingle(0x2<<14 | 0xb);
            double error_count = 0;
            double value=0;
            double link_quality=0;
            if (((errors>>31)&0x1)) {
                error_count = (0x7FFFFFFF & errors);
                if (error_count==204208 || error_count==204207){ 
                    value = 1;
                    link_quality=1;
                } else { 
                    value = 0; 
                    link_quality = std::log(1 / (std::abs(error_count - 204208) / 204208))/13.0;                }
            }
            resultVec[j][i] = value;
            if (link_quality==1){
                std::cout << COLOR_GREEN << std::setw(10) << error_count << COLOR_RESET << " | ";
            } else {            
                std::cout << std::setw(10) << error_count << " | ";
            }
            s+=std::to_string(link_quality)+" | ";

        }
        s+="\n";
        std::cout << std::endl;
    }
    file << s;
	logger->info("Done scanning!  \n");

	logger->info("Determining delay settings:");

    std::vector<int> delayVec;
    delayVec.resize(n_lanes);

    for (uint32_t i = 0 ; i<n_lanes; i++) {
        double start_val=0;
        double width=0;
        double last_width=0;
        double best_val=0;
        double best_width=0;
        for (uint32_t j = 0 ; j<32; j++) {
            if (resultVec[i][j]==1){
                if (width==0) start_val=j;
                width+=1; 
            } else {
                if(width>last_width){
                    best_val=start_val;
                    best_width=width;
                }   
                last_width=width;
                width=0;
            }
        }    
        int delay=0;
        if (best_width!=0){ 
            delay=(int) best_val+(best_width/2);
            logger->info("Delay setting for lane {} with eye width {}: {}", i, best_width, delay);
        } else {
            logger->info("No good delay setting for lane {}", i);
        }
        delayVec[i] = delay;
    }    

	logger->info("Writing to controller config {}", hw_controller_filename);
    jcontroller["ctrlCfg"]["cfg"]["delay"]=delayVec;
    std::ofstream outputFile(hw_controller_filename);
    outputFile << jcontroller << std::endl;
    outputFile.close();
	logger->info("All done! \n");

    return 0;
}
