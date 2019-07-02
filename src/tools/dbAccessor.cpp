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

#include "json.hpp"

void printHelp();
void checkEmpty(bool /*i_empty*/, std::string /*i_key*/, std::string /*i_file_path*/);

int main(int argc, char *argv[]){

    std::string home     = getenv("HOME");
    std::string dbuser   = "";
    std::string registerType = "";

    // Init parameters
    std::string db_cfg_path=home+"/.yarr/localdb/etc/database.json";
    std::string db_dcs_path = "";
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
    while ((c = getopt(argc, argv, "hRUC:E:SDGMt:s:i:d:p:m:f:u:")) != -1 ){
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'R':
                registerType = "Cache";
                break;
            case 'C':
                registerType = "Component";
                conn_path = std::string(optarg);
                break;
            case 'E':
                registerType = "Environment";
                db_dcs_path = std::string(optarg);
                break;
            case 'S':
                registerType = "Check";
                break;
            case 'D':
                registerType = "getData";
                break;
            case 'G':
                registerType = "getConfig";
                break;
            case 'M':
                registerType = "CheckModule";
                break;
            case 't':
                db_test_path = std::string(optarg);
                config_type = std::string(optarg);
                get_type = std::string(optarg);
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
                std::cerr << "-> #DB ERROR# while parsing command line parameters!" << std::endl;
                return 1;
        }
    }

    if (registerType == "") printHelp();

    DBHandler *database = new DBHandler();

    // register cache
    if (registerType == "Cache") {

        std::ifstream db_cfg_ifs(db_cfg_path);
        json db_cfg_json = json::parse(db_cfg_ifs);
        std::string db_cache_path = std::string(db_cfg_json["cachePath"])+"/var/cache/scan";
        std::cout << "DBHandler: Register Cache in '"<< db_cache_path << "'" << std::endl;
        DIR *dp;
        struct dirent *dirp;

	      dp = opendir(db_cache_path.c_str());
	      if (dp==NULL) {
		        return 0; 
	      }
	      while ((dirp = readdir(dp))) {
		        std::string file_name = dirp->d_name;
            if (file_name=="."||file_name=="..") continue;
            std::string cache_path = db_cache_path+"/"+file_name;
            db_cfg_path = cache_path+"/database.json";
            database->initialize(db_cfg_path, "db");
    	      database->setCache(cache_path);
        }
        delete database;
    }

    // register component
    if (registerType=="Component") {
        if (dbuser == "") {
            std::cerr << "#DB ERROR# No user account name given, please specify user account under -u option!" << std::endl;
            return 1;
        }
        std::cout << "DBHandler: Register Component:" << std::endl;
	      std::cout << "\tconnecitivity config file : " << conn_path << std::endl;

        std::string user_cfg_path = home+"/.yarr/localdb/etc/"+dbuser+"_user.json";
        std::string address_cfg_path = home+"/.yarr/localdb/etc/address.json";

        database->initialize(db_cfg_path, "register");

    	  database->setUser(user_cfg_path);
    	  database->setSite(address_cfg_path);
        std::vector<std::string> conn_paths;
        conn_paths.push_back(conn_path);
	      database->setConnCfg(conn_paths);

        delete database;
    }

    // cache DCS
    if (registerType == "Environment") {
        if (db_test_path == "") {
            std::cerr << "#DB ERROR# No test run file path given, please specify file path under -t option!" << std::endl;
            return 1;
        }
        std::cout << "DBHandler: Register Environment:" << std::endl;
	      std::cout << "\tenvironmental config file : " << db_dcs_path << std::endl;

        if (serial_number!="") {
            json tr_json;
            std::ifstream tr_cfg_ifs(db_test_path);
            if (!tr_cfg_ifs) {
                std::cerr << "#DB ERROR# Not found the file.\n\tfile: " + db_test_path << std::end;;
                return 1;
            }
            try {
                tr_json = json::parse(tr_cfg_ifs);
            } catch (json::parse_error &e) {
                std::cerr << "#DB ERROR# Could not parse " << db_test_path << "\n\twhat(): " << e.what() << std::endl;
                return 1;
            }
            tr_json["serialNumber"] = serial_number;

            std::ofstream file_ofs(db_test_path);
            file_ofs << std::setw(4) << tr_json;
            file_ofs.close();
        }

        database->initialize(db_cfg_path, "dcs");
        database->setDCSCfg(db_dcs_path, db_test_path);

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
        if (file_path == "") {
            std::cerr << "#DB ERROR# No file path given, please specify file path under -f option!" << std::endl;
            return 1;
        }

        if (serial_number!="") {
            json tr_json;
            std::ifstream tr_cfg_ifs(file_path);
            if (tr_cfg_ifs) {
                try {
                    tr_json = json::parse(tr_cfg_ifs);
                } catch (json::parse_error &e) {
                    std::cerr << "#DB ERROR# Could not parse " << file_path << "\n\twhat(): " << e.what() << std::endl;
                    return 1;
                }
            }
            tr_json["serialNumber"] = serial_number;

            std::ofstream file_ofs(file_path);
            file_ofs << std::setw(4) << tr_json;
            file_ofs.close();
        }

        database->initialize(db_cfg_path, "db");
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
            std::cerr << "#DB ERROR# Could not parse " << db_cfg_path << "\n\twhat(): " << e.what() << std::endl;
            return 1;
        }
        std::string cache_dir = db_json["cachePath"];

        std::string tmp_file_path = cache_dir + "/tmp/getConfig.json";
        std::ofstream file_ofs(tmp_file_path);
        file_ofs << std::setw(4) << cfg_json;
        file_ofs.close();

        database->initialize(db_cfg_path, "db");
	      database->getData(tmp_file_path, "config");
        delete database;
    }

    // check registered module
    if (registerType == "CheckModule") {
        database->initialize(db_cfg_path, "register");
	      database->checkModuleList();
        delete database;
    }

	  return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << std::endl;
    std::cout << " -d <path/to/db/cfg>: Path to the Database config file where MongoDB server information is written." << std::endl;
    std::cout << std::endl;
    std::cout << " -R: Register data into database from cache." << std::endl;
    std::cout << " -C <path/to/component/cfg>: Register component data into database. Provide path to connectivity config file and user'account name with option -u." << std::endl;
    std::cout << "    -u <user account> " << std::endl; 
    std::cout << " -E <path/to/dcs/cfg>: Store DCS cache data for uploading. Provide path to test run config with option -t." << std::endl;
    std::cout << "    - t <path/to/testrun/cfg>: Path to the file where test run information is written." << std::endl;
    std::cout << " -S: Check connection to MongoDB server." << std::endl;
    std::cout << " -D: Download data from database. Provide the path to the query file with option -f." << std::endl;
    std::cout << "    -t <get_type> " << std::endl;
    std::cout << "    -f <path/to/key/file>: Path to the file where the query key is written." << std::endl;
    std::cout << std::endl;
    std::cout << " -G: Download config data from database." << std::endl;
}

void checkEmpty(bool i_empty, std::string i_key, std::string i_file_path) {
    if (i_empty) {
        std::cerr << "#DB ERROR# '" << i_key << "' is empty in user config file: " << i_file_path << std::endl;
        std::abort();
    }
    return;
}
