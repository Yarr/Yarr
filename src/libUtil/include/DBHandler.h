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

#ifdef MONGOCXX_INCLUDE // When use macro "-DMONGOCXX_INCLUDE" in makefile

// mongocxx v3.2 driver
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/exception/error_code.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/server_error_code.hpp>
#include <mongocxx/instance.hpp>
// openssl
#include <openssl/sha.h>

#endif // End of ifdef MONGOCXX_INCLUDE

class DBHandler {
    public:
        DBHandler();
        ~DBHandler();

        void initialize(std::string /*i_db_cfg_path*/,
                        std::string i_option="register");
        void alert(std::string i_function,
                   std::string i_message="Something wrong.",
                   std::string i_type="error");
        int checkLibrary();
        int checkConnection(std::string /*i_db_cfg_path*/);
        int checkCacheStatus();
        int checkModuleList();

        /// Setting and Writing for using Database including:
        /// - confirmation of the files
        /// - caching : when the option is "cache"
        /// - registeration: then the option is "register"

        void setUser(std::string /*i_user_path*/,
                     std::string /*i_address_path*/);
        void setConnCfg(std::vector<std::string> /*i_conn_paths*/); 
        void setDCSCfg(std::string /*i_dcs_path*/,
                       std::string /*i_tr_path*/);
        //void setDCSCfg(std::string /*i_dcs_path*/,
        //               std::string /*i_conn_path*/); 
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
        void writeConfig(std::string /*i_ctr_oid_str*/, 
                         std::string /*i_file_path*/, 
                         std::string /*i_filename*/,
                         std::string /*i_title*/,
                         std::string /*i_collection*/); 
        void writeAttachment(std::string /*i_ctr_oid_str*/, 
                             std::string /*i_file_path*/, 
                             std::string /*i_histo_name*/);
        void writeDCS(std::string /*i_dcs_file_path*/,
                      std::string /*i_tr_file_path*/);
        void writeCache(std::string /*i_cache_dir*/);

        /// including mongo function
        /// - confirmation
        /// - registration
        /// - retrieve

        void getData(std::string /*i_info_file_path*/,
                     std::string /*i_get_type*/);
    protected:
        std::string registerUser(std::string /*i_user_name*/, 
                                 std::string /*i_institution*/, 
                                 std::string /*i_user_identity*/);
        std::string  registerSite(std::string /*i_adress*/,
                                  std::string /*i_name*/,
                                  std::string /*i_site*/);
        void registerConnCfg(std::string /*i_conn_path*/);
        std::string registerComponent(std::string /*i_serial_number*/,
                                      std::string /*i_component_type*/,
                                      int /*i_chip_id*/,
                                      int /*i_chips*/);
        void registerChildParentRelation(std::string /*i_parent_oid_str*/,
                                         std::string /*i_child_oid_str*/,
                                         int /*i_chip_id*/);
        void registerConfig(std::string /*i_ctr_oid_str*/, 
                            std::string /*i_file_path*/, 
                            std::string /*i_filename*/,
                            std::string /*i_title*/,
                            std::string /*i_collection*/); 
        void registerAttachment(std::string /*i_ctr_oid_str*/, 
                                std::string /*i_file_path*/, 
                                std::string /*i_histo_name*/);
        std::string registerJsonCode(std::string /*i_file_path*/, 
                                     std::string /*i_filename*/, 
                                     std::string /*i_title*/, 
                                     std::string /*i_type*/);
        void registerComponentTestRun(std::string /*i_conn_path*/, 
                                      std::string /*i_tr_oid_str*/, 
                                      std::string /*i_test_type*/, 
                                      int /*i_run_number*/);
        std::string registerTestRun(std::string /*i_test_type*/, 
                                    int /*i_run_number*/, 
                                    int /*i_target_charge*/, 
                                    int /*i_target_tot*/,
                                    int /*i_time*/,
                                    std::string /*i_serial_number*/,
                                    std::string /*i_type*/,
                                    std::string i_tr_oid_str="");
        void registerDCS(std::string /*i_dcs_path*/,
                         std::string /*i_tr_path*/);

        void writeFromDirectory(std::string /*i_collection_name*/, 
                                std::string /*i_oid_str*/, 
                                std::string /*i_output_dir*/, 
                                std::string i_filter="");
        std::string writeGridFsFile(std::string /*i_file_path*/, 
                                    std::string /*i_filename*/);
        std::string writeJsonCode_Msgpack(std::string /*i_file_path*/, 
                                          std::string /*i_filename*/, 
                                          std::string /*i_title*/);
        std::string writeJsonCode_Gridfs(std::string /*i_file_path*/, 
                                         std::string /*i_filename*/, 
                                         std::string /*i_title*/);


