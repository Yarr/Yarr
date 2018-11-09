#ifndef DATABASE_H
#define DATABASE_H

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include "json.hpp"

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

        void setFlags(std::vector<std::string>);
        void write(std::string, std::string, int, std::string);
        std::string uploadFromJson(std::string, std::string);
        void registerFromConnectivity(std::string);
        void viewer();
        void writeFiles(std::string, int, int);

    protected:
        std::string getValue(std::string, std::string, std::string, std::string, std::string i_bson_type="string");
        std::string getValueByOid(std::string, std::string, std::string, std::string i_bson_type="string");
        std::string registerComponentTestRun(std::string, std::string, std::string, int);
        std::string registerTestRun(std::string, int);
        void addComment(std::string, std::string, std::string);
        void addAttachment(std::string, std::string, std::string, std::string, std::string, std::string, std::string);
        void addDefect(std::string, std::string, std::string, std::string);
        std::string uploadAttachment(std::string, std::string);
        void uploadFromDirectory(std::string, std::string, std::string, std::string i_filter="");
        void addEnvironment(std::string, std::string);
        void addSys(std::string, std::string);

//        bsoncxx::builder::stream::document addSys() {
//            bsoncxx::builder::stream::document i_doc_value;
//            i_doc_value <<
//            "sys" << open_document <<
//                "rev" << 0 << // revision number
//                "cts" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // creation timestamp
//                "mts" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // modification timestamp
//            close_document;
//            return i_doc_value;
//        };

    private:
        // Mongo c++
        mongocxx::client client;
        mongocxx::database db;
        std::string m_database_name;
        std::string m_collection_name;

        bool DB_DEBUG;
        bool m_has_flags;
        double m_hv;
        double m_cool_temp;
        std::string m_stage;

        // Schema bson object
};

#endif
