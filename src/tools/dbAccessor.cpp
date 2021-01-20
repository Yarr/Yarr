// #################################
// # Contacts: Arisa Kubota
// # Email: arisa.kubota at cern.ch
// # Date: April 2019
// # Project: Local DBHandler for Yarr
// # Description: upload to mongoDB
// ################################

#include <string>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "logging.h"
#include "LoggingConfig.h"
#include "DBHandler.h"

auto logger = logging::make_log("dbAccessor");

void printHelp();

void setLog() {
    // default log setting
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);
}

int main(int argc, char *argv[]){
    std::string db_cfg_path   = "";
    std::string user_cfg_path = "";
    std::string site_cfg_path = "";
    std::string commandType = "";

    std::string commandLine= argv[0];

    // Init parameters
    std::string scan_dir_path = "";
    std::string dcs_cfg_path = "";
    std::string influx_cfg_path = "";
    std::string conn_cfg_path = "";
    std::string scan_log_path = "";
    std::string comp_name = "";
    std::string user_name = "";
    std::string site_name = "";
    std::string output_dir_path = "";

    bool setQCMode = false;
    bool setInteractiveMode = false;

    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "hNS:E:F:RDCLQIc:s:i:p:d:u:n:")) != -1 ){
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'N': // command
                if (commandType=="") commandType = "Initialize";
                break;
            case 'S': // command
                if (commandType=="") commandType = "Scan";
                scan_dir_path = std::string(optarg);
                break;
            case 'E': // command
                if (commandType=="") commandType = "Environment";
                dcs_cfg_path = std::string(optarg);
                break;
            case 'F' : // command
                if (commandType=="") commandType= "Influxdb";
                influx_cfg_path= std::string(optarg);
  	            break;
            case 'R': // command
                if (commandType=="") commandType = "Cache";
                break;
            case 'D': // command
                if (commandType=="") commandType = "Config";
                break;
            case 'C': // command
                if (commandType=="") commandType = "Component";
                break;
            case 'L': // command
                if (commandType=="") commandType = "Log";
                break;
            case 'Q': // option
                setQCMode = true;
                break;
            case 'I': // option
                setInteractiveMode = true;
                break;
            case 'c': // option
                conn_cfg_path = std::string(optarg);
                break;
            case 's': // option
                scan_log_path = std::string(optarg);
                break;
            case 'i': // option
                site_cfg_path = std::string(optarg);
                site_name = std::string(optarg);
                break;
            case 'p': // option
                output_dir_path = std::string(optarg);
                break;
            case 'd': // option
                db_cfg_path = std::string(optarg);
                break;
            case 'u': // option
                user_cfg_path = std::string(optarg);
                user_name = std::string(optarg);
                break;
            case 'n' : // option
                comp_name= std::string(optarg);
                break;
            case '?':
                if (optopt=='S'||optopt=='E'||optopt=='F') {
                    std::cerr << "-> Option " << (char)optopt << " requires a parameter! Aborting..." << std::endl;
                    return 1;
                } else if (optopt=='c'||optopt=='s'||optopt=='i'||optopt=='p'||optopt=='d'||optopt=='u'||optopt=='n') {
                    std::cerr << "-> Option " << (char)optopt << " requires a parameter! (Proceeding with default)" << std::endl;
                } else {
                    std::cerr << "-> Unknown parameter: " << (char)optopt << std::endl;
                }
                break;
            default:
                std::cerr << "-> Error while parsing command line parameters!" << std::endl;
                return 1;
        }
    }

    if (commandType == "") {
        printHelp();
        return 1;
    }

    setLog();

    std::unique_ptr<DBHandler> database ( new DBHandler() );
    int status = 0;

    // Initialize
    if (commandType == "Initialize") {
        logger->info("DBHandler: Check DB Connection");
        database->initialize(db_cfg_path, commandLine);
        status = database->checkConnection();
    }

    // register cache
    if (commandType == "Cache") {
        logger->info("DBHandler: Register Data from Cache");
        database->initialize(db_cfg_path, commandLine, setQCMode, setInteractiveMode);
        status = database->setCache(user_cfg_path, site_cfg_path);
    }

    // register scan
    if (commandType == "Scan") {
        logger->info("DBHandler: Register Scan Data");
        database->initialize(db_cfg_path, commandLine, setQCMode, setInteractiveMode);
        database->cleanUp("scan", scan_dir_path, false, setInteractiveMode);
        status = 0;
    }

    // register component
    if (commandType == "Component") {
        logger->info("DBHandler: Register Component Data");
        if (conn_cfg_path == "") {
            logger->error("No component connecivity config file path given.");
            logger->error("Please specify file path under -c option!");
            return 1;
        }
        if (user_cfg_path == "") {
            logger->error("No user config file path given.");
            logger->error("Please specify file path under -u option!");
            return 1;
        }
        if (site_cfg_path == "") {
            logger->error("No site config file path given.");
            logger->error("Please specify file path under -i option!");
            return 1;
        }
        database->initialize(db_cfg_path, commandLine, setQCMode, setInteractiveMode);
        status = database->setComponent(conn_cfg_path, user_cfg_path, site_cfg_path);
    }

    // Retrieve/Create config files
    if (commandType == "Config") {
        logger->info("DBHandler: Retrieve Config Files");
        database->initialize(db_cfg_path, commandLine, setQCMode, setInteractiveMode);
        status = database->retrieveData(comp_name, conn_cfg_path, output_dir_path);
    }

    // cache DCS
    if (commandType == "Environment") {
        logger->info("DBHandler: Register Environment Data");
        if (scan_log_path == "") {
            logger->error("No scan log file path given.");
            logger->error("Please specify file path under -s option!");
            return 1;
        }
        else if (scan_log_path.substr(scan_log_path.find_last_of(".") + 1) != "json"){
            logger->error("Please specify the path to scanLog.json (including the file name) under -s theoption");
            return 1;
        }
        database->initialize(db_cfg_path, commandLine, setQCMode, setInteractiveMode);
        database->setDCSCfg(dcs_cfg_path, scan_log_path);
        database->cleanUp("dcs", "", false, setInteractiveMode);
        status = 0;
    }

    if (commandType == "Influxdb") {
        logger->info("DBHandler: Retrieve Influx DB");
        if (scan_log_path == "") {
            logger->error("No scan log file path given.");
            logger->error("Please specify file path under -s option!");
            return 1;
        }
        else if (scan_log_path.substr(scan_log_path.find_last_of(".") + 1) != "json"){
            logger->error("Please specify the path to scanLog.json (including the file name) under -s theoption");
            return 1;
        }
        if (comp_name == "") {
            logger->error("No chip name given.");
            logger->error("Please specify chip name under -n option!");
            return 1;
        }
        int success=0;
        database->initialize(db_cfg_path, commandLine, setQCMode, setInteractiveMode);
        success=database->retrieveFromInflux(influx_cfg_path,comp_name,scan_log_path);
        if(success==0){
            size_t last_slash=scan_log_path.find_last_of('/');
            std::string scandir="";
            if (std::string::npos != last_slash){
                scandir=scan_log_path.substr(0,last_slash);
                //std::string dcs_cfg_path=scandir+"/dcsDataInfo.json";
                std::string dcs_cfg_path="/tmp/dcsDataInfo.json";

                logger->info("DBHandler: Register Environment Data");

                database->setDCSCfg(dcs_cfg_path, scan_log_path);
                database->cleanUp("dcs", "", false, setInteractiveMode);
                database->cleanDataDir();
            }
        }
        status = 0;
    }

    if (commandType == "Log") {
        logger->info("DBHandler: Retrieve Test Log");
        database->initialize(db_cfg_path, commandLine, setQCMode, setInteractiveMode);
        status = database->checkLog(user_name, site_name, comp_name);
    }

    return status;
}

