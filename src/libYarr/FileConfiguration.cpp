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

bool file_configuration_registered =
StdDict::registerConfiguration
  ("File", []() { return std::unique_ptr<Configuration>(new FileConfiguration()); });
