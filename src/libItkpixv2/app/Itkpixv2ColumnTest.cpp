// ############################
// # Author: Charles Hultquist
// # Email: chultquist at lbl dot gov
// # Project: Yarr
// # Description: ITKPIXV2 Core Column Issue Testing
// # Date: January 2024
// ############################

#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <filesystem>
namespace fs = std::filesystem;
#include <unistd.h>

#include "storage.hpp"
#include "logging.h"
#include "LoggingConfig.h"

#include "ScanHelper.h"
#include "HwController.h"
#include "FrontEnd.h"
#include "AllChips.h"
#include "itkpix_efuse_codec.h"

#include "Itkpixv2.h"
#include "Itkpixv2Cmd.h"
#include "Utils.h"


auto logger = logging::make_log("itkpixv2ColumnTest");

void printHelp() {
    std::cout << "-c <string> : path to connectivity" << std::endl;
    std::cout << "-r <string> : path to controller" << std::endl;
    std::cout << "-n <int> : number of tests to run. Default 1" << std::endl;
    std::cout << "-w : write to chip configs. Default false" << std::endl;
    std::cout << "-e: Enable all pixels turned on (1) or off (0). Default 0." << std::endl;
}

namespace itkpixv2Test {
    std::pair<uint32_t, uint32_t> decodeSingleRegRead(uint32_t higher, uint32_t lower) {
        if ((higher & 0x55000000) == 0x55000000) {
            return std::make_pair((lower>>16)&0x3FF, lower&0xFFFF);
        } else if ((higher & 0x99000000) == 0x99000000) {
            return std::make_pair((higher>>10)&0x3FF, ((lower>>26)&0x3F)+((higher&0x3FF)<<6));
        } else {
            logger->error("Could not decode reg read!");
            return std::make_pair(999, 666);
        }
        return std::make_pair(999, 666);
    }
}

std::string toBinary(int n, int len){
  std::string r;
  while(n!=0) {r=(n%2==0 ?"0":"1")+r; n/=2;}
  std::string fullstring = std::string(len - r.length(),'0')+r;
  return fullstring;
}

std::string or_bitwise(std::string str1, std::string str2){
  std::string str;
  //cout << str;
  for(int i=0; i<str1.length(); i++){
    str+= ((str1[i] == '1') || (str2[i] == '1')) ? '1' : '0';
  }
  return str;
}

std::string flip(std::string str){
  std::string str2;
  for(int i=0; i<str.length(); i++){
    str2 += (str[i] == '1') ? '0' : '1';
  }
  return str2;
}

std::vector<int> check_results(std::vector<std::vector<int>> results){

  const int numEnCoreColsVars = 4;
  int numTests = results.size();
  int numColsInVars [numEnCoreColsVars] = {16,16,16,6};
  std::vector<int> final_results = {0,0,0,0};
  std::vector<std::string> final_results_str = {"","","",""};
  for (int ivar=0; ivar<numEnCoreColsVars; ivar++){
    final_results_str[ivar] = toBinary(0,numColsInVars[ivar]);
  }

  for (int ivar=0; ivar<numEnCoreColsVars; ivar++){
    for (int itest=0; itest<numTests; itest++){
      std::string fullstr = toBinary(results[itest][ivar],numColsInVars[ivar]);
      final_results_str[ivar] = or_bitwise(final_results_str[ivar],fullstr);
    }
  }

  for (int ivar=0; ivar<numEnCoreColsVars; ivar++){
    std::string goodcolumns = flip(final_results_str[ivar]);
    logger->info("Columns Passing All Tests in Core Column Set {} : {}", ivar,goodcolumns);
    int encore = std::bitset<64>(goodcolumns).to_ullong();
    logger->info("Corresponding to EnCoreCol{} = {}",ivar ,encore);
    final_results[ivar] = encore;
  }
  
  
  return final_results;
}



