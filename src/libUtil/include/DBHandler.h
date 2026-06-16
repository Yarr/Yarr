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
        void initialize(const std::string& /*i_db_cfg_path*/,
                        const std::string& /*i_command*/,
                        bool isQC=false,
                        bool i_interactive=false);

        /***
        Alert and write message in error log file
        * i_function: previous function name
        * i_message: alert message
        * i_type: error->abort, warning->continue
        ***/
        void alert(const std::string& i_function,
                   const std::string& i_message="Something wrong.",
                   const std::string& i_type="error");

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
        void setDCSCfg(const std::string& /*i_dcs_path*/,
                       const std::string& /*i_scanlog_path*/);
        /***
        Clean up veriables after scanConsole
        ***/
        void cleanUp(const std::string& /*i_option*/,
                     const std::string& /*i_dir*/,
                     //bool        i_back=true);
                     bool        i_back=false,
                     bool        i_interactive=true,
		     const std::string& tag = "");

        /***
        Upload unuploaded test cache data into Local DB
        ***/
        int setCache(const std::string& /*i_user_cfg_path*/,
                     const std::string& /*i_site_cfg_path*/);

        /***
        Registere modules into Local DB
        ***/
        int setComponent(const std::string& /*i_conn_path*/,
                         const std::string& /*i_user_cfg_path*/,
                         const std::string& /*i_site_cfg_path*/);

        /***
        Check the connection to Local DB
        ***/
        int checkConnection(const std::string& i_opt="upload");

        /***
        Check the test log in Local DB
        ***/
        int checkLog(const std::string& i_user="",
                     const std::string& i_site="",
                     const std::string& i_chip="");

        /***
        Check registered modules in Local DB and create module list in ~/.yarr/localdb/${HOSTNAME}_modules.csv
        ***/
        int checkConfigs(const std::string& /*i_user_cfg_path*/,
                         const std::string& /*i_site_cfg_path*/,
                         const std::vector<std::string>& /*i_conn_cfg_paths*/);

        /***
        retrieve DCS data from InfluxDB
        ***/
        int retrieveFromInflux(const std::string& /*influx_conn_path*/,
                               const std::string& /*chipname*/,
                               const std::string& /*i_scanlog_path*/);
        /***
        retrieve data
        ***/
        int retrieveData(const std::string& i_comp_name="",
                         const std::string& i_path="",
                         const std::string& i_dir="");
        void cleanDataDir();


    protected:
        /// check data function
        void checkFile(const std::string& /*i_file_path*/,
                       const std::string& i_description="");
        void checkEmpty(bool /*i_empty*/,
                        const std::string& /*i_key*/,
                        const std::string& /*i_file_path*/,
                        const std::string& i_description="");
        void checkNumber(bool /*i_number*/,
                         const std::string& /*i_key*/,
                         const std::string& /*i_file_path*/);
        void checkList(const std::vector<std::string>& /*i_list*/,
                       const std::string& /*i_value*/,
                       const std::string& /*i_list_path*/,
                       const std::string& /*i_file_path*/);
        json checkDBCfg(const std::string& /*i_db_path*/);
        void checkDCSCfg(const std::string& /*i_dcs_path*/,
                         const std::string& /*i_num*/,
                         json /*i_json*/);
        std::string checkDCSLog(const std::string& /*i_log_path*/,
                                const std::string& /*i_dcs_path*/,
                                const std::string& /*i_key*/,
                                int /*i_num*/);
        int checkCommand(const std::string& i_opt="upload");
        std::string getAbsPath(const std::string& /*i_path*/);

        /// check json
        json toJson(const std::string& /*i_file_path*/);
        void writeJson(const std::string& /*i_key*/,
                       const std::string& /*i_value*/,
                       const std::string& /*i_file_path*/,
                       json /*i_json*/);

        /// split function
        std::vector<std::string> split(const std::string& /*str*/,
                                       char /*del*/);
        void mkdir(const std::string& /*i_dir_path*/);

    private:
        std::string m_db_cfg_path;
        std::string m_chip_type;
        std::string m_output_dir;
        std::string m_upload_command;
        std::string m_retrieve_command;
        std::string m_influx_command;

        std::vector<std::string> m_histo_names;

        bool m_qc;
        bool m_interactive;
};

#endif
