#include "Database.h"

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

Database::Database(std::string i_host_ip) {
    DB_DEBUG = true;
    if (DB_DEBUG) std::cout<<"Database: Initialize" << std::endl;

    mongocxx::instance inst{};
    client = mongocxx::client{mongocxx::uri{i_host_ip}};

    std::string m_database_name = "yarrdb";
    std::string m_collection_name = "yarrcoll";
    db = client[m_database_name];

    m_has_flags = false;
    m_hv = 0;
    m_cool_temp = 25;
    m_stage = "ASSEMBLED";
}

Database::~Database() {
    if (DB_DEBUG) std::cout << "Database: Exit Database" << std::endl;
}

//*****************************************************************************************************
// Public functions
//
void Database::setFlags(std::vector<std::string> i_flags) {
    if (DB_DEBUG) std::cout << "Database: Flags" << std::endl;
    if (i_flags.size() != 0) {
        m_has_flags = true;
        std::cout << "Set flags, ";
        for (unsigned i=0; i<i_flags.size(); i++) {
            std::string flag_name = i_flags[i];
            if (flag_name == "hv") {
                std::cout << "HV: " << m_hv << " V, ";
                i++;
                m_hv = std::stod(i_flags[i]);
            }
            else if (flag_name == "cool") {
                std::cout << "COOL: " << m_cool_temp << " C, ";
                i++;
                m_cool_temp = std::stod(i_flags[i]);
            }
            else if (flag_name == "encapsulation") {
                std::cout << "After encapsulation, ";
                m_stage = flag_name;
            }
            else if (flag_name == "wirebond") {
                std::cout << "After wirebond, ";
                m_stage = flag_name;
            }
        }
        std::cout << std::endl;
    }
}

void Database::write(std::string i_serial_number, std::string i_test_type, int i_run_number, std::string i_output_dir) {
    if (DB_DEBUG) std::cout << "Database: Write" << std::endl;
    std::string component_type_str = this->getValue("component", "serialNumber", i_serial_number, "componentType");
    if (component_type_str == "Module") {
        std::string module_oid_str = this->getValue("component", "serialNumber", i_serial_number, "_id", "oid");
        mongocxx::collection collection = db["childParentRelation"];
        mongocxx::cursor cursor = collection.find(document{} << "parent" << module_oid_str << finalize);
        for (auto doc : cursor) {
            bsoncxx::document::element element = doc["child"];
            std::string child_oid_str = element.get_utf8().value.to_string();
            std::string test_run_oid_str = this->registerTestRun(i_test_type, i_run_number);
            std::string chip_name_str = this->getValueByOid("component", child_oid_str, "name");
            this->registerComponentTestRun(child_oid_str, test_run_oid_str, i_test_type, i_run_number);
            this->uploadFromDirectory("testRun", test_run_oid_str, i_output_dir, chip_name_str);
            if (m_has_flags) this->addEnvironment(test_run_oid_str, "testRun");
        }
    }
    else {
        std::string component_oid_str = this->getValue("component", "serialNumber", i_serial_number, "_id", "oid");
        std::string test_run_oid_str = this->registerTestRun(i_test_type, i_run_number);
        this->registerComponentTestRun(component_oid_str, test_run_oid_str, i_test_type, i_run_number);
        this->uploadFromDirectory("testRun", test_run_oid_str, i_output_dir);
        //this->addComment("testRun", test_run_oid_str, "hoge!!");
        if (m_has_flags) this->addEnvironment(test_run_oid_str, "testRun");
    }
}

void Database::writeFiles(std::string i_serial_number, int i_run_number_s, int i_run_number_e) {
    if (DB_DEBUG) std::cout << "Database: Write files" << std::endl;

    // get component id
    std::string component_oid_str = this->getValue("component", "serialNumber", i_serial_number, "_id", "oid");
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
                    std::string fs_str = uploadAttachment(outputDir+filename+"."+fileextension, filename+"."+fileextension);
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
    if (DB_DEBUG) std::cout << "Database: Upload from json" << std::endl;
    std::ifstream json_ifs(i_json_path);
    if (!json_ifs) {
        std::cerr <<"#ERROR# Cannot open register json file: " << i_json_path << std::endl;
        return "ERROR";
    }
    json json = json::parse(json_ifs);
    bsoncxx::document::value doc_value = bsoncxx::from_json(json.dump()); 
    mongocxx::collection collection = db[i_collection_name];
    auto result = collection.insert_one(doc_value.view());
    bsoncxx::oid oid = result->inserted_id().get_oid().value;
    return oid.to_string();
}

