#include "Database.h"

// #################################
// # Author: Eunchong Kim, Arisa Kubota
// # Email: eunchong.kim at cern.ch, arisa.kubota at cern.ch
// # Date : April 2019
// # Project: Local Database for Yarr
// # Description: Database functions
// ################################

#ifdef MONGOCXX_INCLUDE // When use macro "-DMONGOCXX_INCLUDE" in makefile

// bsoncxx
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>

// Using bson::builder::stream, an iostream like interface to construct BSON objects.
// And these 'using ~' are greatly useful to reduce coding lines and make it readable.
// Please see example to understand what they are for.
// https://github.com/mongodb/mongo-cxx-driver/blob/r3.2.0-rc1/examples/bsoncxx/builder_stream.cpp
using bsoncxx::builder::stream::document;       // = '{', begining of document
using bsoncxx::builder::stream::finalize;       // = '}'
using bsoncxx::builder::stream::open_document;  // = '{', begining of subdocument
using bsoncxx::builder::stream::close_document; // = '}'
using bsoncxx::builder::stream::open_array;     // = '['
using bsoncxx::builder::stream::close_array;    // = ']'

std::vector<std::string> Database::m_stage_list{};
std::vector<std::string> Database::m_env_list{};
std::vector<std::string> Database::m_comp_list{};

Database::Database(std::string i_host_ip):
client(), db(), 
m_home_dir(), m_info_path(""), m_tr_oid_str(""), m_user_oid_str(), m_address(), m_chip_type(),
m_histo_names(), m_db_version(0.9), DB_DEBUG(false)
{
    if (DB_DEBUG) std::cout << "Database: Initialize" << std::endl;

    mongocxx::instance inst{};
    client = mongocxx::client{mongocxx::uri{i_host_ip}};
    db = client["yarrdb"]; // Database name is 'yarrdb'

    std::string home = getenv("HOME"); 
    m_home_dir = home  + "/.yarr/";
}

Database::~Database() {
    if (DB_DEBUG) std::cout << "Database: Exit Database" << std::endl;
}

//*****************************************************************************************************
// Public functions
//
void Database::setConnCfg(std::vector<std::string> i_conn_paths) {
    if (DB_DEBUG) std::cout << "Database: Connectivity Cfg: " << std::endl;
    for (auto conn_path : i_conn_paths) {
        if (DB_DEBUG) std::cout << "\t" << conn_path << std::endl;
        std::ifstream conn_ifs(conn_path);
        json conn_json;
        try {
            conn_json = json::parse(conn_ifs);
        } catch (json::parse_error &e) {
            std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
            abort(); return;
        }

        // Get chip type
        std::string chip_type = conn_json["chipType"];
        if (chip_type == "FEI4B") chip_type = "FE-I4B";
        m_chip_type = chip_type;
        // Module Component
        std::string mo_oid_str = getComponent(conn_json["module"], m_chip_type);
        std::string mo_serial_number = conn_json["module"]["serialNumber"];
        if (mo_oid_str=="") {
            std::cerr <<"#DB ERROR# This Module was not registered: " << mo_serial_number << std::endl;
            abort(); return;
        }
        int num = atoi(this->getValue("component", "_id", mo_oid_str, "oid", "children", "int").c_str());
        // Chip Component
        for (unsigned i=0; i<conn_json["chips"].size(); i++) {
            int chip_id = conn_json["chips"][i]["chipId"];
            bsoncxx::document::value doc_value = document{} <<  
                "parent" << mo_oid_str <<
                "chipId" << chip_id <<
                "status" << "active" <<
            finalize;
            auto maybe_result = db["childParentRelation"].find_one(doc_value.view());
            if (!maybe_result) {
                std::cerr <<"#DB ERROR# This Relationship between Module '" << mo_serial_number << "' and Chip 'chipId" << std::to_string(chip_id) << "' was not registered." << std::endl;
                abort(); return;
            }
        }
        mongocxx::cursor cursor = db["childParentRelation"].find(document{} << "parent" << mo_oid_str <<
                                                                               "status" << "active" << 
        finalize);
        if (num!=(int)std::distance(cursor.begin(), cursor.end())) {
            std::cerr <<"#DB ERROR# The number of chips was not mathced." << std::endl;
            abort(); return;
        }
    }
}

void Database::setTestRunInfo(std::string i_info_path) {//TODO make it enable to change file for each module (now combined)
    if (DB_DEBUG) std::cout << "Database: Test Run Info path: " << i_info_path << std::endl;
    if (i_info_path == "") return;
    std::ifstream info_ifs(i_info_path);
    if (!info_ifs) {
        std::cerr <<"#DB ERROR# Cannot open testRun info config file: " << i_info_path << std::endl;
        abort(); return;
    }
    json tr_info_j;
    try {
        tr_info_j = json::parse(info_ifs);
    } catch (json::parse_error &e) {
        std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
        abort(); return;
    }

    // assembly stage
    if (!tr_info_j["assembly"].empty()&&!tr_info_j["assembly"]["stage"].empty()) {
        std::string stage = tr_info_j["assembly"]["stage"];
        if (std::find(m_stage_list.begin(), m_stage_list.end(), stage)==m_stage_list.end()) {
            std::cerr << "#DB ERROR# Stage key '" << stage << "' was not registered: " << i_info_path << std::endl;
            abort(); return;
        }
    }
    
    // environment
    if (!tr_info_j["environments"].empty()) {
        json env_json = tr_info_j["environments"];
        for (auto j: env_json) {
            std::string j_key = j["key"];
            if (std::find(m_env_list.begin(), m_env_list.end(), j_key)==m_env_list.end()) {
                std::cerr << "#DB ERROR# Environmental key '" << j_key << "' was not registered: " << i_info_path << std::endl;
                abort(); return;
            }
            if (j["path"].empty()&&j["value"].empty()) {
                std::cerr << "#DB ERROR# Environmental path/value of key '" << j_key << "' was empty: " << i_info_path << std::endl;
                abort(); return;
            }
            if (j["description"].empty()) {
                std::cerr << "#DB ERROR# Environmental description of key '" << j_key << "' was empty: " << i_info_path << std::endl;
                abort(); return;
            }
            if (!j["path"].empty()) {
                std::string env_path = j["path"];
                std::ifstream env_ifs(env_path);
                if (!env_ifs) {
                    std::cerr <<"#DB ERROR# Cannot open environmental data file: " << env_path << std::endl;
                    abort(); return;
                }
                int items;
                env_ifs >> items;
                if (items==0) {
                    std::cerr <<"#DB ERROR# Something wrong in environmental data file: " << env_path << std::endl;
                    abort(); return;
                }
                std::string key = "";
                int cnt = 0;
                while (j_key!=key) {
                    if (cnt==items) {
                        std::cerr << "#DB ERROR# Environmental key '" << j_key << "' was not matched: " << env_path << std::endl;
                        abort(); return;
                    }
                    env_ifs >> key;
                    cnt++;
                }
            }
        }
    }
    m_info_path = i_info_path;
}

