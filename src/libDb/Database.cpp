#include "Database.h"

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

Database::Database(std::string i_host_ip) {
    DB_DEBUG = false;
    if (DB_DEBUG) std::cout << "Database: Initialize" << std::endl;

    mongocxx::instance inst{};
    client = mongocxx::client{mongocxx::uri{i_host_ip}};

    db = client["yarrdb"]; // Database name is 'yarrdb'

    std::string home = getenv("HOME"); 
    m_home_dir = home  + "/.yarr/";
    m_db_version = 2;
    m_start_time = "";
}

Database::~Database() {
    if (DB_DEBUG) std::cout << "Database: Exit Database" << std::endl;
}

//*****************************************************************************************************
// Public functions
//
void Database::setConnCfg(std::vector<std::string> i_connCfgPath) {
    if (DB_DEBUG) std::cout << "Database: Connectivity Cfg: " << std::endl;
    for (auto sTmp : i_connCfgPath) {
        if (DB_DEBUG) std::cout << "\t" << sTmp << std::endl;
        std::ifstream gConfig(sTmp);
        json config = json::parse(gConfig);

        // Get module serial #
        if (config["module"]["serialNumber"].empty()) {
            if (config["chips"][0]["serialNumber"].empty()) {
                std::cerr <<"#DB ERROR# No serialNumber in Connectivity! " << sTmp << std::endl;
                abort(); return;
            }
        }

        // Get chip type
        std::string chipType = config["chipType"];
        if (chipType == "FEI4B") {
            chipType = "FE-I4B";
        }
        m_chip_type = chipType;

        // Register module
        // If already registered, it will not register again
        this->registerFromConnectivity(sTmp);
    }
}

//TODO make it to upload environmental information automatically
void Database::setTestRunInfo(std::string i_tr_info_cfg_path) {
    if (DB_DEBUG) std::cout << "Database: Test Run Environment path: " << i_tr_info_cfg_path << std::endl;
    if (i_tr_info_cfg_path != "") {
        // environment 
        std::ifstream info_cfg_ifs(i_tr_info_cfg_path);
        if (info_cfg_ifs) {
            this->addTestRunInfo(i_tr_info_cfg_path);
        }
    }
}

//TODO make login tools to set user information
void Database::setUserInstitution() {
    if (getenv("DBUSER") == NULL) {
        std::cerr << "#DB ERROR# Not logged in Database, abort..." << std::endl;
        std::cerr << "\tLogin by source db_login.sh <USER ACCOUNT>" << std::endl;
        std::abort();
    }

    std::string user = getenv("DBUSER");
    std::string userName, userIdentity, institution;

    std::string user_path = m_home_dir + user + "_user.json";
    if (DB_DEBUG) std::cout << "Database: Set user: " << user_path << std::endl;

    std::fstream user_fs(user_path.c_str(), std::ios::in);
    if (!user_fs) {
        std::cerr << "#DB ERROR# Failed to load the user config, abort..." << std::endl;
        std::cerr << "\tCheck the user config: " << user_path  << std::endl;
        std::abort();
    }
    json user_json = json::parse(user_fs);
    userName = user_json["userName"];
    institution = user_json["institution"];
    userIdentity = user_json["userIdentity"];
    user_fs.close();

    std::cout << "Database: User Information \n\tUser name: " << userName << "\n\tInstitution: " << institution << "\n\tUser identity: " << userIdentity << std::endl;

    mongocxx::collection collection = db["user"];
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(
        document{} << "userName"     << userName << 
                      "institution"  << institution << 
                      "userIdentity" << userIdentity << finalize
    );
    if (!maybe_result) {
        std::cerr << "#DB ERROR# This user was not registerd, abort..." << std::endl;
        std::cerr << "\tCheck the user config: " << user_path  << std::endl;
        std::abort();
    }

    bsoncxx::document::element element = maybe_result->view()["_id"];
    m_user_oid_str = element.get_oid().value.to_string();

    std::string address_path = m_home_dir + "address";
    if (DB_DEBUG) std::cout << "Database: Set address: " << address_path << std::endl;

    std::fstream address_fs(address_path.c_str(), std::ios::in);
    if (!address_fs) {
        std::cerr << "#DB ERROR# Failed to load the address config, abort..." << std::endl;
        std::cerr << "\tCheck the address config: " << address_path  << std::endl;
        std::abort();
    }

    std::string addressInstitution;
    address_fs >> addressInstitution;
    address_fs.close();

    std::cout << "Database: MAC Address: " << addressInstitution << std::endl;

    m_address = addressInstitution;
}