void Database::registerFromConnectivity(std::string i_json_path) {
    if (DB_DEBUG) std::cout << "Database: Register from connectivity" << std::endl;
    std::ifstream json_ifs(i_json_path);
    if (!json_ifs) {
        std::cerr << "#ERROR# Cannot open register json file: " << i_json_path << std::endl;
        return;
    }
    json conn_json = json::parse(json_ifs);

    // chip component
    for (unsigned i=0; i<conn_json["chips"].size(); i++) {
        mongocxx::collection collection = db["component"];
        std::string serialNumber = conn_json["chips"][i]["serialNumber"];
        bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(document{} << "serialNumber" << serialNumber << finalize);
        if (maybe_result) continue;

        std::ifstream j_ifs(conn_json["chips"][i]["config"].get<std::string>());
        if (!j_ifs) {
            std::cerr << "#ERROR# Cannot open register chip config file" << std::endl;
            return;
        }
        json jj = json::parse(j_ifs);
        std::string chipType;
        if (conn_json["chipType"] == "FEI4B") chipType = "FE-I4B";

        bsoncxx::document::value doc_value = document{} <<  
            "sys" << open_document << close_document <<
            "serialNumber" << serialNumber <<
            "componentType" << chipType <<
            "name" << jj[chipType]["name"].get<std::string>() <<
        finalize;
     
        auto result = collection.insert_one(doc_value.view());
        bsoncxx::oid oid = result->inserted_id().get_oid().value;
        this->addSys(oid.to_string(), "component");
    }

    // module component
    if (conn_json["module"].empty() || conn_json["module"]["serialNumber"].empty()) {
        std::cout << "\tDatabase: no module info in connectivity! skip register module" << std::endl;
    } else {
        std::string serialNumber = conn_json["module"]["serialNumber"];
        mongocxx::collection collection = db["component"];
        bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = collection.find_one(document{} << "serialNumber" << serialNumber << finalize);
        if (!maybe_result) {
            bsoncxx::document::value doc_value = document{} <<  
                "sys" << open_document << close_document <<
                "serialNumber" << serialNumber <<
                "componentType" << conn_json["module"]["componentType"].get<std::string>() <<
            finalize;
     
            auto result = collection.insert_one(doc_value.view());
            bsoncxx::oid oid = result->inserted_id().get_oid().value;
            this->addSys(oid.to_string(), "component");
    
            // CP relation
            for (unsigned i=0; i<conn_json["chips"].size(); i++) {
                bsoncxx::document::value doc_value = document{} <<  
                    "sys" << open_document << close_document <<
                    "parent" << getValue("component", "serialNumber", conn_json["module"]["serialNumber"], "_id", "oid") <<
                    "child" << getValue("component", "serialNumber", conn_json["chips"][i]["serialNumber"], "_id", "oid") <<
                finalize;
     
                mongocxx::collection collection = db["childParentRelation"];
                auto result = collection.insert_one(doc_value.view());
                bsoncxx::oid oid = result->inserted_id().get_oid().value;
                this->addSys(oid.to_string(), "childParentRelation");
            }
        }
    }
}

//*****************************************************************************************************
// Protected fuctions
//
std::string Database::getValue(std::string i_collection_name, std::string i_member_key, std::string i_member_value, std::string i_key, std::string i_bson_type){
    if (DB_DEBUG) std::cout << "\tDatabase: get value" << std::endl;
    mongocxx::collection collection = db[i_collection_name];
    bsoncxx::stdx::optional<bsoncxx::document::value> result = collection.find_one(document{} << i_member_key << i_member_value << finalize);
    if(result) {
        if (i_bson_type == "oid") {
            bsoncxx::document::element element = result->view()["_id"];
            return element.get_oid().value.to_string();
        }
        else {
            bsoncxx::document::element element = result->view()[i_key];
            return element.get_utf8().value.to_string();
        }
    }
    else {
        std::cerr <<"#ERROR# Cannot find " << i_key << " from member " << i_member_key << ": " << i_member_value << " in collection name: " << i_collection_name << std::endl;
        abort();
        return "ERROR";
    }
}

