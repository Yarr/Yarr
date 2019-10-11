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
#include "DBHandler.h"



void printHelp();

int main(int argc, char *argv[]){

    std::string home = getenv("HOME");
    std::string hostname = getenv("HOSTNAME");
    std::string dbDirPath = home+"/.yarr/localdb";
    std::string cfg_path = dbDirPath+"/"+hostname+"_database.json";
    std::string user_cfg_path = dbDirPath+"/user.json";
    std::string site_cfg_path = dbDirPath+"/"+hostname+"_site.json";
    std::string registerType = "";

    std::string commandLine= argv[0];

    // Init parameters
    std::string dcs_path = "";
    std::string conn_path = "";
    std::string scanlog_path = "";

    //for influxDB
    std::string influx_conn_path="";
    std::string influx_chip_name="";


    int c;
    while ((c = getopt(argc, argv, "hIRCE:Mc:s:i:d:u:F:n:")) != -1 ){
        switch (c) {
	case 'h':
                printHelp();
                return 0;
                break;
            case 'I':
                registerType = "Initialize";
                break;
            case 'R':
                registerType = "Cache";
                break;
            case 'C':
                registerType = "Component";
                break;
            case 'E':
                registerType = "Environment";
                dcs_path = std::string(optarg);
                break;
            case 'M':
                registerType = "Module";
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
            case 'd':
                cfg_path = std::string(optarg);
                break;
	    case 'u':
                user_cfg_path = std::string(optarg);
                break;
	    case 'F' :
	        registerType= "Influxdb";
	        influx_conn_path= std::string(optarg);
		break;
	    case 'n' :
	        influx_chip_name= std::string(optarg);
		break;
            case '?':
                if(optopt=='R'||optopt=='U'||optopt=='C'||optopt=='E'||optopt=='S'||optopt=='D'||optopt=='G'||optopt=='F'){
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

    if (registerType == "") printHelp();

    // Initialize
    if (registerType == "Initialize") {
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine);
        int status = database->checkConnection();
        delete database;
        return status;
    }

    // register cache
    if (registerType == "Cache") {
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine);
        int status = database->setCache(user_cfg_path, site_cfg_path);
        delete database;
        return status;
    }

    // register component
    if (registerType=="Component") {
        if (conn_path == "") {
            std::cerr << "#DB ERROR# No component connecivity config file path given, please specify file path under -c option!" << std::endl;
            return 1;
        }
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine, "register");
        int status = database->setComponent(conn_path, user_cfg_path, site_cfg_path);
        delete database;
        return status;
    }

    // cache DCS
    if (registerType == "Environment") {
        DBHandler *database = new DBHandler();
        if (scanlog_path == "") {
            std::cerr << "#DB ERROR# No scan log file path given, please specify file path under -s option!" << std::endl;
            return 1;
        }
        std::cout << "DBHandler: Register Environment:" << std::endl;
        std::cout << "\tenvironmental config file : " << dcs_path << std::endl;

        database->initialize(cfg_path, commandLine, "register");
        database->setDCSCfg(dcs_path, scanlog_path, user_cfg_path, site_cfg_path);
        database->cleanUp("dcs", "");

        delete database;
        return 0;
    }

    if (registerType == "Module") {
        DBHandler *database = new DBHandler();
        database->initialize(cfg_path, commandLine);
        int status = database->checkModule();
        delete database;
        return status;
    }
    if (registerType == "Influxdb") {
      if (scanlog_path == "") {
	std::cerr << "#DB ERROR# No scan log file path given, please specify file path under -s option!" << std::endl;
	return 1;
      }
      if (influx_chip_name == "") {
	std::cerr << "#DB ERROR# No chip name given, please specify chipname under -n option!" << std::endl;
	return 1;
      }
      int success=0;
      DBHandler *database = new DBHandler();
      database->initialize(cfg_path, commandLine, "register");
      database->init_influx(commandLine);
      success=database->retrieveFromInflux(influx_conn_path,influx_chip_name,scanlog_path);
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
	  database->cleanUp("dcs", "");
	  //database->cleanDataDir(scanlog_path);                                                                                                                        
	  database->cleanDataDir();
	}
      }
      delete database;
      return 0;
    }
    return 0;
}

void printHelp() {
    std::string home = getenv("HOME");
    std::string hostname = getenv("HOSTNAME");
    std::string dbDirPath = home+"/.yarr/localdb";
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -I: Check the connection to Local DB." << std::endl;
    std::cout << " -C: Upload component data into Local DB." << std::endl;
    std::cout << "     -c <component.json> : Provide component connectivity configuration." << std::endl;
    std::cout << " -R: Upload data into Local DB from cache." << std::endl;
    std::cout << " -E <dcs.json> : Provide DCS configuration to upload DCS data into Local DB." << std::endl;
    std::cout << "     -s <scanLog.json> : Provide scan log file." << std::endl;
    std::cout << " -M : Retrieve Module list from Local DB." << std::endl;
    std::cout << " " << std::endl;
    std::cout << " -d <database.json> : Provide database configuration. (Default " << dbDirPath << "/" << hostname << "_database.json)" << std::endl;
    std::cout << " -i <site.json> : Provide site configuration. (Default " << dbDirPath << "/" << hostname << "_site.json)" << std::endl;
    std::cout << " -u <user.json> : Provide user configuration. (Default " << dbDirPath << "/user.json)" << std::endl;
    std::cout << std::endl;
}
