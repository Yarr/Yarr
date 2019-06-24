// #################################
// # Author: Eunchong Kim, Arisa Kubota
// # Email: eunchong.kim at cern.ch, arisa.kubota at cern.ch
// # Date : April 2019
// # Project: Local DBHandler for Yarr
// # Description: DBHandler functions
// ################################

#include "DBHandler.h"

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
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;
#endif // End of ifdef MONGOCXX_INCLUDE

DBHandler::DBHandler():
#ifdef MONGOCXX_INCLUDE
client(), db(), 
#endif
m_option(""), m_db_cfg_path(""), m_user_oid_str(""), m_site_oid_str(""),
m_chip_type(""), m_log_dir(""), m_log_path(""), m_cache_path(""), m_cache_dir(""), m_tr_oid_str(""),
m_stage_list(), m_env_list(), m_comp_list(),
m_histo_names(), m_tr_oid_strs(), m_serial_numbers(),
m_db_version(1.0), DB_DEBUG(false), m_log_json(), m_cache_json(), counter(0)
{
    if (DB_DEBUG) std::cout << "DBHandler: DBHandler" << std::endl;
}

DBHandler::~DBHandler() {
    if (DB_DEBUG) std::cout << "DBHandler: Exit DBHandler" << std::endl;
}
//*****************************************************************************************************
// Public functions
//
void DBHandler::initialize(std::string i_db_cfg_path, std::string i_option) {
    if (DB_DEBUG) std::cout << "DBHandler: Initializing " << m_option << std::endl;

    m_option = i_option;
    m_db_cfg_path = i_db_cfg_path;

    int now = std::time(NULL);

    json db_json = this->checkDBCfg(m_db_cfg_path);
    std::string host_ip = "mongodb://"+std::string(db_json["hostIp"])+":"+std::string(db_json["hostPort"]);
    std::string db_name = db_json["dbName"];
    m_cache_dir = db_json["cachePath"];

    if (m_stage_list.size()==0&&m_env_list.size()==0&&m_comp_list.size()==0) {
        for(auto s_tmp: db_json["stage"]) m_stage_list.push_back(s_tmp);
        for(auto s_tmp: db_json["environment"]) m_env_list.push_back(s_tmp);
        for(auto s_tmp: db_json["component"]) m_comp_list.push_back(s_tmp);
    }

    //this->checkModuleList();

    // initialize for registeration
    if (m_option=="db"||m_option=="dcs"||m_option=="register") {
#ifdef MONGOCXX_INCLUDE
        mongocxx::instance inst{};
        try {
            client = mongocxx::client{mongocxx::uri{host_ip}};
            db = client[db_name]; 
            auto doc_value = make_document(kvp("hash", 1), kvp("_id", 1));
            db["fs.files"].create_index(doc_value.view()); 
        } catch (const mongocxx::exception & e){
            std::string message = "Could not access to MongoDB server: "+host_ip+"\n\twhat(): "+e.what();
            std::string function = __PRETTY_FUNCTION__;
            this->alert(function, message); return;
        }
#else
        std::string message = "Not found MongoDB C++ Driver!";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
#endif
    }
    if (m_option=="db")        m_log_dir = m_cache_dir+"/var/cache/db/"   + std::to_string(now);
    else if (m_option=="scan") m_log_dir = m_cache_dir+"/var/cache/scan/" + std::to_string(now);
    else if (m_option=="dcs")  m_log_dir = m_cache_dir+"/var/cache/dcs/"  + std::to_string(now);
    else m_log_dir = m_cache_dir+"/tmp/scan";//TODO

    m_log_path = m_log_dir + "/cacheLog.json";
    this->mkdir(m_log_dir);
    m_log_json["version"] = m_db_version;
    m_log_json["logPath"] = m_log_dir;
    this->writeJson("status", "running", m_log_path, m_log_json);
}

void DBHandler::alert(std::string i_function, std::string i_message, std::string i_type) {
    if (DB_DEBUG) std::cout << "DBHandler: Alert '" << i_type << "'" << std::endl;

    std::string alert_message;
    if (i_type=="error") {
        alert_message = "#DB ERROR#";
        if (m_option=="scan"||m_option=="db"||m_option=="dcs") {
            m_log_json["errormessage"] = i_message;
            this->writeJson("status", "failure", m_log_path, m_log_json);
        }
        if (m_option=="db") {
            m_cache_json["errormessage"] = i_message;
            this->writeJson("status", "failure", m_cache_path, m_cache_json);
        }
    } else if (i_type=="warning") {
        alert_message = "#DB WARNING#";
    }
    std::cerr << alert_message << " " << i_message << std::endl;
    std::time_t now = std::time(NULL);
    struct tm *lt = std::localtime(&now);
    char tmp[20];
    strftime(tmp, 20, "%Y%m%d", lt);
    std::string timestamp=tmp;
    std::string log_path = m_cache_dir+"/var/log/"+timestamp+"_error.log";
    std::ofstream file_ofs(log_path, std::ios::app);
    strftime(tmp, 20, "%F_%H:%M:%S", lt);
    timestamp=tmp;
    file_ofs << timestamp << " " << alert_message << " [" << m_option << "] " << i_function << std::endl;
    file_ofs << "**********\n" << i_message << "\n**********" << std::endl;
    file_ofs << "Log: " << m_log_path << "\nCache: " << m_cache_path << "\n--------------------" << std::endl;

    if (i_type=="error") std::abort();
}

int DBHandler::checkLibrary() {
    if (DB_DEBUG) std::cout << "DBHandler: Check Mongocxx library" << std::endl;
#ifdef MONGOCXX_INCLUDE
    std::string message = "Found MongoDB C++ Driver!";
    return 0;
#else
    std::string message = "Not found MongoDB C++ Driver!";
    return 1;
#endif
}

int DBHandler::checkConnection(std::string i_db_cfg_path) {
    if (DB_DEBUG) std::cout << "DBHandler: Check LocalDB server connection" << std::endl;
#ifdef MONGOCXX_INCLUDE
    json db_json = this->checkDBCfg(i_db_cfg_path);
    std::string db_cfg_path = i_db_cfg_path;
    std::string host_ip = "mongodb://"+std::string(db_json["hostIp"])+":"+std::string(db_json["hostPort"]);
    mongocxx::instance inst{};
    try {
        client = mongocxx::client{mongocxx::uri{host_ip}};
        db = client["localdb"]; // Database name is 'localdb'
        auto doc_value = document{} <<
            "hash" << 1 <<
            "_id"  << 1 <<
        finalize;
        db["fs.files"].create_index(doc_value.view()); // set index in fs.files
    } catch (const mongocxx::exception & e){
        std::string message = "Could not access to MongoDB server: "+host_ip+"\n\twhat(): "+e.what();
        return 1;
    }
    return 0;
#else
    std::string message = "Not found MongoDB C++ Driver!";
    return 1;
#endif
}

int DBHandler::checkModuleList() {
    if (DB_DEBUG) std::cout << "DBHandler: Check Module List" << std::endl;

    std::string mod_list_path = m_cache_dir+"/lib/modules.bson";
#ifdef MONGOCXX_INCLUDE
    json mod_list_json = this->toJson(mod_list_path, "bson");

    auto doc_value = make_document(kvp("componentType", "Module"));
    mongocxx::cursor cursor = db["component"].find(doc_value.view());
    for (auto doc : cursor) {
        std::string mo_serial_number = doc["serialNumber"].get_utf8().value.to_string();
        if (!mod_list_json[mo_serial_number].empty()) continue;

        std::string oid_str = doc["_id"].get_oid().value.to_string();
        mod_list_json[mo_serial_number] = {};
        mod_list_json[mo_serial_number]["chipType"] = this->getValue("component", "_id", oid_str, "oid", "chipType");
        mod_list_json[mo_serial_number]["module"]["serialNumber"] = mo_serial_number;
        mod_list_json[mo_serial_number]["module"]["componentType"] = "Module";

        doc_value = make_document(kvp("parent", oid_str));
        mongocxx::cursor cursor_cpr = db["childParentRelation"].find(doc_value.view());
        for (auto child : cursor_cpr) {
            std::string ch_oid_str = child["child"].get_utf8().value.to_string();

            json chip_json;
            chip_json["serialNumber"] = this->getValue("component", "_id", ch_oid_str, "oid", "serialNumber");
            chip_json["componentType"] = this->getValue("component", "_id", ch_oid_str, "oid", "componentType");
            chip_json["chipId"] = stoi(this->getValue("component", "_id", ch_oid_str, "oid", "chipId", "int"));
            mod_list_json[mo_serial_number]["chips"].push_back(chip_json);
        }
    }
    std::ofstream mod_list_file(mod_list_path, std::ios::binary);
    std::vector<std::uint8_t> v_bson = json::to_bson(mod_list_json);
    for (auto tmp : v_bson) {
        mod_list_file.write(reinterpret_cast<const char*>(std::addressof(tmp)), sizeof(std::uint8_t));
    }
    mod_list_file.close();
#endif
    return 0;
}

// public
void DBHandler::setUser(std::string i_user_path, std::string i_address_path) {
    // Set UserID from DB
    if (DB_DEBUG) std::cout << "DBHandler: Set user: " << i_user_path << std::endl;

    json user_json = this->checkUserCfg(i_user_path);
    std::string user_name = user_json["userName"];
    std::string institution = user_json["institution"];
    std::string user_identity = user_json["userIdentity"];
    std::cout << "DBHandler: User Information \n\tUser name: " << user_name << "\n\tInstitution: " << institution << "\n\tUser identity: " << user_identity << std::endl;

    // Set MAC address
    if (DB_DEBUG) std::cout << "DBHandler: Set address" << std::endl;
    json address_json = this->checkAddressCfg(i_address_path);
    std::string address = address_json["macAddress"];
    std::string hostname = address_json["hostname"];
    std::string site = address_json["institution"];
    std::cout << "DBHandler: MAC Address: " << address << std::endl;

#ifdef MONGOCXX_INCLUDE
    if (m_option=="db"||m_option=="dcs") {
        m_user_oid_str = this->registerUser(user_name, institution, user_identity);
        m_site_oid_str = this->registerSite(address, hostname, site);
    }
#endif
    this->cacheUser(i_user_path, i_address_path);

    return;
}

