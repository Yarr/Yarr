Logger configuration file
=========================

These are files for configuring the logging system.

Most units in YARR create a logger and send messages to it, how
these messages are dealt with can be changed by loading a different
configuration (for instance using the '-l' flag in scanConsole).

It is important to note that each logger has an associated level,
below which messages will be discarded. Messages above the level
are sent to sinks configured for that logger, each of which also
have a required level.

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
