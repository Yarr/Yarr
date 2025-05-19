#include <filesystem>

#include "AllConfigurations.h"

#include "logging.h"

#include "ScanHelper.h"

namespace {
  auto fclog = logging::make_log("FileConfiguration");
}

/// For now use the json open function from ScanHelper
using ScanHelper::openJsonFile;

/**
 * Default implementation of Configuration, reading from files on disk.
 */
class FileConfiguration : public Configuration {
public:
    void reportStatus() override;

    json getConnectivity(const std::vector<std::string> &) override;

    json getFrontEndConfig(const std::string &name) override;

    json getControllerConfiguration(const std::string &name) override;

    json getScanConfiguration(const std::string &name) override;
};

void FileConfiguration::reportStatus()
{
    fclog->info("Configuration ready");
}

json FileConfiguration::getConnectivity(const std::vector<std::string> &names)
{
    json connConfig = json::array();
    for (auto &cPath: names) {
        fclog->debug("Loading connectivity from {}", cPath);
        json feConnect;
        try {
            feConnect = openJsonFile(cPath);
            feConnect["_read_path"] = cPath;
        } catch (std::runtime_error &e) {
            fclog->critical("#ERROR# opening connectivity ({}): {}", cPath, e.what());
            return -1;
        }
        connConfig.push_back(feConnect);
    }
    return connConfig;
}

json FileConfiguration::getFrontEndConfig(const std::string &name)
{
    if (!std::filesystem::exists(name)) {
        fclog->warn("Config file not found, using default front end {}!", name);
        return "default";
    }

    try {
        fclog->debug("Loading front end configuration from {}", name);
        return ScanHelper::openJsonFile(name);
    } catch (std::runtime_error &e) {
        fclog->error("Error opening chip config: {}", e.what());
        throw (std::runtime_error("buildChips failure"));
    }
}

json FileConfiguration::getControllerConfiguration(const std::string &name)
{
    try {
        fclog->debug("Loading controller configuration from {}", name);
        return  ScanHelper::openJsonFile(name);
    } catch(std::runtime_error &e) {
        fclog->error("Error opening controller config ({}): {}",
                     name, e.what());
        throw (std::runtime_error("loadConfigFile failure"));
    }
}

json FileConfiguration::getScanConfiguration(const std::string &name)
{
    try {
        fclog->debug("Loading scan configuration from {}", name);
        return openJsonFile(name);
    } catch (std::runtime_error &e) {
        fclog->critical("#ERROR# opening scan config: {}", e.what());
        throw std::runtime_error("Load scan failure");
    }
}

bool file_configuration_registered =
StdDict::registerConfiguration
  ("File", []() { return std::unique_ptr<Configuration>(new FileConfiguration()); });
