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

int main(int argc, char *argv[]){

    std::string cfg_path      = "";
    std::string user_cfg_path = "";
    std::string site_cfg_path = "";
    std::string commandType = "";

    std::string commandLine= argv[0];

    // Init parameters
    std::string scan_path = "";
    std::string dcs_path = "";
    std::string conn_path = "";
    std::string scanlog_path = "";
    std::string comp_name = "";
    std::string dir_path = "";

    bool isQC = false;

    int c;
    while ((c = getopt(argc, argv, "hIRCS:E:DQc:s:i:p:d:u:F:n:")) != -1 ){
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'I':
                commandType = "Initialize";
                break;
            case 'R':
                commandType = "Cache";
                break;
            case 'S':
                commandType = "Scan";
                scan_path = std::string(optarg);
                break;
            case 'C':
                commandType = "Component";
                break;
            case 'E':
                commandType = "Environment";
                dcs_path = std::string(optarg);
                break;
            case 'D':
                commandType = "Config";
                break;
            case 'Q':
                isQC = true;
                break;
            case 'c':
                conn_path = std::string(optarg);
                break;
            case 's':
                scanlog_path = std::string(optarg);
                break;
            case 'i':
                site_cfg_path = std::string(optarg);
                break;
            case 'p':
                dir_path = std::string(optarg);
                break;
            case 'd':
                cfg_path = std::string(optarg);
                break;
            case 'u':
                user_cfg_path = std::string(optarg);
                break;
            case 'F' :
                commandType= "Influxdb";
                conn_path= std::string(optarg);
  	            break;
            case 'n' :
                comp_name= std::string(optarg);
                break;
            case '?':
                if(optopt=='E'||optopt=='S'||optopt=='F'){
                    std::cerr << "-> Option " << (char)optopt
                              << " requires a parameter! Aborting... " << std::endl;
                    return -1;
                } else {
                    std::cerr << "-> Unknown parameter: " << (char)optopt << std::endl;
                }
                break;
            default:
                std::cerr << "-> #DB ERROR# while parsing command line parameters!" << std::endl;
                return 1;
        }
    }

    // default log setting
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);

    if (commandType == "") printHelp();

    // Initialize
    if (commandType == "Initialize") {
        logger->info("DBHandler: Initialize");
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine);
        int status = database->checkConnection();
        delete database;
        return status;
    }

    // register cache
    if (commandType == "Cache") {
        logger->info("DBHandler: Register Data from Cache");
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine, isQC, true);
        int status = database->setCache(user_cfg_path, site_cfg_path);
        delete database;
        return status;
    }

    // register scan
    if (commandType == "Scan") {
        logger->info("DBHandler: Register Scan Data");
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine, isQC, true);
        database->cleanUp("scan", scan_path, false);
        delete database;
        return 0;
    }

    // register component
    if (commandType=="Component") {//TODO
        logger->info("DBHandler: Register Component Data");
        if (conn_path == "") {
            logger->error("No component connecivity config file path given.");
            logger->error("Please specify file path under -c option!");
            return 1;
        }
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine);
        int status = database->setComponent(conn_path, user_cfg_path, site_cfg_path);
        delete database;
        return status;
    }

    // cache DCS
    if (commandType == "Environment") {
        logger->info("DBHandler: Register Environment Data");
        DBHandler *database = new DBHandler();
        if (scanlog_path == "") {
            logger->error("No scan log file path given.");
            logger->error("Please specify file path under -s option!");
            return 1;
        }
        database->initialize(cfg_path, commandLine, isQC);
        database->setDCSCfg(dcs_path, scanlog_path, user_cfg_path, site_cfg_path);
        database->cleanUp("dcs", "", false);

        delete database;
        return 0;
    }

    // Retrieve/Create config files
    if (commandType == "Config") {
        logger->info("DBHandler: Retrieve Config Files");
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine);
        int status = database->retrieveComponentData(comp_name, conn_path, dir_path);
        return status;
    }

    if (commandType == "Influxdb") {
        logger->info("DBHandler: Retrieve Influx DB");
        if (scanlog_path == "") {
            std::cerr << "#DB ERROR# No scan log file path given, please specify file path under -s option!" << std::endl;
            return 1;
        }
        if (comp_name == "") {
            std::cerr << "#DB ERROR# No chip name given, please specify chipname under -n option!" << std::endl;
            return 1;
        }
        int success=0;
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine);
        success=database->retrieveFromInflux(conn_path,comp_name,scanlog_path);
        if(success==0){
            size_t last_slash=scanlog_path.find_last_of('/');
            std::string scandir="";
            if (std::string::npos != last_slash){
                scandir=scanlog_path.substr(0,last_slash);
                //std::string dcs_path=scandir+"/dcsDataInfo.json";
                std::string dcs_path="/tmp/dcsDataInfo.json";

                std::cout << "DBHandler: Register Environment:" << std::endl;
                std::cout << "\tenvironmental config file : " << dcs_path << std::endl;

                database->setDCSCfg(dcs_path, scanlog_path, user_cfg_path, site_cfg_path);
                database->cleanUp("dcs", "", false);
                database->cleanDataDir();
            }
        }
        delete database;
        return 0;
    }
    return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -I: Check the connection to Local DB." << std::endl;
    std::cout << " -S <scan result>: Provide scan result directory to upload scan data into Local DB." << std::endl;
    std::cout << " -E <dcs.json> : Provide DCS configuration to upload DCS data into Local DB." << std::endl;
    std::cout << "     -s <scanLog.json> : Provide scan log file." << std::endl;
    std::cout << " -R: Upload data into Local DB from cache." << std::endl;
    std::cout << " -C: Upload component data into Local DB." << std::endl;
    std::cout << "     -c <component.json> : Provide component connectivity configuration." << std::endl;
    std::cout << " -D: Download/Create config files." << std::endl;
    std::cout << "     -n <component name> : Provide component name." << std::endl;
    std::cout << "     -p <conn dir> : Provide directory path to put config files." << std::endl;
    std::cout << "     -c <component.json> : Provide connectivity configuration to create chip config files." << std::endl;
    std::cout << " " << std::endl;
    std::cout << " -d <database.json> : Provide database configuration." << std::endl;
    std::cout << " -i <site.json> : Provide site configuration." << std::endl;
    std::cout << " -u <user.json> : Provide user configuration." << std::endl;
    std::cout << std::endl;
}