std::string Database::getValueByOid(std::string i_collection_name, std::string i_member_value, std::string i_key, std::string i_bson_type){
    if (DB_DEBUG) std::cout << "\tDatabase: get value by oid" << std::endl;
    mongocxx::collection collection = db[i_collection_name];
    bsoncxx::stdx::optional<bsoncxx::document::value> result = collection.find_one(document{} << "_id" << bsoncxx::oid(i_member_value) << finalize);
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
        std::cerr <<"#ERROR# Cannot find " << i_key << " from member _id : " << i_member_value << " in collection name: " << i_collection_name << std::endl;
        abort();
        return "ERROR";
    }
}

std::string Database::registerComponentTestRun(std::string i_component_oid_str, std::string i_test_run_oid_str, std::string i_test_type, int i_run_number) {
    if (DB_DEBUG) std::cout << "\tDatabase: Register Com-Test Run" << std::endl;
    bsoncxx::document::value doc_value = document{} <<  
        "sys" << open_document << close_document <<
        "component" << i_component_oid_str << // id of component
        "state" << "..." << // code of test run state
        "stage" << "..." << // code of current stage of the component
        "testType" << i_test_type << // id of test type
        "testRun" << i_test_run_oid_str << // id of test run, test is planned if it is null
        "qaTest" << false << // flag if it is the QA test
        "runNumber" << i_run_number << // run number
        "passed" << true << // flag if the test passed
        "problems" << true << // flag if any problem occured
    finalize;
    mongocxx::collection collection = db["componentTestRun"];
    auto result = collection.insert_one(doc_value.view());
    bsoncxx::oid oid = result->inserted_id().get_oid().value;
    this->addSys(oid.to_string(), "componentTestRun");
    return oid.to_string();
}

std::string Database::registerTestRun(std::string i_test_type, int i_run_number) {
    if (DB_DEBUG) std::cout << "\tDatabase: Register Test Run" << std::endl;
    bsoncxx::document::value doc_value = document{} <<  
        "sys" << open_document << close_document <<
        "testType" << i_test_type << // id of test type //TODO make it id
        "runNumber" << i_run_number << // number of test run
        "date" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // date when the test run was taken
        "institution" << "..." << // id of institution
        "userIdentity" << "..." << // identity of user who inserted test results
        "passed" << true << // flag if test passed
        "problems" << true << // flag if any problem occured
        "state" << "ready" << // state of component ["ready", "requestedToTrash", "trashed"]
        "environment" << open_document <<
        close_document <<
        "comments" << open_array << // array of comments
        close_array <<
        "attachments" << open_array << // array of attachments
        close_array <<
        "defects" << open_array << // array of defects
        close_array <<
    finalize;
    mongocxx::collection collection = db["testRun"];
    auto result = collection.insert_one(doc_value.view());
    bsoncxx::oid oid = result->inserted_id().get_oid().value;
    this->addSys(oid.to_string(), "testRun");
    return oid.to_string();
}

void Database::addComment(std::string i_collection_name, std::string i_oid_str, std::string i_comment) {
    if (DB_DEBUG) std::cout << "\t\tDatabase: Add comment" << std::endl;
    bsoncxx::oid i_oid(i_oid_str);
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$push" << open_document <<
            "comments" << open_document <<
                    "code" << "01234567890abcdef01234567890abcdef" << // generated unique code
                    "dateTime" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // comment creation timestamp
                    "userIdentity" << "..." << // user identity who wrote the comment
                    "comment" << i_comment << // text of comment
            close_document <<
        close_document << finalize
    );
}

