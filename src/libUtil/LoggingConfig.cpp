#include "LoggingConfig.h"

#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace logging {

void setupLoggers(const json &j) {
    spdlog::sink_ptr default_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // spdlog::level::level_enum, but then construction doesn't work?
    const std::map<std::string, int> level_map = {
      {"off", SPDLOG_LEVEL_OFF},
      {"critical", SPDLOG_LEVEL_CRITICAL},
      {"err", SPDLOG_LEVEL_ERROR},
      {"error", SPDLOG_LEVEL_ERROR},
      {"warn", SPDLOG_LEVEL_WARN},
      {"warning", SPDLOG_LEVEL_WARN},
      {"info", SPDLOG_LEVEL_INFO},
      {"debug", SPDLOG_LEVEL_DEBUG},
      {"trace", SPDLOG_LEVEL_TRACE},
    };

    if(!j["simple"].empty()) {
        // Don't print log level and timestamp
        if (j["simple"])
            default_sink->set_pattern("%v");
    }

    if(!j["log_config"].empty()) {
        for(auto &jl: j["log_config"]) {
            if(jl["name"].empty()) {
                spdlog::error("Log json file: 'log_config' list item must have 'name");
                continue;
            }

            std::string name = jl["name"];

            auto logger_apply = [&](std::shared_ptr<spdlog::logger> l) {
              l->sinks().push_back(default_sink);
              if(!jl["level"].empty()) {
                  std::string level = jl["level"];

                  spdlog::level::level_enum spd_level = (spdlog::level::level_enum)level_map.at(level);
                  l->set_level(spd_level);
              }
            };

            if(name == "all") {
                spdlog::apply_all(logger_apply);
            } else {
                auto l = spdlog::get(name);
                if(!l) {
                  spdlog::warn("Log json file: logger '{}' doesn't match any known loggers");                  
                } else {
                  logger_apply(l);
                }
            }
        }
    }

    // NB this sets things at the sink level, so not specifying a particular logger...
    // Also doing it last means it applies to all registered sinks
    if(!j["pattern"].empty()) {
        std::string pattern = j["pattern"];
      
        spdlog::set_pattern(pattern);
    }
}

void listLoggers(bool print_details) {
    std::string_view def_name = "(default)";
    std::vector<std::string> log_list;
    spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) {
        if(l->name().empty()) {
            log_list.push_back("(default)");
        } else {
            log_list.push_back(l->name());
        }
    });

    std::sort(log_list.begin(), log_list.end());

    for(auto &l: log_list) {
        std::cout << "  " << l << "\n";

        if(!print_details)
          continue;

        std::string ll = l;
        if(l == def_name) {
          ll = "";
        }
        
        auto curr = spdlog::get(ll);
        std::cout << "    At level " << curr->level() << "\n";
        std::cout << "    Has " << curr->sinks().size() << " sinks\n";

        for(auto &s: curr->sinks()) {
          std::cout << "     at level " << s->level() << "\n";
        }
    }
}

} // End namespace logging