void Database::setHistoName(std::string i_histo_name) {
    if (DB_DEBUG) std::cout << "Database: Histo Name: " << i_histo_name << std::endl;
    m_histo_name.push_back(i_histo_name);
}

void Database::startTestRun(std::string i_ctrl_path, std::string i_scan_path, std::string i_test_type, int i_run_number, int i_target_charge, int i_target_tot) {
    if (DB_DEBUG) std::cout << "Database: Write Test Run (start): " << i_run_number << std::endl;

    mongocxx::collection collection = db["testRun"];
    bsoncxx::document::value doc_value = document{} << "testType"  << i_test_type <<
                                                       "runNumber" << i_run_number <<
                                                       "address"      << m_address <<
                                                       "user_id"   << m_user_oid_str << finalize; 
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(doc_value.view());
    if (maybe_result) {
        std::cerr <<"#DB ERROR# Conflict to existing data, abort..." << std::endl;
        std::abort();
    }
    std::string test_run_oid_str = this->registerTestRun(i_test_type, i_run_number, i_target_charge, i_target_tot);
    this->addUserInstitution("testRun", test_run_oid_str);

    // write scan config
    std::ifstream scanCfgFile(i_scan_path);
    if (scanCfgFile) {
        if (DB_DEBUG) std::cout << "\tWrite scan config file: " << i_scan_path << std::endl;
        json scanCfg;
        scanCfg = json::parse(scanCfgFile);
        std::string scan_oid_str = this->writeJsonCode(scanCfg, i_test_type+".json", "scanCfg");
        this->addDocument(test_run_oid_str, "testRun", "scanCfg", scan_oid_str);
    }

    // write controller config
    std::ifstream ctrlCfgFile(i_ctrl_path);
    if (ctrlCfgFile) {
        if (DB_DEBUG) std::cout << "\tWrite controller config file: " << i_ctrl_path << std::endl;
        json ctrlCfg;
        ctrlCfg = json::parse(ctrlCfgFile);
        std::string ctrl_oid_str = this->writeJsonCode(ctrlCfg, "controller.json", "ctrlCfg");
        this->addDocument(test_run_oid_str, "testRun", "ctrlCfg", ctrl_oid_str);
    }

    m_tr_oid_str = test_run_oid_str;
}

std::string Database::writeComponentTestRun(std::string i_serial_number, int i_chip_tx, int i_chip_rx) {
    // write component-testrun documents
    int run_number = atoi(this->getValue("testRun", "_id", m_tr_oid_str, "oid", "runNumber", "int").c_str());
    std::string test_type = this->getValue("testRun", "_id", m_tr_oid_str, "oid", "testType");
    if (DB_DEBUG) std::cout << "Database: Write Component Test Run: " << run_number << std::endl;

    std::string child_oid_str = this->getValue("component", "serialNumber", i_serial_number, "", "_id", "oid");
    std::string ctr_oid_str = this->registerComponentTestRun(child_oid_str, m_tr_oid_str, test_type, run_number, i_chip_tx, i_chip_rx);

    return ctr_oid_str;
}

void Database::writeConfig(std::string i_ctr_oid_str, json &i_config, std::string i_title) {
    if (DB_DEBUG) std::cout << "Database: Write Config Json." << std::endl;

    std::string cfg_oid_str = this->writeJsonCode(i_config, i_title+".json", "chipCfg");
    this->addDocument(i_ctr_oid_str, "componentTestRun", i_title, cfg_oid_str);
}