void Database::setUser() {
    if (getenv("DBUSER") == NULL) {
        std::cerr << "#DB ERROR# Not logged in Database, login by source db_login.sh <USER ACCOUNT>" << std::endl;
        std::abort();
    }
    std::string dbuser = getenv("DBUSER");

    // Set UserID from DB
    std::string user_path = m_home_dir + dbuser + "_user.json";
    if (DB_DEBUG) std::cout << "Database: Set user: " << user_path << std::endl;
    std::ifstream user_ifs(user_path);
    if (!user_ifs) {
        std::cerr << "#DB ERROR# Failed to load the user config: " << user_path << std::endl;
        std::abort();
    }
    json user_json;
    try {
        user_json = json::parse(user_ifs);
    } catch (json::parse_error &e) {
        std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
        abort(); return;
    }
    std::string userName     = user_json["userName"];
    std::string institution  = user_json["institution"];
    std::string userIdentity = user_json["userIdentity"];
    std::cout << "Database: User Information \n\tUser name: " << userName << "\n\tInstitution: " << institution << "\n\tUser identity: " << userIdentity << std::endl;
    mongocxx::collection collection = db["user"];
    auto maybe_result = collection.find_one(
        document{} << "userName"     << userName << 
                      "institution"  << institution << 
                      "userIdentity" << userIdentity << finalize
    );
    if (!maybe_result) {
        std::cerr << "#DB ERROR# User '" << userName << "' was not registerd: " << user_path << std::endl;
        std::abort();
    }
    m_user_oid_str = maybe_result->view()["_id"].get_oid().value.to_string();

    // Set MAC address
    std::string address_path = m_home_dir + "address";
    if (DB_DEBUG) std::cout << "Database: Set address: " << address_path << std::endl;
    std::ifstream address_ifs(address_path);
    if (!address_ifs) {
        std::cerr << "#DB ERROR# Failed to load the address config: " << address_path << std::endl;
        std::abort();
    }
    address_ifs >> m_address;
    std::cout << "Database: MAC Address: " << m_address << std::endl;

    // Set DB config
    if (m_stage_list.size()==0&&m_env_list.size()==0&&m_comp_list.size()==0&&!user_json["dbCfg"].empty()) {
        std::string db_cfg_path = user_json["dbCfg"];
        std::ifstream db_ifs(db_cfg_path);
        if (!db_ifs) {
            std::cerr << "#DB ERROR# Failed to load the DB config: " << db_cfg_path << std::endl;
            std::abort();
        }
        json db_json;
        try {
            db_json = json::parse(db_ifs);
        } catch (json::parse_error &e) {
            std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
            abort(); return;
        }
        for(auto sTmp: db_json["stage"]) {
            m_stage_list.push_back( sTmp );
        }
        for(auto sTmp: db_json["environment"]) {
            m_env_list.push_back( sTmp );
        }
        for(auto sTmp: db_json["component"]) {
            m_comp_list.push_back( sTmp );
        }
    }
}

void Database::writeTestRunStart(std::string i_test_type, std::vector<std::string> i_conn_paths, int i_run_number, int i_target_charge, int i_target_tot) {
    if (DB_DEBUG) std::cout << "Database: Write Test Run (start): " << i_run_number << std::endl;

    mongocxx::collection collection = db["testRun"];
    bsoncxx::document::value doc_value = document{} << "testType"  << i_test_type <<
                                                       "runNumber" << i_run_number <<
                                                       "address"   << m_address <<
                                                       "user_id"   << m_user_oid_str << finalize; 
    auto maybe_result = collection.find_one(doc_value.view());
    if (maybe_result) {
        std::cerr <<"#DB ERROR# Conflict to existing data, abort..." << std::endl;
        std::abort();
    }
    std::string test_run_oid_str = this->registerTestRun(i_test_type, i_run_number, i_target_charge, i_target_tot);
    this->addUser("testRun", test_run_oid_str);

    m_tr_oid_str = test_run_oid_str;

    // write component-testrun documents
    if (DB_DEBUG) std::cout << "Database: Write Component Test Run: " << i_run_number << std::endl;
    
    for (auto conn_path : i_conn_paths) {
        std::ifstream conn_ifs(conn_path);
        json conn_json = json::parse(conn_ifs);
        std::string mo_oid_str = getComponent(conn_json["module"], m_chip_type);
        std::string ctr_oid_str = this->registerComponentTestRun(mo_oid_str, m_tr_oid_str, i_test_type, i_run_number, -1, -1);
        mongocxx::cursor cursor = db["childParentRelation"].find(document{} << "parent" << mo_oid_str <<
                                                                               "status" << "active" << 
        finalize);
        for (auto doc : cursor) {
            int chip_id = doc["chipId"].get_int32(); 
            int tx = -1;
            int rx = -1;
            std::string chip_oid_str = doc["child"].get_utf8().value.to_string();
            for (unsigned i=0; i<conn_json["chips"].size(); i++) {
                if (conn_json["chips"][i]["chipId"]==chip_id) {
                    tx = conn_json["chips"][i]["tx"];
                    rx = conn_json["chips"][i]["rx"];
                }
            }
            this->registerComponentTestRun(chip_oid_str, m_tr_oid_str, i_test_type, i_run_number, tx, rx);
        }
    }
}

void Database::writeTestRunFinish(std::string i_test_type, std::vector<std::string> i_conn_paths, int i_run_number, int i_target_charge=-1, int i_target_tot=-1) {
    if (DB_DEBUG) std::cout << "Database: Write Test Run (finish): " << i_run_number << std::endl;

    mongocxx::collection collection = db["testRun"];
    bsoncxx::document::value doc_value = document{} << "testType"  << i_test_type <<
                                                       "runNumber" << i_run_number <<
                                                       "address"   << m_address <<
                                                       "user_id"   << m_user_oid_str << finalize; 
    auto maybe_result = collection.find_one(doc_value.view());
    std::string test_run_oid_str;
    if (!maybe_result) {
        this->writeTestRunStart(i_test_type, i_conn_paths, i_run_number, i_target_charge, i_target_tot);
        maybe_result = collection.find_one(doc_value.view());
    }
    test_run_oid_str = maybe_result->view()["_id"].get_oid().value.to_string();

    document builder{};
    auto docs = builder << "finishTime" << bsoncxx::types::b_date{std::chrono::system_clock::now()};
    auto in_array = docs << "plots" << open_array;
    std::sort(m_histo_names.begin(), m_histo_names.end());
    m_histo_names.erase(std::unique(m_histo_names.begin(), m_histo_names.end()), m_histo_names.end());
    for (auto sTmp: m_histo_names) {
        in_array = in_array << sTmp;
    }
    auto after_array = in_array << close_array;
    doc_value = after_array << finalize;

    db["testRun"].update_one(
        document{} << "_id" << bsoncxx::oid(test_run_oid_str) << finalize,
        document{} << "$set" << doc_value.view() << finalize
    );
    
    // register environment
    if (m_info_path!="") this->registerEnvironment();
}