void DBHandler::setConnCfg(std::vector<std::string> i_conn_paths) {
    if (DB_DEBUG) std::cout << "DBHandler: Set connectivity config" << std::endl;

    for (auto conn_path : i_conn_paths) {
        if (DB_DEBUG) std::cout << "\tDBHandler: setting connectivity config file: " << conn_path << std::endl;
        json conn_json = this->checkConnCfg(conn_path);
        m_chip_type = conn_json["chipType"];
        if (m_chip_type == "FEI4B") m_chip_type = "FE-I4B";

#ifdef MONGOCXX_INCLUDE
        if (m_option=="db"||m_option=="register") this->registerConnCfg(conn_path);
#endif
        this->cacheConnCfg(conn_path);
    }
}

void DBHandler::setDCSCfg(std::string i_dcs_path, std::string i_tr_path) {
    if (DB_DEBUG) std::cout << "DBHandler: Set DCS config: " << i_dcs_path << std::endl;

    if (m_option!="dcs") {
        std::string message = "This function is disabled";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }
 
    json dcs_json = this->toJson(i_dcs_path);
    if (dcs_json["environments"].empty()) return;
    json env_json = dcs_json["environments"];

    for (int i=0; i<(int)env_json.size(); i++) {
        std::string num_str = std::to_string(i);
        this->checkEmpty(env_json[i]["status"].empty(), "environments."+num_str+".status", i_dcs_path, "Set enabled/disabled to register.");
        if (env_json[i]["status"]!="enabled") continue;

        this->checkDCSCfg(i_dcs_path, num_str, env_json[i]);
        if (!env_json[i]["path"].empty()) {
            int j_num = env_json[i]["num"];
            std::string log_path = env_json[i]["path"];
            this->checkDCSLog(log_path, i_dcs_path, env_json[i]["key"], j_num); 
        } else {
            this->checkNumber(env_json[i]["value"].is_number(), "environments."+num_str+".value", i_dcs_path);
        }
    }
    this->cacheDCSCfg(i_dcs_path, i_tr_path);
#ifdef MONGOCXX_INCLUDE
    this->registerDCS(i_dcs_path, i_tr_path);
#endif

    return;
}

void DBHandler::writeTestRunStart(std::string i_test_type, std::vector<std::string> i_conn_paths, int i_run_number, int i_target_charge, int i_target_tot) {
    if (DB_DEBUG) std::cout << "DBHandler: Write Test Run (start)" << std::endl;

    if (m_option!="scan"&&m_option!="db") {
        std::string message = "This function is disabled";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }

    int timestamp;
    if (m_cache_json["startTime"].empty()) timestamp = std::time(NULL);
    else timestamp = m_cache_json["startTime"];
    for (auto conn_path : i_conn_paths) {
        json conn_json = this->toJson(conn_path);
        std::string mo_serial_number = conn_json["module"]["serialNumber"];
#ifdef MONGOCXX_INCLUDE
        if (m_option=="db") {
            std::string tr_oid_str = this->registerTestRun(i_test_type, i_run_number, i_target_charge, i_target_tot, timestamp, mo_serial_number, "start");
            // stage
            if (!conn_json["stage"].empty()) {
                std::string stage = conn_json["stage"];
                this->addValue(tr_oid_str, "testRun", "stage", stage);
            }
            m_tr_oid_strs.push_back(tr_oid_str);
            this->registerComponentTestRun(conn_path, tr_oid_str, i_test_type, i_run_number);
            m_serial_numbers.push_back(mo_serial_number);
        }
#endif
        if (m_option=="scan") {
            std::time_t now = std::time(NULL);
            struct tm *lt = std::localtime(&now);
            char tmp[20];
            strftime(tmp, 20, "%Y%m%d", lt);
            std::string ts=tmp;
            std::string log_path = m_cache_dir+"/var/log/"+ts+"_scan.log";
            std::ofstream log_file_ofs(log_path, std::ios::app);
            strftime(tmp, 20, "%F_%H:%M:%S", lt);
            ts=tmp;
            log_file_ofs << ts << "," << mo_serial_number << "," << timestamp << "," << i_run_number << "," << i_test_type << std::endl;
            log_file_ofs.close();

            json tr_json;
            tr_json["startTime"] = timestamp;
            tr_json["serialNumber"] = mo_serial_number;
            tr_json["runNumber"] = i_run_number;
            tr_json["testType"] = i_test_type;

            std::string tmp_path = m_cache_dir+"/tmp/scan.json";
            std::ofstream tmp_file_ofs(tmp_path);
            tmp_file_ofs << std::setw(4) << tr_json;
            tmp_file_ofs.close();
        }
    }
    this->cacheTestRun(i_test_type, i_run_number, i_target_charge, i_target_tot, timestamp, -1, "");

    return;
}

void DBHandler::writeTestRunFinish(std::string i_test_type, std::vector<std::string> i_conn_paths, int i_run_number, int i_target_charge=-1, int i_target_tot=-1) {
    if (DB_DEBUG) std::cout << "DBHandler: Write Test Run (finish)" << std::endl;

    if (m_option!="scan"&&m_option!="db") {
        std::string message = "This function is disabled";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }

    int timestamp;
    if (m_cache_json["finishTime"].empty()) timestamp = std::time(NULL);
    else timestamp = m_cache_json["finishTime"];
    std::sort(m_histo_names.begin(), m_histo_names.end());
    m_histo_names.erase(std::unique(m_histo_names.begin(), m_histo_names.end()), m_histo_names.end());
#ifdef MONGOCXX_INCLUDE
    if (m_option=="db") {
        for (auto tr_oid_str : m_tr_oid_strs) {
            this->registerTestRun(i_test_type, i_run_number, i_target_charge, i_target_tot, timestamp, "", "finish", tr_oid_str);
        }
    }
#endif
    this->cacheTestRun(i_test_type, i_run_number, i_target_charge, i_target_tot, -1, timestamp, "");
    this->writeJson("status", "waiting", m_log_path, m_log_json);
    
    return;
}

void DBHandler::writeConfig(std::string i_serial_number, std::string i_file_path, std::string i_filename, std::string i_title, std::string i_collection) {
    if (DB_DEBUG) std::cout << "DBHandler: Write Config Json: " << i_file_path << std::endl;

    std::ifstream file_ifs(i_file_path);
    if (!file_ifs) return;
    file_ifs.close();

#ifdef MONGOCXX_INCLUDE
    if (m_option=="db") this->registerConfig(i_serial_number, i_file_path, i_filename, i_title, i_collection);
#endif
    this->cacheConfig(i_serial_number, i_file_path, i_filename, i_title, i_collection);

    return;
}

void DBHandler::writeAttachment(std::string i_serial_number, std::string i_file_path, std::string i_histo_name) {
    if (DB_DEBUG) std::cout << "DBHandler: Write Attachment: " << i_file_path << std::endl;

    std::ifstream file_ifs(i_file_path);
    if (!file_ifs) return;
    file_ifs.close();

#ifdef MONGOCXX_INCLUDE
    if (m_option=="db") this->registerAttachment(i_serial_number, i_file_path, i_histo_name);
#endif
    this->cacheAttachment(i_serial_number, i_file_path, i_histo_name);

    m_histo_names.push_back(i_histo_name);

    return;
}

void DBHandler::writeDCS(std::string i_dcs_file_path, std::string i_tr_file_path) {
    if (DB_DEBUG) std::cout << "DBHandler: Write DCS config: " << i_dcs_file_path << std::endl;

    // check test run data registered
    json tr_json = this->toJson(i_tr_file_path);
    if (tr_json["id"].empty()) {
        this->checkEmpty(tr_json["startTime"].empty()||tr_json["serialNumber"].empty(), "testRun.startTime&&testRun.serialNumber", i_tr_file_path);
    }
    std::string tr_oid_str = "";
    std::string serial_number = "";
    int timestamp = -1;
    if (!tr_json["id"].empty()) tr_oid_str = tr_json["id"];
    if (!tr_json["serialNumber"].empty()) serial_number = tr_json["serialNumber"];
    if (!tr_json["startTime"].empty()) timestamp = tr_json["startTime"];
    this->getTestRunData(tr_oid_str, serial_number, timestamp);

#ifdef MONGOCXX_INCLUDE

    this->setDCSCfg(i_dcs_file_path, i_tr_file_path);
    //this->writeJson("status", "done", m_cache_path, m_cache_json);
#else
    std::string message = "Not found MongoDB C++ Driver!";
    std::string function = __PRETTY_FUNCTION__;
    this->alert(function, message, "warning");
#endif

    return;
}

void DBHandler::writeCache(std::string i_cache_dir) {
    if (DB_DEBUG) std::cout << "DBHandler: Write cache data: " << i_cache_dir << std::endl;

#ifdef MONGOCXX_INCLUDE
    m_cache_path = i_cache_dir+"/cacheLog.json";
    m_cache_json = this->toJson(m_cache_path);
    m_log_json["cachePath"] = m_cache_json["logPath"];
    // check cache directory
    if (m_cache_json["status"]!="waiting") return;
    // write status
    this->writeJson("status", "writing", m_cache_path, m_cache_json);

    // set user
    this->setUser(i_cache_dir+"/user.json", i_cache_dir+"/address.json");

    // set connectivity config
    std::vector<std::string> conn_paths;
    for (int i=0; i<(int) m_cache_json["configs"]["connCfg"].size(); i++) {
        std::string conn_path = m_cache_json["configs"]["connCfg"][i]["path"];
        conn_paths.push_back(conn_path);
    }
    this->setConnCfg(conn_paths);

    // write test run
    std::string scan_type = m_cache_json["testType"];
    int run_number        = m_cache_json["runNumber"];
    int target_charge     = m_cache_json["targetCharge"];
    int target_tot        = m_cache_json["targetTot"];
    this->writeTestRunStart(scan_type, conn_paths, run_number, target_charge, target_tot);

    // write config
    // controller config
    std::string ctrl_cfg_path = m_cache_json["configs"]["ctrlCfg"][0]["path"];
    this->writeConfig("", ctrl_cfg_path, "controller", "ctrlCfg", "testRun"); //controller config

    // scan config
    std::string scan_cfg_path;
    if (!m_cache_json["configs"]["scanCfg"].empty()) scan_cfg_path = m_cache_json["configs"]["scanCfg"][0]["path"];
    else scan_cfg_path = "";
    this->writeConfig("", scan_cfg_path, scan_type, "scanCfg", "testRun"); 

    // chip config
    for (auto chip_json: m_cache_json["configs"]["chipCfg"]) {
        std::string chip_id   = chip_json["_id"];
        std::string chip_path = chip_json["path"];
        std::string filename  = chip_json["filename"];
        std::string title     = chip_json["title"];
        this->writeConfig(chip_id, chip_path, filename, title, "componentTestRun");
    }

    // attachments
    for (auto attachment: m_cache_json["attachments"]) {
        std::string chip_id   = attachment["_id"];
        std::string file_path = attachment["path"];
        std::string histoname = attachment["histoname"];
        this->writeAttachment(chip_id, file_path, histoname);
    }

    // finish
    this->writeTestRunFinish(scan_type, conn_paths, run_number, target_charge, target_tot);
    this->writeJson("status", "done", m_cache_path, m_cache_json);
    this->writeJson("status", "done", m_log_path, m_log_json);

    for (auto serial_number : m_serial_numbers) {
        int start_time = m_cache_json["startTime"];
        this->getTestRunData("", serial_number, start_time);
        if (m_tr_oid_str=="") {
            this->writeJson("status", "failure", m_cache_path, m_cache_json);
            this->writeJson("status", "failure", m_log_path, m_log_json);
        }
    }
#else
    std::string message = "Not found MongoDB C++ Driver!";
    std::string function = __PRETTY_FUNCTION__;
    this->alert(function, message);
#endif

    return;
}

