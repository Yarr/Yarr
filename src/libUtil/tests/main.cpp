#define CATCH_CONFIG_RUNNER

#include "catch.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "LoggingConfig.h"
#include "logging.h"

namespace {
auto logger = logging::make_log("test_main");
static const size_t ENV_BUFFER_SIZE = 256;
// have to pass static array to putenv
char env_update_buffer[ENV_BUFFER_SIZE];
}

void update_config_path() {
    // Look up in dir hierarchy so we can run from anywhere in YARR directory
    namespace fs = std::filesystem;
    fs::path baseDir{fs::current_path()};
    while(!fs::exists(baseDir/".git")) {
        if(!baseDir.has_parent_path()) {
            // Can't find file
            logger->error("Can't find configs, no parent of {}", std::string(baseDir));
            return;
        }
        if(baseDir == baseDir.parent_path()) {
            // Can't find file
            logger->error("Can't find configs from {}", std::string(baseDir));
            return;
        }
        baseDir = baseDir.parent_path();
    }

    std::string update_string = "YARR_CONFIG_PATH=" + std::string(baseDir) + "/";
    if(update_string.size() > ENV_BUFFER_SIZE-1) {
      logger->critical("Path to configs too long, aborting: {}", update_string);
    }
    strncpy(env_update_buffer, update_string.c_str(), ENV_BUFFER_SIZE);
    putenv(env_update_buffer);
}

int main( int argc, char* argv[] )
{
  Catch::Session session; // There must be exactly one instance

  bool known_loggers = false;
  std::string logging_config;
  
  // Build a new parser on top of Catch's
  using namespace Catch::Clara;
  auto cli 
    = session.cli() // Get Catch's composite command line parser
    | Opt(known_loggers)
          ["-k"]("Print known loggers")
    | Opt( logging_config, "log_config" ) // bind variable to a new option, with a hint string
    ["--logger-config"]    // the option names it will respond to
    ("Configure logging using json file");        // description string for the help output
        
  // Now pass the new composite back to Catch so it uses that
  session.cli( cli ); 

  // Let Catch (using Clara) parse the command line
  int returnCode = session.applyCommandLine( argc, argv );
  if( returnCode != 0 ) // Indicates a command line error
    return returnCode;

  if(known_loggers) {
    std::cout << " Known loggers:\n";
    logging::listLoggers();
    return 0;
  }

  json lj;
  if( logging_config.empty() ) {
    // default to logging errors and above
    lj["log_config"][0]["name"] = "all";
    lj["log_config"][0]["level"] = "error";
  } else {
    std::ifstream log_file(logging_config);
    lj = json::parse(log_file);
    if(lj.is_null()) {
      std::cout << "Failed to load logging config, aborting\n";
      return 1;
    }
  }
  logging::setupLoggers(lj);

  // Always point to files in repo
  update_config_path();

  return session.run();
}
