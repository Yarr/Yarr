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

int main(int argc, char *argv[]){

    std::string home = getenv("HOME");
    std::string dbDirPath = home+"/.yarr/localdb";
    std::string cfg_path = dbDirPath+"/database.json";
    std::string user_cfg_path = dbDirPath+"/user.json";
    std::string site_cfg_path = dbDirPath+"/site.json";
    std::string registerType = "";

    // Init parameters
    std::string dcs_path = "";
    std::string conn_path = "";
    std::string file_path = "";
    std::string test_path = "";
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
                dcs_path = std::string(optarg);
                break;
            case 'S':
                registerType = "Check";
                break;
            case 't':
                test_path = std::string(optarg);
                config_type = std::string(optarg);
                get_type = std::string(optarg);
                break;
            case 's':
                serial_number = std::string(optarg);
                break;
            case 'i':
                config_id = std::string(optarg);
                site_cfg_path = std::string(optarg);
                break;
            case 'd':
                cfg_path = std::string(optarg);
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
                user_cfg_path = std::string(optarg);
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
        std::string cmd = "localdbtool-upload test 2> /dev/null";
        if (system(cmd.c_str())!=0) {
            std::cerr << "#ERROR# Not found Local DB command: 'localdbtool-upload'" << std::endl;
            std::cerr << "        Set Local DB function by YARR/localdb/setup_db.sh'" << std::endl;
            return 1;
        }
        cmd = "localdbtool-upload cache --database "+cfg_path+" --user "+user_cfg_path+" --site "+site_cfg_path;
        system(cmd.c_str());
        return 0;
    }

    // register component
    if (registerType=="Component") {
        std::string cmd = "localdbtool-upload test 2> /dev/null";
        if (system(cmd.c_str())!=0) {
            std::cerr << "#ERROR# Not found Local DB command: 'localdbtool-upload'" << std::endl;
            std::cerr << "        Set Local DB function by YARR/localdb/setup_db.sh'" << std::endl;
            return 1;
        }
        cmd = "localdbtool-upload comp "+conn_path+" --database "+cfg_path+" --user "+user_cfg_path+" --site "+site_cfg_path;
        system(cmd.c_str());
        return 0;
    }

    delete database;
    // cache DCS
    if (registerType == "Environment") {
        if (test_path == "") {
            std::cerr << "#DB ERROR# No test run file path given, please specify file path under -t option!" << std::endl;
            return 1;
        }
        std::cout << "DBHandler: Register Environment:" << std::endl;
	      std::cout << "\tenvironmental config file : " << dcs_path << std::endl;

        if (serial_number!="") {
            json tr_json;
            std::ifstream tr_cfg_ifs(test_path);
            if (!tr_cfg_ifs) {
                std::cerr << "#DB ERROR# Not found the file.\n\tfile: " + test_path << std::endl;
                return 1;
            }
            try {
                tr_json = json::parse(tr_cfg_ifs);
            } catch (json::parse_error &e) {
                std::cerr << "#DB ERROR# Could not parse " << test_path << "\n\twhat(): " << e.what() << std::endl;
                return 1;
            }
            tr_json["serialNumber"] = serial_number;

            std::ofstream file_ofs(test_path);
            file_ofs << std::setw(4) << tr_json;
            file_ofs.close();
        }

        database->initialize(cfg_path, "dcs");
        database->setDCSCfg(dcs_path, test_path);
        database->cleanUp();

        delete database;
    }
    return 0;
}

void printHelp() {
    std::string home = getenv("HOME");
    std::string dbDirPath = home+"/.yarr/localdb";
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -d: <database.json> Provide database configuration. (Default " << dbDirPath << "/etc/localdb/database.json" << std::endl;
    std::cout << " -i: <site.json> Provide site configuration. (Default " << dbDirPath << "/etc/localdb/site.json" << std::endl;
    std::cout << " -u: <user.json> Provide user configuration. (Default " << dbDirPath << "/etc/localdb/${USER}_user.json" << std::endl;
    std::cout << std::endl;
}