void DBHandler::getData(std::string i_info_file_path, std::string i_get_type) {
    if (DB_DEBUG) std::cout << "DBHandler: Get Data" << std::endl;
    
#ifdef MONGOCXX_INCLUDE
    json info_json;
    if (i_info_file_path!="") info_json = toJson(i_info_file_path);
    if (i_get_type == "testRunLog") this->getTestRunLog(info_json);
    if (i_get_type == "testRunId") this->getTestRunId(info_json);
    if (i_get_type == "config") this->getConfigData(info_json);
    if (i_get_type == "userId") this->getUserId(info_json);
    if (i_get_type == "dat") this->getDatData(info_json);
#else
    std::string message = "Not found MongoDB C++ Driver!";
    std::string function = __PRETTY_FUNCTION__;
    this->alert(function, message);
#endif

    return;
}

//*****************************************************************************************************
// Protected fuctions
//
#ifdef MONGOCXX_INCLUDE
std::string DBHandler::getValue(std::string i_collection_name, std::string i_member_key, std::string i_member_value, std::string i_member_bson_type, std::string i_key, std::string i_bson_type){
    if (DB_DEBUG) std::cout << "\tDBHandler: Get value of key: '" << i_key << "' from: '" << i_collection_name << "', {'" << i_member_key << ": '" << i_member_value << "'}" << std::endl;
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
        std::string message = "Cannot find '" + i_key + "' from member '" + i_member_key + ": " + i_member_value + "' in collection name: '" + i_collection_name + "'";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return "ERROR";
    }
}

std::string DBHandler::getComponent(std::string i_serial_number, std::string i_file_path) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Get component data: 'serialnumber':" << i_serial_number << std::endl;
    std::string oid_str = "";
    bsoncxx::document::value query_doc = document{} <<  
        "serialNumber"  << i_serial_number <<
    finalize;
    auto maybe_result = db["component"].find_one(query_doc.view());
    if (maybe_result) {
        oid_str = maybe_result->view()["_id"].get_oid().value.to_string();
    }
    return oid_str;
}

// Register Function
std::string DBHandler::registerUser(std::string i_user_name, std::string i_institution, std::string i_user_identity) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Register user \n\tUser name: " << i_user_name << "\n\tInstitution: " << i_institution << "\n\tUser identity: " << i_user_identity << std::endl;
    mongocxx::collection collection = db["user"];

    auto doc_value = document{} <<
        "userName"     << i_user_name <<        
        "institution"  << i_institution << 
        "userIdentity" << i_user_identity << 
    finalize;
    auto maybe_result = collection.find_one(doc_value.view());
    if (maybe_result) return maybe_result->view()["_id"].get_oid().value.to_string();

    doc_value = document{} <<  
        "sys"           << open_document << close_document <<
        "userName"      << i_user_name <<
        "userIdentity"  << i_user_identity <<
        "institution"   << i_institution <<
        "userType"      << "readWrite" <<
        "dbVersion"     << -1 <<
    finalize;
    auto oid = collection.insert_one(doc_value.view())->inserted_id().get_oid().value;
    this->addSys(oid.to_string(), "user");
    this->addVersion("user", "_id", oid.to_string(), "oid");

    return oid.to_string();
}

std::string DBHandler::registerSite(std::string i_address, std::string i_hostname, std::string i_site) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Register site \n\tAddress: " << i_address << "\n\tName: " << i_hostname << "\n\tInstitution: " << i_site << std::endl;
    mongocxx::collection collection = db["institution"];
    // Set MAC address
    auto doc_value = document{} << 
        "address"     << i_address << 
        "institution" << i_site << 
    finalize;
    auto maybe_result = collection.find_one(doc_value.view());
    if (maybe_result) return maybe_result->view()["_id"].get_oid().value.to_string();

    doc_value = document{} <<
        "sys"         << open_document << close_document <<
        "name"        << i_hostname <<
        "institution" << i_site <<
        "address"     << i_address <<
        "dbVersion"     << -1 <<
    finalize;
    bsoncxx::oid oid = collection.insert_one(doc_value.view())->inserted_id().get_oid().value;
    this->addSys(oid.to_string(), "institution");
    this->addVersion("institution", "_id", oid.to_string(), "oid");

    return oid.to_string();
}

// Will be deleted // TODO enable to register component in viewer application
void DBHandler::registerConnCfg(std::string i_conn_path) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Register from connectivity: " << i_conn_path << std::endl;

    std::string mod_list_path = m_cache_dir+"/lib/modules.bson";
    json mod_list_json = this->toJson(mod_list_path, "bson");
    json conn_json = this->toJson(i_conn_path);
    std::string mo_serial_number = conn_json["module"]["serialNumber"];
    if (!mod_list_json[mo_serial_number].empty()) conn_json = mod_list_json[mo_serial_number];

    /// Confirmation
    bool module_is_exist = false;
    bool chip_is_exist = false;
    bool cpr_is_fine = true;
    // Module
    std::string mo_oid_str = this->getComponent(mo_serial_number, i_conn_path);
    if (mo_oid_str!="") module_is_exist = true;
    // Chip
    int chips = 0;
    for (unsigned i=0; i<conn_json["chips"].size(); i++) {
        std::string ch_serial_number = conn_json["chips"][i]["serialNumber"];
        std::string chip_oid_str = this->getComponent(ch_serial_number, i_conn_path);
        if (chip_oid_str!="") {
            chip_is_exist = true;
            auto doc_value = document{} <<
                "parent" << mo_oid_str <<
                "child" << chip_oid_str <<
                "status" << "active" <<
            finalize;
            auto maybe_result = db["childParentRelation"].find_one(doc_value.view());
            if (!maybe_result) cpr_is_fine = false;
        }
        chips++;
    }
    if (module_is_exist&&!chip_is_exist) {
        std::string message = "There are registered module in connectivity : "+i_conn_path;
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    } else if (!module_is_exist&&chip_is_exist) {
        std::string message = "There are registered chips in connectivity : "+i_conn_path;
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    } else if (!cpr_is_fine) {
        std::string message = "There are wrong relationship between module and chips in connectivity : "+i_conn_path;
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    } else if (module_is_exist&&chip_is_exist&&cpr_is_fine) {
        return;
    }
 
    // Register module component
    std::string mo_component_type = conn_json["module"]["componentType"];
    mo_oid_str = this->registerComponent(mo_serial_number, mo_component_type, -1, chips);

    mod_list_json[mo_serial_number] = {};
    mod_list_json[mo_serial_number]["chipType"] = m_chip_type;
    mod_list_json[mo_serial_number]["module"]["serialNumber"] = mo_serial_number;
    mod_list_json[mo_serial_number]["module"]["componentType"] = mo_component_type;

    for (unsigned i=0; i<conn_json["chips"].size(); i++) {
        std::string ch_serial_number  = conn_json["chips"][i]["serialNumber"];
        std::string ch_component_type = conn_json["chips"][i]["componentType"];
        int chip_id = conn_json["chips"][i]["chipId"];
        // Register chip component
        std::string ch_oid_str = this->registerComponent(ch_serial_number, ch_component_type, chip_id, -1);
        this->registerChildParentRelation(mo_oid_str, ch_oid_str, chip_id);

        json chip_json;
        chip_json["serialNumber"] = ch_serial_number;
        chip_json["componentType"] = ch_component_type;
        chip_json["chipId"] = chip_id;
        mod_list_json[mo_serial_number]["chips"].push_back(chip_json);
    }

    std::ofstream mod_list_file(mod_list_path, std::ios::binary);
    std::vector<std::uint8_t> v_bson = json::to_bson(mod_list_json);
    for (auto tmp : v_bson) {
        mod_list_file.write(reinterpret_cast<const char*>(std::addressof(tmp)), sizeof(std::uint8_t));
    }
    mod_list_file.close();
}

std::string DBHandler::registerComponent(std::string i_serial_number, std::string i_component_type, int i_chip_id, int i_chips) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Register Component: " << i_serial_number << std::endl;
    mongocxx::collection collection = db["component"];

    auto doc_value = document{} <<  
        "serialNumber"  << i_serial_number <<
    finalize;
    auto maybe_result = collection.find_one(doc_value.view());
    if (maybe_result) return maybe_result->view()["_id"].get_oid().value.to_string();

    doc_value = document{} <<  
        "sys"           << open_document << close_document <<
        "serialNumber"  << i_serial_number <<
        "chipType"      << m_chip_type <<
        "componentType" << i_component_type <<
        "children"      << i_chips <<
        "chipId"        << i_chip_id <<
        "dbVersion"     << -1 <<
        "address"       << "..." <<
        "user_id"       << "..." <<
    finalize;
    auto oid = collection.insert_one(doc_value.view())->inserted_id().get_oid().value;
    std::string oid_str = oid.to_string();
    this->addSys(oid_str, "component");
    this->addUser("component", oid_str);
    this->addVersion("component", "_id", oid_str, "oid");

    return oid_str;
}

void DBHandler::registerChildParentRelation(std::string i_parent_oid_str, std::string i_child_oid_str, int i_chip_id) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Register childParentRelation." << std::endl;
    mongocxx::collection collection = db["childParentRelation"];

    auto doc_value = document{} <<  
        "parent" << i_parent_oid_str <<
        "child"  << i_child_oid_str <<
        "status" << "active" <<
    finalize;
    auto maybe_result = collection.find_one(doc_value.view());
    if (maybe_result) return;

    doc_value = document{} <<  
        "sys"       << open_document << close_document <<
        "parent"    << i_parent_oid_str <<
        "child"     << i_child_oid_str <<
        "chipId"    << i_chip_id <<
        "status"    << "active" <<
        "dbVersion" << -1 <<
    finalize;

    std::string oid_str = collection.insert_one(doc_value.view())->inserted_id().get_oid().value.to_string();
    this->addSys(oid_str, "childParentRelation");
    this->addVersion("childParentRelation", "_id", oid_str, "oid");

    return;
}

