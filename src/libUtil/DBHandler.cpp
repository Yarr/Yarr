// #################################
// # Author: Eunchong Kim, Arisa Kubota
// # Email: eunchong.kim at cern.ch, arisa.kubota at cern.ch
// # Date : April 2019
// # Project: Local DBHandler for Yarr
// # Description: DBHandler functions
// ################################

#include "DBHandler.h"

DBHandler::DBHandler():
m_option(""), m_db_cfg_path(""), m_user_oid_str(""), m_site_oid_str(""),
m_chip_type(""), m_log_dir(""), m_log_path(""), m_cache_path(""), m_cache_dir(""), m_tr_oid_str(""),
m_stage_list(), m_env_list(), m_comp_list(),
m_histo_names(), m_tr_oid_strs(), m_serial_numbers(),
m_db_version(1.0), DB_DEBUG(false), m_log_json(), m_cache_json(), m_conn_json(), counter(0)
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

    if (i_option=="null") {
        std::string message = "Set option in initialize";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }

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

    // initialize for registeration
    if (m_option=="scan") {
        std::string cmd = "rm "+m_cache_dir+"/tmp/localdb/*.json";
        if (system(cmd.c_str()) < 0) {
            std::string message = "Problem removing json files in "+m_cache_dir+"/tmp/localdb";
            std::string function = __PRETTY_FUNCTION__;
            this->alert(function, message); return;
        }
    }
    if (m_option=="scan"||m_option=="dcs") m_log_dir = m_cache_dir+"/tmp/localdb/cache/"+std::to_string(now);
    else m_log_dir = m_cache_dir+"/tmp/localdb/log";//TODO

    m_log_path = m_log_dir + "/cacheLog.json";
    this->mkdir(m_log_dir);
    m_log_json["version"] = m_db_version;
    m_log_json["logPath"] = m_log_dir;
    m_log_json["dbOption"] = m_option;
    this->writeJson("status", "running", m_log_path, m_log_json);
    this->cacheDBCfg(); 
}

void DBHandler::alert(std::string i_function, std::string i_message, std::string i_type) {
    if (DB_DEBUG) std::cout << "DBHandler: Alert '" << i_type << "'" << std::endl;

    std::string alert_message;
    if (i_type=="error") {
        alert_message = "#DB ERROR#";
        m_log_json["errormessage"] = i_message;
        this->writeJson("status", "failure", m_log_path, m_log_json);
    } else if (i_type=="warning") {
        alert_message = "#DB WARNING#";
    } else if (i_type=="status") {
        alert_message = "#DB STATUS#";
    }
    std::cerr << alert_message << " " << i_message << std::endl;
    std::time_t now = std::time(NULL);
    struct tm *lt = std::localtime(&now);
    char tmp[20];
    strftime(tmp, 20, "%Y%m%d", lt);
    std::string timestamp=tmp;
    std::string log_path = m_cache_dir+"/var/log/localdb/"+timestamp+"_error.log";
    std::ofstream file_ofs(log_path, std::ios::app);
    strftime(tmp, 20, "%F_%H:%M:%S", lt);
    timestamp=tmp;
    file_ofs << timestamp << " " << alert_message << " [" << m_option << "] " << i_function << std::endl;
    file_ofs << "Message: " << i_message << std::endl;
    file_ofs << "Log: " << m_log_path << std::endl;
    file_ofs << "Cache: " << m_cache_path << std::endl;
    file_ofs << "--------------------" << std::endl;

    if (i_type=="error") std::abort();

    return;
}

// public
void DBHandler::setUser(std::string i_user_path) {
    // Set UserID from DB
    if (DB_DEBUG) std::cout << "DBHandler: Set user: " << i_user_path << std::endl;
    std::string user_name, institution;
    if (i_user_path=="") {
        user_name = getenv("USER");
        institution = getenv("HOSTNAME");
    } else {
        json user_json = this->checkUserCfg(i_user_path);
        user_name = user_json["userName"];
        institution = user_json["institution"];
    }
    std::cout << "DBHandler: User Information \n\tUser name: " << user_name << "\n\tInstitution: " << institution << std::endl;
    //this->cacheUser(i_user_path);

    return;
}

