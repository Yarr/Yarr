#include "LoggingConfig.h"

#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

// spdlog::level::level_enum, but then construction doesn't work?
static const std::map<std::string, int> level_map = {
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

static std::string_view level_string(int lvl) {
  auto i = std::find_if(level_map.begin(), level_map.end(),
                        [lvl](const auto &it)
                        { return it.second == lvl;}
                        );
  if(i == level_map.end()) {
    return "";
  }
  return i->first;
}

namespace logging {

void setupLoggers(const json &j) {
    spdlog::sink_ptr default_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

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
                  spdlog::warn("Log json file: logger '{}' doesn't match any known loggers", name);
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
        std::cout << "    At level " << curr->level();
        auto lvl = level_string(curr->level());
        if(!lvl.empty()) {
          std::cout << " (" << lvl << ")\n";
        } else {
          std::cout << "\n";
        }
        std::cout << "    Has " << curr->sinks().size() << " sinks\n";

        for(auto &s: curr->sinks()) {
          auto lvl = level_string(s->level());
          std::cout << "     at level " << s->level();
          if(!lvl.empty()) {
            std::cout << " (" << lvl << ")\n";
          } else {
            std::cout << "\n";
          }
        }
    }
}

} // End namespace logging
