#ifndef YARR_LOGGING_H
#define YARR_LOGGING_H

#include <memory>

#include "spdlog/logger.h"
#include "spdlog/spdlog.h"

namespace logging {

typedef spdlog::logger Logger;
typedef std::shared_ptr<spdlog::logger> LoggerStore;

// Make a logger without any sinks
inline std::shared_ptr<spdlog::logger> make_log(std::string name) {
  auto log = spdlog::get(name);
  if(log == nullptr)
  {
    auto tmp = std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list{});
    spdlog::register_logger(tmp);
    return tmp;
  }
  return log;
}

}; // End logging namespace

#endif
