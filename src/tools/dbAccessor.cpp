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
void checkEmpty(bool /*i_empty*/, std::string /*i_key*/, std::string /*i_file_path*/);

int main(int argc, char *argv[]){

    std::string home     = getenv("HOME");
    std::string hostname = getenv("HOSTNAME");
    std::string dbuser   = "";
    std::string registerType = "";

    // Init parameters
    std::string db_cfg_path=home+"/.yarr/"+hostname+"_database.json";
    std::string db_cache_path = "";
	  std::string conn_path = "";
    std::string file_path = "";
    std::string db_test_path = "";
    std::string config_id = "";
    std::string serial_number = "";
    std::string config_type = "";
    std::string config_path = "";
    std::string merge_config_path = "";
    std::string get_type = "";

    int c;
    while ((c = getopt(argc, argv, "hR:UC:E:SD:Gt:s:i:d:p:m:f:u:")) != -1 ){
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
                break;
            case 'C':
                registerType = "Component";
                conn_path = std::string(optarg);
                break;
            case 'E':
                registerType = "Environment";
                db_cache_path = std::string(optarg);
                break;
            case 'S':
                registerType = "Check";
                break;
            case 'D':
                registerType = "getData";
                get_type = std::string(optarg);
                break;
            case 'G':
                registerType = "getConfig";
                break;
            case 't':
                db_test_path = std::string(optarg);
                config_type = std::string(optarg);
                break;
            case 's':
                serial_number = std::string(optarg);
                break;
            case 'i':
                config_id = std::string(optarg);
                break;
            case 'd':
                db_cfg_path = std::string(optarg);
                break;
            case 'p':
                config_path = std::string(optarg);
                break;
            case 'm':
                merge_config_path = std::string(optarg);
                break;
            case 'f':
                file_path = std::string(optarg);
                break;
            case 'u':
                dbuser = std::string(optarg);
                break;
            case '?':
                if(optopt=='R'||optopt=='U'||optopt=='C'||optopt=='E'||optopt=='S'||optopt=='D'||optopt=='G'){
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

    DBHandler *database = new DBHandler();

    // register cache
    if (registerType == "Cache") {
        std::cout << "DBHandler: Register Cache '"<< db_cache_path << "'" << std::endl;

        db_cfg_path = db_cache_path+"/database.json";

        database->initialize(db_cfg_path, "db");
    	  database->writeCache(db_cache_path);
        delete database;
    }

    // register user
    if (registerType=="User") {
        if (dbuser == "") {
            std::cerr << "#ERROR# No user account name given, please specify user account under -u option!" << std::endl;
            return -1;
        }
        std::cout << "DBHandler: Login user: \n\taccount: " << dbuser << std::endl;
        std::string user_cfg_path = home+"/.yarr/"+dbuser+"_user.json";
        std::string address_cfg_path = home+"/.yarr/"+hostname+"_address.json";

        std::string user_name, institution, user_identity;

        std::ifstream user_cfg_ifs(user_cfg_path);
        json user_cfg_json = json::parse(user_cfg_ifs);
        checkEmpty(user_cfg_json["userName"].empty(),"userName",user_cfg_path);
        checkEmpty(user_cfg_json["institution"].empty(),"institution",user_cfg_path);

        user_name = user_cfg_json["userName"];
        institution = user_cfg_json["institution"];
        if (!user_cfg_json["userIdentity"].empty()) user_identity = user_cfg_json["userIdentity"];
        else user_identity = "default";
        if (user_name == ""||institution == "") {
            std::cerr << "#ERROR# Something wrong in user config file: " << user_cfg_path << std::endl;
            return -1;
        }
    
        std::replace(user_name.begin(), user_name.end(), ' ', '_');
        std::replace(institution.begin(), institution.end(), ' ', '_');
        std::replace(user_identity.begin(), user_identity.end(), ' ', '_');

        std::cout << std::endl;
        std::cout << "DBHandler: Register user's information: " << std::endl;
    	  std::cout << "\tuser name : " << user_name << std::endl;
    	  std::cout << "\tinstitution : " << institution << std::endl;
    	  std::cout << "\tuser identity : " << user_identity << std::endl;
        std::cout << std::endl;

        database->initialize(db_cfg_path);
    	  database->setUser(user_cfg_path, address_cfg_path);
        delete database;
    }

    // register component
    if (registerType=="Component") {
        if (dbuser == "") {
            std::cerr << "#ERROR# No user account name given, please specify user account under -u option!" << std::endl;
            return -1;
        }
        std::cout << "DBHandler: Register Component:" << std::endl;
	      std::cout << "\tconnecitivity config file : " << conn_path << std::endl;

        std::string user_cfg_path = home+"/.yarr/"+dbuser+"_user.json";
        std::string address_cfg_path = home+"/.yarr/"+hostname+"_address.json";

        database->initialize(db_cfg_path);

    	  database->setUser(user_cfg_path, address_cfg_path);
        std::vector<std::string> conn_paths;
        conn_paths.push_back(conn_path);
	      database->setConnCfg(conn_paths);

        delete database;
    }

    // register environment
    if (registerType == "Environment") {
        if (db_test_path == "") {
            std::cerr << "#ERROR# No test run file path given, please specify file path under -t option!" << std::endl;
            return -1;
        }
        std::cout << "DBHandler: Register Environment:" << std::endl;
	      std::cout << "\tenvironmental config file : " << db_cache_path << std::endl;

        if (serial_number!="") {
            json tr_json;
            std::ifstream tr_cfg_ifs(db_test_path);
            try {
                tr_json = json::parse(tr_cfg_ifs);
            } catch (json::parse_error &e) {
                std::cerr << "#ERROR# Could not parse " << db_test_path << "\n\twhat(): " << e.what() << std::endl;
                return 0;
            }
            tr_json["serialNumber"] = serial_number;

            std::ofstream file_ofs(db_test_path);
            file_ofs << std::setw(4) << tr_json;
            file_ofs.close();
        }

        database->initialize(db_cfg_path, "dcs");
        database->writeDCS(db_cache_path, db_test_path);

        delete database;
    }

    // register cache
    if (registerType == "Check") {
        std::cout << "DBHandler: Check Local DB server status" << std::endl;

        //database->initialize(db_cfg_path);

        if (database->checkLibrary()==1) return 1;
    	  if (database->checkConnection(db_cfg_path)==1) return 1; 
        delete database;
    }

    // get Dat
    if (registerType == "getData") {
        //if (file_path == "") {
        //    std::cerr << "#ERROR# No information file given, please specify it under -f option!" << std::endl;
        //    return -1;
        //}

        if (serial_number!="") {
            json tr_json;
            std::ifstream tr_cfg_ifs(file_path);
            try {
                tr_json = json::parse(tr_cfg_ifs);
            } catch (json::parse_error &e) {
                std::cerr << "#ERROR# Could not parse " << file_path << "\n\twhat(): " << e.what() << std::endl;
                return 0;
            }
            tr_json["serialNumber"] = serial_number;

            std::ofstream file_ofs(file_path);
            file_ofs << std::setw(4) << tr_json;
            file_ofs.close();
        }

        database->initialize(db_cfg_path);
	      database->getData(file_path, get_type);
        delete database;
    }

    // get Dat
    if (registerType == "getConfig") {
        json cfg_json;
        cfg_json["function"] = "getConfigData";
        cfg_json["id"] = config_id;
        cfg_json["serialNumber"] = serial_number;
        cfg_json["type"] = config_type;
        cfg_json["filePath"] = config_path;
        cfg_json["mergePath"] = merge_config_path;

        json db_json;
        std::ifstream db_cfg_ifs(db_cfg_path);
        try {
            db_json = json::parse(db_cfg_ifs);
        } catch (json::parse_error &e) {
            std::cerr << "#ERROR# Could not parse " << db_cfg_path << "\n\twhat(): " << e.what() << std::endl;
            return 0;
        }
        std::string cache_dir = db_json["cachePath"];

        std::string tmp_file_path = cache_dir + "/tmp/getConfig.json";
        std::ofstream file_ofs(tmp_file_path);
        file_ofs << std::setw(4) << cfg_json;
        file_ofs.close();

        database->initialize(db_cfg_path);
	      database->getData(tmp_file_path, "config");
        delete database;
    }

	  return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -R <path/to/cache/dir> : Register test data into database from cache. Provide path to cache directory." << std::endl;
    std::cout << " -U : Register user data into database. Provide user's account name with option -u." << std::endl;
    std::cout << "    -u <user account> " << std::endl;
    std::cout << " -C <path/to/conn/cfg> : Register component data into database. Provide path to connectivity config file and user'account name with option -u." << std::endl;
    std::cout << "    -u <user account> " << std::endl; 
    std::cout << " -E <path/to/dcs/cfg> : Register DCS data into database. Provide path to test run config with option -t." << std::endl;
    std::cout << "    - t <path/to/testrun/cfg> : Path to the file where test run information is written." << std::endl;
    std::cout << " -S : Check connection to MongoDB server." << std::endl;
    std::cout << " -G <get_type> : Download data from database. Provide the path to the query file with option -f." << std::endl;
    std::cout << "    get_type ... - testRunLog: display the log of testRun for the component" << std::endl;
    std::cout << "                 - testRunId:  display the id of latest testRun for the component" << std::endl;
    std::cout << "                 - config:     display the config data" << std::endl;
    std::cout << "                 - userId:     display the id of user data" << std::endl;
    std::cout << "                 - dat:        display the dat data" << std::endl;
    std::cout << "    -f <path/to/key/file> : Path to the file where the query key is written." << std::endl;
    std::cout << std::endl;
    std::cout << " -d <path/to/db/cfg> : Path to the Database config file where MongoDB server information is written." << std::endl;
}

void checkEmpty(bool i_empty, std::string i_key, std::string i_file_path) {
    if (i_empty) {
        std::cerr << "#ERROR# '" << i_key << "' is empty in user config file: " << i_file_path << std::endl;
        std::abort();
    }
    return;
}

