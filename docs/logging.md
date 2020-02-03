
# Logging

Yarr uses spdlog for logging. This is used to enable turning on/off output
from different parts of the system dynamically.

Each logger provides output with specific level:

* trace
* debug
* info
* warn
* error
* critical

The default is to enable output of message of info level or higher from all
loggers.

The loggers can be configured by passing the -l flag to scanConsole, with a
configuration file. For example see: configs/logging.json.

You can list all available loggers as part of:

``scanConsole -k``