void DBHandler::registerConfig(std::string i_serial_number, std::string i_file_path, std::string i_filename, std::string i_title, std::string i_collection) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Register Config Json: " << i_file_path << std::endl;

    std::string ctr_oid_str;
    if (i_serial_number!="") {
        auto doc_value = document{} << 
            "serialNumber" << i_serial_number << 
        finalize;
        std::string cmp_oid_str = db["component"].find_one(doc_value.view())->view()["_id"].get_oid().value.to_string();
        std::string oid_str;
        for (auto tr_oid_str : m_tr_oid_strs) {
            doc_value = document{} <<
                "component" << cmp_oid_str <<
                "testRun"   << tr_oid_str  <<
            finalize;
            auto result = db["componentTestRun"].find_one(doc_value.view());
            if (result) {
                ctr_oid_str = result->view()["_id"].get_oid().value.to_string();
                break;
            }
        }
    }

    std::string cfg_oid_str;
    if (i_collection=="testRun") {
        cfg_oid_str = this->registerJsonCode(i_file_path, i_filename+".json", i_title, "gj");
        for (auto tr_oid_str : m_tr_oid_strs) {
            this->addValue(tr_oid_str, i_collection, i_title, cfg_oid_str);
        }
    }
    if (i_collection == "componentTestRun") {
        cfg_oid_str = this->registerJsonCode(i_file_path, i_title+".json", i_title, "gj");
        this->addValue(ctr_oid_str, i_collection, i_filename, cfg_oid_str);
    }
    return;
}

void DBHandler::registerAttachment(std::string i_serial_number, std::string i_file_path, std::string i_histo_name) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Register Attachment: " << i_file_path << std::endl;

    std::string ctr_oid_str;
    if (i_serial_number!="") {
        auto doc_value = document{} << 
            "serialNumber" << i_serial_number <<
        finalize; 
        std::string cmp_oid_str = db["component"].find_one(doc_value.view())->view()["_id"].get_oid().value.to_string();
        std::string oid_str;
        for (auto tr_oid_str : m_tr_oid_strs) {
            doc_value = document{} <<
                "component" << cmp_oid_str <<
                "testRun" << tr_oid_str <<
            finalize;
            auto result = db["componentTestRun"].find_one(doc_value.view());
            if (result) {
                ctr_oid_str = result->view()["_id"].get_oid().value.to_string();
                break;
            }
        }
    }

    std::string file_path = i_file_path;
    std::ifstream file_ifs(file_path);
    if (file_ifs) {
        std::string oid_str = this->writeGridFsFile(file_path, i_histo_name + ".dat");
        this->addAttachment(ctr_oid_str, "componentTestRun", oid_str, i_histo_name, "describe", "dat", i_histo_name+".dat");
    }
}