        std::string getValue(std::string /*i_collection_name*/, 
                             std::string /*i_member_key*/, 
                             std::string /*i_member_value*/, 
                             std::string /*i_member_bson_type*/, 
                             std::string /*i_key*/, 
                             std::string i_bson_type="string");
        std::string getComponent(std::string /*i_serial_number*/, 
                                 std::string /*i_file_path*/);

        void addComment(std::string /*i_collection_name*/, 
                        std::string /*i_oid_str*/, 
                        std::string /*i_comment*/);
        void addValue(std::string /*i_oid_str*/, 
                      std::string /*i_collection_name*/, 
                      std::string /*i_key*/, 
                      std::string /*i_value*/);
        void addAttachment(std::string /*i_oid_str*/, 
                           std::string /*i_collection_name*/, 
                           std::string /*i_file_oid_str*/, 
                           std::string /*i_title*/, 
                           std::string /*i_description*/,
                           std::string /*i_content_type*/, 
                           std::string /*i_filename*/);
        void addDefect(std::string /*i_oid_str*/, 
                       std::string /*i_collection_name*/, 
                       std::string /*i_defect_name*/, 
                       std::string /*i_description*/);
        void addUser(std::string /*i_collection_name*/, 
                     std::string /*i_oid_str*/);
        void addSys(std::string /*i_oid_str*/, 
                    std::string /*i_collection_name*/);
        void addVersion(std::string /*i_collection_name*/, 
                        std::string /*i_member_key*/, 
                        std::string /*i_member_value*/, 
                        std::string /*i_member_bson_type*/);

         /// Get SHA-1 hash
        std::string getHash(std::string /*i_file_path*/);
 
        /// For chaching scheme
        void cacheUser(std::string /*i_user_path*/,
                       std::string /*i_address_path*/);
        void cacheConnCfg(std::string /*i_conn_path*/); 
        void cacheTestRun(std::string /*i_test_type*/, 
                          int /*i_run_number*/, 
                          int /*i_target_charge*/, 
                          int /*i_target_tot*/,
                          int /*i_start_time*/,
                          int /*i_finish_time*/,
                          std::string /*i_command*/);
        void cacheConfig(std::string /*i_ctr_oid_str*/, 
                         std::string /*i_file_path*/, 
                         std::string /*i_filename*/,
                         std::string /*i_title*/,
                         std::string /*i_collection*/); 
        void cacheAttachment(std::string /*i_ctr_oid_str*/, 
                             std::string /*i_file_path*/, 
                             std::string /*i_histo_name*/);
        void cacheDCSCfg(std::string /*i_dcs_path*/,
                         std::string /*i_tr_path*/);
        void cacheDBCfg();

        /// check file exist
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

        /// get
        void getTestRunLog(json /*i_info_json*/);
        void getTestRunId(json /*i_info_json*/);
        void getConfigId(json /*i_info_json*/);
        void getUserId(json /*i_info_json*/);
        void getDatData(json /*i_info_json*/);
        void getConfigData(json /*i_info_json*/); 

        void getTestRunData(std::string /*i_tr_oid_str*/,
                            std::string /*i_serial_number*/,
                            int /*i_time*/);

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
#ifdef MONGOCXX_INCLUDE // When use macro "-DMONGOCXX_INCLUDE" in makefile
        // Mongo c++
        mongocxx::client client;
        mongocxx::database db;

#endif // End of ifdef MONGOCXX_INCLUDE

        std::string m_option;
        std::string m_db_cfg_path;
        std::string m_user_oid_str;
        std::string m_site_oid_str;
        std::string m_chip_type;
        std::string m_log_dir;
        std::string m_log_path;
        std::string m_cache_path;
        std::string m_cache_dir;
        std::string m_host_ip;
        std::string m_tr_oid_str;

        std::vector<std::string> m_stage_list;
        std::vector<std::string> m_env_list;
        std::vector<std::string> m_comp_list;

        std::vector<std::string> m_histo_names;
        std::vector<std::string> m_tr_oid_strs;
        std::vector<std::string> m_serial_numbers;

        double m_db_version;
        bool DB_DEBUG;

        json m_log_json;
        json m_cache_json;
        int counter;
};

#endif
