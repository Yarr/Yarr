// #################################
// # Author: Eunchong Kim, Arisa Kubota
// # Email: eunchong.kim at cern.ch, arisa.kubota at cern.ch
// # Date : April 2019
// # Project: Local DBHandler for Yarr
// # Description: DBHandler functions
// ################################

#include "DBHandler.h"

DBHandler::DBHandler():
m_db_cfg_path(""), m_user_oid_str(""), m_site_oid_str(""),
m_tr_oid_str(""), m_output_dir(""),
m_stage_list(), m_env_list(), m_comp_list(),
m_tr_oid_strs(), m_serial_numbers(),
m_db_version(1.0), DB_DEBUG(false), m_conn_json(), counter(0)
{
    if (DB_DEBUG) std::cout << "DBHandler: DBHandler" << std::endl;

    this->mkdir("/tmp/localdb");
}

DBHandler::~DBHandler() {
    if (DB_DEBUG) std::cout << "DBHandler: Exit DBHandler" << std::endl;
}
//*****************************************************************************************************
// Public functions
//
void DBHandler::initialize(std::string i_db_cfg_path) {
    if (DB_DEBUG) std::cout << "DBHandler: Initializing." << std::endl;

    m_db_cfg_path = i_db_cfg_path;
    json db_json = toJson(m_db_cfg_path);

    if (m_stage_list.size()==0&&m_env_list.size()==0&&m_comp_list.size()==0) {
        for(auto s_tmp: db_json["stage"]) m_stage_list.push_back(s_tmp);
        for(auto s_tmp: db_json["environment"]) m_env_list.push_back(s_tmp);
        for(auto s_tmp: db_json["component"]) m_comp_list.push_back(s_tmp);
    }
}

void DBHandler::alert(std::string i_function, std::string i_message, std::string i_type) {
    if (DB_DEBUG) std::cout << "DBHandler: Alert '" << i_type << "'" << std::endl;

    std::string alert_message;
    if (i_type=="error") alert_message = "#DB ERROR#";
    else if (i_type=="warning") alert_message = "#DB WARNING#";
    else if (i_type=="status") alert_message = "#DB STATUS#";

    std::cerr << alert_message << " " << i_message << std::endl;
    std::time_t now = std::time(NULL);
    struct tm *lt = std::localtime(&now);
    char tmp[20];
    strftime(tmp, 20, "%Y%m%d", lt);
    std::string timestamp=tmp;
    std::string log_path = "/tmp/localdb/"+timestamp+"_error.log";
    std::ofstream file_ofs(log_path, std::ios::app);
    strftime(tmp, 20, "%F_%H:%M:%S", lt);
    timestamp=tmp;
    file_ofs << timestamp << " " << alert_message << " " << i_function << std::endl;
    file_ofs << "Message: " << i_message << std::endl;
    file_ofs << "--------------------" << std::endl;

    if (i_type=="error") std::abort();

    return;
}

void DBHandler::setUser(std::string i_user_path) {
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
    std::cout << "\tUser name: " << user_name << "\n\tInstitution: " << institution << std::endl;

    return;
}

void DBHandler::setSite(std::string i_address_path) {
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
    std::cout << "\tMAC Address: " << address << std::endl;

    return;
}

void DBHandler::setConnCfg(std::vector<std::string> i_conn_paths) {
    if (DB_DEBUG) std::cout << "DBHandler: Set connectivity config" << std::endl;

    for (auto conn_path : i_conn_paths) {
        if (DB_DEBUG) std::cout << "\tDBHandler: setting connectivity config file: " << conn_path << std::endl;
        json conn_json = this->checkConnCfg(conn_path);
    }
}

