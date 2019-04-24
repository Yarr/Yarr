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
#include <iostream>
#include <string>
#include "json.hpp"

#ifdef MONGOCXX_INCLUDE // When use macro "-DMONGOCXX_INCLUDE" in makefile

// mongocxx v3.2 driver
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
// openssl
#include <openssl/sha.h>

#endif // End of ifdef MONGOCXX_INCLUDE

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class DBHandler {
    public:
        DBHandler(std::string i_host_ip = "mongodb://localhost:27017");
        ~DBHandler();

        //// Functions for setting before scan (not included writeDB function)
        /// Confirm and set component from connectivity config 
        void setConnCfg(std::vector<std::string> /*i_conn_paths*/); 
        /// Confirm and set stage/dcs information from testRun info config
        void setTestRunInfo(std::string /*i_info_path*/); 
        /// Set user and institution from $HOME/.yarr/$DBUSER_user.json, and MAC address from $HOME/.yarr/address created by `source dbLogin.sh $DBUSER`
        void setUser();

        //// Functions for writing documents or values into DB
        /// Write test timing(-Start/Finish), test information and config files(-Start), and environmental data(-Finish) 
        void writeTestRunStart(std::string /*i_test_type*/, 
                               std::vector<std::string> /*i_conn_paths*/,
                               int /*i_run_number*/, 
                               int /*i_target_charge*/, 
                               int /*i_target_tot*/);
        void writeTestRunFinish(std::string /*i_test_type*/, 
                                std::vector<std::string> /*i_conn_paths*/,
                                int /*i_run_number*/, 
                                int /*i_target_charge*/, 
                                int /*i_target_tot*/);
        /// Write config file for each chip
        void writeConfig(std::string /*i_ctr_oid_str*/, 
                         std::string /*i_file_path*/, 
                         std::string /*i_filename*/,
                         std::string /*i_title*/,
                         std::string /*i_collection*/); 
        /// Write output files for each chip
        void writeAttachment(std::string /*i_ctr_oid_str*/, 
                             std::string /*i_file_path*/, 
                             std::string /*i_histo_name*/);
        /// Write json data from json file 
        std::string writeJsonCode(std::string /*i_file_path*/, 
                                  std::string /*i_filename*/, 
                                  std::string /*i_title*/, 
                                  std::string /*i_type*/);
        /// Will be deleted //TODO
        void writeFiles(std::string /*i_serial_number*/, 
                        int /*i_run_number_s*/, 
                        int /*i_run_number_e*/); 
        /// Will be deleted //TODO
        std::string uploadFromJson(std::string /*i_collection_name*/, 
                                   std::string /*i_json_path*/);  
        
        //// Functions for registering data into DB
        /// Register user, institution, and identity key from $HOME/.yarr/$DBUSER_user.json, and MAC address from $HOME/.yarr/address 
        void registerUser(std::string /*i_user_name*/, 
                          std::string /*i_institution*/, 
                          std::string /*i_user_identity*/);
        /// Register address and institution
        void registerSite();
        /// Register component data from connectivity config 
        void registerComponent(std::string /*i_conn_path*/);
        /// Register environmental data 
        void registerEnvironment();

        //// Functions for get value from DB
        /// Get json data into json file 
        void getJsonCode(std::string /*i_oid_str*/, 
                         std::string /*i_filename*/,
                         std::string /*i_name*/,
                         std::string /*i_type*/,
                         int /*i_chip_id*/);
        /// Get componentTestRun Id
        std::string getComponentTestRun(std::string /*i_serial_number*/,
                                        int /*i_chip_id*/);

    #ifdef MONGOCXX_INCLUDE // When use macro "-DMONGOCXX_INCLUDE" in makefile

    protected:
        //// Functions for get value from DB
        /// Get value with i_key
        std::string getValue(std::string /*i_collection_name*/, 
                             std::string /*i_member_key*/, 
                             std::string /*i_member_value*/, 
                             std::string /*i_member_bson_type*/, 
                             std::string /*i_key*/, 
                             std::string i_bson_type="string");
        
        /// Get component data
        std::string getComponent(json& /*i_json*/, 
                                 std::string /*i_chip_type*/);
        /// Get SHA-1 hash
        std::string getHash(std::string /*i_file_path*/);

        //// Functions for registering data into DB
        /// Register test data for component 
        std::string registerComponentTestRun(std::string /*i_cmp_oid_str*/, 
                                             std::string /*i_tr_oid_str*/, 
                                             std::string /*i_test_type*/, 
                                             int /*i_run_number*/, 
                                             int /*i_chip_tx*/, 
                                             int /*i_chip_rx*/);
        /// Register test data 
        std::string registerTestRun(std::string /*i_test_type*/, 
                                    int /*i_run_number*/, 
                                    int /*i_target_charge*/, 
                                    int /*i_target_tot*/);

        //// Functions for adding value to document in DB
        /// Add comment 
        void addComment(std::string /*i_collection_name*/, 
                        std::string /*i_oid_str*/, 
                        std::string /*i_comment*/);
        /// Add key and value
        void addValue(std::string /*i_oid_str*/, 
                      std::string /*i_collection_name*/, 
                      std::string /*i_key*/, 
                      std::string /*i_value*/);
        /// Add attachment data
        void addAttachment(std::string /*i_oid_str*/, 
                           std::string /*i_collection_name*/, 
                           std::string /*i_file_oid_str*/, 
                           std::string /*i_title*/, 
                           std::string /*i_description*/,
                           std::string /*i_content_type*/, 
                           std::string /*i_filename*/);
        /// Add defect data
        void addDefect(std::string /*i_oid_str*/, 
                       std::string /*i_collection_name*/, 
                       std::string /*i_defect_name*/, 
                       std::string /*i_description*/);
        /// Add user data
        void addUser(std::string /*i_collection_name*/, 
                     std::string /*i_oid_str*/);
        /// Add system information 
        void addSys(std::string /*i_oid_str*/, 
                    std::string /*i_collection_name*/);
        /// Add version of DB
        void addVersion(std::string /*i_collection_name*/, 
                        std::string /*i_member_key*/, 
                        std::string /*i_member_value*/, 
                        std::string /*i_member_bson_type*/);
        
        //// Functions for writing documents into DB
        /// Write output data in directory 
        void writeFromDirectory(std::string /*i_collection_name*/, 
                                std::string /*i_oid_str*/, 
                                std::string /*i_output_dir*/, 
                                std::string i_filter="");
        /// Will be deleted //TODO
        std::string writeDatFile(std::string /*i_file_path*/, 
                                 std::string /*i_filename*/);
        /// Write file data using GridFS 
        std::string writeGridFsFile(std::string /*i_file_path*/, 
                                    std::string /*i_filename*/);
        /// Will be deleted //TODO
        std::string writeJsonCode_Bson(json& /*i_json*/, 
                                       std::string /*i_filename*/, 
                                       std::string /*i_title*/);
        /// Will be deleted //TODO
        std::string writeJsonCode_Json(std::string /*i_file_path*/, 
                                       std::string /*i_filename*/, 
                                       std::string /*i_title*/);
        /// Write json file using MessagePack 
        std::string writeJsonCode_Msgpack(std::string /*i_file_path*/, 
                                          std::string /*i_filename*/, 
                                          std::string /*i_title*/);
        /// Write json file using GridFS 
        std::string writeJsonCode_Gridfs(std::string /*i_file_path*/, 
                                         std::string /*i_filename*/, 
                                         std::string /*i_title*/);

        std::string writeJsonCode_Test(std::string /*i_file_path*/, 
                                       std::string /*i_filename*/, 
                                       std::string /*i_title*/);

    private:
        // Mongo c++
        mongocxx::client client;
        mongocxx::database db;

        static std::vector<std::string> m_stage_list;
        static std::vector<std::string> m_env_list;
        static std::vector<std::string> m_comp_list;

        std::string m_home_dir;
        std::string m_info_path;
        std::string m_tr_oid_str;
        std::string m_user_oid_str;
        std::string m_address;
        std::string m_chip_type;
        std::string m_serial_number;

        std::vector<std::string> m_histo_names;

        double m_db_version;
        bool DB_DEBUG;

    #endif // End of ifdef MONGOCXX_INCLUDE
};

#endif