void Database::writeConfig(std::string i_ctr_oid_str, std::string i_file_path, std::string i_filename, std::string i_title, std::string i_collection) {
    if (DB_DEBUG) std::cout << "Database: Write Config Json." << std::endl;

    std::ifstream file_ifs(i_file_path);
    if (!file_ifs) {
        std::cerr <<"#DB ERROR# Cannot open the config file: " << i_file_path << std::endl;
        abort(); return;
    }
    json file_json;
    try {
        file_json = json::parse(file_ifs);
    } catch (json::parse_error &e) {
        std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
        abort(); return;
    }
    file_ifs.close();

    std::string cfg_oid_str = this->writeJsonCode(i_file_path, i_filename+".json", i_title, "gj");
    std::string oid_str = "";
    if (i_collection == "testRun") oid_str = m_tr_oid_str;
    if (i_collection == "componentTestRun") oid_str = i_ctr_oid_str;
    if (oid_str!="") {
        this->addValue(oid_str, i_collection, i_filename, cfg_oid_str);
    }
}

// Will be deleted
void Database::writeFiles(std::string i_serial_number, int i_run_number_s, int i_run_number_e) {
    if (DB_DEBUG) std::cout << "Database: Write files" << std::endl;

    // get component id
    std::string component_oid_str = this->getValue("component", "serialNumber", i_serial_number, "", "_id", "oid");
    // find testrun of run number
    mongocxx::collection ctr_collection = db["componentTestRun"];
    mongocxx::cursor cursor = ctr_collection.find(document{} << "component" << component_oid_str << finalize);
    for (auto doc : cursor) {
        bsoncxx::document::element tr_element = doc["testRun"];
        std::string tr_oid_str = tr_element.get_utf8().value.to_string();

        mongocxx::collection tr_collection = db["testRun"];
        auto maybe_result = tr_collection.find_one(document{} << "_id" << bsoncxx::oid(tr_oid_str) << finalize);

        if (maybe_result) {
            json result_json = json::parse(bsoncxx::to_json(*maybe_result)); // testRun
            int runNumber = result_json["runNumber"];
            std::string testType = result_json["testType"];
            std::cout << runNumber << ", " << testType << std::endl;
            for (auto attachment : result_json["attachments"]) {
                std::string code = attachment["code"];
                std::string filename = attachment["filename"];
                std::string fileextension = attachment["contentType"];
                std::cout << code << ", " << filename << "." << fileextension << std::endl;

                mongocxx::collection fc_collection = db["fs.chunks"];
                auto fc_result = fc_collection.find_one(document{} << "files_id" << bsoncxx::oid(code) << finalize);

                if (!fc_result) {
                    std::cout << "Not Found!" << std::endl;
                    std::ostringstream os;
                    os << std::setfill('0') << std::setw(6) << runNumber;
                    std::string output_dir = ("./data/" + os.str() + "_" + testType + "/");
                    std::cout << output_dir << filename << "." << fileextension << std::endl;
                    std::string fs_str = writeGridFsFile(output_dir+filename+"."+fileextension, filename+"."+fileextension);

                    db["testRun"].update_one(
                        document{} << "_id" << bsoncxx::oid(tr_oid_str) << "attachments.code" << code << finalize,
                        document{} << "$set" << open_document <<
                            "attachments.$.code" << fs_str <<
                        close_document << finalize
                    );
                }
                else std::cout << "Found!" << std::endl;
            }
        }
    }
}

// Will be deleted
std::string Database::uploadFromJson(std::string i_collection_name, std::string i_json_path) {
    if (DB_DEBUG) std::cout << "Database: Upload from json: " << i_json_path << std::endl;
    std::ifstream json_ifs(i_json_path);
    if (!json_ifs) {
        std::cerr <<"#DB ERROR# Cannot open register json file: " << i_json_path << std::endl;
        abort(); return "ERROR";
    }
    json json;
    try {
        json = json::parse(json_ifs);
    } catch (json::parse_error &e) {
        std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
        abort(); return "ERROR";
    }
    bsoncxx::document::value doc_value = bsoncxx::from_json(json.dump()); 
    mongocxx::collection collection = db[i_collection_name];
    auto result = collection.insert_one(doc_value.view());
    bsoncxx::oid oid = result->inserted_id().get_oid().value;
    return oid.to_string();
}

void Database::registerUser(std::string i_user_name, std::string i_institution, std::string i_user_identity, std::string i_address="") {
    if (DB_DEBUG) std::cout << "Database: Register user \n\tUser name: " << i_user_name << "\n\tInstitution: " << i_institution << "\n\tUser identity: " << i_user_identity << std::endl;
    if (getenv("DBUSER") == NULL) {
        std::cerr << "#DB ERROR# Not logged in Database, abort..." << std::endl;
        std::cerr << "\tLogin by ./bin/dbRegister -U: <USER ACCOUNT>" << std::endl;
        std::abort();
    }
    std::string dbuser = getenv("DBUSER");
    auto maybe_result = db["user"].find_one(document{} << "userName"     << i_user_name << 
                                                          "institution"  << i_institution << 
                                                          "userIdentity" << i_user_identity << 
    finalize);
    if (maybe_result) {
        std::cout << "Database: Already exist, exit." << std::endl;
        return;
    }

    bsoncxx::document::value doc_value = document{} <<  
        "sys"           << open_document << close_document <<
        "userName"      << i_user_name <<
        "userIdentity"  << i_user_identity <<
        "institution"   << i_institution <<
        "userType"      << "readWrite" <<
    finalize;
    auto result = db["user"].insert_one(doc_value.view());
    bsoncxx::oid oid = result->inserted_id().get_oid().value;
    this->addSys(oid.to_string(), "user");
    this->addVersion("user", "_id", oid.to_string(), "oid");
}