void DBHandler::setSite(std::string i_address_path) {
    // Set MAC address
    if (DB_DEBUG) std::cout << "DBHandler: Set site: " << i_address_path << std::endl;
    std::string address;
    if (i_address_path=="") {
        std::string hostname = getenv("HOSTNAME");
        std::string user = getenv("USER");
        address = hostname+"_"+user;
    } else {
        json address_json = this->checkAddressCfg(i_address_path);
        address = address_json["macAddress"];
    }
    std::cout << "DBHandler: MAC Address: " << address << std::endl;
    //this->cacheSite(i_address_path);

    return;
}

void DBHandler::setConnCfg(std::vector<std::string> i_conn_paths) {
    if (DB_DEBUG) std::cout << "DBHandler: Set connectivity config" << std::endl;

    for (auto conn_path : i_conn_paths) {
        if (DB_DEBUG) std::cout << "\tDBHandler: setting connectivity config file: " << conn_path << std::endl;
        json conn_json = this->checkConnCfg(conn_path);
        m_chip_type = conn_json["chipType"];
        if (m_chip_type == "FEI4B") m_chip_type = "FE-I4B";
    }
    this->cacheConnCfg(i_conn_paths);
}

void DBHandler::setDCSCfg(std::string i_dcs_path, std::string i_tr_path) {
    if (DB_DEBUG) std::cout << "DBHandler: Set DCS config: " << i_dcs_path << std::endl;

    json tr_json = this->toJson(i_tr_path);
    if (tr_json["id"].empty()) {
        this->checkEmpty(tr_json["startTime"].empty(), "testRun.startTime", i_tr_path);
        this->checkEmpty(tr_json["serialNumber"].empty(), "testRun.serialNumber", i_tr_path);
    }
    std::string tr_oid_str = "";
    std::string serial_number = "";
    int timestamp = -1;
    if (!tr_json["id"].empty()) tr_oid_str = tr_json["id"];
    if (!tr_json["serialNumber"].empty()) serial_number = tr_json["serialNumber"];
    if (!tr_json["startTime"].empty()) timestamp = tr_json["startTime"];
    this->getTestRunData(tr_oid_str, serial_number, timestamp);
 
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
            std::string log_path = "";
            log_path = env_json[i]["path"];
            this->checkDCSLog(log_path, i_dcs_path, env_json[i]["key"], j_num); 
        } else {
            this->checkNumber(env_json[i]["value"].is_number(), "environments."+num_str+".value", i_dcs_path);
        }
    }
    this->cacheDCSCfg(i_dcs_path, i_tr_path);

    return;
}

void DBHandler::setTestRunStart(std::string i_test_type, std::vector<std::string> i_conn_paths, int i_run_number, int i_target_charge, int i_target_tot, int i_timestamp, std::string i_command) {
    if (DB_DEBUG) std::cout << "DBHandler: Write Test Run (start)" << std::endl;

    for (int i=0; i<(int)i_conn_paths.size(); i++) {
        std::string conn_path = i_conn_paths[i];
        json conn_json = this->toJson(conn_path);
        std::string mo_serial_number;
        std::string chip_type = conn_json["chipType"];
        if (conn_json["module"]["serialNumber"].empty()) mo_serial_number = "DUMMY_" + std::to_string(i);
        else mo_serial_number = conn_json["module"]["serialNumber"];
        if (m_option=="scan") {
            std::time_t now = std::time(NULL);
            struct tm *lt = std::localtime(&now);
            char tmp[20];
            strftime(tmp, 20, "%Y%m", lt);
            std::string ts=tmp;
            std::string log_path = m_cache_dir+"/var/lib/localdb/"+ts+"_scan.csv";
            std::ofstream log_file_ofs(log_path, std::ios::app);
            strftime(tmp, 20, "%F_%H:%M:%S", lt);
            ts=tmp;
            log_file_ofs << ts << "," << mo_serial_number << "," << i_timestamp << "," << i_run_number << "," << i_test_type << std::endl;
            log_file_ofs.close();

            json tr_json;
            tr_json["startTime"] = i_timestamp;
            tr_json["serialNumber"] = mo_serial_number;
            tr_json["runNumber"] = i_run_number;
            tr_json["testType"] = i_test_type;

            std::string tmp_path = m_cache_dir+"/tmp/localdb/"+mo_serial_number+"_scan.json";
            std::ofstream tmp_file_ofs(tmp_path);
            tmp_file_ofs << std::setw(4) << tr_json;
            tmp_file_ofs.close();
        }
    }
    this->cacheTestRun(i_test_type, i_run_number, i_target_charge, i_target_tot, i_timestamp, -1, i_command);

    return;
}

