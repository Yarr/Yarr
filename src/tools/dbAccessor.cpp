// #################################
// # Contacts: Arisa Kubota
// # Email: arisa.kubota at cern.ch
// # Date: April 2019
// # Project: Local DBHandler for Yarr
// # Description: upload to mongoDB
// ################################


#include <iostream>
#include <cstdlib>
#include <string>
#include "json.hpp"
#include <unistd.h>

#include "DBHandler.h"

void printHelp();

int main(int argc, char *argv[]){

    std::string registerType = "";

    // register component
	  std::string dbConnPath = "";
    
    // register user
    std::string dbUserAccount = "";
    std::string dbUserName = "";
    std::string dbUserIdentity = "";
	  std::string dbInstitution = "";
	  std::string dbUserCfgPath = "";

    // register environment
    std::string dbEnvPath = "";
    
    // register json
    std::string dbJsonPath = "";
    std::string dbJsonType = "";
    std::string dbJsonTitle = "";

    // get json
    std::string dbJsonId = "";
    std::string dbChipId = "0";
    std::string dbName = "test";
    std::string dbType = "";

    // register from directory
    std::string dbDir = "";

    int c;
    while ((c = getopt(argc, argv, "hD:U:C:J:E:G:Sn:u:i:p:c:d:j:t:")) != -1 ){
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'D':
                registerType = "Directory";
                dbDir = std::string(optarg);
                break;
            case 'U':
                registerType = "User";
                dbUserAccount = std::string(optarg);
                break;
            case 'C':
                registerType = "Component";
                dbConnPath = std::string(optarg);
                break;
            case 'E':
                registerType = "Environment";
                dbEnvPath = std::string(optarg);
                break;
            case 'J':
                registerType = "Json";
                dbJsonPath = std::string(optarg);
                break;
            case 'G':
                registerType = "getJson";
                dbJsonPath = std::string(optarg);
                break;
            case 'S':
                registerType = "Site";
                break;
            case 'n':
                dbUserName = std::string(optarg);
                dbName = std::string(optarg);
                break;
            case 'i':
                dbInstitution = std::string(optarg);
                dbJsonId = std::string(optarg);
                break;
            case 'u':
                dbUserIdentity = std::string(optarg);
                break;
            case 'j':
                dbJsonType = std::string(optarg);
                break;
            case 'p':
                dbUserCfgPath = std::string(optarg);
                break;
            case 'c':
                dbChipId = std::string(optarg);
                break;
            case 't':
                dbType = std::string(optarg);
                dbJsonTitle = std::string(optarg);
                break;
            case '?':
                if(optopt == 'D'||optopt == 'U'||optopt == 'C'||optopt == 'E'||optopt == 'J'||optopt == 'G'){
                    std::cerr << "-> Option " << (char)optopt
                              << " requires a parameter! Aborting... " << std::endl;
                    return -1;
                } else {
                    std::cerr << "-> Unknown parameter: " << (char)optopt << std::endl;
                }
                break;
            default:
                std::cerr << "-> Error while parsing command line parameters!" << std::endl;
                return -1;
        }
    }

    if (registerType == "") printHelp();