// Will be deleted // TODO enable to register component in viewer application
void Database::registerComponent(std::string i_conn_path) {
    if (DB_DEBUG) std::cout << "Database: Register from connectivity: " << i_conn_path << std::endl;

    std::ifstream conn_ifs(i_conn_path);
    if (!conn_ifs) {
        std::cerr << "#DB ERROR# Cannot open register json file: " << i_conn_path << std::endl;
        abort(); return;
    }
    json conn_json;
    try {
        conn_json = json::parse(conn_ifs);
    } catch (json::parse_error &e) {
        std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
        abort(); return;
    }

    /// Confirmation
    // chip type
    std::string chip_type = conn_json["chipType"];
    if (chip_type == "FEI4B") chip_type = "FE-I4B";
    m_chip_type = chip_type;
    // Module
    std::string mo_oid_str = getComponent(conn_json["module"], m_chip_type);
    std::string mo_serial_number = conn_json["module"]["serialNumber"];
    if (mo_oid_str!="") {
        std::cout << "#DB ERROR# This component was already registered: " << mo_serial_number << std::endl;
        return;
    }
    // Chip
    int chips = 0;
    for (unsigned i=0; i<conn_json["chips"].size(); i++) {
        std::ifstream chip_cfg_ifs(conn_json["chips"][i]["config"].get<std::string>());
        if (!chip_cfg_ifs) {
            std::cerr << "#DB ERROR# Cannot open chip config file" << std::endl;
            abort(); return;
        }
        json chip_cfg_json;
        try {
            chip_cfg_json = json::parse(chip_cfg_ifs);
        } catch (json::parse_error &e) {
            std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
            abort(); return;
        }
        std::string chip_oid_str = getComponent(conn_json["chips"][i], m_chip_type);
        std::string serial_number  = conn_json["chips"][i]["serialNumber"];
        if (chip_oid_str!="") {
            std::cout << "#DB ERROR# This component was already registered: " << serial_number << std::endl;
            return;
        }
        chips++;
    }
    // Register module component
    {
        if (DB_DEBUG) std::cout << "\tRegister Module SN: " << mo_serial_number << std::endl;
        std::string componentType = conn_json["module"]["componentType"];
        bsoncxx::document::value doc_value = document{} <<  
            "sys"           << open_document << close_document <<
            "serialNumber"  << mo_serial_number <<
            "chipType"      << m_chip_type <<
            "componentType" << componentType <<
            "children"      << chips <<
        finalize;
        auto result = db["component"].insert_one(doc_value.view());
        bsoncxx::oid oid = result->inserted_id().get_oid().value;
        this->addSys(oid.to_string(), "component");
        this->addUser("component", oid.to_string());
        this->addVersion("component", "_id", oid.to_string(), "oid");
    }
    for (unsigned i=0; i<conn_json["chips"].size(); i++) {
        std::string serial_number  = conn_json["chips"][i]["serialNumber"];
        std::string componentType = conn_json["chips"][i]["componentType"];
        std::ifstream chip_cfg_ifs(conn_json["chips"][i]["config"].get<std::string>());
        json chip_cfg_json = json::parse(chip_cfg_ifs);
        int chip_id = 0;
        if (m_chip_type == "FE-I4B") {
            chip_id   = chip_cfg_json[chip_type]["Parameter"]["chipId"];
        } else if (m_chip_type == "RD53A") { 
            chip_id   = chip_cfg_json[chip_type]["Parameter"]["ChipId"];
        }
        // Register chip component
        {
            if (DB_DEBUG) std::cout << "\tRegister Chip SN: " << serial_number << std::endl;
            bsoncxx::document::value insert_doc = document{} <<  
                "sys"           << open_document << close_document <<
                "serialNumber"  << serial_number <<
                "componentType" << componentType <<
                "chipType"      << m_chip_type <<
                "chipId"        << chip_id <<
            finalize;
     
            auto result = db["component"].insert_one(insert_doc.view());
            bsoncxx::oid oid = result->inserted_id().get_oid().value;
            this->addSys(oid.to_string(), "component");
            this->addUser("component", oid.to_string());
            this->addVersion("component", "_id", oid.to_string(), "oid");
        }
        // Register CP relationship 
        {
            bsoncxx::document::value doc_value = document{} <<  
                "sys"       << open_document << close_document <<
                "parent"    << getValue("component", "serialNumber", mo_serial_number, "","_id", "oid") <<
                "child"     << getValue("component", "serialNumber", serial_number, "", "_id", "oid") <<
                "chipId"    << chip_id <<
                "status"    << "active" <<
            finalize;

            auto result = db["childParentRelation"].insert_one(doc_value.view());
            bsoncxx::oid oid = result->inserted_id().get_oid().value;
            this->addSys(oid.to_string(), "childParentRelation");
            this->addVersion("childParentRelation", "_id", oid.to_string(), "oid");
        }
    }
}

void Database::registerEnvironment() {
    if (DB_DEBUG) std::cout << "Database: Register Environment: " << m_info_path << std::endl;

    mongocxx::collection collection = db["environment"];
    // register the environmental key and description
    std::ifstream env_ifs(m_info_path);
    json test_json = json::parse(env_ifs);
    if (test_json["environments"].empty()) return;
    json env_json = test_json["environments"];
    std::vector<std::string> env_keys;
    std::vector<std::string> descriptions;
    std::vector<std::string> env_paths;
    std::vector<std::string> env_vals;
    for (auto j: env_json) {
        std::string j_key = j["key"];
        if (!j["path"].empty()) {
            std::string j_path = j["path"];
            env_paths.push_back(j_path);
        } else {
            env_paths.push_back("null");
        }
        if (!j["value"].empty()) {
            std::string val = j["value"].dump();
            env_vals.push_back(val);
        } else {
            env_vals.push_back("null");
        }
        env_keys.push_back(j_key);
        descriptions.push_back(j["description"]);
    }

    // get start time from scan data
    std::time_t starttime;
    if (m_tr_oid_str!="") {
        bsoncxx::oid i_oid(m_tr_oid_str);
        mongocxx::collection tr_collection = db["testRun"];
        auto run_result = tr_collection.find_one(document{} << "_id" << i_oid << finalize);
        if (!run_result) return;
        bsoncxx::document::element element = run_result->view()["startTime"];
        std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(element.get_date().value);
        starttime = s.count();
        if (DB_DEBUG) {
            char buf[80];
            struct tm *lt = std::localtime(&starttime);
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
            std::cout << "\tDatabase: Register Environment Start Time: " << buf << std::endl;
        }
    } else {
        std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
        starttime = std::chrono::system_clock::to_time_t(startTime);
    }

    // insert environment doc
    auto result = collection.insert_one(document{} << "type" << "data" << finalize);
    bsoncxx::oid env_oid = result->inserted_id().get_oid().value;

    for (int i=0; i<(int)env_keys.size(); i++) {
        if (DB_DEBUG) std::cout << "\tDatabase: Register Environment: " << env_keys[i] << std::endl;
        if (env_paths[i]!="null") {
            std::string env_path = env_paths[i];
            std::ifstream env_ifs(env_path);
            int data_num = 0;
            std::vector<std::string> key_values;
            std::vector<std::string> dates;
            while (!env_ifs.eof()) {
                std::string datetime = "";
                std::string tmp;
                env_ifs >> datetime >> tmp;
                if (tmp == ""||datetime == "") break;
                struct tm timepoint;
                memset(&timepoint, 0X00, sizeof(struct tm));
                strptime(datetime.c_str(), "%Y-%m-%dT%H:%M:%S", &timepoint);

                char buffer[80];
                strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",&timepoint);

                std::time_t timestamp  = mktime(&timepoint);
                if (difftime(starttime,timestamp)<60) { // store data from 1 minute before the starting time of the scan
                    key_values.push_back(tmp);
                    dates.push_back(datetime);
                    data_num++;
                }
            }
            auto array_builder = bsoncxx::builder::basic::array{};
            for (int j=0;j<data_num;j++) {
                struct tm timepoint;
                memset(&timepoint, 0X00, sizeof(struct tm));
                strptime(dates[j].c_str(), "%Y-%m-%dT%H:%M:%S", &timepoint);
                std::time_t timestamp  = mktime(&timepoint);
                array_builder.append(
                    document{} << 
                        "date"        << bsoncxx::types::b_date{std::chrono::system_clock::from_time_t(timestamp)} <<
                        "value"       << std::stof(key_values[j]) <<
                    finalize
                ); 
            }
            collection.update_one(
                document{} << "_id" << env_oid << finalize,
                document{} << "$push" << open_document <<
                    env_keys[i] << open_document <<
                        "data"        << array_builder <<
                        "description" << descriptions[i] <<
                    close_document <<
                close_document << finalize
            );
        } else {
            auto array_builder = bsoncxx::builder::basic::array{};
            array_builder.append(
                document{} << 
                    "date"        << bsoncxx::types::b_date{std::chrono::system_clock::from_time_t(starttime)} <<
                    "value"       << std::stof(env_vals[i]) <<
                finalize
            ); 
            collection.update_one(
                document{} << "_id" << env_oid << finalize,
                document{} << "$push" << open_document <<
                    env_keys[i] << open_document <<
                        "data"        << array_builder <<
                        "description" << descriptions[i] <<
                    close_document <<
                close_document << finalize
            );
        }
    }
    this->addSys(env_oid.to_string(), "environment");
    this->addVersion("environment", "_id", env_oid.to_string(), "oid");
    if (m_tr_oid_str!="") {
        this->addValue(m_tr_oid_str, "testRun", "environment", env_oid.to_string());
    }
    
}