void DBHandler::setTestRunFinish(std::string i_test_type, std::vector<std::string> i_conn_paths, int i_run_number, int i_target_charge, int i_target_tot, int i_timestamp, std::string i_command) {
    if (DB_DEBUG) std::cout << "DBHandler: Write Test Run (finish)" << std::endl;

    std::sort(m_histo_names.begin(), m_histo_names.end());
    m_histo_names.erase(std::unique(m_histo_names.begin(), m_histo_names.end()), m_histo_names.end());
    this->cacheTestRun(i_test_type, i_run_number, i_target_charge, i_target_tot, -1, i_timestamp, i_command);
    
    return;
}

void DBHandler::setConfig(int i_tx_channel, int i_rx_channel, std::string i_file_path, std::string i_filename, std::string i_title, std::string i_collection, std::string i_serial_number) {
    if (DB_DEBUG) std::cout << "DBHandler: Write Config Json: " << i_file_path << std::endl;

    std::ifstream file_ifs(i_file_path);
    if (!file_ifs) return;
    file_ifs.close();

    std::string serial_number;
    if (i_serial_number!="") {
        serial_number = i_serial_number;
    } else {
        for (auto conn_json : m_conn_json["connectivity"]) {
            for (auto chip_json : conn_json["chips"]) {
                if (chip_json["tx"]==i_tx_channel&&chip_json["rx"]==i_rx_channel) serial_number = chip_json["serialNumber"];
            }
        }
    }
    this->cacheConfig(serial_number, i_file_path, i_filename, i_title, i_collection);

    return;
}

//void DBHandler::setAttachment(std::string i_serial_number, std::string i_file_path, std::string i_histo_name) {
void DBHandler::setAttachment(int i_tx_channel, int i_rx_channel, std::string i_file_path, std::string i_histo_name, std::string i_serial_number) {
    if (DB_DEBUG) std::cout << "DBHandler: Write Attachment: " << i_file_path << std::endl;

    std::ifstream file_ifs(i_file_path);
    if (!file_ifs) return;
    file_ifs.close();

    std::string serial_number;
    if (i_serial_number!="") {
        serial_number = i_serial_number;
    } else {
        for (auto conn_json : m_conn_json["connectivity"]) {
            for (auto chip_json : conn_json["chips"]) {
                if (chip_json["tx"]==i_tx_channel&&chip_json["rx"]==i_rx_channel) serial_number = chip_json["serialNumber"];
            }
        }
    }
    this->cacheAttachment(serial_number, i_file_path, i_histo_name);

    m_histo_names.push_back(i_histo_name);

    return;
}

void DBHandler::cleanUp(std::string i_dir) {
    if (m_option=="scan") {
        int now = std::time(NULL);
        std::string cmd = "mv "+m_log_dir+" "+m_cache_dir+"/var/cache/localdb/"+std::to_string(now);
        if (system(cmd.c_str()) < 0) {
            std::string message = "Problem moving directory "+m_log_dir+" to "+m_cache_dir+"/var/cache/localdb/"+std::to_string(now);
            std::string function = __PRETTY_FUNCTION__;
            this->alert(function, message); return;
        }
        m_log_dir = m_cache_dir+"/var/cache/localdb/"+std::to_string(now);
        m_log_json["logPath"] = m_log_dir;
        m_log_path = m_log_dir + "/cacheLog.json";
        this->writeJson("status", "waiting", m_log_path, m_log_json);
    } else if (m_option=="dcs") {
        int now = std::time(NULL);
        std::string cmd = "mv "+m_log_dir+" "+m_cache_dir+"/var/cache/localdb/"+std::to_string(now);
        if (system(cmd.c_str()) < 0) {
            std::string message = "Problem moving directory "+m_log_dir+" to "+m_cache_dir+"/var/cache/localdb/"+std::to_string(now);
            std::string function = __PRETTY_FUNCTION__;
            this->alert(function, message); return;
        }
        m_log_dir = m_cache_dir+"/var/cache/localdb/"+std::to_string(now);
        m_log_json["logPath"] = m_log_dir;
        m_log_path = m_log_dir + "/cacheLog.json";
        this->writeJson("status", "waiting", m_log_path, m_log_json);
    }
    // initialize
    m_option = "";
    m_db_cfg_path = "";
    m_user_oid_str = "";
    m_site_oid_str = "";
    m_chip_type = "";
    m_log_dir = "";
    m_log_path = "";
    m_cache_path = "";
    m_cache_dir = "";
    m_tr_oid_str = "";
    m_stage_list.clear();
    m_env_list.clear();
    m_comp_list.clear();
    m_histo_names.clear();
    m_tr_oid_strs.clear();
    m_serial_numbers.clear();
    m_log_json = NULL;
    m_cache_json = NULL;
    m_conn_json = NULL;
    counter = 0;
}

