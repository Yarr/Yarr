
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

The default for scanConsole is to enable output of message of info level or
higher from all loggers. The loggers can be configured, using a json file,
by passing the -l flag to scanConsole.

The same configuration can be used for investigating test failures passing 
--logger-config FILENAME to test_main.

You can list all available loggers (that can be enabled/disabled) as part of:

``scanConsole -k``

## Logger configuration

Most units in YARR create a logger with a particular name and send messages
to it. The configuration file describes how these messages are dealt with.

In general the steps are:

1. Generate message with associated logger and level
2. Check message against level configured for the logger
3. For each configured sink
   1. Check message against level configured for the sink
   2. Send message to sink, which formats it using the format pattern.

It is important to note that all three of the message, the logger and the
sink have an associated level. The message will be discarded if it's below
either the logger or sink level.

For examples see the files in: configs/logging.

### Configuration file

The configuration file is based on json and should allow flexible
configuration of the log levels and sinks.

The top level properties "pattern" and "level" are applied to the 
default sink, which outputs to the console. See the spdlog 
documentation for the format of the pattern.

The sinks property is an array of dictionaries, which are used to
configure additional sinks (for instance writing to file). Each item
contains a "name" field which is used to specify a key that can be
used later. The "file_name" property specifies the output file name.
The "level" and "pattern" properties are the same as for the default
sink.

For debugging logger files, the "report_loggers" flag can be set
which reports information on the logger and sink setup.