void Database::writeAttachment(std::string i_ctr_oid_str, std::string i_file_path, std::string i_histo_name) {
    if (DB_DEBUG) std::cout << "\tDatabase: Write Attachment: " << i_file_path << std::endl;

    std::string fileextension;
    std::string oid_str;

    for (int i=0;i<3;i++) {
        if (i==0) fileextension = "dat";
        if (i==1) fileextension = "png";
        if (i==2) fileextension = "pdf";
        std::string file_path = i_file_path + "." + fileextension;
        std::ifstream file_ifs(file_path);
        if (file_ifs) {
            oid_str = this->writeGridFsFile(file_path, i_histo_name + "." + fileextension);
            this->addAttachment(i_ctr_oid_str, "componentTestRun", oid_str, i_histo_name, "describe", fileextension, i_histo_name+"."+fileextension);
        }
    }
    m_histo_names.push_back(i_histo_name);
}

std::string Database::writeJsonCode(std::string i_file_path, std::string i_filename, std::string i_title, std::string i_type) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload json file" << std::endl;

    std::string type_doc;
    std::string data_id;
    if (i_type == "j") {
        data_id = writeJsonCode_Json(i_file_path, i_filename, i_title);
        type_doc = "json";
    } else if (i_type == "m") {
        data_id = writeJsonCode_Msgpack(i_file_path, i_filename, i_title);
        type_doc = "msgpack";
    } else if (i_type == "gj") {
        data_id = writeJsonCode_Gridfs(i_file_path, i_filename, i_title);
        type_doc = "fs.files";
    } else if (i_type == "gm") {
    }

    mongocxx::collection collection = db["config"];
    bsoncxx::document::value doc_value = document{} << 
      "filename"  << i_filename  <<
      "chipType"  << m_chip_type << 
      "title"     << i_title     <<
      "format"    << type_doc    <<
      "data_id"   << data_id     <<
    finalize; 
    auto result = collection.insert_one(doc_value.view());
    std::string oid_str = result->inserted_id().get_oid().value.to_string();
    this->addSys(oid_str, "config");
    this->addVersion("config", "_id", oid_str, "oid");
    return oid_str;
}

void Database::getJsonCode(std::string i_oid_str, std::string i_filename) {
    if (DB_DEBUG) std::cout << "\tDatabase: download json file" << std::endl;

    mongocxx::collection collection = db["config"];

    bsoncxx::oid i_oid(i_oid_str);
    auto maybe_result = collection.find_one(document{} << "_id" << i_oid << finalize);
    if (maybe_result) {
        std::string format  = getValue("config", "_id", i_oid_str, "oid", "format");
        std::string data_id = getValue("config", "_id", i_oid_str, "oid", "data_id");
        bsoncxx::oid data_oid(data_id);
        mongocxx::collection data_collection = db[format];
        auto result = data_collection.find_one(document{} << "_id" << data_oid << finalize);
        if (result) {
            std::ofstream cfgFile(i_filename.c_str());
            json data;
            if(format=="bson"||format=="json") {
                json json_doc = json::parse(bsoncxx::to_json(*result));
                if(format == "bson") {
                    data = json_doc["data"];
                } else if (format == "json") {
                    std::string json_data = json_doc["data"];
                    try {
                        data = json::parse(json_data);
                    } catch (json::parse_error &e) {
                        std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
                        abort(); return;
                    }
                }
            } else if (format == "msgpack") {
                bsoncxx::document::element element_gl = result->view()["data"]["GlobalConfig"];
                auto array_element_gl = element_gl.get_array();
                bsoncxx::array::view subarray_gl{array_element_gl.value};
                std::vector<uint8_t> gl_msgpack;
                for (bsoncxx::array::element ele : subarray_gl){
                    gl_msgpack.push_back(ele.get_value().get_int32());
                }
                json gl = json::from_msgpack(gl_msgpack);

                bsoncxx::document::element element_pi = result->view()["data"]["PixelConfig"];
                auto array_element_pi = element_pi.get_array();
                bsoncxx::array::view subarray_pi{array_element_pi.value};
                std::vector<uint8_t> pi_msgpack;
                for (bsoncxx::array::element ele : subarray_pi){
                    pi_msgpack.push_back(ele.get_value().get_int32());
                }
                json pi = json::from_msgpack(pi_msgpack);

                json par_doc = json::parse(bsoncxx::to_json(result->view()["data"]["Parameter"].get_document()));

                data["RD53A"]["GlobalConfig"] = gl; 
                data["RD53A"]["PixelConfig"] = pi; 
                data["RD53A"]["Parameter"] = par_doc; 
            } else {
                mongocxx::gridfs::bucket gb = db.gridfs_bucket();
                std::ostringstream os;
                bsoncxx::types::value d_id{bsoncxx::types::b_oid{data_oid}};
                gb.download_to_stream(d_id, &os);
                std::string str = os.str();
                try {
                    data = json::parse(str);
                } catch (json::parse_error &e) {
                    std::cerr << __PRETTY_FUNCTION__ << "#DB ERROR# Could not parse config: " << e.what() << std::endl;
                    abort(); return;
                }
            }
            cfgFile << std::setw(4) << data;
            cfgFile.close();
        }
    }
}

std::string Database::getComponentTestRun(std::string i_serial_number, int i_chip_id) {
    // write component-testrun documents
    int run_number = atoi(this->getValue("testRun", "_id", m_tr_oid_str, "oid", "runNumber", "int").c_str());
    std::string test_type = this->getValue("testRun", "_id", m_tr_oid_str, "oid", "testType");
    if (DB_DEBUG) std::cout << "Database: Write Component Test Run: " << run_number << std::endl;

    auto pre_result = db["childParentRelation"].find_one(document{} << "parent" << getValue("component", "serialNumber", i_serial_number, "","_id", "oid") <<
                                                                       "chipId" << i_chip_id <<
                                                                       "status" << "active" << 
    finalize);
    std::string oid_str = pre_result->view()["child"].get_utf8().value.to_string();
    auto result = db["componentTestRun"].find_one(document{} << "component" << oid_str <<
                                                                "testRun" << m_tr_oid_str << 
    finalize);

    return result->view()["_id"].get_oid().value.to_string();
}