//*****************************************************************************************************
// Protected fuctions
//
void DBHandler::getTestRunData(std::string i_tr_oid_str, std::string i_serial_number, int i_time) {
    if (DB_DEBUG) std::cout << "DBHandler: Get TestRun Data" << std::endl;

    m_tr_oid_str = "";

    if (i_tr_oid_str==""&&(i_serial_number==""||i_time==-1)) {
        std::string message = "testRun Id or (serialNumber and startTime) is required to get testRun Data.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message); return;
    }
}

/////////////////
// Cache Function
void DBHandler::cacheUser(std::string i_user_path) {
    // user config
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache user: " << i_user_path << std::endl;

    std::string user_name, institution, user_identity;
    json user_json;
    if (i_user_path=="") {
        user_name = getenv("USER");
        institution = getenv("HOSTNAME");
        user_json["userName"] = user_name;
        user_json["institution"] = institution;
        user_json["userIdentity"] = "default";
    } else {
        user_json = this->toJson(i_user_path);
    }
    std::ofstream cache_user_file(m_log_dir+"/user.json");
    cache_user_file << std::setw(4) << user_json;
    cache_user_file.close();

    return;
}

void DBHandler::cacheSite(std::string i_address_path) {
    // MAC address
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache address: " << i_address_path << std::endl;
    std::string address, hostname, site;
    json site_json;
    if (i_address_path=="") {
        hostname = getenv("HOSTNAME");
        site_json["macAddress"] = hostname;
        site_json["hostname"] = hostname;
        site_json["institution"] = "null";
    } else {
        site_json = this->toJson(i_address_path);
    }

    std::ofstream cache_site_file(m_log_dir+"/address.json");
    cache_site_file << std::setw(4) << site_json;
    cache_site_file.close();

    return;
}
void DBHandler::cacheConnCfg(std::vector<std::string> i_conn_paths) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache connectivity config" << std::endl;
    std::string mo_serial_number;
    for (int i=0; i<(int)i_conn_paths.size(); i++) {
        std::string conn_path = i_conn_paths[i];
        if (DB_DEBUG) std::cout << "\tDBHandler: Cache connectivity config file: " << conn_path << std::endl;

        json conn_json = toJson(conn_path);
        std::string chip_type = conn_json["chipType"];
        if (conn_json["module"]["serialNumber"].empty()) {
            mo_serial_number = "DUMMY_" + std::to_string(i);
            conn_json["module"]["serialNumber"] = mo_serial_number;
            if (conn_json["module"]["componentType"].empty()) conn_json["module"]["componentType"] = "Module"; //TODO for module only currently
            for (unsigned j=0; j<conn_json["chips"].size(); j++) {
                if (conn_json["chips"][j]["serialNumber"].empty()) conn_json["chips"][j]["serialNumber"] = "DUMMY_chip_" + std::to_string(j);
                if (conn_json["chips"][j]["componentType"].empty()) conn_json["chips"][j]["componentType"] = "Front-end Chip";
                if (conn_json["chips"][j]["geomId"].empty()) conn_json["chips"][j]["geomId"] = j+1;
                if (conn_json["chips"][j]["chipId"].empty()) conn_json["chips"][j]["chipId"] = j+1;
            }
            conn_json["dummy"] = true;
        } else {
            mo_serial_number = conn_json["module"]["serialNumber"];
            conn_json["dummy"] = false;
            char tmp[1000];
            std::string del = ",";
            char separator = del[0];
            std::string mod_list_path = m_cache_dir+"/var/lib/localdb/modules.csv";
            std::ifstream list_ifs(mod_list_path);
            while (list_ifs.getline(tmp, 1000)) {
                if (split(tmp, separator).size()==0) continue;
                if (split(tmp, separator)[0] == mo_serial_number) break; 
            }
            for (unsigned j=0; j<conn_json["chips"].size(); j++) {
                int chip_id = conn_json["chips"][j]["chipId"];
                bool is_chip = false;
                std::string ch_serial_number = "";
                for (const auto s_tmp : split(tmp, separator)) {
                    if (is_chip) {
                        ch_serial_number = s_tmp;
                        break;
                    }
                    if (s_tmp==std::to_string(chip_id)) is_chip=true;
                }
                if (conn_json["chips"][j]["serialNumber"].empty()) conn_json["chips"][j]["serialNumber"] = ch_serial_number;
                if (conn_json["chips"][j]["geomId"].empty()) conn_json["chips"][j]["geomId"] = j+1;
            }
        }
        std::ofstream conn_file(m_cache_dir+"/tmp/localdb/conn.json");
        conn_file << std::setw(4) << conn_json;
        conn_file.close();
    
        cacheConfig(mo_serial_number, m_cache_dir+"/tmp/localdb/conn.json", "connectivity", "connCfg", "");
        m_conn_json["connectivity"].push_back(conn_json);
    }

    return;
}