void DBHandler::setDCSCfg(std::string i_dcs_path, std::string i_scanlog_path, std::string i_serial_number) {
    if (DB_DEBUG) std::cout << "DBHandler: Set DCS config: " << i_dcs_path << std::endl;

    json log_json = this->toJson(i_scanlog_path);
    if (log_json["id"].empty()) {
        this->checkEmpty(log_json["startTime"].empty()&&log_json["timestamp"].empty(), "testRun.startTime||testRun.timestamp", i_scanlog_path);
    }

    char path[1000];
    std::string current_dir = getcwd(path, sizeof(path));
    std::size_t prefixPos = i_scanlog_path.find_first_of('/');
    std::string log_path;
    if (prefixPos!=0) {
        if (i_scanlog_path.substr(0,1)=="~") {
            std::string home = getenv("HOME");
            log_path = home+i_scanlog_path.substr(1);
        } else if (i_scanlog_path.substr(0,1)==".") {
            log_path = current_dir+i_scanlog_path;
        } else {
            log_path = current_dir+"/"+i_scanlog_path;
        }
    } else {
        log_path = i_scanlog_path;
    }
    std::size_t pathPos = log_path.find_last_of('/');
    log_path = log_path.substr(0, pathPos);
    char buf[100];
    if (readlink(log_path.c_str(), buf, sizeof(buf))>=0) {
        pathPos = log_path.find_last_of('/');
        log_path = log_path.substr(0, pathPos)+"/"+buf;
    }
    m_output_dir = log_path;
    log_path = log_path+"/dbDcsLog.json";

    json dcs_log_json;

    std::string serial_number = "";
    int timestamp_int = -1;
    std::string timestamp_str = "";
    dcs_log_json["serialNumber"] = i_serial_number;
    if (!log_json["id"].empty()) dcs_log_json["id"] = log_json["id"];
    if (!log_json["startTime"].empty()) dcs_log_json["startTime"] = log_json["startTime"];
    if (!log_json["timestamp"].empty()) dcs_log_json["timestamp"] = log_json["timestamp"];
 
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
            std::string env_log_path = "";
            env_log_path = env_json[i]["path"];
            std::string extension = this->checkDCSLog(env_log_path, i_dcs_path, env_json[i]["key"], j_num); 
            std::string file_path = m_output_dir+"/"+std::string(env_json[i]["key"])+"_"+std::to_string(j_num)+"."+extension;
            std::string cmd = "cp "+env_log_path+" "+file_path;
            env_json[i]["path"] = file_path;
            if (system(cmd.c_str()) < 0) {
                std::string message = "Cannot copy the DCS data log file.";
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message);
            }
        } else {
            this->checkNumber(env_json[i]["value"].is_number(), "environments."+num_str+".value", i_dcs_path);
        }
    }

    dcs_log_json["environments"] = env_json;

    json db_json = toJson(m_db_cfg_path);

    dcs_log_json["dbCfg"] = db_json;

    std::ofstream log_file(log_path);
    log_file << std::setw(4) << dcs_log_json;
    log_file.close();

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
        std::time_t now = std::time(NULL);
        struct tm *lt = std::localtime(&now);
        char tmp[20];
        strftime(tmp, 20, "%Y%m", lt);
        std::string ts=tmp;
        std::string log_path = "/tmp/localdb/"+ts+"_scan.csv";
        std::ofstream log_file_ofs(log_path, std::ios::app);
        strftime(tmp, 20, "%F_%H:%M:%S", lt);
        ts=tmp;
        log_file_ofs << ts << "," << mo_serial_number << "," << i_timestamp << "," << i_run_number << "," << i_test_type << std::endl;
        log_file_ofs.close();
    }

    return;
}