//*****************************************************************************************************
// Protected fuctions
//
std::string Database::getValue(std::string i_collection_name, std::string i_member_key, std::string i_member_value, std::string i_member_bson_type, std::string i_key, std::string i_bson_type){
    if (DB_DEBUG) std::cout << "\tDatabase: get value of key: '" << i_key << "' from: '" << i_collection_name << "', '{" << i_member_key << ": '" << i_member_value << "'}" << std::endl;
    mongocxx::collection collection = db[i_collection_name];
    auto query = document{};
    if (i_member_bson_type == "oid") query << i_member_key << bsoncxx::oid(i_member_value);
    else query << i_member_key << i_member_value;
    auto result = collection.find_one(query.view());
    if(result) {
        if (i_bson_type == "oid") {
            bsoncxx::document::element element = result->view()["_id"];
            return element.get_oid().value.to_string();
        }
        else if (i_bson_type == "sys_cts") {
            bsoncxx::document::element element = result->view()["sys"]["cts"];
            std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(element.get_date().value);
            std::time_t t = s.count();
            std::tm time_tm = *std::localtime(&t);
            char buffer[80];
            strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",&time_tm);
            std::string str(buffer);
            return str;
        }
        else if (i_bson_type == "sys_rev") {
            bsoncxx::document::element element = result->view()["sys"]["rev"];
            return std::to_string(element.get_int32().value);
        }
        else if (i_bson_type == "sys_mts") {
            bsoncxx::document::element element = result->view()["sys"]["mts"];
            std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(element.get_date().value);
            std::time_t t = s.count();
            std::tm time_tm = *std::localtime(&t);
            char buffer[80];
            strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",&time_tm);
            std::string str(buffer);
            return str;
        }
        else if (i_bson_type == "int") {
            bsoncxx::document::element element = result->view()[i_key];
            return std::to_string(element.get_int32().value);
        }
        else {
            bsoncxx::document::element element = result->view()[i_key];
            if (!element) return "ERROR";
            return element.get_utf8().value.to_string();
        }
    }
    else {
        std::cerr <<"#DB ERROR# Cannot find '" << i_key << "' from member '" << i_member_key << ": " << i_member_value << "' in collection name: '" << i_collection_name << "'" << std::endl;
        abort(); return "ERROR";
    }
}

std::string Database::getComponent(json &i_json, std::string i_chip_type) {
    if (i_json["serialNumber"].empty()) {
        std::cerr <<"#DB ERROR# No serialNumber in Connectivity!" << std::endl;
        abort(); return "ERROR";
    }
    if (i_json["componentType"].empty()) {
        std::cerr << "#DB ERROR# No componentType in Connectivity!" << std::endl;
        abort(); return "ERROR";
    }
    std::string serial_number  = i_json["serialNumber"];
    std::string component_type = i_json["componentType"];
    if (std::find(m_comp_list.begin(), m_comp_list.end(), component_type)==m_comp_list.end()) {
        std::cerr << "#DB ERROR# This component type was not registered: " << component_type << std::endl;
        abort(); return "ERROR";
    }

    if (DB_DEBUG) std::cout << "\tDatabase: get component data: 'serialnumber':" << serial_number << ", 'componentType':" << component_type << ", 'chipType':" << i_chip_type << std::endl;
    std::string oid_str = "";
    bsoncxx::document::value query_doc = document{} <<  
        "serialNumber"  << serial_number <<
        "componentType" << component_type <<
        "chipType"      << i_chip_type <<
    finalize;
    auto maybe_result = db["component"].find_one(query_doc.view());
    if (maybe_result) {
        oid_str = maybe_result->view()["_id"].get_oid().value.to_string();
    }
    return oid_str;
}

std::string Database::getHash(std::string i_file_path) {
    SHA_CTX context;
    if (!SHA1_Init(&context)) return "ERROR"; 
    static const int K_READ_BUF_SIZE{ 1024*16 };
    char buf[K_READ_BUF_SIZE];
    std::ifstream file(i_file_path, std::ifstream::binary);
    while (file.good()) {
        file.read(buf, sizeof(buf));
        if(!SHA1_Update(&context, buf, file.gcount())) return "ERROR";
    }
    unsigned char result[SHA_DIGEST_LENGTH];
    if (!SHA1_Final(result, &context)) return "ERROR";
    std::stringstream shastr;
    shastr << std::hex << std::setfill('0');
    for (const auto &byte: result) {
        shastr << std::setw(2) << (int)byte;
    }
    return shastr.str();
}

std::string Database::registerComponentTestRun(std::string i_cmp_oid_str, std::string i_tr_oid_str, std::string i_test_type, int i_run_number, int i_chip_tx, int i_chip_rx) {
    if (DB_DEBUG) std::cout << "\tDatabase: Register Com-Test Run" << std::endl;
    bsoncxx::document::value doc_value = document{} <<  
        "sys"       << open_document << close_document <<
        "component" << i_cmp_oid_str << // id of component
        "state"     << "..." << // code of test run state
        "testType"  << i_test_type << // id of test type
        "testRun"   << i_tr_oid_str << // id of test run, test is planned if it is null
        "qaTest"    << false << // flag if it is the QA test
        "runNumber" << i_run_number << // run number
        "passed"    << true << // flag if the test passed
        "problems"  << true << // flag if any problem occured
        "tx"        << i_chip_tx <<
        "rx"        << i_chip_rx <<
    finalize;
    mongocxx::collection collection = db["componentTestRun"];
    auto result = collection.insert_one(doc_value.view());
    bsoncxx::oid oid = result->inserted_id().get_oid().value;
    this->addSys(oid.to_string(), "componentTestRun");
    this->addVersion("componentTestRun", "_id", oid.to_string(), "oid");
    return oid.to_string();
}

std::string Database::registerTestRun(std::string i_test_type, int i_run_number, int i_target_charge=-1, int i_target_tot=-1) {
    if (DB_DEBUG) std::cout << "\tDatabase: Register Test Run" << std::endl;

    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    std::string stage = "null";
    if (m_info_path != "") {
        std::ifstream info_ifs(m_info_path);
        json tr_info_j = json::parse(info_ifs);
        if (!tr_info_j["assembly"].empty()&&!tr_info_j["assembly"]["stage"].empty()) {
            stage = tr_info_j["assembly"]["stage"].get<std::string>();
        }
    } 

    document builder{};
    auto docs = builder << "sys"          << open_document << close_document <<
                           "testType"     << i_test_type << // id of test type //TODO make it id
                           "runNumber"    << i_run_number << // number of test run
                           "startTime"    << bsoncxx::types::b_date{startTime} << // date when the test run was taken
                           "passed"       << true << // flag if test passed
                           "problems"     << true << // flag if any problem occured
                           "state"        << "ready" << // state of component ["ready", "requestedToTrash", "trashed"]
                           "stage"        << stage <<
                           "targetCharge" << i_target_charge <<
                           "targetTot"    << i_target_tot <<
                           "comments"     << open_array << // array of comments
                                             close_array <<
                           "defects"      << open_array << // array of defects
                                             close_array; 

    bsoncxx::document::value doc_value = docs << finalize;

    mongocxx::collection collection = db["testRun"];
    auto result = collection.insert_one(doc_value.view());
    bsoncxx::oid oid = result->inserted_id().get_oid().value;
    this->addSys(oid.to_string(), "testRun");
    this->addVersion("testRun", "_id", oid.to_string(), "oid");

    return oid.to_string();
}

void Database::addComment(std::string i_collection_name, std::string i_oid_str, std::string i_comment) { // To be deleted or seperated
    if (DB_DEBUG) std::cout << "\tDatabase: Add comment" << std::endl;
    bsoncxx::oid i_oid(i_oid_str);
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$push" << open_document <<
            "comments" << open_document <<
                    "code" << "01234567890abcdef01234567890abcdef" << // generated unique code
                    "dateTime" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // comment creation timestamp
                    "comment" << i_comment << // text of comment
            close_document <<
        close_document << finalize
    );
}

