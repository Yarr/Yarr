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
#include "DBHandler.h"

#include "json.hpp"

void printHelp();

int main(int argc, char *argv[]){

    std::string home = getenv("HOME");
    std::string hostname = getenv("HOSTNAME");
    std::string registerType = "";

    // Init parameters
    std::string db_cfg_path=home+"/.yarr/"+hostname+"_database.json";
    std::string db_cache_path = "";
	  std::string conn_path = "";
    std::string db_user = "";
    std::string json_path = "";
    std::string json_type = "";
    std::string json_title = "";
    std::string json_id = "";
    std::string chip_id = "0";
    std::string name = "test";
    std::string type = "";
    std::string data_path = "";
    std::string data_id = "";
    //std::string db_status = "";

    int c;
    while ((c = getopt(argc, argv, "hR:U:C:J:E:G:T:Sn:i:c:j:t:")) != -1 ){
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'R':
                registerType = "Cache";
                db_cache_path = std::string(optarg);
                break;
            case 'U':
                registerType = "User";
                db_user = std::string(optarg);
                break;
            case 'C':
                registerType = "Component";
                conn_path = std::string(optarg);
                break;
            case 'E':
                registerType = "Environment";
                db_cache_path = std::string(optarg);
                break;
            case 'J':
                registerType = "Json";
                json_path = std::string(optarg);
                break;
            case 'G':
                registerType = "getJson";
                json_path = std::string(optarg);
                break;
            case 'T':
                registerType = "getDat";
                data_path = std::string(optarg);
                break;
            case 'S':
                registerType = "Check";
                break;
            case 'n':
                name = std::string(optarg);
                break;
            case 'i':
                json_id = std::string(optarg);
                data_id = std::string(optarg);
                break;
            case 'j':
                json_type = std::string(optarg);
                break;
            case 'c':
                chip_id = std::string(optarg);
                break;
            case 't':
                type = std::string(optarg);
                json_title = std::string(optarg);
                break;
            case '?':
                if(optopt=='R'||optopt=='U'||optopt=='C'||optopt=='E'||optopt=='J'||optopt=='G'||optopt=='T'||optopt=='S'){
                    std::cerr << "-> Option " << (char)optopt
                              << " requires a parameter! Aborting... " << std::endl;
                    return -1;
                } else {
                    std::cerr << "-> Unknown parameter: " << (char)optopt << std::endl;
                }
                break;
            default:
                std::cerr << "-> #ERROR# while parsing command line parameters!" << std::endl;
                return -1;
        }
    }

    if (registerType == "") printHelp();

    if (getenv("DBUSER")==NULL) {
        std::cerr << "#ERROR# Not logged in DBHandler, login by source dbLogin.sh <USER ACCOUNT>" << std::endl;
        std::abort();
    }
    std::string dbuser;
    if (db_user=="") dbuser = getenv("DBUSER");
    else dbuser = db_user;
    std::string userCfgPath, addressCfgPath;
    if (registerType=="Cache"||registerType=="Environment") {
        userCfgPath = db_cache_path+"/user.json";
        addressCfgPath = db_cache_path+"/address.json";
    } else {
        userCfgPath = home+"/.yarr/"+dbuser+"_user.json";
        addressCfgPath = home+"/.yarr/"+hostname+"_address.json";
    }
    std::fstream userCfgFile((userCfgPath).c_str(), std::ios::in);
    json userCfg;
    try {
        userCfg = json::parse(userCfgFile);
    } catch (json::parse_error &e) {
        std::cerr << "#ERROR# Could not parse config: " << e.what() << std::endl;
        return 0;
    }
    if (!userCfg["dbCfg"].empty()) db_cfg_path=userCfg["dbCfg"];
    
    DBHandler *database = new DBHandler(db_cfg_path, true);

    // register cache
    if (registerType == "Cache") {
        std::cout << "DBHandler: Register Cache '"<< db_cache_path << "'" << std::endl;

        database->initialize("cache");
    	  database->writeCache(db_cache_path);
        delete database;
    }

    // register user
    if (registerType == "User") {
        std::cout << "DBHandler: Login user: \n\taccount: " << db_user << std::endl;
        std::string home = getenv("HOME");
        std::string dbUserCfgPath = home + "/.yarr/" + db_user + "_user.json";

        std::string userName, institution, userIdentity;
        std::ifstream userCfgFile(dbUserCfgPath);

        json userCfg = json::parse(userCfgFile);
        if (!userCfg["userName"].empty()) userName = userCfg["userName"];
        else {
            std::cerr << "#ERROR# 'userName' is empty in user config file: " << dbUserCfgPath << std::endl;
            return -1;
        }
        if (!userCfg["institution"].empty()) institution = userCfg["institution"];
        else {
            std::cerr << "#ERROR# 'institution' is empty in user config file: " << dbUserCfgPath << std::endl;
            return -1;
        }
        if (!userCfg["userIdentity"].empty()) userIdentity = userCfg["userIdentity"];
        else userIdentity = "default";
        if (userName == ""||institution == "") {
            std::cerr << "#ERROR# Something wrong in user config file: " << dbUserCfgPath << std::endl;
            return -1;
        }
    
        std::replace(userName.begin(), userName.end(), ' ', '_');
        std::replace(institution.begin(), institution.end(), ' ', '_');
        std::replace(userIdentity.begin(), userIdentity.end(), ' ', '_');

        std::cout << std::endl;
        std::cout << "DBHandler: Register user's information: " << std::endl;
    	  std::cout << "\tuser name : " << userName << std::endl;
    	  std::cout << "\tinstitution : " << institution << std::endl;
    	  std::cout << "\tuser identity : " << userIdentity << std::endl;
        std::cout << std::endl;

        database->initialize();
    	  database->setUser(userCfgPath, addressCfgPath);
        delete database;
    }

    // register component
    if (registerType == "Component") {
        std::cout << "DBHandler: Register Component:" << std::endl;
	      std::cout << "\tconnecitivity config file : " << conn_path << std::endl;

        database->initialize();
    	  database->setUser(userCfgPath, addressCfgPath);
        std::vector<std::string> conn_paths;
        conn_paths.push_back(conn_path);
	      database->setConnCfg(conn_paths);

        delete database;
    }

    // register environment
    if (registerType == "Environment") {
        std::cout << "DBHandler: Register Environment:" << std::endl;
	      std::cout << "\tenvironmental config file : " << db_cache_path << std::endl;

        database->initialize("dcs");
    	  database->setUser(userCfgPath, addressCfgPath);
        database->writeDCS(db_cache_path);

        delete database;
    }

