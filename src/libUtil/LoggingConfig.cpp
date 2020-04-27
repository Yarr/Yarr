#include "LoggingConfig.h"

#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

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

    std::map<std::string, spdlog::sink_ptr> other_sinks;

    std::string default_pattern = "";

    if(!j["simple"].empty()) {
        // Don't print log level and timestamp
        if (j["simple"])
            default_pattern = "%v";
    }

    if(!j["level"].empty()) {
        std::string level_name = j["level"];
        default_sink->set_level((spdlog::level::level_enum)level_map.at(level_name));
    }

    // NB this sets things at the sink level, so not specifying a particular logger...
    // Also doing it last means it applies to all registered sinks
    if(!j["pattern"].empty()) {
        default_pattern = j["pattern"];
    }

    if(!j["sinks"].empty()) {
      for(auto &jl: j["sinks"]) {
        if(jl["name"].empty()) {
          spdlog::warn("Log json file: missing 'name' field for additional sink configuration");
          continue;
        }
        if(jl["file_name"].empty()) {
          spdlog::warn("Log json file: missing 'file_name' field for additional sink configuration");
          continue;
        }

        std::string key = jl["name"];
        std::string fname = jl["file_name"];

        auto new_sink = std::shared_ptr<spdlog::sinks::basic_file_sink_mt>
          (new spdlog::sinks::basic_file_sink_mt(fname));

        if(!jl["level"].empty()) {
            std::string level_name = jl["level"];
            new_sink->set_level((spdlog::level::level_enum)level_map.at(level_name));
        }

        std::string pattern = default_pattern;

        if(!jl["pattern"].empty()) {
          pattern = jl["pattern"];
        }

        new_sink->set_pattern(pattern);
        other_sinks[key] = new_sink;
      }
    }

    if(!default_pattern.empty()) {
      default_sink->set_pattern(default_pattern);
    }

    if(!j["log_config"].empty()) {
        for(auto &jl: j["log_config"]) {
            if(jl["name"].empty()) {
                spdlog::error("Log json file: 'log_config' list item must have 'name");
                continue;
            }

            std::string name = jl["name"];

            std::optional<spdlog::level::level_enum> opt_level;
            if(!jl["level"].empty()) {
                std::string level_name = jl["level"];

                opt_level = (spdlog::level::level_enum)level_map.at(level_name);
            }

            auto sink = default_sink;
            if(!jl["sink"].empty()) {
                std::string sink_name = jl["sink"];
                sink = other_sinks[sink_name];
            }

            auto logger_apply = [&](std::shared_ptr<spdlog::logger> l) {
                l->sinks().push_back(sink);
                if(opt_level) {
                    l->set_level(*opt_level);
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

    // Remove the initial sink from the default logger
    auto default_logger = spdlog::default_logger();
    auto &old_sinks = default_logger->sinks();
    std::remove_reference<decltype(old_sinks)>::type new_sinks;
    for(int i=1; i<old_sinks.size(); i++) {
      new_sinks.push_back(old_sinks[i]);
    }
    old_sinks = new_sinks;    

    // By this point the default logger should have been configured appropriately
    //  so this won't appear unless requested
    spdlog::trace("Log json file: A quick message using the default logger");

    if(!j["report_loggers"].empty()) {
      listLoggers(true);
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
        std::cout << "    Sends to sinks at level " << curr->level();
        auto lvl = level_string(curr->level());
        if(!lvl.empty()) {
          std::cout << " (" << lvl << ")\n";
        } else {
          std::cout << "\n";
        }
        std::cout << "    Has " << curr->sinks().size() << " sinks\n";

        for(auto &s: curr->sinks()) {
          auto lvl = level_string(s->level());
          std::cout << "      Reports at level " << s->level();
          if(!lvl.empty()) {
            std::cout << " (" << lvl << ")\n";
          } else {
            std::cout << "\n";
          }
        }
    }
}

} // End namespace logging
