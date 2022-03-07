#include "AllHwControllers.h"
#include "HwController.h"

#include "storage.hpp"

#include <iostream>

int main(int argc, char **argv) {

  json j;
  j["FelixClient"]["local_ip_or_interface"] = "lo";
  j["FelixClient"]["log_level"] = "trace";
  j["FelixClient"]["bus_interface"] = "<local_interface>";
  j["FelixClient"]["bus_dir"] = "./bus";
  j["FelixClient"]["bus_group_name"] = "FELIX";
  j["FelixClient"]["verbose_bus"] = true;
  j["FelixClient"]["verbose_zyre"] = true;
  j["FelixClient"]["timeout"] = 0;
  j["FelixClient"]["netio_pages"] = 256;
  j["FelixClient"]["netio_pagesize"] = 64*1024;

  std::unique_ptr<HwController> hwCtrl = StdDict::getHwController("FelixClient");
  try {
    hwCtrl->loadConfig(j);
  } catch (std::runtime_error& e) {
    // Probably because felix software is not set up
    // Need to add path to libfelix-client-lib.so to LD_LIBRARY_PATH
    std::cout << e.what() << std::endl;
    return 1;
  }

  return 0;
}