void Database::writeTestRun(std::string i_ctrl_path, std::string i_scan_path, std::string i_test_type, int i_run_number, std::string i_output_dir, int i_target_charge=-1, int i_target_tot=-1) {
    if (DB_DEBUG) std::cout << "Database: Write Test Run (finish): " << i_run_number << std::endl;

    mongocxx::collection collection = db["testRun"];
    bsoncxx::document::value doc_value = document{} << "testType"   << i_test_type <<
                                                       "runNumber"  << i_run_number <<
                                                       "address"       << m_address <<
                                                       "user_id" << m_user_oid_str << finalize; 
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(doc_value.view());
    std::string test_run_oid_str;
    if (!maybe_result) {
        this->startTestRun(i_ctrl_path, i_scan_path, i_test_type, i_run_number, i_target_charge, i_target_tot);
        maybe_result = collection.find_one(doc_value.view());
    }
    test_run_oid_str = maybe_result->view()["_id"].get_oid().value.to_string();

    document builder{};
    auto docs = builder << "finishTime" << bsoncxx::types::b_date{std::chrono::system_clock::now()};
    auto in_array = docs << "plots" << open_array;
    std::sort(m_histo_name.begin(), m_histo_name.end());
    m_histo_name.erase(std::unique(m_histo_name.begin(), m_histo_name.end()), m_histo_name.end());
    for (auto sTmp: m_histo_name) {
        in_array = in_array << sTmp;
    }
    auto after_array = in_array << close_array;
    doc_value = after_array << finalize;

    db["testRun"].update_one(
        document{} << "_id" << bsoncxx::oid(test_run_oid_str) << finalize,
        document{} << "$set" << doc_value.view() << finalize
    );
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
        bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = tr_collection.find_one(document{} << "_id" << bsoncxx::oid(tr_oid_str) << finalize);

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
                bsoncxx::stdx::optional<bsoncxx::document::value> fc_result = fc_collection.find_one(document{} << "files_id" << bsoncxx::oid(code) << finalize);

                if (!fc_result) {
                    std::cout << "Not Found!" << std::endl;
                    std::ostringstream os;
                    os << std::setfill('0') << std::setw(6) << runNumber;
                    std::string outputDir = ("./data/" + os.str() + "_" + testType + "/");
                    std::cout << outputDir << filename << "." << fileextension << std::endl;
                    std::string fs_str = writeGridFsFile(outputDir+filename+"."+fileextension, filename+"."+fileextension);
                    //this->addAttachment(i_test_run_oid_str, i_collection_name, oid_str, "title", "describe", fileextension, filename);

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

std::string Database::uploadFromJson(std::string i_collection_name, std::string i_json_path) {
    if (DB_DEBUG) std::cout << "Database: Upload from json: " << i_json_path << std::endl;
    std::ifstream json_ifs(i_json_path);
    if (!json_ifs) {
        std::cerr <<"#DB ERROR# Cannot open register json file: " << i_json_path << std::endl;
        abort(); return "ERROR";
    }
    json json = json::parse(json_ifs);
    bsoncxx::document::value doc_value = bsoncxx::from_json(json.dump()); 
    mongocxx::collection collection = db[i_collection_name];
    auto result = collection.insert_one(doc_value.view());
    bsoncxx::oid oid = result->inserted_id().get_oid().value;
    return oid.to_string();
}

void Database::registerUserInstitution(std::string i_user_name, std::string i_institution, std::string i_user_identity, std::string i_address="") {
    if (DB_DEBUG) std::cout << "Database: Register user \n\tUser name: " << i_user_name << "\n\tInstitution: " << i_institution << "\n\tUser identity: " << i_user_identity << std::endl;
    if (getenv("DBUSER") == NULL) {
        std::cerr << "#DB ERROR# Not logged in Database, abort..." << std::endl;
        std::cerr << "\tLogin by ./bin/dbRegister -U: <USER ACCOUNT>" << std::endl;
        std::abort();
    }
 
    mongocxx::collection collection = db["user"];
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(document{} << "userName" << i_user_name << "institution" << i_institution << "userIdentity" << i_user_identity << finalize);

    std::string user = getenv("DBUSER");
    std::string user_path = m_home_dir + user + "_user.json";

    json user_json;
    // Add to scan log
    user_json["userName"] = i_user_name;
    user_json["institution"] = i_institution;
    user_json["userIdentity"] = i_user_identity;

    std::fstream user_fs(user_path.c_str(), std::ios::out);
    user_fs << std::setw(4) << user_json;
    user_fs.close();

    if (i_address != "") {
        std::string address_path = m_home_dir + "address";
        std::fstream address_fs(address_path.c_str(), std::ios::out);
        address_fs << i_address << std::endl;
        address_fs.close();
    }

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
 
    auto result = collection.insert_one(doc_value.view());
    bsoncxx::oid oid = result->inserted_id().get_oid().value;
    this->addSys(oid.to_string(), "user");
    this->addVersion("user", "_id", oid.to_string(), "oid");
}

void Database::registerFromConnectivity(std::string i_conn_path) {
    if (DB_DEBUG) std::cout << "Database: Register from connectivity: " << i_conn_path << std::endl;

    std::ifstream conn_ifs(i_conn_path);
    if (!conn_ifs) {
        std::cerr << "#DB ERROR# Cannot open register json file: " << i_conn_path << std::endl;
        abort(); return;
    }
    json conn_json = json::parse(conn_ifs);

    // Register module component
    // If there is no module SN in conn. cgf, use chip 0 SN instead
    std::string mo_serialNumber = "";
    if (conn_json["module"].empty() || conn_json["module"]["serialNumber"].empty()) {
        std::cout << "\tDatabase: no module info in connectivity! use chip 0 SN!" << std::endl;
        if (!conn_json["chips"][0]["serialNumber"].empty()) {
            mo_serialNumber = conn_json["chips"][0]["serialNumber"];
            mo_serialNumber += "_module";
        } else {
            std::cerr <<"#DB ERROR# No serialNumber in Connectivity! " << i_conn_path << std::endl;
            abort(); return;
        }
    } else {
        mo_serialNumber = conn_json["module"]["serialNumber"];
    }

    std::string chipType = conn_json["chipType"];
    if (chipType == "FEI4B") chipType = "FE-I4B";
    m_chip_type = chipType;
    mongocxx::collection cmp_collection = db["component"];
    mongocxx::collection cpr_collection = db["childParentRelation"];

    // component (module)
    {
        std::string componentType  = "";
        if (!conn_json["module"]["componentType"].empty()) {
            componentType = conn_json["module"]["componentType"];
        } else {
            componentType = "Module";
        }
        bsoncxx::document::value query_doc = document{} <<  
            "serialNumber"  << mo_serialNumber <<
            "componentType" << componentType <<
            "chipType"      << m_chip_type <<
        finalize;
        bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = cmp_collection.find_one( query_doc.view() );

        if (!maybe_result) {
            if (DB_DEBUG) std::cout << "\tRegister Module SN: " << mo_serialNumber << std::endl;
            bsoncxx::document::value doc_value = document{} <<  
                "sys" << open_document << close_document <<
                "serialNumber"  << mo_serialNumber <<
                "chipType"      << m_chip_type <<
                "componentType" << componentType <<
            finalize;
        
            auto result = cmp_collection.insert_one(doc_value.view());
            bsoncxx::oid oid = result->inserted_id().get_oid().value;
            this->addSys(oid.to_string(), "component");
            this->addUserInstitution("component", oid.to_string());
            this->addVersion("component", "_id", oid.to_string(), "oid");
        }
    }

    // Register chip component
    for (unsigned i=0; i<conn_json["chips"].size(); i++) {
        std::string serialNumber  = conn_json["chips"][i]["serialNumber"];
        std::string componentType = conn_json["chips"][i]["componentType"];
        bsoncxx::document::value query_doc = document{} <<  
            "serialNumber"  << serialNumber << 
            "componentType" << componentType <<
            "chipType"      << m_chip_type << 
        finalize;
        bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = cmp_collection.find_one( query_doc.view() );
        if (maybe_result) continue;

        std::ifstream chip_cfg_ifs(conn_json["chips"][i]["config"].get<std::string>());
        if (!chip_cfg_ifs) {
            std::cerr << "#DB ERROR# Cannot open register chip config file" << std::endl;
            abort(); return;
        }
        json chip_cfg_json = json::parse(chip_cfg_ifs);
        std::string chip_name = "Moomin";
        int chip_id = 0;
        if (m_chip_type == "FE-I4B") {
            chip_name = chip_cfg_json[chipType]["name"];
            chip_id   = chip_cfg_json[chipType]["Parameter"]["chipId"];
        } else if (m_chip_type == "RD53A") {
            chip_name = chip_cfg_json[chipType]["Parameter"]["Name"];
            chip_id   = chip_cfg_json[chipType]["Parameter"]["ChipId"];
        }

        // component (chip)
        {
            if (DB_DEBUG) std::cout << "\tRegister Chip SN: " << serialNumber << std::endl;
            bsoncxx::document::value insert_doc = document{} <<  
                "sys"           << open_document << close_document <<
                "serialNumber"  << serialNumber <<
                "componentType" << componentType <<
                "chipType"      << m_chip_type <<
                "name"          << chip_name <<
                "chipId"        << chip_id <<
            finalize;
     
            auto result = cmp_collection.insert_one(insert_doc.view());
            bsoncxx::oid oid = result->inserted_id().get_oid().value;
            this->addSys(oid.to_string(), "component");
            this->addUserInstitution("component", oid.to_string());
            this->addVersion("component", "_id", oid.to_string(), "oid");
        }

        // CP relation
        {
            bsoncxx::document::value doc_value = document{} <<  
                "sys"       << open_document << close_document <<
                "parent"    << getValue("component", "serialNumber", mo_serialNumber, "","_id", "oid") <<
                "child"     << getValue("component", "serialNumber", serialNumber, "", "_id", "oid") <<
            finalize;

            auto result = cpr_collection.insert_one(doc_value.view());
            bsoncxx::oid oid = result->inserted_id().get_oid().value;
            this->addSys(oid.to_string(), "childParentRelation");
            this->addVersion("childParentRelation", "_id", oid.to_string(), "oid");
        }
    }
}

void Database::registerEnvironment(std::string i_env_key, std::string i_description) {
    if (DB_DEBUG) std::cout << "Database: Register Environment: " << i_env_key << std::endl;

    mongocxx::collection collection = db["environment"];
    if (i_description != "") {
        if (i_env_key == "") {
            std::cerr <<"#DB ERROR# Cannot load environmental config key: " << i_description << std::endl;
            return;
        }
        bsoncxx::document::value doc_value = document{} <<  
            "key"         << i_env_key <<
            "type"        << "description" <<
            "description" << i_description <<
        finalize;
        bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(doc_value.view());
        if (!maybe_result) {
            if (DB_DEBUG) std::cout << "\tDatabase: Register Environment Description: " << i_description << std::endl;
            auto result = collection.insert_one(doc_value.view());
            bsoncxx::oid oid = result->inserted_id().get_oid().value;
            this->addSys(oid.to_string(), "environment");
            this->addVersion("environment", "_id", oid.to_string(), "oid");
        }
    } else {
        std::string env_cfg_path;
        if (i_env_key != "") env_cfg_path = i_env_key;
        else env_cfg_path = "/tmp/" + std::string(getenv("USER")) + "env.dat";
        std::ifstream env_cfg_ifs(env_cfg_path);
        if (!env_cfg_ifs) {
            std::cerr <<"#DB ERROR# Cannot open environmental config file: " << env_cfg_path << std::endl;
            return;
        }
        
        // get start time from scan data
        bsoncxx::oid i_oid(m_tr_oid_str);
        mongocxx::collection tr_collection = db["testRun"];
        bsoncxx::stdx::optional<bsoncxx::document::value> run_result = tr_collection.find_one(document{} << "_id" << i_oid << finalize);
        if (!run_result) return;
        bsoncxx::document::element element = run_result->view()["startTime"];
        std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(element.get_date().value);
        std::time_t starttime = s.count();
        if (DB_DEBUG) {
            char buf[80];
            struct tm *lt = std::localtime(&starttime);
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
            std::cout << "\tDatabase: Register Environment Start Time: " << buf << std::endl;
        }

        std::string line, tmp;
        std::stringstream key_line;
        std::getline(env_cfg_ifs, line);
        key_line << line;
        std::vector<std::string> env_key;
        std::vector<std::string> description;
        int key_num = 0;
        while (key_line>>tmp) {
            env_key.push_back(tmp);
            key_num++;
            if (tmp != "time") {
                bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(document{} << "key" << tmp << "type" << "description" << finalize);
                if (!maybe_result) {
                    std::cerr << "#DB ERROR# This environmental key was not registerd, abort..." << std::endl;
                    std::cerr << "\tCheck the environmental config: " << env_cfg_path << std::endl;
                    return;
                }
                bsoncxx::document::element element = maybe_result->view()["description"];
                description.push_back(element.get_utf8().value.to_string());
            } else {
                description.push_back("time");
            }
        }

        int data_num = 0;
        std::vector<std::string> key_value[key_num];
        std::vector<std::string> date;
        while (!env_cfg_ifs.eof()) {
            std::string datetime = "";
            std::string tmp[key_num];
            for (int i=0;i<key_num;i++) {
                env_cfg_ifs>>tmp[i];
                if (tmp[i] == "") break;
                if (env_key[i] == "time") datetime = tmp[i];
            }
            if (datetime == "") break; 
            struct tm timepoint;
            memset(&timepoint, 0X00, sizeof(struct tm));
            strptime(datetime.c_str(), "%Y-%m-%dT%H:%M:%S", &timepoint);

            char buffer[80];
            strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",&timepoint);

            std::time_t timestamp  = mktime(&timepoint);
            if (difftime(starttime,timestamp)<60) { // store data from 1 minute before the starting time of the scan
                for (int i=0;i<key_num;i++) {
                    key_value[i].push_back(tmp[i]);
                    if (env_key[i] == "time") date.push_back(tmp[i]);
                }
                data_num++;
            }
        }
        document builder{};
        auto in_array = builder << "data" << open_array;
        for (int i=0;i<key_num;i++) {
            if (env_key[i] == "time") continue;
            document doc_builder{};
            auto array_builder = bsoncxx::builder::basic::array{};
            for (int j=0;j<data_num;j++) {
                struct tm timepoint;
                memset(&timepoint, 0X00, sizeof(struct tm));
                strptime(date[j].c_str(), "%Y-%m-%dT%H:%M:%S", &timepoint);
                std::time_t timestamp  = mktime(&timepoint);
                array_builder.append(
                    document{} << 
                        "date"        << bsoncxx::types::b_date{std::chrono::system_clock::from_time_t(timestamp)} <<
                        "value"       << std::stof(key_value[i][j]) <<
                    finalize
                ); 
            }
            auto in_doc = doc_builder <<
                "key"         << env_key[i] <<
                "description" << description[i] <<
                "value"       << open_array << 
                    array_builder << 
                close_array << 
            finalize;
            in_array = in_array << in_doc;
        }
        auto after_array = in_array << close_array;
        bsoncxx::document::value doc_value = after_array << finalize;

        auto result = collection.insert_one(doc_value.view());
        bsoncxx::oid oid = result->inserted_id().get_oid().value;
        this->addSys(oid.to_string(), "environment");
        this->addVersion("environment", "_id", oid.to_string(), "oid");

        this->addDocument(m_tr_oid_str, "testRun", "environment", oid.to_string());
   }
}

void Database::writeAttachment(std::string i_ctr_oid_str, std::string i_file_path, std::string i_histo_name) {
    if (DB_DEBUG) std::cout << "\tDatabase: Write Attachment: " << i_file_path << std::endl;

    std::string fileextension;
    std::string oid_str;

    {
        fileextension = "dat";
        std::string file_path = i_file_path + "." + fileextension;
        std::fstream file_fs(file_path.c_str(), std::ios::in);
        if (file_fs) {
            oid_str = this->writeDatFile(file_path, i_histo_name + ".dat");
            this->addAttachment(i_ctr_oid_str, "componentTestRun", oid_str, i_histo_name, "describe", fileextension, i_histo_name+".dat");
        }
        file_fs.close();
    }
    {
        fileextension = "png";
        std::string file_path = i_file_path + "." + fileextension;
        std::fstream file_fs(file_path.c_str(), std::ios::in);
        if (file_fs) {
            oid_str = this->writeGridFsFile(file_path, i_histo_name + ".png");
            this->addAttachment(i_ctr_oid_str, "componentTestRun", oid_str, i_histo_name, "describe", fileextension, i_histo_name+".png");
        }
        file_fs.close();
    }
    {
        fileextension = "pdf";
        std::string file_path = i_file_path + "." + fileextension;
        std::fstream file_fs(file_path.c_str(), std::ios::in);
        if (file_fs) {
            oid_str = this->writeGridFsFile(file_path, i_histo_name + ".pdf");
            this->addAttachment(i_ctr_oid_str, "componentTestRun", oid_str, i_histo_name, "describe", fileextension, i_histo_name+".pdf");
        }
        file_fs.close();
    }
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
    bsoncxx::stdx::optional<bsoncxx::document::value> result = collection.find_one(query.view());
    if(result) {
        if (i_bson_type == "oid") {
            bsoncxx::document::element element = result->view()["_id"];
            return element.get_oid().value.to_string();
        }
        else if (i_bson_type == "sys_datetime") {
            bsoncxx::document::element element = result->view()["sys"]["cts"];
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
            return element.get_utf8().value.to_string();
        }
    }
    else {
        std::cerr <<"#DB ERROR# Cannot find '" << i_key << "' from member '" << i_member_key << ": " << i_member_value << "' in collection name: '" << i_collection_name << "'" << std::endl;
        abort(); return "ERROR";
    }
}

std::string Database::registerComponentTestRun(std::string i_component_oid_str, std::string i_test_run_oid_str, std::string i_test_type, int i_run_number, int i_chip_tx, int i_chip_rx) {
    if (DB_DEBUG) std::cout << "\tDatabase: Register Com-Test Run" << std::endl;
    bsoncxx::document::value doc_value = document{} <<  
        "sys"       << open_document << close_document <<
        "component" << i_component_oid_str << // id of component
        "state"     << "..." << // code of test run state
        "testType"  << i_test_type << // id of test type
        "testRun"   << i_test_run_oid_str << // id of test run, test is planned if it is null
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
    std::time_t now = std::chrono::system_clock::to_time_t(startTime);
    struct tm *lt = std::localtime(&now);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
    m_start_time = buf;

    document builder{};
    auto docs = builder << "sys"          << open_document << close_document <<
                           "testType"     << i_test_type << // id of test type //TODO make it id
                           "runNumber"    << i_run_number << // number of test run
                           "startTime"    << bsoncxx::types::b_date{startTime} << // date when the test run was taken
                           "passed"       << true << // flag if test passed
                           "problems"     << true << // flag if any problem occured
                           "state"        << "ready" << // state of component ["ready", "requestedToTrash", "trashed"]
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

void Database::addDocument(std::string i_oid_str, std::string i_collection_name, std::string i_key, std::string i_value) {
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
void Database::writeFromDirectory(std::string i_collection_name, std::string i_oid_str, std::string outputDir, std::string i_filter) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload from directory" << std::endl;
    // Get file path from directory
    std::array<char, 128> buffer;
    std::stringstream temp_strstream;
    temp_strstream.str(""); temp_strstream << "ls -v " << outputDir << "/*";
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

void Database::addTestRunInfo(std::string i_tr_info_cfg_path) {
    if (DB_DEBUG) std::cout << "\tDatabase: Add test run info.: " << i_tr_info_cfg_path << std::endl;
    bsoncxx::oid i_oid(m_tr_oid_str);

    std::ifstream tr_info_cfg_ifs(i_tr_info_cfg_path);
    json tr_info_j = json::parse(tr_info_cfg_ifs);

    // For assembly
    if (!tr_info_j["assembly"].empty()) {
        if (!tr_info_j["assembly"]["stage"].empty()) {
            db["testRun"].update_one(
                document{} << "_id" << i_oid << finalize,
                document{} << "$set" << open_document <<
                    "stage" << tr_info_j["assembly"]["stage"].get<std::string>() << // "wirebond" or "encapsulation" or "module"
                close_document << finalize
            );
        }
    }
}

void Database::addUserInstitution(std::string i_collection_name, std::string i_oid_str) {
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
    std::fstream file_fs(i_file_path, std::ios::in);

    document builder{};

    std::string type;
    std::string name;
    std::string xaxistitle, yaxistitle, zaxistitle;
    int xbins, ybins, zbins;
    double xlow, ylow, zlow;
    double xhigh, yhigh, zhigh;
    int underflow, overflow;

    std::getline(file_fs, type);
    std::getline(file_fs, name);
    std::getline(file_fs, xaxistitle);
    std::getline(file_fs, yaxistitle);
    std::getline(file_fs, zaxistitle);
    file_fs >> xbins >> xlow >> xhigh;
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
        file_fs >> ybins >> ylow >> yhigh;
        docs = docs << "ybins" << ybins
                    << "ylow"  << bsoncxx::types::b_double{ylow}
                    << "yhigh" << bsoncxx::types::b_double{yhigh};
    } else if (type == "Histo3d") {
        file_fs >> ybins >> ylow >> yhigh >> zbins >> zlow >> zhigh;
        docs = docs << "ybins" << ybins
                    << "ylow"  << bsoncxx::types::b_double{ylow}
                    << "yhigh" << bsoncxx::types::b_double{yhigh}
                    << "zbins" << zbins
                    << "zlow"  << bsoncxx::types::b_double{zlow}
                    << "zhigh" << bsoncxx::types::b_double{zhigh};
    }
    file_fs >> underflow >> overflow;

    docs = docs << "underflow" << underflow
                << "overflow"  << overflow;

    if (!file_fs) {
        std::cerr << "Something wrong with file ..." << std::endl;
        return "ERROR";
    }

    auto in_array = docs << "dat" << open_array;

    if (type == "Histo1d") {
        for (int j=0; j<xbins; j++) {
            double tmp;
            file_fs >> tmp;
            in_array = in_array << tmp;
        }
    }

    if (type == "Histo2d") {
        for (int i=0; i<ybins; i++) {
            auto array_builder = bsoncxx::builder::basic::array{};
            for (int j=0; j<xbins; j++) {
                double tmp;
                file_fs >> tmp;
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
                    file_fs >> tmp;
                    array_builder.append( tmp );
                }
            }
            in_array = in_array << array_builder;
        }
    } 

    auto after_array = in_array << close_array;
    bsoncxx::document::value doc_value = after_array << finalize;
    mongocxx::collection collection = db["dat"];

    // TODO  soleve the speed to upload data into database in the case checking that the data has been already uploaded
    //bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(document{} << "data" << doc_value.view() << "filename" << i_filename << finalize);
    std::string oid_str = "";
    //if (maybe_result) {
    //    bsoncxx::document::element element = maybe_result->view()["_id"];
    //    oid_str = element.get_oid().value.to_string();
    //} else {
    //    auto result = collection.insert_one( document{} << "data" << doc_value.view() << "filename" << i_filename << "dbVersion" << m_db_version << finalize );
    //    oid_str = result->inserted_id().get_oid().value.to_string();
    //}
    auto result = collection.insert_one( document{} << "data" << doc_value.view() << "filename" << i_filename << "dbVersion" << m_db_version << finalize );
    oid_str = result->inserted_id().get_oid().value.to_string();

    return oid_str;
}

std::string Database::writeGridFsFile(std::string i_file_path, std::string i_filename) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload attachment" << std::endl;
    mongocxx::gridfs::bucket gb = db.gridfs_bucket();
    std::ifstream file_ifs(i_file_path);
    std::istream &file_is = file_ifs;
    auto result = gb.upload_from_stream(i_filename, &file_is);
    this->addVersion("fs.files", "_id", result.id().get_oid().value.to_string(), "oid");
    this->addVersion("fs.chunks", "files_id", result.id().get_oid().value.to_string(), "oid");
    return result.id().get_oid().value.to_string();
}

std::string Database::writeJsonCode(json &i_json, std::string i_filename, std::string i_title) {
    if (DB_DEBUG) std::cout << "\tDatabase: upload json file" << std::endl;

    bsoncxx::document::value json_doc = bsoncxx::from_json(i_json.dump()); 
    mongocxx::collection collection = db["json"];
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(document{} << "data" << json_doc.view() << "filename" << i_filename << "chipType" << m_chip_type << finalize);
    std::string oid_str = "";
    if (maybe_result) {
        bsoncxx::document::element element = maybe_result->view()["_id"];
        oid_str = element.get_oid().value.to_string();
    } else {
        bsoncxx::stdx::optional<bsoncxx::document::value> maybe_original = collection.find_one(
            document{} << "title"    << i_title     << 
                          "chipType" << m_chip_type << 
                          "dataType" << "original"  << finalize
        );
        document builder{};
        auto docs = builder << "data"      << json_doc.view() <<
                               "filename"  << i_filename      <<
                               "chipType"  << m_chip_type     << 
                               "title"     << i_title;
        if (maybe_original) {
            json original_json = json::parse(bsoncxx::to_json(*maybe_original));
            json diff_json = json::diff(original_json["data"], i_json);
            bsoncxx::document::element element = maybe_original->view()["_id"];
            oid_str = element.get_oid().value.to_string();
            bsoncxx::document::value patch_doc = bsoncxx::from_json(diff_json.dump()); 
            docs = docs << "dataType" << "diff"  <<
                           "original" << oid_str <<
                           "patch"    << patch_doc.view(); 
        } else {
            docs = docs << "dataType" << "default"; 
        }
        bsoncxx::document::value doc_value = docs << finalize;
        auto result = collection.insert_one(doc_value.view());
        oid_str = result->inserted_id().get_oid().value.to_string();
        this->addVersion("json", "_id", oid_str, "oid");
    }
    return oid_str;
}