void printHelp() {
    std::cout << "Usage: dbAccessor <-command> [-option]" << std::endl;
    std::cout << "These are common DB commands used in verious situations:" << std::endl;
    std::cout << std::endl;
    std::cout << "    -h                     Shows this." << std::endl;
    std::cout << "    -N                     Check the connection to Local DB." << std::endl;
    std::cout << "    -S <result dir>        Upload scan data from the specified scan result directory into Local DB." << std::endl;
    std::cout << "    -E <dcs.json>          Upload DCS data according to the specified DCS config file into Local DB." << std::endl;
    std::cout << "       -s <scanLog.json>   Provide path to log file of scan result data to link the DCS data." << std::endl;
    std::cout << "    -F <influx.json>       Retrieve DCS data from influxDB and Upload the data according to the specified influxDB config file into Local DB." << std::endl;
    std::cout << "       -n <component name> Provide component name to link the DCS data." << std::endl;
    std::cout << "       -s <scanLog.json>   Provide path to log file of scan result data to link the DCS data." << std::endl;
    std::cout << "    -R                     Upload scan/DCS data recorded in the cache ($HOME/.yarr/localdb/run.dat or dcs.dat) into Local DB." << std::endl;
    std::cout << "       [-u <user.json>]    Provide path to user config file." << std::endl;
    std::cout << "       [-i <site.json>]    Provide path to site config file." << std::endl;
    std::cout << "    -D                     Retrieve data from Local DB. (Default. most recently saved data)" << std::endl;
    std::cout << "       [-n <cmp name>]     Provide component (chip/module) name." << std::endl;
    std::cout << "       [-p <output dir>]   Provide path to directory to put config files. (Default. ./db-data)" << std::endl;
    std::cout << "       [-c <cmp.json>]     Provide path to component connectivity config file to create chip config files." << std::endl;
    std::cout << "    -C                     Upload non-QC component data into Local DB." << std::endl;
    std::cout << "       -c <cmp.json>       Provide path to component connectivity config file to upload." << std::endl;
    std::cout << "       -u <user.json>      Provide path to user config file." << std::endl;
    std::cout << "       -i <site.json>      Provide path to site config file." << std::endl;
    std::cout << "    -L                     Display log of test data in Local DB." << std::endl;
    std::cout << "       [-u <user name>]    Provide user name to query." << std::endl;
    std::cout << "       [-i <site name>]    Provide site name to query." << std::endl;
    std::cout << "       [-n <cmp name>]     Provide component (chip/module) name to query." << std::endl;
    std::cout << std::endl;
    std::cout << "optional arguments:" << std::endl;
    std::cout << "    -Q                     Set QC mode (add a step to check if the data to upload is suitable for QC)." << std::endl;
    std::cout << "    -I                     Set interactive mode (add a step to ask the user to check the data to upload interactively)." << std::endl;
    std::cout << "    -d <database.json>     Provide path to database config file." << std::endl;
    std::cout << std::endl;
}