void Database::addValue(std::string i_oid_str, std::string i_collection_name, std::string i_key, std::string i_value) {
    if (DB_DEBUG) std::cout << "\tDatabase: Add document: " << i_key << " to " << i_collection_name << std::endl;
    bsoncxx::oid i_oid(i_oid_str);
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$set" << open_document <<
            i_key << i_value << 
        close_document << finalize
    );
}

void Database::addAttachment(std::string i_oid_str, std::string i_collection_name, std::string i_file_oid_str, std::string i_title, std::string i_description, std::string i_content_type, std::string i_filename) {
    if (DB_DEBUG) std::cout << "\tDatabase: Add attachment: " << i_filename << "." << i_content_type << std::endl;
    bsoncxx::oid i_oid(i_oid_str);
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$push" << open_document <<
            "attachments" << open_document <<
                "code" << i_file_oid_str << // generated unique code
                "dateTime" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // attachment creation timestamp
                "title" << i_title << // attachment title
                "description" << i_description << // brief description of attachment
                "contentType" << i_content_type << // content type of attachment
                "filename" << i_filename << // file name of attachment
            close_document <<
        close_document << finalize
    );
}

void Database::addDefect(std::string i_oid_str, std::string i_collection_name, std::string i_defect_name, std::string i_description) { // To de deleted
    if (DB_DEBUG) std::cout << "\tDatabase: Add defect" << std::endl;
    bsoncxx::oid i_oid(i_oid_str);
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$push" << open_document <<
            "defects" << open_document <<
                "code" << "01234567890abcdef01234567890abcdef" << // generated unique code
                "dateTime" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // attachment creation timestamp
                "name" << i_defect_name << // defect name
                "description" << i_description << // defect description
                "properties" << "..." << // properties object, optional
            close_document <<
        close_document << finalize
    );
}
void Database::writeFromDirectory(std::string i_collection_name, std::string i_oid_str, std::string i_output_dir, std::string i_filter) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload from directory" << std::endl;
    // Get file path from directory
    std::array<char, 128> buffer;
    std::stringstream temp_strstream;
    temp_strstream.str(""); temp_strstream << "ls -v " << i_output_dir << "/*";
    std::shared_ptr<FILE> pipe(popen(temp_strstream.str().c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");

    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr) {
            std::string file_path = buffer.data();
            file_path = file_path.substr(0, file_path.size()-1); // remove indent
            std::size_t pathPos = file_path.find_last_of('/');
            std::size_t suffixPos = file_path.find_last_of('.');
            std::string filename = file_path.substr(pathPos+1, suffixPos-pathPos-1);
            std::size_t pos =  filename.find(i_filter);
            std::size_t namePos =  filename.find_last_of('_');
            if (pos != std::string::npos) {
                std::string fileextension = file_path.substr(suffixPos + 1);
                std::string oid_str = "";
                if (fileextension == "dat")
                    oid_str = this->writeDatFile(file_path, filename);
                else if (fileextension == "pdf" || fileextension == "png")
                    oid_str = this->writeGridFsFile(file_path, filename+"."+fileextension);
                else
                    oid_str = "ERROR";
                if ( oid_str != "ERROR" )
                    this->addAttachment(i_oid_str, i_collection_name, oid_str, filename.substr(namePos+1), "describe", fileextension, filename);
            }
        }
    }
}

void Database::addUser(std::string i_collection_name, std::string i_oid_str) {
    if (DB_DEBUG) std::cout << "\tDatabase: Add user and institution" << std::endl;
    bsoncxx::oid i_oid(i_oid_str);

    // Update document
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$set" << open_document <<
            "address" << m_address <<
            "user_id" << m_user_oid_str <<
        close_document << finalize
    );
}

void Database::addSys(std::string i_oid_str, std::string i_collection_name) {
    if (DB_DEBUG) std::cout << "\tDatabase: Add sys" << std::endl;
    bsoncxx::oid i_oid(i_oid_str);
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$set" << open_document <<
            "sys" << open_document <<
                "rev" << 0 << // revision number
                "cts" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // creation timestamp
                "mts" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // modification timestamp
            close_document <<
        close_document << finalize
    );
}

void Database::addVersion(std::string i_collection_name, std::string i_member_key, std::string i_member_value, std::string i_member_bson_type) {
    if (DB_DEBUG) std::cout << "\tDatabase: Add DB Version " << m_db_version << std::endl;
    if (i_member_bson_type == "oid") {
        db[i_collection_name].update_many(
            document{} << i_member_key << bsoncxx::oid(i_member_value) << finalize,
            document{} << "$set" << open_document <<
                "dbVersion" << m_db_version <<
            close_document << finalize
        );
    } else {
        db[i_collection_name].update_many(
            document{} << i_member_key << i_member_value << finalize,
            document{} << "$set" << open_document <<
                "dbVersion" << m_db_version <<
            close_document << finalize
        );
    }
}

std::string Database::writeDatFile(std::string i_file_path, std::string i_filename) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload dat file" << std::endl;
    std::ifstream file_ifs(i_file_path);

    document builder{};

    std::string type;
    std::string name;
    std::string xaxistitle, yaxistitle, zaxistitle;
    int xbins, ybins, zbins;
    double xlow, ylow, zlow;
    double xhigh, yhigh, zhigh;
    int underflow, overflow;

    std::getline(file_ifs, type);
    std::getline(file_ifs, name);
    std::getline(file_ifs, xaxistitle);
    std::getline(file_ifs, yaxistitle);
    std::getline(file_ifs, zaxistitle);
    file_ifs >> xbins >> xlow >> xhigh;
    type = type.substr(0,7); // EOL char kept by getline()

    auto docs = builder << "type"       << type
                        << "name"       << name
                        << "xaxisTitle" << xaxistitle
                        << "yaxisTitle" << yaxistitle
                        << "zaxisTitle" << zaxistitle
                        << "xbins"      << xbins
                        << "xlow"       << bsoncxx::types::b_double{xlow}
                        << "xhigh"      << bsoncxx::types::b_double{xhigh};

    if (type == "Histo2d") {
        file_ifs >> ybins >> ylow >> yhigh;
        docs = docs << "ybins" << ybins
                    << "ylow"  << bsoncxx::types::b_double{ylow}
                    << "yhigh" << bsoncxx::types::b_double{yhigh};
    } else if (type == "Histo3d") {
        file_ifs >> ybins >> ylow >> yhigh >> zbins >> zlow >> zhigh;
        docs = docs << "ybins" << ybins
                    << "ylow"  << bsoncxx::types::b_double{ylow}
                    << "yhigh" << bsoncxx::types::b_double{yhigh}
                    << "zbins" << zbins
                    << "zlow"  << bsoncxx::types::b_double{zlow}
                    << "zhigh" << bsoncxx::types::b_double{zhigh};
    }
    file_ifs >> underflow >> overflow;

    docs = docs << "underflow" << underflow
                << "overflow"  << overflow;

    if (!file_ifs) {
        std::cerr << "Something wrong with file ..." << std::endl;
        return "ERROR";
    }

    auto in_array = docs << "dat" << open_array;

    if (type == "Histo1d") {
        for (int j=0; j<xbins; j++) {
            double tmp;
            file_ifs >> tmp;
            in_array = in_array << tmp;
        }
    }

    if (type == "Histo2d") {
        for (int i=0; i<ybins; i++) {
            auto array_builder = bsoncxx::builder::basic::array{};
            for (int j=0; j<xbins; j++) {
                double tmp;
                file_ifs >> tmp;
                array_builder.append( tmp );
            }
            in_array = in_array << array_builder;
        }
    } 

    if (type == "Histo3d") {
        for (int i=0; i<ybins; i++) {
            auto array_builder = bsoncxx::builder::basic::array{};
            for (int j=0; j<xbins; j++) {
                for (int k=0; k<zbins; k++) {
                    double tmp;
                    file_ifs >> tmp;
                    array_builder.append( tmp );
                }
            }
            in_array = in_array << array_builder;
        }
    } 

    auto after_array = in_array << close_array;
    bsoncxx::document::value doc_value = after_array << finalize;
    mongocxx::collection collection = db["dat"];

    auto result = collection.insert_one( document{} << "data" << doc_value.view() << "filename" << i_filename << "dbVersion" << m_db_version << finalize );
    std::string oid_str = result->inserted_id().get_oid().value.to_string();

    return oid_str;
}

