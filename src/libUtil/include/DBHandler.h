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
        * i_option: DB fucntion option
          - db: access to MongoDB
          - scan/dcs: store cache data
        ***/
        void initialize(std::string /*i_db_cfg_path*/,
                        std::string i_option="null");

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
        Check cache directory/function 
        TODO to be implemented
        ***/
        int checkCacheStatus();

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
        void setUser(std::string /*i_user_path*/);
        void setSite(std::string /*i_address_path*/);
        void setConnCfg(std::vector<std::string> /*i_conn_paths*/); 
        void setDCSCfg(std::string /*i_dcs_path*/,
                       std::string /*i_tr_path*/);
        void setTestRunStart(std::string /*i_test_type*/, 
                             std::vector<std::string> /*i_conn_paths*/,
                             int /*i_run_number*/, 
                             int /*i_target_charge*/, 
                             int /*i_target_tot*/,
                             int /*i_timestamp*/,
                             std::string /*i_command*/);
        void setTestRunFinish(std::string /*i_test_type*/, 
                              std::vector<std::string> /*i_conn_paths*/,
                              int /*i_run_number*/, 
                              int /*i_target_charge*/, 
                              int /*i_target_tot*/,
                              int /*i_timestamp*/,
                              std::string /*i_command*/);
        //void setConfig(std::string /*i_ctr_oid_str*/, 
        void setConfig(int /*i_tx_channel*/,
                       int /*i_rx_channel*/, 
                       std::string /*i_file_path*/, 
                       std::string /*i_filename*/,
                       std::string /*i_title*/,
                       std::string /*i_collection*/,
                       std::string i_seiral_number=""); 
        //void setAttachment(std::string /*i_ctr_oid_str*/, 
        void setAttachment(int /*i_tx_channel*/,
                           int /*i_rx_channel*/, 
                           std::string /*i_file_path*/, 
                           std::string /*i_histo_name*/,
                           std::string i_serial_number="");

        /***
        Clean up veriables after scanConsole
        ***/
        void cleanUp(std::string i_dir="");

    protected:
        void getTestRunData(std::string /*i_tr_oid_str*/,
                            std::string /*i_serial_number*/, 
                            int /*i_time*/);
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
        json checkConnCfg(std::string /*i_conn_path*/); 
        json checkUserCfg(std::string /*i_user_path*/);
        json checkAddressCfg(std::string /*i_address_path*/);
        void checkDCSCfg(std::string /*i_dcs_path*/,
                         std::string /*i_num*/,
                         json /*i_json*/); 
        void checkDCSLog(std::string /*i_log_path*/,
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
        std::string m_option;
        std::string m_db_cfg_path;
        std::string m_user_oid_str;
        std::string m_site_oid_str;
        std::string m_chip_type;
        std::string m_tr_oid_str;

        std::vector<std::string> m_stage_list;
        std::vector<std::string> m_env_list;
        std::vector<std::string> m_comp_list;

        std::vector<std::string> m_histo_names;
        std::vector<std::string> m_tr_oid_strs;
        std::vector<std::string> m_serial_numbers;

        double m_db_version;
        bool DB_DEBUG;

        json m_conn_json;
        int counter;
};

#endif