void clear_and_flush(std::unique_ptr<HwController>& hwCtrl, int msec){
  hwCtrl->writeFifo(0xAAAA0000 |Itkpixv2Cmd::genClear(16)[0]);
  std::this_thread::sleep_for(std::chrono::milliseconds(msec));
  hwCtrl->flushBuffer();
  return;
}

bool check_data(std::unique_ptr<HwController>& hwCtrl){
  
  std::vector<RawDataPtr> dataVec = hwCtrl->readData();
  RawDataPtr data;

  //Case 1: No data received
  if  (dataVec.size() <= 0) {
    logger->critical("Didn't receive data back");
    clear_and_flush(hwCtrl,200);
    logger->info("Continuing test...");
    return false;
  }
 
  data= dataVec[0];

  std::vector<int> tags(8);
  int fill_count = 0;
  std::vector<int> include_mask;
  for (unsigned i=0; i<data->getSize();i+=2) {
    uint32_t tag = (data->get(i) & 0x7F800000) >> 23; 
    int include = data->get(i) >> 31;
    include_mask.push_back(include);

    if (fill_count <= 7){
      if(include){
	tags.at(fill_count) = tag;
	fill_count += 1;
	int corecol = (data->get(i) & 0x7E0000) >> 17;
	int is_last = (data->get(i) & 0x10000) >> 16;
	int is_neighbor = (data->get(i) & 0x8000) >> 15;
	int qrow = (data->get(i) & 0x7F80) >> 7;
      } 
    }
  }
  
  int ok=0;
  for (int i=0; i<8; i++){
    if (tags.at(i)==i) ok++;
  }

  //Case 2: Data received, was wrong
  if (ok != 8){
    logger->error("Only {} of the tags were correct. {} tags were received. 8 tags were expected", ok, (data->getSize())/2);
    clear_and_flush(hwCtrl,100);
    return false; 
  }

  //Case 3: Data received, was correct
  clear_and_flush(hwCtrl,20);
  return true;
  
}

