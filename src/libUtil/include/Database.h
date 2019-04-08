#ifndef DATABASE_H
#define DATABASE_H

// #################################
// # Author: Eunchong Kim, Arisa Kubota
// # Email: eunchong.kim at cern.ch, arisa.kubota at cern.ch
// # Date : April 2019
// # Project: Local Database for Yarr
// # Description: Database functions
// ################################

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include "json.hpp"

// Use macro "-DMONGOCXX_INCLUDE" in makefile
#ifdef MONGOCXX_INCLUDE

// bsoncxx
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
// mongocxx v3.2 driver
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Database {
    public:
        Database(std::string i_host_ip = "mongodb://localhost:27017");
        ~Database();

        void setConnCfg(std::vector<std::string>);
        void setTestRunInfo(std::string);
        void setUserInstitution();
        void setHistoName(std::string);
        void startTestRun(std::string, std::string, std::string, int, int, int);
        std::string writeComponentTestRun(std::string, int, int);
        void writeConfig(std::string, json&, std::string); 
        void writeTestRun(std::string, std::string, std::string, int, std::string, int, int);
        void writeFiles(std::string, int, int);
        std::string uploadFromJson(std::string, std::string);
        void registerUserInstitution(std::string, std::string, std::string, std::string);
        void registerFromConnectivity(std::string);
        void registerEnvironment(std::string, std::string);
        void writeAttachment(std::string, std::string, std::string);
        std::string writeJsonCode(json&, std::string, std::string, std::string);
        void getJsonCode(std::string, std::string);

    protected:
        std::string getValue(std::string, std::string, std::string, std::string, std::string, std::string i_bson_type="string");
        std::string registerComponentTestRun(std::string, std::string, std::string, int, int, int);
        std::string registerTestRun(std::string, int, int, int);
        void addComment(std::string, std::string, std::string);
        void addDocument(std::string, std::string, std::string, std::string);
        void addAttachment(std::string, std::string, std::string, std::string, std::string, std::string, std::string);
        void addDefect(std::string, std::string, std::string, std::string);
        void writeFromDirectory(std::string, std::string, std::string, std::string i_filter="");
        void addTestRunInfo(std::string);
        void addUserInstitution(std::string, std::string);
        void addSys(std::string, std::string);
        void addVersion(std::string, std::string, std::string, std::string);
        std::string writeDatFile(std::string, std::string);
        std::string writeGridFsFile(std::string, std::string);
        std::string writeJsonCode_Json(json&, std::string, std::string);
        std::string writeJsonCode_Bson(json&, std::string, std::string);
        std::string writeJsonCode_Msgpack(json&, std::string, std::string);
        std::string writeJsonCode_Gridfs(json&, std::string, std::string);

    private:
        // Mongo c++
        mongocxx::client client;
        mongocxx::database db;

        std::string m_home_dir;
        std::vector<std::string> m_histo_name;

        std::string m_user_oid_str;
        std::string m_address;
        std::string m_tr_oid_str;
        std::string m_chip_type;

        int m_db_version;

        bool DB_DEBUG;
};
#else // if no mongocxx driver, disable database function
using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;
class Database {
    public:
        Database(std::string i_host_ip = "mongodb://localhost:27017"){};
        ~Database(){};

        void setConnCfg(std::vector<std::string>){};
        void setTestRunInfo(std::string){};
        void setUserInstitution(){};
        void setHistoName(std::string){};
        void startTestRun(std::string, std::string, std::string, int, int, int){};
        std::string writeComponentTestRun(std::string, int, int){return "ERROR";};
        void writeConfig(std::string, json&, std::string){}; 
        void writeTestRun(std::string, std::string, std::string, int, std::string, int, int){};
        void writeFiles(std::string, int, int){};
        std::string uploadFromJson(std::string, std::string){return "ERROR";};
        void registerUserInstitution(std::string, std::string, std::string, std::string){};
        void registerFromConnectivity(std::string){};
        void registerEnvironment(std::string, std::string){};
        void writeAttachment(std::string, std::string, std::string){};
        std::string writeJsonCode(json&, std::string, std::string, std::string){return "ERROR";};
        void getJsonCode(std::string, std::string){};

    protected:
        std::string getValue(std::string, std::string, std::string, std::string, std::string, std::string i_bson_type="string"){return "ERROR";};
        std::string registerComponentTestRun(std::string, std::string, std::string, int, int, int){return "ERROR";};
        std::string registerTestRun(std::string, int, int, int){return "ERROR";};
        void addComment(std::string, std::string, std::string){};
        void addDocument(std::string, std::string, std::string, std::string){};
        void addAttachment(std::string, std::string, std::string, std::string, std::string, std::string, std::string){};
        void addDefect(std::string, std::string, std::string, std::string){};
        void writeFromDirectory(std::string, std::string, std::string, std::string i_filter=""){};
        void addTestRunInfo(std::string){};
        void addUserInstitution(std::string, std::string){};
        void addSys(std::string, std::string){};
        void addVersion(std::string, std::string, std::string, std::string){};
        std::string writeDatFile(std::string, std::string){return "ERROR";};
        std::string writeGridFsFile(std::string, std::string){return "ERROR";};
        std::string writeJsonCode_Json(json&, std::string, std::string){return "ERROR";};
        std::string writeJsonCode_Bson(json&, std::string, std::string){return "ERROR";};
        std::string writeJsonCode_Msgpack(json&, std::string, std::string){return "ERROR";};
        std::string writeJsonCode_Gridfs(json&, std::string, std::string){return "ERROR";};
};
#endif // End of ifdef MONGOCXX_INCLUDE

#endif