void DBHandler::cacheTestRun(std::string i_test_type, int i_run_number, int i_target_charge, int i_target_tot, int i_start_time, int i_finish_time, std::string i_command) {
    if (DB_DEBUG) std::cout << "\tDBHandler: Cache Test Run." << std::endl;

    m_log_json["testType"] = i_test_type;
    m_log_json["runNumber"] = i_run_number;
    m_log_json["targetCharge"] = i_target_charge;
    m_log_json["targetTot"] = i_target_tot;
    m_log_json["command"] = i_command;
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
    data_json["path"]       = std::to_string(counter)+".json"; 
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
    data_json["path"] = std::to_string(counter)+".dat"; 
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
            std::string log_path = "";
            log_path = env_json[i]["path"];
            std::size_t suffix = log_path.find_last_of('.');
            std::string extension = log_path.substr(suffix + 1);

            std::string cmd = "cp "+log_path+" "+m_log_dir+"/"+std::to_string(i)+"."+extension;
            if (system(cmd.c_str()) < 0) {
                std::string message = "Problem copying "+log_path+" to cache folder.";
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message); return;
            }
            env_json[i]["path"] = std::to_string(i)+"."+extension;
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
    for (auto key : key_list) this->checkEmpty(db_json[key].empty(), key, "Set database config by ../localdb/setup_db.sh");

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
    if (!conn_json["module"].empty()) {
        this->checkEmpty(conn_json["module"]["serialNumber"].empty(), "module.serialNumber", i_conn_path); 
        bool is_listed = false;
        char tmp[1000];
        std::string del = ",";
        char separator = del[0];
        std::string mo_serial_number = conn_json["module"]["serialNumber"];
        std::string mod_list_path = m_cache_dir+"/var/lib/localdb/modules.csv";
        std::ifstream list_ifs(mod_list_path);
        if (!list_ifs) {
            std::string message = "Not found modules list: "+mod_list_path;
            std::string function = __PRETTY_FUNCTION__;
            this->alert(function, message);
        }
        while (list_ifs.getline(tmp, 1000)) {
            if (split(tmp, separator).size()==0) continue;
            if (split(tmp, separator)[0] == mo_serial_number) {
                is_listed = true;
                break; 
            }
        }
        if (!is_listed) {
            std::string message = "This module "+mo_serial_number+" is not registered: "+i_conn_path+"\nTry ./bin/dbAccessor -M for pulling component data in local from Local DB";
            std::string function = __PRETTY_FUNCTION__;
            this->alert(function, message);
        }
        
        // chips
        for (unsigned i=0; i<conn_json["chips"].size(); i++) {
            this->checkEmpty(conn_json["chips"][i]["chipId"].empty(), "chips."+std::to_string(i)+".chipId", i_conn_path);
            int chip_id = conn_json["chips"][i]["chipId"];
            is_listed = false;
            for (const auto s_tmp : split(tmp, separator)) {
                if (s_tmp==std::to_string(chip_id)) {
                    is_listed = true;
                    break;
                }
            }
            if (!is_listed) {
                std::string message = "This chip (chipId: "+std::to_string(chip_id)+") is not registered: "+i_conn_path;
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message);
            }
        }
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