void DBHandler::registerDCS(std::string i_dcs_path, std::string i_tr_path) {
    if (DB_DEBUG) std::cout << "DBHandler: Register Environment: " << i_dcs_path << std::endl;

    json dcs_json = toJson(i_dcs_path);
    json tr_json = toJson(i_tr_path);

    mongocxx::collection collection = db["environment"];

    // register the environmental key and description
    if (m_tr_oid_str=="") {
        std::string message = "Not found relational test run data in DB";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
    auto doc_value = make_document(kvp("_id", bsoncxx::oid(m_tr_oid_str)));
    auto result = db["testRun"].find_one(doc_value.view());
    if (result->view()["environment"].get_utf8().value.to_string()!="...") {
        std::string message = "Already registered dcs data to this test run data";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
    if (dcs_json["environments"].empty()) return;
    json env_json = dcs_json["environments"];
    std::vector<std::string> env_keys, descriptions, env_modes, env_paths;
    std::vector<float> env_settings, env_vals, env_margins;
    std::vector<int> env_nums;
    for (auto j: env_json) {
        if (j["status"]!="enabled") continue;

        env_keys.push_back(j["key"]);
        env_nums.push_back(j["num"]);
        descriptions.push_back(j["description"]);
        if (!j["path"].empty()) env_paths.push_back(j["path"]);
        else env_paths.push_back("null");
        if (!j["value"].empty()) env_vals.push_back(j["value"]);
        else env_vals.push_back(-1); 
        if (!j["margin"].empty()) env_margins.push_back(j["margin"]);
        else env_margins.push_back(-1);
    }

    // get start time from scan data
    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(result->view()["startTime"].get_date().value);
    std::time_t starttime = s.count();
    if (DB_DEBUG) {
        char buf[80];
        struct tm *lt = std::localtime(&starttime);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
        std::cout << "\tDBHandler: Register Environment Start Time: " << buf << std::endl;
    }

    std::chrono::seconds f = std::chrono::duration_cast<std::chrono::seconds>(result->view()["finishTime"].get_date().value);
    std::time_t finishtime = f.count();
    if (DB_DEBUG) {
        char buf[80];
        struct tm *lt = std::localtime(&finishtime);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
        std::cout << "\tDBHandler: Register Environment Finish Time: " << buf << std::endl;
    }

    // insert environment doc
    document builder{};
    auto docs = builder << 
        "dbVersion" << -1 <<
        "sys"       << open_document << close_document;
    for (unsigned i=0;i<env_keys.size();i++) {
        docs = docs << env_keys[i] << open_array << close_array;
    }
    bsoncxx::document::value doc_value2 = docs << finalize;
    bsoncxx::oid env_oid = collection.insert_one(doc_value2.view())->inserted_id().get_oid().value;

    for (int i=0; i<(int)env_keys.size(); i++) {
        if (DB_DEBUG) std::cout << "\tDBHandler: Register Environment: " << env_keys[i] << std::endl;
        if (env_paths[i]!="null") {
            std::ifstream env_ifs(env_paths[i]);
            std::size_t suffix = env_paths[i].find_last_of('.');
            std::string extension = env_paths[i].substr(suffix + 1);
            std::string del;
            if (extension=="dat") del = " ";
            else if (extension=="csv") del = ",";
            char separator = del[0];

            char tmp_key[1000], tmp_num[1000], tmp[1000];
            int data_num = 0;
            std::vector<float> key_values;
            std::vector<int> dates;
            // key and num
            env_ifs.getline(tmp_key, 1000);
            env_ifs.getline(tmp_num, 1000);
            int columns = 0;
            int key=-1;
            for (const auto s_tmp : split(tmp_key, separator)) {
                if (env_keys[i]==s_tmp) {
                    int cnt=0;
                    for (const auto s_tmp_num : split(tmp_num, separator)) {
                        if (cnt==columns&&env_nums[i]==stoi(s_tmp_num)) key = columns;
                        cnt++;
                    }
                }
                columns++;
            }
            // mode
            env_ifs.getline(tmp, 1000);
            int cnt = 0;
            for (const auto s_tmp : split(tmp, separator)) {
                if (cnt==key) env_modes.push_back(s_tmp);
                cnt++;
            }
            // setting
            env_ifs.getline(tmp, 1000);
            cnt = 0;
            for (const auto s_tmp : split(tmp, separator)) {
                if (cnt==key) env_settings.push_back(stof(s_tmp));
                cnt++;
            }
            // value
            while (env_ifs.getline(tmp, 1000)) {
                std::string datetime = "";
                std::string value = "";
                cnt = 0;
                for (const auto s_tmp : split(tmp, separator)) {
                    if (cnt==1) datetime = s_tmp;    
                    if (cnt==key) value = s_tmp;
                    cnt++;
                }
                if (value==""||datetime == "") break;
                std::time_t timestamp = stoi(datetime);
                if (env_margins[i]!=-1) {
                    if (difftime(starttime,timestamp)<env_margins[i]&&difftime(timestamp,finishtime)<env_margins[i]) { // store data from 1 minute before the starting time of the scan
                        key_values.push_back(stof(value));
                        dates.push_back(stoi(datetime));
                        data_num++;
                    }
                } else {
                    key_values.push_back(stof(value));
                    dates.push_back(stoi(datetime));
                    data_num++;
                }
            }
            auto array_builder = bsoncxx::builder::basic::array{};
            for (int j=0;j<data_num;j++) {
                std::time_t timestamp = dates[j];
                array_builder.append(
                    document{} << 
                        "date"        << bsoncxx::types::b_date{std::chrono::system_clock::from_time_t(timestamp)} <<
                        "value"       << key_values[j] <<
                    finalize
                ); 
            }
            collection.update_one(
                document{} << "_id" << env_oid << finalize,
                document{} << "$push" << open_document <<
                    env_keys[i] << open_document <<
                        "data"        << array_builder <<
                        "description" << descriptions[i] <<
                        "mode" << env_modes[i] <<
                        "setting" << env_settings[i] <<
                        "num" << env_nums[i] <<
                    close_document <<
                close_document << finalize
            );
        } else {
            auto array_builder = bsoncxx::builder::basic::array{};
            array_builder.append(
                document{} << 
                    "date"        << bsoncxx::types::b_date{std::chrono::system_clock::from_time_t(starttime)} <<
                    "value"       << env_vals[i] <<
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
    this->addValue(m_tr_oid_str, "testRun", "environment", env_oid.to_string());

    if (m_option=="dcs") {
        this->writeJson("status", "done", m_log_path, m_log_json);
        doc_value = make_document(kvp("_id", bsoncxx::oid(m_tr_oid_str)));
        result = db["testRun"].find_one(doc_value.view());
        if (result->view()["environment"].get_utf8().value.to_string()=="...") {
            this->writeJson("status", "failure", m_log_path, m_log_json);
        }
    }

    return;
}

std::string DBHandler::registerJsonCode(std::string i_file_path, std::string i_filename, std::string i_title, std::string i_type) {
    if (DB_DEBUG) std::cout << "\tDBHandler: upload json file" << std::endl;

    std::string type_doc;
    std::string data_id;
    if (i_type == "m") {
        data_id = writeJsonCode_Msgpack(i_file_path, i_filename, i_title);
        type_doc = "msgpack";
    } else if (i_type == "gj") {
        data_id = writeJsonCode_Gridfs(i_file_path, i_filename, i_title);
        type_doc = "fs.files";
    } else {
        std::string message = "Unknown type '" + i_type + "' to upload json file.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return "ERROR";
    } 
    mongocxx::collection collection = db["config"];
    bsoncxx::document::value doc_value = document{} << 
      "sys"       << open_document << close_document <<
      "filename"  << i_filename <<
      "chipType"  << m_chip_type << 
      "title"     << i_title <<
      "format"    << type_doc <<
      "data_id"   << data_id <<
      "dbVersion" << -1 <<
    finalize; 
    std::string oid_str = collection.insert_one(doc_value.view())->inserted_id().get_oid().value.to_string();
    this->addSys(oid_str, "config");
    this->addVersion("config", "_id", oid_str, "oid");
    return oid_str;
}

void DBHandler::registerComponentTestRun(std::string i_conn_path, std::string i_tr_oid_str, std::string i_test_type, int i_run_number) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Register Com-Test Run" << std::endl;

    json conn_json = this->toJson(i_conn_path);
    std::vector<std::string> cmp_oid_strs;
    std::string mo_serial_number = conn_json["module"]["serialNumber"];
    std::string mo_oid_str = this->getComponent(mo_serial_number, i_conn_path);
    cmp_oid_strs.push_back(mo_oid_str);
    auto doc_value = document{} <<
        "parent" << mo_oid_str <<
        "status" << "active" <<
    finalize;
    mongocxx::cursor cursor = db["childParentRelation"].find(doc_value.view());
    for (auto doc : cursor) {
        std::string chip_oid_str = doc["child"].get_utf8().value.to_string();
        cmp_oid_strs.push_back(chip_oid_str);
    }

    for (auto cmp_oid_str: cmp_oid_strs) {
        std::string serial_number = this->getValue("component", "_id", cmp_oid_str, "oid", "serialNumber");
        int chip_tx = -1;
        int chip_rx = -1;
        for (unsigned i=0; i<conn_json["chips"].size(); i++) {
            if (conn_json["chips"][i]["serialNumber"]==serial_number) {
                chip_tx = conn_json["chips"][i]["tx"];
                chip_rx = conn_json["chips"][i]["rx"];
            }
        }
        auto doc_value = document{} <<  
            "sys"         << open_document << close_document <<
            "component"   << cmp_oid_str << // id of component
            "state"       << "..." << // code of test run state
            "testType"    << i_test_type << // id of test type
            "testRun"     << i_tr_oid_str << // id of test run, test is planned if it is null
            "qaTest"      << false << // flag if it is the QA test
            "runNumber"   << i_run_number << // run number
            "passed"      << true << // flag if the test passed
            "problems"    << true << // flag if any problem occured
            "attachments" << open_array << close_array <<
            "tx"          << chip_tx <<
            "rx"          << chip_rx <<
            "beforeCfg"   << "..." <<
            "afterCfg"    << "..." <<
            "dbVersion"   << -1 <<
        finalize;
        mongocxx::collection collection = db["componentTestRun"];
        bsoncxx::oid oid = collection.insert_one(doc_value.view())->inserted_id().get_oid().value;
        this->addSys(oid.to_string(), "componentTestRun");
        this->addVersion("componentTestRun", "_id", oid.to_string(), "oid");
    }
}

std::string DBHandler::registerTestRun(std::string i_test_type, int i_run_number, int i_target_charge, int i_target_tot, int i_time, std::string i_serial_number, std::string i_type, std::string i_tr_oid_str) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Register Test Run" << std::endl;

    mongocxx::collection collection = db["testRun"];
    std::string oid_str;

    if (i_type=="start") {
        std::time_t timestamp = i_time;
        auto startTime = std::chrono::system_clock::from_time_t(timestamp);
        auto doc_value = document{} <<
            "startTime"    << bsoncxx::types::b_date{startTime} <<
            "serialNumber" << i_serial_number <<
        finalize;
        auto maybe_result = collection.find_one(doc_value.view());
        if (maybe_result) return maybe_result->view()["_id"].get_oid().value.to_string();

        doc_value = document{} <<  
            "sys"          << open_document << close_document <<
            "testType"     << i_test_type << // id of test type //TODO make it id
            "runNumber"    << i_run_number << // number of test run
            "startTime"    << bsoncxx::types::b_date{startTime} << // date when the test run was taken
            "passed"       << false << // flag if test passed
            "problems"     << false << // flag if any problem occured
            "state"        << "ready" << // state of component ["ready", "requestedToTrash", "trashed"]
            "targetCharge" << i_target_charge <<
            "targetTot"    << i_target_tot <<
            "comments"     << open_array << close_array <<
            "defects"      << open_array << close_array <<
            "finishTime"   << bsoncxx::types::b_date{startTime} <<
            "plots"        << open_array << close_array <<
            "serialNumber" << i_serial_number << // module serial number
            "stage"        << "..." <<
            "ctrlCfg"      << "..." << 
            "scanCfg"      << "..." <<
            "environment"  << "..." <<
            "address"      << "..." <<
            "user_id"      << "..." << 
            "dbVersion"    << -1 << 
        finalize;

        auto oid = collection.insert_one(doc_value.view())->inserted_id().get_oid().value;
        oid_str = oid.to_string();
        this->addSys(oid_str, "testRun");
        this->addVersion("testRun", "_id", oid_str, "oid");
        this->addUser("testRun", oid_str);
    }
    if (i_type=="finish") {
        std::time_t timestamp = i_time;
        auto finishTime = std::chrono::system_clock::from_time_t(timestamp);
        auto array_builder = bsoncxx::builder::basic::array{};
        for (auto s_tmp: m_histo_names) array_builder.append(s_tmp); 
        bsoncxx::document::value doc_value = document{} <<  
            "passed"       << true << // flag if test passed
            "problems"     << true << // flag if any problem occured
            "finishTime"   << bsoncxx::types::b_date{finishTime} <<
            "plots"        << array_builder <<
        finalize;
        collection.update_one(
            document{} << "_id" << bsoncxx::oid(i_tr_oid_str) << finalize,
            document{} << "$set" << doc_value.view() << finalize
        );
        oid_str = i_tr_oid_str;
    }
    return oid_str;
}

// Add function
void DBHandler::addComment(std::string i_collection_name, std::string i_oid_str, std::string i_comment) { // To be deleted or seperated
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Add comment" << std::endl;
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

void DBHandler::addValue(std::string i_oid_str, std::string i_collection_name, std::string i_key, std::string i_value) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Add document: " << i_key << " to " << i_collection_name << std::endl;
    bsoncxx::oid i_oid(i_oid_str);
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$set" << open_document <<
            i_key << i_value << 
        close_document << finalize
    );
}

void DBHandler::addAttachment(std::string i_oid_str, std::string i_collection_name, std::string i_file_oid_str, std::string i_title, std::string i_description, std::string i_content_type, std::string i_filename) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Add attachment: " << i_filename << std::endl;
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

void DBHandler::addDefect(std::string i_oid_str, std::string i_collection_name, std::string i_defect_name, std::string i_description) { // To de deleted
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Add defect" << std::endl;
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

void DBHandler::addUser(std::string i_collection_name, std::string i_oid_str) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Add user and institution" << std::endl;
    bsoncxx::oid i_oid(i_oid_str);

    // Update document
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$set" << open_document <<
            "address" << m_site_oid_str <<
            "user_id" << m_user_oid_str <<
        close_document << finalize
    );
}

void DBHandler::addSys(std::string i_oid_str, std::string i_collection_name) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Add sys" << std::endl;
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

void DBHandler::addVersion(std::string i_collection_name, std::string i_member_key, std::string i_member_value, std::string i_member_bson_type) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Add DB Version " << m_db_version << std::endl;
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

// will be deleted
void DBHandler::writeFromDirectory(std::string i_collection_name, std::string i_oid_str, std::string i_output_dir, std::string i_filter) {
    if (DB_DEBUG) std::cout << "DBHandler: Write from directory" << std::endl;
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
            std::size_t suffix = file_path.find_last_of('.');
            std::string filename = file_path.substr(pathPos+1, suffix-pathPos-1);
            std::size_t pos =  filename.find(i_filter);
            std::size_t namePos =  filename.find_last_of('_');
            if (pos != std::string::npos) {
                std::string extension = file_path.substr(suffix + 1);
                std::string oid_str = "";
                if (extension=="dat")
                    oid_str = this->writeGridFsFile(file_path, filename+"."+extension);
                else
                    oid_str = "ERROR";
                if ( oid_str != "ERROR" )
                    this->addAttachment(i_oid_str, i_collection_name, oid_str, filename.substr(namePos+1), "describe", extension, filename);
            }
        }
    }
}

std::string DBHandler::writeGridFsFile(std::string i_file_path, std::string i_filename) {
    if (DB_DEBUG) std::cout << "DBHandler: Write attachment: " << i_file_path << std::endl;
    mongocxx::gridfs::bucket gb = db.gridfs_bucket();
    mongocxx::collection collection = db["fs.files"];
      
    std::ifstream file_ifs(i_file_path);
    std::istream &file_is = file_ifs;
    std::string oid_str = gb.upload_from_stream(i_filename, &file_is).id().get_oid().value.to_string();
    file_ifs.close();

    this->addVersion("fs.files", "_id", oid_str, "oid");
    this->addVersion("fs.chunks", "files_id", oid_str, "oid");
    return oid_str;
}

std::string DBHandler::writeJsonCode_Msgpack(std::string i_file_path, std::string i_filename, std::string i_title) {
    if (DB_DEBUG) std::cout << "DBHandler: Write json file: " << i_file_path << std::endl;

    mongocxx::collection collection = db["msgpack"];
    std::string hash = this->getHash(i_file_path);
    mongocxx::options::find opts;
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(
        document{} << "hash" << hash << finalize,
        opts.return_key(true)
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
        "hash" << hash <<
        "dbVersion" << -1 << 
    finalize; 
    std::string oid_str = collection.insert_one(doc_value.view())->inserted_id().get_oid().value.to_string();
    this->addVersion("json", "_id", oid_str, "oid");
    return oid_str;
}

std::string DBHandler::writeJsonCode_Gridfs(std::string i_file_path, std::string i_filename, std::string i_title) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Write json file: " << i_file_path << std::endl;

    std::string hash = this->getHash(i_file_path);
    mongocxx::options::find opts;
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = db["fs.files"].find_one(
        document{} << "hash" << hash << finalize,
        opts.return_key(true)
    );
    if (maybe_result) return maybe_result->view()["_id"].get_oid().value.to_string();

    std::string oid_str = this->writeGridFsFile(i_file_path, i_filename); 
    db["fs.files"].update_one(
        document{} << "_id" << bsoncxx::oid(oid_str) << finalize,
        document{} << "$set" << open_document <<
            "hash" << hash << 
        close_document << finalize
    );
    this->addVersion("fs.files", "_id", oid_str, "oid");
    this->addVersion("fs.chunks", "files_id", oid_str, "oid");
    return oid_str;
}