std::string Database::writeGridFsFile(std::string i_file_path, std::string i_filename) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload attachment" << std::endl;
    mongocxx::gridfs::bucket gb = db.gridfs_bucket();
    mongocxx::collection collection = db["fs.files"];
      
    std::ifstream file_ifs(i_file_path);
    std::istream &file_is = file_ifs;
    auto result = gb.upload_from_stream(i_filename, &file_is);
    file_ifs.close();

    this->addVersion("fs.files", "_id", result.id().get_oid().value.to_string(), "oid");
    this->addVersion("fs.chunks", "files_id", result.id().get_oid().value.to_string(), "oid");
    return result.id().get_oid().value.to_string();
}

std::string Database::writeJsonCode_Json(std::string i_file_path, std::string i_filename, std::string i_title) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload json file" << std::endl;

    mongocxx::collection collection = db["json"];
    std::string sha1 = this->getHash(i_file_path);
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(
        document{} << "hash" << sha1 << finalize
    );
    if (maybe_result) return maybe_result->view()["_id"].get_oid().value.to_string();

    std::ifstream file_ifs(i_file_path);
    json file_json = json::parse(file_ifs);
    std::string json_doc = file_json.dump(); 
    bsoncxx::document::value doc_value = document{} << 
        "data" << json_doc  << 
        "hash" << sha1 << 
    finalize; 
    auto result = collection.insert_one(doc_value.view());
    std::string oid_str = result->inserted_id().get_oid().value.to_string();
    this->addVersion("json", "_id", oid_str, "oid");
    return oid_str;
}

std::string Database::writeJsonCode_Msgpack(std::string i_file_path, std::string i_filename, std::string i_title) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload json file" << std::endl;

    mongocxx::collection collection = db["msgpack"];
    std::string sha1 = this->getHash(i_file_path);
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(
        document{} << "hash" << sha1 << finalize
    );
    if (maybe_result) return maybe_result->view()["_id"].get_oid().value.to_string();

    std::ifstream file_ifs(i_file_path);
    json file_json = json::parse(file_ifs);
    json gCfg = file_json["RD53A"]["GlobalConfig"];
    std::vector<uint8_t> gl_msgpack = json::to_msgpack(gCfg);
    auto array_builder_gl = bsoncxx::builder::basic::array{};
    for (auto tmp : gl_msgpack) {
        array_builder_gl.append( tmp );
    }

    json pCfg = file_json["RD53A"]["PixelConfig"];
    std::vector<uint8_t> pi_msgpack = json::to_msgpack(file_json["RD53A"]["PixelConfig"]);
    auto array_builder_pi = bsoncxx::builder::basic::array{};
    for (auto tmp : pi_msgpack) {
        array_builder_pi.append( tmp );
    }

    json par = file_json["RD53A"]["Parameter"];
    bsoncxx::document::value par_doc = bsoncxx::from_json(par.dump()); 

    bsoncxx::document::value doc_value = document{} << 
        "data" << open_document <<
            "GlobalConfig" << array_builder_gl << 
            "Parameter"    << par_doc.view() << 
            "PixelConfig"  << array_builder_pi << 
        close_document <<
        "hash" << sha1 <<
    finalize; 
    auto result = collection.insert_one(doc_value.view());
    std::string oid_str = result->inserted_id().get_oid().value.to_string();
    this->addVersion("json", "_id", oid_str, "oid");
    return oid_str;
}

std::string Database::writeJsonCode_Gridfs(std::string i_file_path, std::string i_filename, std::string i_title) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload json file" << std::endl;

    std::string sha1 = this->getHash(i_file_path);
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = db["fs.files"].find_one(
        document{} << "hash" << sha1 << finalize
    );
    if (maybe_result) return maybe_result->view()["_id"].get_oid().value.to_string();

    std::string oid_str = this->writeGridFsFile(i_file_path, i_filename); 
    db["fs.files"].update_one(
        document{} << "_id" << bsoncxx::oid(oid_str) << finalize,
        document{} << "$set" << open_document <<
            "hash" << sha1 << 
        close_document << finalize
    );
    this->addVersion("fs.files", "_id", oid_str, "oid");
    this->addVersion("fs.chunks", "files_id", oid_str, "oid");
    return oid_str;
}
#else // Else if there is no MONGOCXX_INCLUDE

Database::Database(std::string i_host_ip) {std::cout << "[LDB] Warning! Database function is disabled!" << std::endl;}
Database::~Database() {}
void Database::setConnCfg(std::vector<std::string> i_conn_paths) {}
void Database::setTestRunInfo(std::string i_info_path) {}
void Database::setUser() {}
void Database::writeTestRunStart(std::string i_test_type, std::vector<std::string> i_conn_paths, int i_run_number, int i_target_charge, int i_target_tot) {}
void Database::writeTestRunFinish(std::string i_test_type, std::vector<std::string> i_conn_paths, int i_run_number, int i_target_charge=-1, int i_target_tot=-1) {}
void Database::writeConfig(std::string i_ctr_oid_str, std::string i_file_path, std::string i_filename, std::string i_title, std::string i_collection) {}
void Database::writeFiles(std::string i_serial_number, int i_run_number_s, int i_run_number_e) {}
std::string Database::uploadFromJson(std::string i_collection_name, std::string i_json_path) {return "ERROR";}
void Database::registerUser(std::string i_user_name, std::string i_institution, std::string i_user_identity, std::string i_address="") {}
void Database::registerComponent(std::string i_conn_path) {}
void Database::registerEnvironment() {}
void Database::writeAttachment(std::string i_ctr_oid_str, std::string i_file_path, std::string i_histo_name) {}
std::string Database::writeJsonCode(std::string i_file_path, std::string i_filename, std::string i_title, std::string i_type) {return "ERROR";}
void Database::getJsonCode(std::string i_oid_str, std::string i_filename) {}
std::string Database::getComponentTestRun(std::string i_serial_number, int i_chip_id) {}

#endif // End of ifdef MONGOCXX_INCLUDE