void DBHandler::cleanUp(std::string i_option, std::string i_dir) {

    std::string result_dir;
    if (i_dir=="") {
        result_dir = m_output_dir;
    } else {
        char path[1000];
        std::string current_dir = getcwd(path, sizeof(path));
        if (i_dir.substr(0,1)==".") {
            result_dir = current_dir + i_dir.substr(1);
        } else {
            result_dir = i_dir;
        }
    }

    if (result_dir=="") {
        std::string message = "Not provided output directory.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }
    if (result_dir.back() != '/')
        result_dir += "/";

    std::string home = getenv("HOME");
    std::string log_path;
    if (i_option=="scan") log_path = home+"/.yarr/run.dat";
    else if (i_option=="dcs") log_path = home+"/.yarr/dcs.dat";
    else {
        std::string message = "Unsupported option.";
        std::string function = __PRETTY_FUNCTION__;
        this->alert(function, message);
    }

    std::fstream db_file(log_path, std::ios::out|std::ios::app);

    std::string cmd = "localdbtool-upload test 2> /dev/null";
    if (system(cmd.c_str())!=0) {
        std::cerr << "#DB ERROR# Cannot upload result data into Local DB" << std::endl;
        std::cerr << "           Not found Local DB command: 'localdbtool-upload'" << std::endl;
        std::cerr << "           Try 'YARR/localdb/setup_db.sh' to set Local DB command and 'localdbtool-upload cache' to upload data." << std::endl;
        db_file << result_dir << std::endl;
    } else {
        cmd = "localdbtool-upload init --database "+m_db_cfg_path;
        if (system(cmd.c_str())==0) {
            if (i_option=="scan") cmd = "localdbtool-upload scan "+result_dir+" --log &";
            else if (i_option=="dcs") cmd = "localdbtool-upload dcs "+result_dir+" --log &";
            system(cmd.c_str());
            std::cout << "#DB INFO# Uploading in the back ground. (log: ~/.yarr/localdb/log/)" << std::endl;
        } else {
            std::cerr << "           Try 'localdbtool-upload cache' to upload data in the good connection to Local DB Server." << std::endl;
            db_file << result_dir << std::endl;
        }
    }
    db_file.close();

    // initialize
    m_db_cfg_path = "";
    m_user_oid_str = "";
    m_site_oid_str = "";
    m_tr_oid_str = "";
    m_stage_list.clear();
    m_env_list.clear();
    m_comp_list.clear();
    m_tr_oid_strs.clear();
    m_serial_numbers.clear();
    m_conn_json = NULL;
    counter = 0;
}

//*****************************************************************************************************
// Protected fuctions
//
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
    this->checkEmpty(db_json["hostIp"].empty(), "hostIp", i_db_path);
    this->checkEmpty(db_json["hostPort"].empty(), "hostPort", i_db_path);
    this->checkEmpty(db_json["dbName"].empty(), "dbName", i_db_path);

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
        std::string home = getenv("HOME");
        std::string mo_serial_number = conn_json["module"]["serialNumber"];
        std::string mod_list_path = home+"/.yarr/localdb/modules.csv";
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
            this->checkEmpty(conn_json["chips"][i]["serialNumber"].empty(), "chips."+std::to_string(i)+".serialNumber", i_conn_path);
            std::string ch_serial_number = conn_json["chips"][i]["serialNumber"];
            is_listed = false;
            for (const auto s_tmp : split(tmp, separator)) {
                if (s_tmp==ch_serial_number) {
                    is_listed = true;
                    break;
                }
            }
            if (!is_listed) {
                std::string message = "This chip ("+ch_serial_number+") is not registered: "+i_conn_path;
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

std::string DBHandler::checkDCSLog(std::string i_log_path, std::string i_dcs_path, std::string i_key, int i_num) {
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
        this->alert(function, message); return "ERROR";
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
                this->alert(function, message); return "ERROR";
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
                    this->alert(function, message); return "ERROR";
                }
            } else if (i==4&&cnt==1) {
                try {
                    stoi(s_tmp);
                } catch (const std::invalid_argument& e) {
                    std::string message = "Could not convert the unixtime text to int: "+i_log_path+"\n\ttext: "+s_tmp;
                    std::string function = __PRETTY_FUNCTION__;
                    this->alert(function, message); return "ERROR";
                }
            } else if ((i==3||i==4)&&(cnt==key_cnt)) {
                try {
                    stof(s_tmp);
                } catch (const std::invalid_argument& e) {
                    std::string message = "Could not convert the setting value text to float: "+i_log_path+"\n\tkey: "+i_key+"\n\ttext: "+s_tmp;
                    std::string function = __PRETTY_FUNCTION__;
                    this->alert(function, message); return "ERROR";
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
                this->alert(function, message); return "ERROR";
            }
            if (i==1&&key_cnt==0) {
                std::string message = "Environmental key '"+i_key+"' (num: "+std::to_string(i_num)+") was not written in environmental data file: "+i_log_path;
                std::string function = __PRETTY_FUNCTION__;
                this->alert(function, message); return "ERROR";
            }
        }
    }
    return extension;
}

json DBHandler::checkUserCfg(std::string i_user_path) {
    if (DB_DEBUG) std::cout << "\t\tDBHandler: Check user config file: " << i_user_path << std::endl;

    json user_json = this->toJson(i_user_path);
    this->checkEmpty(user_json["userName"].empty(), "userName", i_user_path);
    this->checkEmpty(user_json["institution"].empty(), "institution", i_user_path);
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

    json file_json;

    std::ifstream file_ifs(i_file_path);

    std::size_t pathPos = i_file_path.find_last_of('.');
    if (pathPos==std::string::npos||i_file_path.substr(pathPos+1)!="json") {
        return file_json;
    }

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