std::string DBHandler::getHash(std::string i_file_path) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Get hash code from: " << i_file_path << std::endl;
    SHA256_CTX context;
    if (!SHA256_Init(&context)) return "ERROR"; 
    static const int K_READ_BUF_SIZE{ 1024*16 };
    char buf[K_READ_BUF_SIZE];
    std::ifstream file(i_file_path, std::ifstream::binary);
    while (file.good()) {
        file.read(buf, sizeof(buf));
        if(!SHA256_Update(&context, buf, file.gcount())) return "ERROR";
    }
    unsigned char result[SHA256_DIGEST_LENGTH];
    if (!SHA256_Final(result, &context)) return "ERROR";
    std::stringstream shastr;
    shastr << std::hex << std::setfill('0');
    for (const auto &byte: result) {
        shastr << std::setw(2) << (int)byte;
    }
    return shastr.str();
}

void DBHandler::getTestRunLog(json i_info_json) {
    if (DB_DEBUG) std::cout << "DBHandler: Get TestRun Log" << std::endl;

    this->checkEmpty(i_info_json["serialNumber"].empty(), "serialNumber", "");
    std::string serial_number = i_info_json["serialNumber"];
    document builder{};
    auto docs = builder << "serialNumber" << serial_number;
//TODO
    for (int i=0;i<(int)i_info_json["options"].size();i++) {
        this->checkEmpty(i_info_json["options"][i]["status"].empty(), "options."+std::to_string(i)+".status", "Set enabled/disabled.");
        if (i_info_json["options"][i]["status"]!="enabled") continue;
        std::string key = i_info_json["options"][i]["key"];
        auto value = i_info_json["options"][i]["value"].dump();
        docs << key << value;
    }
    auto doc_value = docs << finalize;
    mongocxx::options::find opts;
    opts.sort(make_document(kvp("$natural", -1)));
    //opts.limit(10);
    mongocxx::cursor cursor = db["testRun"].find(doc_value.view(), opts);
    for (auto doc : cursor) {
        std::string oid_str = doc["_id"].get_oid().value.to_string();
        std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(doc["startTime"].get_date().value);
        std::time_t starttime = s.count();
        char ts[80];
        struct tm *lt = std::localtime(&starttime);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", lt);
        std::string startTime = ts;

        std::string user_name = this->getValue("user", "_id", doc["user_id"].get_utf8().value.to_string(), "oid", "userName");
        std::string site_name = this->getValue("institution", "_id", doc["address"].get_utf8().value.to_string(), "oid", "institution");

        std::cout << "\033[1;33mtestRun data ID: " << oid_str << "\033[0m" << std::endl;
        std::cout << "User: " << user_name << " at " << site_name << std::endl;
        std::cout << "Date: " << startTime << std::endl;
        std::cout << "\tRun Number: " << std::to_string(doc["runNumber"].get_int32().value) << std::endl;
        std::cout << "\tTest Type:  " << doc["testType"].get_utf8().value.to_string() << std::endl;
        std::cout << "\t\033[1;33mctrlCfg:    " << doc["ctrlCfg"].get_utf8().value.to_string() << "\033[0m" << std::endl;
        std::cout << "\t\033[1;33mscanCfg:    " << doc["scanCfg"].get_utf8().value.to_string() << "\033[0m" << std::endl;

        doc_value = document{} <<
            "testRun" << oid_str <<
        finalize;
        mongocxx::cursor ctr_cursor = db["componentTestRun"].find(doc_value.view());
        for (auto ctr_doc : ctr_cursor) {
            std::string ch_oid_str = ctr_doc["component"].get_utf8().value.to_string();
            std::string ch_serial_number = this->getValue("component", "_id", ch_oid_str, "oid", "serialNumber");
            if (serial_number==ch_serial_number) continue;
            std::cout << "\tChip Serial Number: " << ch_serial_number << std::endl;
            std::cout << "\t\t\033[1;33mbeforeCfg: " << ctr_doc["beforeCfg"].get_utf8().value.to_string() << "\033[0m" << std::endl;
            std::cout << "\t\t\033[1;33mafterCfg:  " << ctr_doc["afterCfg"].get_utf8().value.to_string() << "\033[0m" << std::endl;
        }
        std::cout << std::endl;
    }
    return;
}

void DBHandler::getTestRunId(json i_info_json) {
    if (DB_DEBUG) std::cout << "DBHandler: Get TestRun Id" << std::endl;

    this->checkEmpty(i_info_json["serialNumber"].empty(), "serialNumber", "");
    std::string serial_number = i_info_json["serialNumber"];
    document builder{};
    auto docs = builder << "serialNumber" << serial_number;
    for (int i=0;i<(int)i_info_json["options"].size();i++) {
        this->checkEmpty(i_info_json["options"][i]["status"].empty(), "options."+std::to_string(i)+".status", "Set enabled/disabled.");
        if (i_info_json["options"][i]["status"]!="enabled") continue;
        std::string key = i_info_json["options"][i]["key"];
        auto value = i_info_json["options"][i]["value"].dump();
        std::cout << key << " " << value << std::endl;
        docs << key << value;
    }
    auto doc_value = docs << finalize;
    mongocxx::options::find opts;
    opts.sort(make_document(kvp("$natural", -1)));
    opts.limit(1);
    mongocxx::cursor cursor = db["testRun"].find(doc_value.view(), opts);
    for (auto doc : cursor) {
        std::string oid_str = doc["_id"].get_oid().value.to_string();
        std::cout << oid_str << std::endl;
        break;
    }
    return;
}