//    // register Json
//    if (registerType == "Json") {
//        if (json_type == "") {
//            std::cerr << "#ERROR# no upload format given, please specify upload format under -j option!" << std::endl;
//            return -1;
//        }
//        if (json_title == "") {
//            std::cerr << "#ERROR# no config title given, please specify config title under -t option!" << std::endl;
//            return -1;
//        }
//        std::cout << "DBHandler: Register Json:" << std::endl;
//	      std::cout << "\tjson file : " << json_path << std::endl;
//        
//        std::ifstream jsonFile(json_path);
//        if (!jsonFile) {
//            std::cerr <<"#ERROR# Cannot open json file: " << json_path << std::endl;
//            return -1;
//        }
//        json j;
//        try {
//            j = json::parse(jsonFile);
//            database->initialize();
//            database->writeJsonCode(json_path, json_title + ".json", json_title, json_type); 
//            delete database;
//        } catch (json::parse_error &e) {
//            std::cerr << "#ERROR# Could not parse config: " << e.what() << std::endl;
//            std::abort();
//        }
//    }
//
//    // get Json
//    if (registerType == "getJson") {
//        if (json_id == "") {
//            std::cerr << "#ERROR# no config id given, please specify config id under -i option!" << std::endl;
//            return -1;
//        }
//        if (type == "") {
//            std::cerr << "#ERROR# no config type given, please specify config type under -t option!" << std::endl;
//            std::cerr << "\t-t chipCfg" << std::endl;
//            std::cerr << "\t-t ctrlCfg" << std::endl;
//            std::cerr << "\t-t scanCfg" << std::endl;
//            return -1;
//        }
//        std::cout << "DBHandler: Get Json:" << std::endl;
//	      std::cout << "\tsave json file : " << json_path << std::endl;
//        
//        database->initialize();
//	      database->getJsonCode(json_id, json_path, name, type, std::atoi(chip_id.c_str()));
//        delete database;
//    }
//
//    // get Dat
//    if (registerType == "getDat") {
//        if (data_id == "") {
//            std::cerr << "#ERROR# no data id given, please specify data id under -i option!" << std::endl;
//            return -1;
//        }
//        std::cout << "DBHandler: Get Data:" << std::endl;
//	      std::cout << "\tsave data file : " << data_path << std::endl;
//        
//        database->initialize();
//	      database->getDatCode(data_id, data_path);
//        delete database;
//    }

    // register cache
    if (registerType == "Check") {
        std::cout << "DBHandler: Check Local DB server status" << std::endl;

        if (database->checkLibrary()==1) return 1;
    	  if (database->checkConnection()==1) return 1; 
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
    std::cout << "\t -c <test place> (Default: ~/.yarr/address.json)" << std::endl;
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
