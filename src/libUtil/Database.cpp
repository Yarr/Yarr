#include "Database.h"

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

Database::Database(std::string i_host_ip) {
    DB_DEBUG = true;
    if (DB_DEBUG) std::cout<<"Database: Initialize" << std::endl;

    mongocxx::instance inst{};
    client = mongocxx::client{mongocxx::uri{i_host_ip}};

    std::string m_database_name = "yarrdb";
    std::string m_collection_name = "yarrcoll";
    db = client[m_database_name];
}

Database::~Database() {
    if (DB_DEBUG) std::cout << "Database: Exit Database" << std::endl;
}

//*****************************************************************************************************
// Public functions
//
void Database::write(std::string i_test_type, int i_run_number, std::string i_output_dir) {
    if (DB_DEBUG) std::cout << "Database: Write" << std::endl;
    std::string test_run_oid_str = this->registerTestRun(i_test_type, i_run_number);
    this->registerComponentTestRun(i_test_type, test_run_oid_str, i_run_number);
    this->addComment("testRun", test_run_oid_str, "hoge!!");
    this->uploadFromDirectory(i_output_dir, test_run_oid_str, "testRun");
}

std::string Database::uploadFromJson(std::string i_collection_name, std::string i_json_path) {
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

//*****************************************************************************************************
// Protected fuctions
//

std::string Database::registerComponentTestRun(std::string i_test_type, std::string i_test_run_oid_str, int i_run_number) {
    if (DB_DEBUG) std::cout << "\tDatabase: Register Com-Test Run" << std::endl;
    bsoncxx::document::value doc_value = document{} <<  
        "sys" << open_document <<
            "rev" << 0 << // revision number
            "cts" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // creation timestamp
            "mts" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // modification timestamp
        close_document <<
        "component" << "..." << // id of component
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
    return oid.to_string();
}

std::string Database::registerTestRun(std::string i_test_type, int i_run_number) {
    if (DB_DEBUG) std::cout << "\tDatabase: Register Test Run" << std::endl;
    bsoncxx::document::value doc_value = document{} <<  
        "sys" << open_document <<
            "rev" << 0 << // revision number
            "cts" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // creation timestamp
            "mts" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // modification timestamp
        close_document <<
        "testType" << i_test_type << // id of test type //TODO make it id
        "runNumber" << i_run_number << // number of test run
        "date" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << // date when the test run was taken
        "institution" << "..." << // id of institution
        "userIdentity" << "..." << // identity of user who inserted test results
        "passed" << true << // flag if test passed
        "problems" << true << // flag if any problem occured
        "state" << "ready" << // state of component ["ready", "requestedToTrash", "trashed"]
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
    if (DB_DEBUG) std::cout << "\t\tDatabase: Add attachment" << std::endl;
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

void Database::uploadFromDirectory(std::string outputDir, std::string i_test_run_oid_str, std::string i_collection_name) {
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
            file_path = file_path.substr(0, file_path.size()-1); // remove indent
            std::size_t pathPos = file_path.find_last_of('/');
            std::size_t suffixPos = file_path.find_last_of('.');
            std::string filename;
            filename = file_path.substr(pathPos+1, suffixPos-pathPos-1);
            std::string oid_str = this->uploadAttachment(file_path, filename);
            this->addAttachment(i_test_run_oid_str, i_collection_name, oid_str, "title", "descrip", "type", filename);
        }
    }
}

//void Database::insertScan() {
//    client.insert(m_database_name+"."m_collection_name,
//        BSON("serialNumber" << "qwertyuiop"),
//        BSON("$set" << BSON(
//            "scan" << BSON_ARRAY(BSON(
//                "id" << "0123456789abcdef" <<
//                "dateTime" << BSON::DATENOW <<
//                "testType" << "" <<
//                "runNumber" << "" <<
//                "confidId" << "" <<
//                "resultId" << ""
//                ))
//            )
//        )
//   );
//}
//
//std::string Database::getSN() {
//
//}