//    // register from directory
//    if (registerType == "Directory") {
//        std::cout << "DBHandler: Register directory: " << dbDir << std::endl;
//
//        DBHandler *database = new DBHandler();
//    	  database->writeFromDirectory("","",dbDir,"");
//        delete database;
//    }
//
    // register user
    if (registerType == "User") {
        std::cout << "DBHandler: Login user: \n\taccount: " << dbUserAccount << std::endl;
        std::string home = getenv("HOME");
        if (dbUserCfgPath == "") {
            dbUserCfgPath = home + "/.yarr/" + dbUserAccount + "_user.json";
        }
        std::ifstream dbUserCfgFile(dbUserCfgPath);
        if (dbUserName == ""||dbInstitution == ""||dbUserIdentity == "") {
            if (dbUserCfgFile) {
                json dbUserCfg = json::parse(dbUserCfgFile);
                if (dbUserName == ""&&!dbUserCfg["userName"].empty()) dbUserName = dbUserCfg["userName"];
                if (dbInstitution == ""&&!dbUserCfg["institution"].empty()) dbInstitution = dbUserCfg["institution"];
                if (dbUserIdentity == ""&&!dbUserCfg["userIdentity"].empty()) dbUserIdentity = dbUserCfg["userIdentity"];
            }
            if (dbUserIdentity == "") dbUserIdentity = "default";
        }
    
        if (dbUserName == ""||dbInstitution == "") {
            std::cerr << "Error: no username/institution given, please specify username/institution under -n/i option!" << std::endl;
            return -1;
        }
    
        std::replace(dbUserName.begin(), dbUserName.end(), ' ', '_');
        std::replace(dbInstitution.begin(), dbInstitution.end(), ' ', '_');
        std::replace(dbUserIdentity.begin(), dbUserIdentity.end(), ' ', '_');

        std::cout << std::endl;
        std::cout << "DBHandler: Register user's information: " << std::endl;
    	  std::cout << "\tuser name : " << dbUserName << std::endl;
    	  std::cout << "\tinstitution : " << dbInstitution << std::endl;
    	  std::cout << "\tuser identity : " << dbUserIdentity << std::endl;
        std::cout << std::endl;

        DBHandler *database = new DBHandler();
    	  database->registerUser(dbUserName, dbInstitution, dbUserIdentity);
        delete database;
    }

    // register site
    if (registerType == "Site") {
        DBHandler *database = new DBHandler();
    	  database->registerSite();
        delete database;
    }

    // register component
    if (registerType == "Component") {
        std::cout << "DBHandler: Register Component:" << std::endl;
	      std::cout << "\tconnecitivity config file : " << dbConnPath << std::endl;

        DBHandler *database = new DBHandler();
        database->setUser();
	      database->registerComponent(dbConnPath);

        delete database;
    }

    // register environment
    if (registerType == "Environment") {
        std::cout << "DBHandler: Register Environment:" << std::endl;
	      std::cout << "\tenvironmental config file : " << dbEnvPath << std::endl;

        DBHandler *database = new DBHandler();
        database->setUser();
        database->setTestRunInfo(dbEnvPath);
	      database->registerEnvironment();

        delete database;
    }

    // register Json
    if (registerType == "Json") {
        if (dbJsonType == "") {
            std::cerr << "Error: no upload format given, please specify upload format under -j option!" << std::endl;
            return -1;
        }
        if (dbJsonTitle == "") {
            std::cerr << "Error: no config title given, please specify config title under -t option!" << std::endl;
            return -1;
        }
        std::cout << "DBHandler: Register Json:" << std::endl;
	      std::cout << "\tjson file : " << dbJsonPath << std::endl;
        
        std::ifstream jsonFile(dbJsonPath);
        if (!jsonFile) {
            std::cerr <<"#ERROR# Cannot open json file: " << dbJsonPath << std::endl;
            return -1;
        }
        json j;
        try {
            j = json::parse(jsonFile);
            DBHandler *database = new DBHandler();
            database->writeJsonCode(dbJsonPath, dbJsonTitle + ".json", dbJsonTitle, dbJsonType); 
            delete database;
        } catch (json::parse_error &e) {
            std::cerr << "#ERROR# Could not parse config: " << e.what() << std::endl;
            std::abort();
        }
    }

    // get Json
    if (registerType == "getJson") {
        if (dbJsonId == "") {
            std::cerr << "Error: no config id given, please specify config id under -i option!" << std::endl;
            return -1;
        }
        if (dbType == "") {
            std::cerr << "Error: no config type given, please specify config type under -t option!" << std::endl;
            std::cerr << "\t-t chipCfg" << std::endl;
            std::cerr << "\t-t ctrlCfg" << std::endl;
            std::cerr << "\t-t scanCfg" << std::endl;
            return -1;
        }
        std::cout << "DBHandler: Get Json:" << std::endl;
	      std::cout << "\tsave json file : " << dbJsonPath << std::endl;
        
        DBHandler *database = new DBHandler();
	      database->getJsonCode(dbJsonId, dbJsonPath, dbName, dbType, std::atoi(dbChipId.c_str()));
        delete database;
    }
	  return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -C <path/to/conn.json> : Register component data into database. Provide connectivity config." << std::endl;
    std::cout << " -U <user account> : Register user data into database. Provide user's information with options. (Default: -p ~/.yarr/<user account>_user.json)" << std::endl;
    std::cout << "\t -n <user name>" << std::endl;
    std::cout << "\t -i <institution>" << std::endl;
    std::cout << "\t -u <user identity>" << std::endl;
    std::cout << "\t -p <user config path>" << std::endl;
    std::cout << "\t -c <test place> (Default: ~/.yarr/address)" << std::endl;
    std::cout << " -S <site name> : Register site data into database. Provide site's name and institution with option -i." << std::endl;
    std::cout << "\t -i <institution>" << std::endl;
    std::cout << " -J <path/to/config.json> : Register config data into database. Provide the path to config.json." << std::endl;
    std::cout << "\t -j <format type>" << std::endl;
    std::cout << "\t -t <config title>" << std::endl;
    std::cout << " -G <path/to/save.json> : Download config data from database. Provide the path to save.json." << std::endl;
    std::cout << "\t -i <config id in database>" << std::endl;
    std::cout << "\t -t <config type in database>" << std::endl;
    std::cout << " -E <path/to/env.json> : Register environmental information into database. Provide the path to env.json." << std::endl;
}