void DBHandler::getConfigData(json i_info_json) {
    if (DB_DEBUG) std::cout << "DBHandler: Get Config Data" << std::endl;

    std::string config_data_id;
    json data;
    std::vector<std::string> key_list = { "id", "serialNumber", "type", "filePath", "mergePath" };
    for (auto key : key_list) this->checkEmpty(i_info_json[key].empty(), key, "");
    if (i_info_json["id"]!="") {
        config_data_id = i_info_json["id"];
    } else {
        std::string serial_number = i_info_json["serialNumber"];
        std::string cmp_oid_str = this->getComponent(serial_number, "");
        if (cmp_oid_str=="") {
            std::string message = "Not found the component: "+serial_number;
            std::string function = __PRETTY_FUNCTION__;
            this->alert(function, message); return;
        }
        auto doc_value = make_document(kvp("component", cmp_oid_str));
        mongocxx::options::find opts;
        opts.sort(make_document(kvp("$natural", -1)));
        opts.limit(5);
        mongocxx::cursor cursor = db["componentTestRun"].find(doc_value.view(), opts);
        for (auto doc : cursor) {
            std::string config_type = "afterCfg";
            if (i_info_json["type"]!="") config_type = i_info_json["type"];
            if (config_type=="scanCfg"||config_type=="ctrlCfg") {
                std::string tr_oid_str = doc["testRun"].get_utf8().value.to_string();
                config_data_id = this->getValue("testRun", "_id", tr_oid_str, "oid", config_type);
            } else {
                config_data_id = doc[config_type].get_utf8().value.to_string();
            }
            if (config_data_id=="...") continue;
            else break;
        }
    }
    if (config_data_id=="...") return;
    std::string data_id = this->getValue("config", "_id", config_data_id, "oid", "data_id");
    bsoncxx::oid data_oid(data_id);
    mongocxx::gridfs::bucket gb = db.gridfs_bucket();
    std::ostringstream os;
    bsoncxx::types::value d_id{bsoncxx::types::b_oid{data_oid}};
    gb.download_to_stream(d_id, &os);
    std::string str = os.str();
    try {
        data = json::parse(str);
    } catch (json::parse_error &e) {
        std::string message = "Could not parse "+str+"\n\twhat(): "+e.what();
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
    if (i_info_json["mergePath"]!="") {
        std::string merge_file_path = i_info_json["mergePath"];
        json overwrite_json = toJson(merge_file_path);
        data.merge_patch(overwrite_json);
    }
    if (i_info_json["filePath"]!="") {
        std::string file_path = i_info_json["filePath"];
        std::ofstream file_ofs(file_path.c_str());
        file_ofs << std::setw(4) << data;
        file_ofs.close();
    }
    std::cout << std::setw(4) << data;
    
    return;
}

void DBHandler::getUserId(json i_info_json) {
    if (DB_DEBUG) std::cout << "DBHandler: Get User Id" << std::endl;

    document builder{};
    auto docs = builder << "userType" << "readWrite";
    std::vector<std::string> key_list = { "userName", "institution", "userIdentity" };
    for (auto key : key_list) {
        if (!i_info_json[key].empty()) {
            std::string value = i_info_json[key];
            docs << key << value;
        }
    }
    auto doc_value = docs << finalize;
    mongocxx::options::find opts;
    opts.limit(10);
    mongocxx::cursor cursor = db["user"].find(doc_value.view(), opts);
    for (auto doc : cursor) {
        std::string oid_str = doc["_id"].get_oid().value.to_string();

        std::cout << "\033[1;33muser data ID: " << oid_str << "\033[0m" << std::endl;
        std::cout << "\tUser Name:     " << doc["userName"].get_utf8().value.to_string() << std::endl; 
        std::cout << "\tInstitution:   " << doc["institution"].get_utf8().value.to_string() << std::endl; 
        std::cout << "\tUser Identity: " << doc["userIdentity"].get_utf8().value.to_string() << std::endl; 
        std::cout << std::endl;
    }
    return;
}

void DBHandler::getDatData(json i_info_json) {
    if (DB_DEBUG) std::cout << "DBHandler: Get Dat Data" << std::endl;

    // directory to save
    this->checkEmpty(i_info_json["dirPath"].empty(), "dirPath", "");
    std::string dir_path = i_info_json["dirPath"];
    this->mkdir(dir_path);
    std::string cmd = "rm "+dir_path+"/*";
    if (system(cmd.c_str()) < 0) {
        std::string message = "Problem creating "+dir_path;
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
    this->checkEmpty(i_info_json["testRun"].empty()&&i_info_json["serialNumber"].empty(), "testRun||serialNumber", "");
    std::string tr_oid_str;
    if (!i_info_json["serialNumber"].empty()) {
        std::string serial_number = i_info_json["serialNumber"];
        document builder{};
        auto docs = builder << "serialNumber" << serial_number;
        std::vector<std::string> key_list = { "testType", "runNumber", "address", "user_id" };
        for (auto key : key_list) {
            if (!i_info_json[key].empty()) {
                std::string value = i_info_json[key];
                docs = docs << key << value;
            }
        }
        auto doc_value = docs << finalize;
        mongocxx::options::find opts;
        opts.sort(make_document(kvp("$natural", -1)));
        opts.limit(1);
        mongocxx::cursor cursor = db["testRun"].find(doc_value.view(), opts);
        for (auto doc : cursor) {
            tr_oid_str = doc["_id"].get_oid().value.to_string();
            break;
        }
    } else if (!i_info_json["testRun"].empty()) {
        tr_oid_str = i_info_json["testRun"];
    }
    auto doc_value = document{} <<
        "testRun" << tr_oid_str <<
    finalize;
    mongocxx::options::find opts;
    opts.limit(10);
    mongocxx::cursor cursor = db["componentTestRun"].find(doc_value.view(), opts);
    for (auto doc : cursor) {
        std::string cmp_oid_str = doc["component"].get_utf8().value.to_string();
        std::string serial_number = this->getValue("component", "_id", cmp_oid_str, "oid", "serialNumber");
        for (auto attachment : doc["attachments"].get_array().value) {
            std::string file_path = dir_path + "/" + serial_number + "_" + attachment["filename"].get_utf8().value.to_string();
            std::ofstream file_ofs(file_path.c_str());
            bsoncxx::oid data_oid(attachment["code"].get_utf8().value.to_string());
            mongocxx::gridfs::bucket gb = db.gridfs_bucket();
            std::ostringstream os;
            bsoncxx::types::value d_id{bsoncxx::types::b_oid{data_oid}};
            gb.download_to_stream(d_id, &os);
            file_ofs << os.str();
        }
    }
    return;
}

#endif

void DBHandler::getTestRunData(std::string i_tr_oid_str, std::string i_serial_number, int i_time) {
    if (DB_DEBUG) std::cout << "DBHandler: Get TestRun Data" << std::endl;

#ifdef MONGOCXX_INCLUDE
    if (i_tr_oid_str==""&&(i_serial_number==""||i_time==-1)) {
        std::string message = "testRun Id or (serialNumber and startTime) is required to get testRun Data.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
    if (i_tr_oid_str=="") {
        std::time_t timestamp = i_time;
        auto startTime = std::chrono::system_clock::from_time_t(timestamp);
        auto doc_value = document{} <<
            "startTime"    << bsoncxx::types::b_date{startTime} <<
            "serialNumber" << i_serial_number <<
        finalize;
        auto result = db["testRun"].find_one(doc_value.view());
        if (result) m_tr_oid_str = result->view()["_id"].get_oid().value.to_string();
    } else {
        auto doc_value = make_document(kvp("_id", bsoncxx::oid(i_tr_oid_str)));
        auto result = db["testRun"].find_one(doc_value.view());
        if (result) m_tr_oid_str = result->view()["_id"].get_oid().value.to_string();
    }
#else
    std::cout << "Not found MongoDB C++ Driver!" << std::endl; //TODO check if data is exist in cache file
    m_tr_oid_str = "";
#endif
}

/////////////////
// Cache Function
void DBHandler::cacheUser(std::string i_user_path, std::string i_address_path) {
    // user config
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache user: " << i_user_path << std::endl;

    json user_json = this->toJson(i_user_path);
    std::ofstream cache_user_file(m_log_dir+"/user.json");
    cache_user_file << std::setw(4) << user_json;
    cache_user_file.close();

    // MAC address
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache address: " << i_address_path << std::endl;
    std::string cmd = "cp " + i_address_path + " " + m_log_dir + "/address.json";
    if (system(cmd.c_str()) < 0) {
        std::string message = "Problem copying " + i_address_path + " to cache folder.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
    // DBCfg
    this->cacheDBCfg();

    return;
}

void DBHandler::cacheConnCfg(std::string i_conn_path) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache connectivity config: " << i_conn_path << std::endl;
    json conn_json = toJson(i_conn_path);
    std::string mo_serial_number = conn_json["module"]["serialNumber"];
    std::string mod_list_path = m_cache_dir+"/lib/modules.bson";
    json mod_list_json = this->toJson(mod_list_path, "bson");
    if (mod_list_json[mo_serial_number].empty()) {
        std::cout << "#DB WARNING# Unregistered component:" << std::endl;
        std::cout << "\tModule serial number: " << mo_serial_number << std::endl;
        std::cout << "\tChips: " << std::endl;
        for (auto chip_json: conn_json["chips"]) {
            std::cout << "\t\tChip serial number: " << chip_json["serialNumber"] << std::endl;
            std::cout << "\t\tChip ID: " << chip_json["serialNumber"] << std::endl;
            std::cout << std::endl;
        }
        std::cout << "Make sure to register this Module? > [y/n]" << std::endl;
        char line[100];
        std::cin.getline(line, sizeof(line));
        std::string answer = line;
        if (answer!="y") {
            std::string message = "Not registered Module: "+mo_serial_number;
            std::string function = __PRETTY_FUNCTION__;
            this->alert(function, message); return;
        }
        mod_list_json[mo_serial_number] = conn_json;

        std::ofstream mod_list_file(mod_list_path, std::ios::binary);
        std::vector<std::uint8_t> v_bson = json::to_bson(mod_list_json);
        for (auto tmp : v_bson) {
            mod_list_file.write(reinterpret_cast<const char*>(std::addressof(tmp)), sizeof(std::uint8_t));
        }
        mod_list_file.close();
    }

    std::ofstream conn_file(m_log_dir+"/conn.json");
    conn_file << std::setw(4) << conn_json;
    conn_file.close();

    cacheConfig(mo_serial_number, m_log_dir+"/conn.json", "connectivity", "connCfg", "");

    return;
}

void DBHandler::cacheTestRun(std::string i_test_type, int i_run_number, int i_target_charge, int i_target_tot, int i_start_time, int i_finish_time, std::string i_command) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache Test Run." << std::endl;

    m_log_json["testType"] = i_test_type;
    m_log_json["runNumber"] = i_run_number;
    m_log_json["targetCharge"] = i_target_charge;
    m_log_json["targetTot"] = i_target_tot;
    if (i_start_time!=-1) m_log_json["startTime"] = i_start_time;
    if (i_finish_time!=-1) m_log_json["finishTime"] = i_finish_time;

    return;
}

void DBHandler::cacheConfig(std::string i_oid_str, std::string i_file_path, std::string i_filename, std::string i_title, std::string i_collection) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache Config Json: " << i_file_path << std::endl;

    counter++;

    std::ifstream file_ifs(i_file_path);
    std::string cmd = "cp "+i_file_path+" "+m_log_dir+"/"+std::to_string(counter)+".json";
    if (system(cmd.c_str()) < 0) {
        std::string message = "Problem copying "+i_file_path+" to cache folder.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
    json data_json;
    data_json["_id"]        = i_oid_str;
    data_json["path"]       = m_log_dir+"/"+std::to_string(counter)+".json"; 
    data_json["filename"]   = i_filename;
    data_json["title"]      = i_title;
    data_json["collection"] = i_collection;
    m_log_json["configs"][i_title].push_back(data_json);

    return;
}

void DBHandler::cacheAttachment(std::string i_oid_str, std::string i_file_path, std::string i_histo_name) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache Attachment: " << i_file_path << std::endl;

    std::string oid_str;
    std::string cmd = "cp "+i_file_path+" "+m_log_dir+"/"+std::to_string(counter)+".dat";
    if (system(cmd.c_str()) < 0) {
        std::string message = "Problem copying "+i_file_path+" to cache folder.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
    json data_json;
    data_json["_id"] = i_oid_str;
    data_json["path"] = m_log_dir+"/"+std::to_string(counter)+".dat"; 
    data_json["histoname"] = i_histo_name;
    m_log_json["attachments"].push_back(data_json);
    counter++;

    return;
}

void DBHandler::cacheDCSCfg(std::string i_dcs_path, std::string i_tr_path) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache DCS config: " << i_dcs_path << std::endl;

    json dcs_json = this->toJson(i_dcs_path);
    json env_json = dcs_json["environments"];
    for (int i=0; i<(int)env_json.size(); i++) {
        std::string num_str = std::to_string(i);
        if (env_json[i]["status"]!="enabled") continue;

        std::string j_key = env_json[i]["key"];
        if (!env_json[i]["path"].empty()) {
            std::string log_path = env_json[i]["path"];
            std::size_t suffix = log_path.find_last_of('.');
            std::string extension = log_path.substr(suffix + 1);

            std::string cmd = "cp "+log_path+" "+m_log_dir+"/"+std::to_string(i)+"."+extension;
            if (system(cmd.c_str()) < 0) {
                std::string message = "Problem copying "+log_path+" to cache folder.";
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message); return;
            }
            env_json[i]["path"] = m_log_dir+"/"+std::to_string(i)+"."+extension;
        }
    }
    dcs_json["environments"] = env_json;
    std::ofstream cache_dcs_file(m_log_dir+"/dcs.json");
    cache_dcs_file << std::setw(4) << dcs_json;
    cache_dcs_file.close();

    std::string cmd = "cp "+i_tr_path+" "+m_log_dir+"/tr.json";
    if (system(cmd.c_str()) < 0) {
        std::string message = "Problem copying "+i_tr_path+" to cache folder.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }

    // DBCfg
    this->cacheDBCfg();

    return;
}

void DBHandler::cacheDBCfg() {
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache Database Config: " << m_db_cfg_path << std::endl;
    // DB config
    std::string cmd = "cp " + m_db_cfg_path + " " + m_log_dir + "/database.json";
    if (system(cmd.c_str()) < 0) {
        std::string message = "Problem copying " + m_db_cfg_path + " to cache folder.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
}

void DBHandler::writeJson(std::string i_key, std::string i_value, std::string i_file_path, json i_json) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache log file: " << i_file_path << std::endl;

    i_json[i_key] = i_value;
    std::ofstream log_file(i_file_path);
    log_file << std::setw(4) << i_json;
    log_file.close();

    return;
}

/////////////////
// Check Function
void DBHandler::checkFile(std::string i_file_path, std::string i_description) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check file: " << i_file_path << std::endl;
    std::ifstream file_ifs(i_file_path);
    if (!file_ifs.is_open()) {
        std::string message = "Not found the file.\n\tfile: " + i_file_path;
        if (i_description!="") message += "    description: " + i_description;
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }
    file_ifs.close();
    return;
}

