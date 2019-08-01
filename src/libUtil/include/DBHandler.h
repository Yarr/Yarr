#ifndef DATABASE_H
#define DATABASE_H

// #################################
// # Author: Eunchong Kim, Arisa Kubota
// # Email: eunchong.kim at cern.ch, arisa.kubota at cern.ch
// # Date : April 2019
// # Project: Local DBHandler for Yarr
// # Description: DBHandler functions
// ################################

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <cctype> 
#include <sys/stat.h>
#include <unistd.h>
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class DBHandler {
    public:
        DBHandler();
        ~DBHandler();

        /*** 
        Initialize for Local Database
        * i_db_cfg_path: path to database config
          - hostIp: ip address of Local DB server
          - port: opened port number of Local DB server 
          - dbName: database name of Local DB (default: localdb)
        ***/
        void initialize(std::string /*i_db_cfg_path*/,
                        std::string /*i_command*/,
                        std::string i_option="scan");

        /***
        Alert and write message in error log file
        * i_function: previous function name
        * i_message: alert message
        * i_type: error->abort, warning->continue
        ***/
        void alert(std::string i_function,
                   std::string i_message="Something wrong.",
                   std::string i_type="error");

        /***
        Setting function for using Database
        mongocxx library is not required.
        - User: requires user config file and site config file to set user/site data
        - ConnCfg: requires connectivity config file to set component data
        - DCSCfg: requires dcs config file and test run information to set DCS data
        - TestRun(start): requires test run information (in begining of scan) to set test data
        - TestRun(finish): requires test run information (in ending of scan) to set test data
        - Config: requires config file and information to set config data
        - Attachment: requires dat file and information to set dat data
        ***/
        json setUser(std::string /*i_user_path*/);
        json setSite(std::string /*i_site_path*/);
        void setConnCfg(std::vector<std::string> /*i_conn_paths*/); 
        void setDCSCfg(std::string /*i_dcs_path*/,
                       std::string /*i_scanlog_path*/,
                       std::string /*i_conn_path*/,
                       std::string /*i_user_path*/,
                       std::string /*i_site_path*/);
        /***
        Clean up veriables after scanConsole
        ***/
        void cleanUp(std::string /*i_option*/,
                     std::string /*i_dir*/);

        /***
        Upload unuploaded test cache data into Local DB
        ***/
        int setCache(std::string /*i_user_cfg_path*/,
                     std::string /*i_site_cfg_path*/);

        /***
        Registere modules into Local DB
        ***/
        int setComponent(std::string /*i_conn_path*/,
                         std::string /*i_user_cfg_path*/,
                         std::string /*i_site_cfg_path*/);

        /***
        Check registered modules in Local DB and create module list in ~/.yarr/localdb/${HOSTNAME}_modules.csv
        ***/
        int checkModule();

    protected:
        /// check data function
        void checkFile(std::string /*i_file_path*/,
                       std::string i_description="");
        void checkEmpty(bool /*i_empty*/,
                        std::string /*i_key*/,
                        std::string /*i_file_path*/,
                        std::string i_description="");
        void checkNumber(bool /*i_number*/,
                         std::string /*i_key*/,
                         std::string /*i_file_path*/);
        void checkList(std::vector<std::string> /*i_list*/,
                       std::string /*i_value*/,
                       std::string /*i_list_path*/,
                       std::string /*i_file_path*/);
        json checkDBCfg(std::string /*i_db_path*/); 
        void checkConnCfg(std::string /*i_conn_path*/); 
        json checkUserCfg(std::string /*i_user_path*/);
        json checkSiteCfg(std::string /*i_site_path*/);
        void checkDCSCfg(std::string /*i_dcs_path*/,
                         std::string /*i_num*/,
                         json /*i_json*/); 
        std::string checkDCSLog(std::string /*i_log_path*/,
                                std::string /*i_dcs_path*/,
                                std::string /*i_key*/,
                                int /*i_num*/); 

        /// check json
        json toJson(std::string /*i_file_path*/,
                    std::string i_file_type="json");
        void writeJson(std::string /*i_key*/,
                       std::string /*i_value*/,
                       std::string /*i_file_path*/,
                       json /*i_json*/); 

        /// split function
        std::vector<std::string> split(std::string /*str*/, 
                                       char /*del*/);
        void mkdir(std::string /*i_dir_path*/);
      
    private:
        std::string m_db_cfg_path;
        std::string m_chip_type;
        std::string m_output_dir;
        std::string m_command;

        std::vector<std::string> m_stage_list;
        std::vector<std::string> m_env_list;
        std::vector<std::string> m_comp_list;

        std::vector<std::string> m_histo_names;

        double m_db_version;
        bool DB_DEBUG;
        bool m_verify;

        json m_conn_json;
        int counter;
};

#endif
