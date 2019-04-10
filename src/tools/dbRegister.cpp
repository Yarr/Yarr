// #################################
// # Contacts: Arisa Kubota
// # Email: arisa.kubota at cern.ch
// # Date: April 2019
// # Project: Local Database for Yarr
// # Description: upload to mongoDB
// ################################


#include <iostream>
#include <cstdlib>
#include <string>
#include "json.hpp"
#include <unistd.h>

#include "Database.h"

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

  // register address
  std::string dbAddress = "";

  // register environment
  std::string dbEnvPath = "";
  // register json
  std::string dbJsonPath = "";
  std::string dbJsonType = "";
  // get json
  std::string dbJsonId = "";

  int c;
  while ((c = getopt(argc, argv, "hU:C:J:E:G:n:u:i:p:c:d:j:")) != -1 ){
    switch (c) {
        case 'h':
            printHelp();
            return 0;
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
        case 'n':
            dbUserName = std::string(optarg);
            break;
        case 'i':
            dbInstitution = std::string(optarg);
            dbJsonId = dbInstitution;
            break;
        case 'u':
            dbUserIdentity = std::string(optarg);
            break;
        case 'c':
            dbAddress = std::string(optarg);
            break;
        case 'j':
            dbJsonType = std::string(optarg);
            break;
        case 'p':
            dbUserCfgPath = std::string(optarg);
            break;
        case '?':
            if(optopt == 'U'||optopt == 'C'||optopt == 'E'||optopt == 'J'||optopt == 'G'){
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

  // register user
  if (registerType == "User") {
    std::cout << "Database: Login user: \n\taccount: " << dbUserAccount << std::endl;
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
    std::cout << "Database: Register user's information: " << std::endl;
  	std::cout << "\tuser name : " << dbUserName << std::endl;
  	std::cout << "\tinstitution : " << dbInstitution << std::endl;
  	std::cout << "\tuser identity : " << dbUserIdentity << std::endl;
    std::cout << std::endl;

    Database *database = new Database();
  	database->registerUser(dbUserName, dbInstitution, dbUserIdentity, dbAddress);
    delete database;
  }

  // register component
  if (registerType == "Component") {
    std::cout << "Database: Register Component:" << std::endl;
	  std::cout << "\tconnecitivity config file : " << dbConnPath << std::endl;

    Database *database = new Database();
    database->setUser();
	  database->registerComponent(dbConnPath);

    delete database;
  }

  // register environment
  if (registerType == "Environment") {
    std::cout << "Database: Register Environment:" << std::endl;
	  std::cout << "\tenvironmental config file : " << dbEnvPath << std::endl;

    Database *database = new Database();
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
    std::cout << "Database: Register Json:" << std::endl;
	  std::cout << "\tjson file : " << dbJsonPath << std::endl;
    
    std::ifstream jsonFile(dbJsonPath);
    if (!jsonFile) {
        std::cerr <<"#ERROR# Cannot open json file: " << dbJsonPath << std::endl;
        return -1;
    }
    json j;
    try {
        j = json::parse(jsonFile);
        Database *database = new Database();
	      database->writeJsonCode(j, "testfile", "testtitle", dbJsonType);
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
    std::cout << "Database: Get Json:" << std::endl;
	  std::cout << "\tsave json file : " << dbJsonPath << std::endl;
    
    Database *database = new Database();
	  database->getJsonCode(dbJsonId, dbJsonPath);
    delete database;
  }

	return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -C <path/to/conn.json> : Register component data into database. Provide connectivity config." << std::endl;
    std::cout << " -U <user account> : Register user data into database. Provide user information with options. (Default: -p ~/.yarr/<user account>_user.json)" << std::endl;
    std::cout << "\t -n <user name>" << std::endl;
    std::cout << "\t -i <institution>" << std::endl;
    std::cout << "\t -u <user identity>" << std::endl;
    std::cout << "\t -p <user config path>" << std::endl;
    std::cout << "\t -c <test place> (Default: ~/.yarr/address)" << std::endl;
    std::cout << " -J <path/to/config.json> : Register config data into database. Provide the path to config.json." << std::endl;
    std::cout << "\t -j <format type>" << std::endl;
    std::cout << " -G <path/to/save.json> : Download config data from database. Provide the path to save.json." << std::endl;
    std::cout << "\t -i <config id in database>" << std::endl;
    std::cout << " -E <path/to/env.json> : Register environmental information into database. Provide the path to env.json." << std::endl;
}