void DBHandler::checkEmpty(bool i_empty, std::string i_key, std::string i_file_path, std::string i_description) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check empty: " << i_key << " in " << i_file_path << std::endl;
    if (i_empty) {
        std::string message = "Found an empty field in json file.\n\tfile: " + i_file_path + "    key: " + i_key;
        if (i_description!="") message += "    description: " + i_description;
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }
    return;
}

void DBHandler::checkNumber(bool i_number, std::string i_key, std::string i_file_path) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check number: " << i_key << " in " << i_file_path << std::endl;
    if (!i_number) {
        std::string message = "This field value must be the number.\n\tfile: " + i_file_path + "    key: '" + i_key + "'";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }
    return;
}

void DBHandler::checkList(std::vector<std::string> i_list, std::string i_value, std::string i_list_path, std::string i_file_path) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check list: " << i_value << " in " << i_file_path << std::endl;
    if (std::find(i_list.begin(), i_list.end(), i_value)==i_list.end()) {
        std::string message = "Not registered '" + i_value + "' in " + i_list_path + "\n\tCheck the file: " + i_file_path + "\n\tList : ";
        for (unsigned i=0;i<i_list.size();i++) message += i_list[i] + " ";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }
    return;
}

json DBHandler::checkDBCfg(std::string i_db_path) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check database config: " << i_db_path << std::endl;

    json db_json = toJson(i_db_path);
    std::vector<std::string> key_list = { "hostIp", "hostPort", "cachePath", "dbName" };
    for (auto key : key_list) this->checkEmpty(db_json[key].empty(), key, "Set database config by ../localdb/setup.sh");

    std::string cache_dir = db_json["cachePath"];
    struct stat statbuf;
    if (stat(cache_dir.c_str(), &statbuf)!=0) {
        std::string message = "Not exist cache directory: "+cache_dir;
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }

    return db_json;
}

json DBHandler::checkConnCfg(std::string i_conn_path) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check connectivity config: " << i_conn_path << std::endl;
    json conn_json = this->toJson(i_conn_path);
    // chip type
    this->checkEmpty(conn_json["chipType"].empty(), "chipType", i_conn_path);
    // module
    this->checkEmpty(conn_json["module"].empty(), "module", i_conn_path);
    this->checkEmpty(conn_json["module"]["serialNumber"].empty(), "module.serialNumber", i_conn_path);
    this->checkEmpty(conn_json["module"]["componentType"].empty(), "module.componentType", i_conn_path);
    this->checkList(m_comp_list, std::string(conn_json["module"]["componentType"]), m_db_cfg_path, i_conn_path);
    // chips
    for (unsigned i=0; i<conn_json["chips"].size(); i++) {
        this->checkEmpty(conn_json["chips"][i]["serialNumber"].empty(), "chips."+std::to_string(i)+".serialNumber", i_conn_path);
        this->checkEmpty(conn_json["chips"][i]["componentType"].empty(), "chips."+std::to_string(i)+".componentType", i_conn_path);
        this->checkList(m_comp_list, std::string(conn_json["chips"][i]["componentType"]), m_db_cfg_path, i_conn_path);
        this->checkEmpty(conn_json["chips"][i]["chipId"].empty(), "chips."+std::to_string(i)+".chipId", i_conn_path);
    }
    // stage
    if (!conn_json["stage"].empty()) {
        std::string stage = conn_json["stage"];
        this->checkList(m_stage_list, stage, m_db_cfg_path, i_conn_path);
    }

    return conn_json;
}

void DBHandler::checkDCSCfg(std::string i_dcs_path, std::string i_num, json i_json) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check DCS config: " << i_dcs_path << std::endl;

    this->checkEmpty(i_json["key"].empty(), "environments."+i_num+".key", i_dcs_path, "Set the environmental key from the key list.");
    std::string j_key = i_json["key"];
    this->checkList(m_env_list, j_key, m_db_cfg_path, i_dcs_path);
    this->checkEmpty(i_json["path"].empty()&&i_json["value"].empty(), "environments."+i_num+".path/value", i_dcs_path);
    this->checkEmpty(i_json["description"].empty(), "environments."+i_num+".description", i_dcs_path);
    this->checkEmpty(i_json["num"].empty(), "environments."+i_num+".num", i_dcs_path);
    this->checkNumber(i_json["num"].is_number(), "environments."+i_num+".num", i_dcs_path);
    if (!i_json["margin"].empty()) this->checkNumber(i_json["margin"].is_number(), "environments."+i_num+".margin", i_dcs_path);

    return;
}

void DBHandler::checkDCSLog(std::string i_log_path, std::string i_dcs_path, std::string i_key, int i_num) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check DCS log file: " << i_log_path << std::endl;

    this->checkFile(i_log_path, "Check environmental data file of key '"+i_key+"' in file "+i_dcs_path+".");
    std::ifstream log_ifs(i_log_path);
    std::size_t suffix = i_log_path.find_last_of('.');
    std::string extension = i_log_path.substr(suffix + 1);
    std::string del;
    if (extension=="dat") del = " ";
    else if (extension=="csv") del = ",";
    else {
        std::string message = "Environmental data file must be 'dat' or 'csv' format: "+i_log_path;
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
    char separator = del[0];
    char tmp[1000];

    std::vector<std::string> log_lines = { "key", "num", "mode", "setting", "value" };
    std::vector<std::string> env_keys;
    int cnt=0;
    int key_cnt=0;
    int columns = 0;
    for (unsigned i=0; i<log_lines.size(); i++) {
        log_ifs.getline(tmp, 1000);
        cnt = 0;
        for (const auto s_tmp : split(tmp, separator)) {
            // check the first column
            if (i!=4&&columns==0&&s_tmp!=log_lines[i]) {
                std::string message = "Set "+log_lines[i]+" in the "+std::to_string(i+1)+ "th line: "+i_log_path;
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message); return;
            }

            // check the values for each line
            if (i==0) {
                env_keys.push_back(s_tmp);
                columns++;
            } else if (i==1&&env_keys[cnt]==i_key) {
                try {
                    if (stoi(s_tmp)==i_num) key_cnt=cnt;
                } catch (const std::invalid_argument& e) {
                    std::string message = "Could not convert the key number text to int: "+i_log_path+"\n\tkey: "+i_key+"\n\ttext: "+s_tmp;
                    std::string function = __PRETTY_FUNCTION__;
                    this->alert(function, message); return;
                }
            } else if (i==4&&cnt==1) {
                try {
                    stoi(s_tmp);
                } catch (const std::invalid_argument& e) {
                    std::string message = "Could not convert the unixtime text to int: "+i_log_path+"\n\ttext: "+s_tmp;
                    std::string function = __PRETTY_FUNCTION__;
                    this->alert(function, message); return;
                }
            } else if ((i==3||i==4)&&(cnt==key_cnt)) {
                try {
                    stof(s_tmp);
                } catch (const std::invalid_argument& e) {
                    std::string message = "Could not convert the setting value text to float: "+i_log_path+"\n\tkey: "+i_key+"\n\ttext: "+s_tmp;
                    std::string function = __PRETTY_FUNCTION__;
                    this->alert(function, message); return;
                }
            }
            cnt++;
        }
        if (i==0) {
            this->checkList(env_keys, i_key, i_log_path, i_dcs_path);
        } else {
            if (cnt!=columns) {
                std::string message = "Not match the number of the "+log_lines[i]+" "+std::to_string(cnt)+" to keys "+std::to_string(columns);
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message); return;
            }
            if (i==1&&key_cnt==0) {
                std::string message = "Environmental key '"+i_key+"' (num: "+std::to_string(i_num)+") was not written in environmental data file: "+i_log_path;
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message); return;
            }
        }
    }
    return;
}

json DBHandler::checkUserCfg(std::string i_user_path) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check user config file: " << i_user_path << std::endl;

    json user_json = this->toJson(i_user_path);
    this->checkEmpty(user_json["userName"].empty(), "userName", i_user_path);
    this->checkEmpty(user_json["institution"].empty(), "institution", i_user_path);
    this->checkEmpty(user_json["userIdentity"].empty(), "userIdentity", i_user_path);
    return user_json;
}

json DBHandler::checkAddressCfg(std::string i_address_path) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check address config file: " << i_address_path << std::endl;

    json address_json = this->toJson(i_address_path);
    this->checkEmpty(address_json["macAddress"].empty(), "macAddress", i_address_path);
    this->checkEmpty(address_json["hostname"].empty(), "hostname", i_address_path);
    this->checkEmpty(address_json["institution"].empty(), "institution", i_address_path);
    return address_json;
}


//////////
// Others
json DBHandler::toJson(std::string i_file_path, std::string i_file_type) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Convert to json code from: " << i_file_path << std::endl;

    std::ifstream file_ifs(i_file_path);
    json file_json;
    if (file_ifs) {
        if (i_file_type=="json") {
            try {
                file_json = json::parse(file_ifs);
            } catch (json::parse_error &e) {
                std::string message = "Could not parse " + i_file_path + "\n\twhat(): " + e.what();
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message);
            }
        } else if (i_file_type=="bson") {
            std::uint8_t tmp;
            std::vector<std::uint8_t> v_bson;
            while (file_ifs.read(reinterpret_cast<char*>(std::addressof(tmp)), sizeof(std::uint8_t))) v_bson.push_back(tmp);
            try {
                file_json = json::from_bson(v_bson);
            } catch (json::parse_error &e) {
                std::string message = "Could not parse " + i_file_path + "\n\twhat(): " + e.what();
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message);
            }
        }
    }
    return file_json;
}

std::vector<std::string> DBHandler::split(std::string str, char del) {
    std::size_t first = 0;
    std::size_t last = str.find_first_of(del);
 
    std::vector<std::string> result;
 
    while (first < str.size()) {
        std::string subStr(str, first, last - first);
 
        result.push_back(subStr);
 
        first = last + 1;
        last = str.find_first_of(del, first);
 
        if (last == std::string::npos) {
            last = str.size();
        }
    }
 
    return result;
}

void DBHandler::mkdir(std::string i_dir_path) {
    std::string cmd = "mkdir -p "+i_dir_path;
    if (system(cmd.c_str()) < 0) {
        std::string message = "Problem creating "+i_dir_path;
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
}