void writeConfig(json &jconn, int fe_num, std::vector<int> results){
  std::string chip_type = jconn["chipType"];
  auto fe = StdDict::getFrontEnd(chip_type);
  auto cfg = dynamic_cast<FrontEndCfg*>(fe.get());
  auto chip_configs = jconn["chips"];
  auto chip_config = chip_configs[fe_num];
  auto chip_register_file_path = chip_config["__config_path__"];
  fs::path pconfig{chip_register_file_path};
  auto chip_register_json = ScanHelper::openJsonFile(chip_register_file_path);


  std::string varNames [4] = {"EnCoreCol0","EnCoreCol1", "EnCoreCol2", "EnCoreCol3"};
  for (int ivar=0; ivar<4; ivar++){
    chip_register_json[chip_type]["GlobalConfig"][varNames[ivar]]=results[ivar];
  }
  std::ofstream outputFile(chip_register_file_path);
  outputFile << chip_register_json << std::endl;
  outputFile.close();
  return;

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
    unsigned tx = chip_config["tx"];
    unsigned rx = chip_config["rx"];
    FrontEndConnectivity fecon(tx,rx);
    //fe->init(&*hw, tx, rx);
    fe->init(&*hw, fecon);
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

int main (int argc, char *argv[]) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    j["log_config"][1]["name"] = "all";
    j["log_config"][1]["level"] = "info";
    j["log_config"][1]["sink"] = "file";
    j["log_config"][2]["name"] = "Itkpixv2GlobalCfg";
    j["log_config"][2]["level"] = "critical";
    j["sinks"][0]["name"]="file";
    j["sinks"][0]["level"]="info";
    j["sinks"][0]["file_name"]="console_column.log";
    j["sinks"][0]["pattern"]=defaultLogPattern;
    logging::setupLoggers(j);

    logger->info("Parsing command line parameters ...");
    int c;
    int numTests = 1;
    bool editConfig = 0;
    bool use_fcn=false;
    int turnOnPixels=0;
    std::string connFilePath = "configs/JohnDoe.json";
    std::string ctrlFilePath = "configs/controller/specCfg.json";
    while ((c = getopt(argc, argv, "hc:r:n:e:w")) != -1) {
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'r':
                ctrlFilePath = std::string(optarg);
                break;
            case 'c':
                connFilePath = std::string(optarg);
                break;
	    case 'n': 
	        numTests = std::atoi(optarg);
		break;
	    case 'w': 
	        editConfig = 1;
		break;
	    case 'e': 
	        turnOnPixels = std::atoi(optarg);
		break;

            default:
                spdlog::critical("No command line parameters given!");
                return -1;
        }
    }

    logger->info("Connectivity file path  : {}", connFilePath);
    logger->info("Ctrl config file path  : {}", ctrlFilePath);
    logger->info("Number of tests : {}", numTests);
    if (turnOnPixels!=0 && turnOnPixels!=1) {
      logger->critical("-e Option must be 0 or 1");
      return -1;
    }

    logger->info("\033[1;31m#################\033[0m");
    logger->info("\033[1;31m# Init Hardware #\033[0m");
    logger->info("\033[1;31m#################\033[0m");

    logger->info("-> Opening controller config: {}", ctrlFilePath);

    std::unique_ptr<HwController> hwCtrl = nullptr;
    json ctrlCfg;
    try {
        ctrlCfg = ScanHelper::openJsonFile(ctrlFilePath);
        hwCtrl = ScanHelper::loadController(ctrlCfg);
    } catch (std::runtime_error &e) {
        logger->critical("Error opening or loading controller config: {}", e.what());
        return -1;
    }
    
    hwCtrl->runMode();
    hwCtrl->setCmdEnable(0);
    hwCtrl->setTrigEnable(0x0);
    
    logger->info("\033[1;31m###########################\033[0m");
    logger->info("\033[1;31m##  Connectivity Config  ##\033[0m");
    logger->info("\033[1;31m###########################\033[0m");

    std::ifstream connFile(connFilePath);
    json conn;
    if (connFile) {
        // Load config
        logger->info("Loading connnectivity file: {}", connFilePath);
        
        try {
            conn = ScanHelper::openJsonFile(connFilePath);
        } catch (std::runtime_error &e) {
            logger->error("Error opening connectivity config: {}", e.what());
            throw(std::runtime_error("loadChips failure"));
        }
        connFile.close();
    } else {
      logger->error("No connectivity file supplied");
      throw(std::runtime_error("loadChips failure"));
    }

    std::string chipType = ScanHelper::loadChipConfigs(conn,false,Utils::dirFromPath(connFilePath));

    json chip_configs = conn["chips"];
    size_t n_chips = chip_configs.size();

    hwCtrl->runMode();
    hwCtrl->setTrigEnable(0);
    hwCtrl->disableRx();
    std::vector<int> testing (n_chips,0);

    std::vector<std::vector<std::vector<int>>> badCoreCols_all_chips;
    //Run through chips
    for (size_t ichip = 0; ichip < n_chips; ichip++){

      //Set Up Chips
      logger->info("Testing Chip {} ...", ichip);
      if (chip_configs[ichip]["enable"] == 0)
	continue;
      
      testing.at(ichip)=1;

      fs::path chip_register_file_path{chip_configs[ichip]["__config_path__"]};
      auto itkpixv2_fe = init_fe(hwCtrl, conn, ichip);
      Itkpixv2 *itkpixv2 = dynamic_cast<Itkpixv2*>(itkpixv2_fe.get());
      auto feCfg = dynamic_cast<FrontEndCfg*>(itkpixv2_fe.get());

      logger->info("Enable Tx and Rx");
      hwCtrl->setCmdEnable(feCfg->getTxChannel());
      hwCtrl->setTrigEnable(0x0);
      hwCtrl->setRxEnable(feCfg->getRxChannel());

      logger->info("Configure Chip...");
      itkpixv2->configure();
      clear_and_flush(hwCtrl,10);

      const int numEnCoreColsVars = 4;
      int numColsInVars [numEnCoreColsVars] = {16,16,16,6};
      std::string varNames [numEnCoreColsVars] = {"EnCoreCol0","EnCoreCol1", "EnCoreCol2", "EnCoreCol3"};
      std::string varNamesCal [numEnCoreColsVars] = {"EnCoreColCal0","EnCoreColCal1", "EnCoreColCal2", "EnCoreColCal3"};
      std::string varNamesHitOr [numEnCoreColsVars] = {"HitOrMask0","HitOrMask1", "HitOrMask2", "HitOrMask3"};
      std::string varNamesRes [numEnCoreColsVars] = {"RstCoreCol0","RstCoreCol1", "RstCoreCol2", "RstCoreCol3"};
      std::string varNamesPtot [numEnCoreColsVars] = {"PtotCoreColEn0","PtotCoreColEn1","PtotCoreColEn2","PtotCoreColEn3"};

      unsigned nCol = Itkpixv2::n_Col;
      unsigned nRow = Itkpixv2::n_Row;

      if (turnOnPixels == 0) logger->info("Turning all pixels off");
      else if (turnOnPixels == 1) logger->info("Turning all pixels on");
      
      std::vector<std::pair<unsigned,unsigned>> allPixels;
      for (unsigned col=0; col<nCol; col++){
	for (unsigned row =0; row<nRow; row++){
	  itkpixv2->setEn(col,row,turnOnPixels);
	  itkpixv2->setInjEn(col,row,turnOnPixels);
	  itkpixv2->setHitbus(col,row,turnOnPixels);
	  allPixels.push_back(std::make_pair(col,row));
	}
      }
      itkpixv2->configurePixels(allPixels);
      //logger->info("Done");

      
      

      while(!hwCtrl->isCmdEmpty());
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      hwCtrl->flushBuffer();

      std::vector<std::vector<int>> badCoreCols_all;

      //Loop through tests
      for (int itest = 0; itest<numTests; itest++){

	logger->info("Starting test {}", itest);
	hwCtrl->writeFifo(0xAAAA0000 | Itkpixv2Cmd::genClear(16)[0]);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	//Set everything to 0 to start
	for (int ivar = 0; ivar<numEnCoreColsVars; ivar++){
	  std::string colName = varNames[ivar];
	  itkpixv2->writeNamedRegister(colName,0);
	}

	hwCtrl->writeFifo(0xAAAA0000 | Itkpixv2Cmd::genClear(16)[0]);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	hwCtrl->flushBuffer();
	hwCtrl->writeFifo(0xAAAA0000 | Itkpixv2Cmd::genClear(16)[0]);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	hwCtrl->flushBuffer();
	yarrStatus stat;

	uint32_t efuse_data_raw_orig = itkpixv2->readEfusesRaw();
	itkpix_efuse_codec::EfuseData efuse_data_original = itkpix_efuse_codec::EfuseData{itkpix_efuse_codec::decode(efuse_data_raw_orig)};
	logger->info("Read efuse with everything off as 0x{:x}",efuse_data_original.chip_sn());
	if( efuse_data_original.chip_sn() == 0){
	  logger->critical("Unable to read efuses with all columns turned off. Please check chip connection");
	}

	std::vector<int> badCoreCols = {0,0,0,0};

	//Loop through column variables
	//for (int ivar = 0; ivar<numEnCoreColsVars; ivar++){
	for (int ivar = 0; ivar<numEnCoreColsVars; ivar++){
	  int numCols = numColsInVars[ivar];
	  std::string colName = varNames[ivar];
	  std::string colNameCal = varNamesCal[ivar];
	  std::string colNameHitOr = varNamesHitOr[ivar];
	  std::string colNameRes = varNamesRes[ivar];
	  std::string colNamePtot = varNamesPtot[ivar];
	  for (int icol = 0; icol<numCols; icol++){
	    //Turn one column on at a time
	    int valToSet = (int) pow(2, icol);
	    int valHitOr = (int) (pow(2, numCols) - pow(2,icol))-1;
	    
	    uint16_t init_ptot;
	    stat = itkpixv2->readNamedRegister(colNamePtot, init_ptot);
	    stat = itkpixv2->writeNamedRegister(colName,valToSet);

	    logger->debug("Set {} from chip {} to {}",colName,ichip,valToSet);
	    stat = itkpixv2->writeNamedRegister(colNameRes,valToSet);
	    stat = itkpixv2->writeNamedRegister(colNameCal,valToSet);
	    stat = itkpixv2->writeNamedRegister(colNameHitOr,valHitOr);

	    logger->info("Set {} from chip {} to {}",colName,ichip,valToSet);

	    //Read Efuses
	    uint32_t efuse_data_raw = itkpixv2->readEfusesRaw();
	    itkpix_efuse_codec::EfuseData efuse_data = itkpix_efuse_codec::EfuseData{itkpix_efuse_codec::decode(efuse_data_raw)};
	    logger->info("Read efuses as 0x{:x}",efuse_data.chip_sn());
	    //If efuse isn't read correctly, mark as bad column
	    bool good_result = (efuse_data_original.chip_sn() == efuse_data.chip_sn());
       
	    if (good_result){
	      //In case where there is a consistent issue with some dead pixel
	      //often pixel region, try sending trigger 
	      auto trigger1 = (uint32_t)Itkpixv2Cmd::genTrigger(0xF,0)[0] << 16;
	      auto trigger2 = Itkpixv2Cmd::genTrigger(0xF,1)[0];
	      hwCtrl->writeFifo(trigger1 | trigger2);
	      hwCtrl->releaseFifo();

	      while(!hwCtrl->isCmdEmpty());
	      std::this_thread::sleep_for(std::chrono::milliseconds(200));

	    
	      //Send and read triggers
	      good_result = check_data(hwCtrl);
	      if (!good_result) badCoreCols[ivar] += valToSet;
	      if (good_result) logger->info("Can send and read triggers properly");
	    } else {
	      badCoreCols[ivar] += valToSet;
	    }

	    //Reset
	    stat = itkpixv2->writeNamedRegister(colName,0);
	    stat = itkpixv2->writeNamedRegister(colNameRes,0);
	    stat = itkpixv2->writeNamedRegister(colNameCal,0);
	    stat = itkpixv2->writeNamedRegister(colNameHitOr,(int)pow(2, numCols)-1);
	    hwCtrl->writeFifo(0xAAAA0000 | Itkpixv2Cmd::genClear(16)[0]);
	    std::this_thread::sleep_for(std::chrono::milliseconds(200));
	    hwCtrl->writeFifo(0xAAAA0000 | Itkpixv2Cmd::genClear(16)[0]);
	    std::this_thread::sleep_for(std::chrono::milliseconds(100));
	    hwCtrl->flushBuffer();

	  }//End loop over columns in one EnCoreCol
	}//End loop over EnCoreCol variables

	logger->info("Results for test {}", itest);
	for (int ivar = 0; ivar<numEnCoreColsVars; ivar++){
	  logger->info("Bad Core Column Variables in {} = {}", ivar,badCoreCols[ivar]);
	}
	badCoreCols_all.push_back(badCoreCols);
      
      }//End loop through tests
      badCoreCols_all_chips.push_back(badCoreCols_all);

    }//End loop over chips

    logger->info("\033[1;31m#####################\033[0m");
    logger->info("\033[1;31m##  <3 Results <3  ##\033[0m");
    logger->info("\033[1;31m#####################\033[0m");

    int r_count = 0;
    for (size_t ichip = 0; ichip < n_chips; ichip++){

      if (!(testing.at(ichip))) continue;
      
      logger->info("");
      logger->info("#### Chip {} ####",ichip);
      std::vector<int> final_results = check_results(badCoreCols_all_chips[r_count]);
      r_count +=1;
      if(editConfig){
	logger->info("Writing results to Config for chip {}", ichip);
	writeConfig(conn, ichip, final_results);
      } else {
	logger->info("NOT writing results to Config for chip {}", ichip);
      }
    }
    
    

    logger->info("... done! Thanks for playing!");
    hwCtrl->disableRx();
    return 0;       
}
