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


#include "storage.hpp"

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
                        bool isQC=false,
                        bool i_interactive=false);

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
        void setDCSCfg(std::string /*i_dcs_path*/,
                       std::string /*i_scanlog_path*/);
        /***
        Clean up veriables after scanConsole
        ***/
        void cleanUp(std::string /*i_option*/,
                     std::string /*i_dir*/,
                     //bool        i_back=true);
                     bool        i_back=false,
                     bool        i_interactive=true,
		     std::string tag = "");

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
        Check the connection to Local DB
        ***/
        int checkConnection(std::string i_opt="upload");

        /***
        Check the test log in Local DB
        ***/
        int checkLog(std::string i_user="",
                     std::string i_site="",
                     std::string i_chip="");

        /***
        Check registered modules in Local DB and create module list in ~/.yarr/localdb/${HOSTNAME}_modules.csv
        ***/
        int checkConfigs(std::string /*i_user_cfg_path*/,
                         std::string /*i_site_cfg_path*/,
                         std::vector<std::string> /*i_conn_cfg_paths*/);

        /***
        retrieve DCS data from InfluxDB
        ***/
        int retrieveFromInflux(std::string /*influx_conn_path*/,
                               std::string /*chipname*/,
                               std::string /*i_scanlog_path*/);
        /***
        retrieve data
        ***/
        int retrieveData(std::string i_comp_name="",
                         std::string i_path="",
                         std::string i_dir="");
        void cleanDataDir();


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
        void checkDCSCfg(std::string /*i_dcs_path*/,
                         std::string /*i_num*/,
                         json /*i_json*/);
        std::string checkDCSLog(std::string /*i_log_path*/,
                                std::string /*i_dcs_path*/,
                                std::string /*i_key*/,
                                int /*i_num*/);
        int checkCommand(std::string i_opt="upload");
        std::string getAbsPath(std::string /*i_path*/);

        /// check json
        json toJson(std::string /*i_file_path*/);
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
        std::string m_upload_command;
        std::string m_retrieve_command;
        std::string m_influx_command;

        std::vector<std::string> m_histo_names;

        double m_db_version;
        bool m_qc;
        bool m_interactive;
};

#endif