void Database::addAttachment(std::string i_oid_str, std::string i_collection_name, std::string i_file_oid_str, std::string i_title, std::string i_description, std::string i_content_type, std::string i_filename) {
    if (DB_DEBUG) std::cout << "\t\tDatabase: Add attachment: " << i_filename << "." << i_content_type << std::endl;
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

void Database::addDefect(std::string i_oid_str, std::string i_collection_name, std::string i_defect_name, std::string i_description) {
    if (DB_DEBUG) std::cout << "\t\tDatabase: Add defect" << std::endl;
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

std::string Database::uploadAttachment(std::string i_file_path, std::string i_filename) {
    if (DB_DEBUG) std::cout << "\t\tDatabase: upload attachment" << std::endl;
    mongocxx::gridfs::bucket gb = db.gridfs_bucket();
    std::ifstream file_ifs(i_file_path);
    std::istream &file_is = file_ifs;
    auto result = gb.upload_from_stream(i_filename, &file_is);
    return result.id().get_oid().value.to_string();
}

void Database::uploadFromDirectory(std::string i_collection_name, std::string i_test_run_oid_str, std::string outputDir, std::string i_filter) {
    if (DB_DEBUG) std::cout << "\t\tDatabase: upload from directory" << std::endl;
    // Get file path from directory
    std::array<char, 128> buffer;
    std::stringstream temp_strstream;
    temp_strstream.str(""); temp_strstream << "ls -v " << outputDir << "/*";
    std::shared_ptr<FILE> pipe(popen(temp_strstream.str().c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");

    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr) {
            std::string file_path = buffer.data();
            std::size_t pos =  file_path.find(i_filter);
            if (pos != std::string::npos) {
                file_path = file_path.substr(0, file_path.size()-1); // remove indent
                std::size_t pathPos = file_path.find_last_of('/');
                std::size_t suffixPos = file_path.find_last_of('.');
                std::string filename = file_path.substr(pathPos+1, suffixPos-pathPos-1);
                std::string fileextension = file_path.substr(suffixPos + 1);
                std::string oid_str = this->uploadAttachment(file_path, filename+"."+fileextension);
                this->addAttachment(i_test_run_oid_str, i_collection_name, oid_str, "title", "describe", fileextension, filename);
            }
        }
    }
}

void Database::addEnvironment(std::string i_oid_str, std::string i_collection_name) {
    if (DB_DEBUG) std::cout << "\t\tDatabase: Add environment" << std::endl;
    bsoncxx::oid i_oid(i_oid_str);
    db[i_collection_name].update_one(
        document{} << "_id" << i_oid << finalize,
        document{} << "$set" << open_document <<
            "environment" << open_document <<
                "hv" << m_hv << 
                "cool" << m_cool_temp << 
                "stage" << m_stage << 
            close_document <<
        close_document << finalize
    );
}

void Database::addSys(std::string i_oid_str, std::string i_collection_name) {
    if (DB_DEBUG) std::cout << "\t\tDatabase: Add environment" << std::endl;
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

//To be deleted in the future
void Database::viewer() {
    std::string input;
    while (1) {
        std::cout << "Type \"module\" to select module" << std::endl;
        std::cout << "Type \"chip\" to select chip" << std::endl;
        std::cout << "Type \"q\" to quit." << std::endl;
        mongocxx::collection collection = db["component"];
        mongocxx::cursor cursor = collection.find({document() << "componentType" << "Module" << finalize});
        for (auto doc : cursor) {
            bsoncxx::document::element element = doc["serialNumber"];
            std::string sn_str = element.get_utf8().value.to_string();
            std::cout << "SN: " << sn_str << std::endl;
        }
        std::cin >> input;
        if (input == "q") break;
        else if (input == "module") {
            std::cout << "Input module SN" << std::endl;
            std::cin >> input;
            if (input == "q") break;
            else {
                mongocxx::collection collection = db["childParentRelation"];
                mongocxx::cursor cursor = collection.find({document() << "parent" << getValue("component", "serialNumber", input, "_id", "oid") << finalize});
                for (auto doc : cursor) {
                    bsoncxx::document::element element = doc["child"];
                    std::string id_str = element.get_utf8().value.to_string();
                    std::cout << "\tSN: " << getValueByOid("component", id_str, "serialNumber") << " - " << getValueByOid("component", id_str, "componentType") << std::endl;
                }
            }
        }
        else if (input == "chip") {
            std::cout << "Input chip SN" << std::endl;
            std::cin >> input;
            if (input == "q") break;
            else {
                mongocxx::collection collection = db["componentTestRun"];
                mongocxx::cursor cursor = collection.find({document() << "component" << getValue("component", "serialNumber", input, "_id", "oid") << finalize});
                for (auto doc : cursor) {
                    bsoncxx::document::element element = doc["testRun"];
                    std::string id_str = element.get_utf8().value.to_string();
                    std::cout << "\t\tTest Type: " << getValueByOid("testRun", id_str, "testType") << " - " << 
                        getValueByOid("testRun", id_str, "", "sys_datetime") << " - " <<
                        getValueByOid("testRun", id_str, "runNumber", "int") << std::endl;
                }
            }
        }
    }
}
