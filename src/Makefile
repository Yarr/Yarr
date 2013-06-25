######
##  Default configuration
##
## Modify this configuration by creating a local.config file and resetting the
## variables there. See local.config.sample for an example and a description
## of all used variables.

MODE = RELEASE

AR = ar

CPP = g++
CPPFLAGS += -Wall -Wextra -IlibSpec/include -IlibYarr/include `root-config --cflags`
CPPFLAGS_DEBUG += -g -O0
CPPFLAGS_RELEASE += -O2 -DNDEBUG

LD = g++
LDFLAGS += `root-config --ldflags`
LDFLAGS_DEBUG +=
LDFLAGS_RELEASE += 
LIBS += `root-config --libs --glibs`

BIN = bin
SRC = tools
SPEC = libSpec
YARR = libYarr
BUILD = build
LLIB = lib

-include local.config

# Set build mode specific variables
CPPFLAGS += $(CPPFLAGS_$(MODE))
LDFLAGS += $(LDFLAGS_$(MODE))

######
##  File lists

SRC_FILES = $(wildcard $(SRC)/*.cpp) 
SPEC_FILES = $(wildcard $(SPEC)/*.cpp)
YARR_FILES = $(wildcard $(YARR)/*.cpp)

SRC_DEPS = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.d,$(SRC_FILES))
SPEC_DEPS = $(patsubst $(SPEC)/%.cpp,$(BUILD)/%.d,$(SPEC_FILES))
YARR_DEPS = $(patsubst $(YARR)/%.cpp,$(BUILD)/%.d,$(YARR_FILES))

ALL_DEPS = $(SRC_DEPS) $(SPEC_DEPS) $(YARR_DEPS)

SRC_OBJS = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(SRC_FILES))
SPEC_OBJS = $(patsubst $(SPEC)/%.cpp,$(BUILD)/%.o,$(SPEC_FILES))
YARR_OBJS = $(patsubst $(YARR)/%.cpp,$(BUILD)/%.o,$(YARR_FILES))
ALL_OBJS = $(SRC_OBJS) $(SPEC_OBJS) $(YARR_OBJS) 

TARGETS = $(patsubst $(SRC)/%.cpp,$(BIN)/%,$(SRC_FILES))

######
##  Targets

all: spec yarr $(TARGETS)

spec: $(BUILD)/libspec.a

yarr: $(BUILD)/libyarr.a

help:
	@echo # Use 'make' to create all programs
	@echo # Use 'make tools' to create the librce_tools.a
	@echo # Use 'make analysis' to create the librce_ana.a
	@echo # Special targets:
	@echo "#   list           - List all targets"
	@echo "#   mkinfo         - List makefile internals"
	@echo "#   help           - Show this help message"

# Dependency file creation
$(BUILD)/%.d: $(SRC)/%.cpp
	@$(CPP) $(CPPFLAGS) -MM -MT $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$<) $< > $@

$(BUILD)/%.d: $(SPEC)/%.cpp
	@$(CPP) $(CPPFLAGS) -MM -MT $(patsubst $(SPEC)/%.cpp,$(BUILD)/%.o,$<) $< > $@

$(BUILD)/%.d: $(YARR)/%.cpp
	@$(CPP) $(CPPFLAGS) -MM -MT $(patsubst $(YARR)/%.cpp,$(BUILD)/%.o,$<) $< > $@

# Object creation
$(BUILD)/%.o:
	@echo [Compiling] $@
	@$(CPP) $(CPPFLAGS) -c $< -o $@
        
# Target
$(BIN)/%: $(BUILD)/%.o $(SPEC_OBJS) $(YARR_OBJS)
	@echo [Linking] $@
	@$(LD) $(LDFLAGS) $< $(SPEC_OBJS) $(YARR_OBJS) $(LIBS) -o $@

$(BUILD)/libyarr.a: $(YARR_OBJS)
	@echo [Creating] $@
	@$(AR) rcs $@ $(YARR_OBJS)

$(BUILD)/libspec.a: $(SPEC_OBJS)
	@echo [Creating] $@
	@$(AR) rcs $@ $(SPEC_OBJS)

-include $(ALL_DEPS)

clean:
	@$(RM) $(ALL_OBJS) $(ALL_DEPS) $(TARGETS) $(BUILD)/libspec.a $(BUILD)/libyarr.a 
	@echo "[Clean] … done!"

list:
	@echo Target List
	@echo ===========
	@echo $(TARGETS)

mkinfo:
	@echo Make Variables
	@echo ==============
	@echo MODE = $(MODE)
	@echo
	@echo CPP = $(CPP)
	@echo CPPFLAGS = $(CPPFLAGS)
	@echo LD = $(LD)
	@echo LDFLAGS = $(LDFLAGS)
	@echo
	@echo Program Files
	@echo =============
	@echo Source files:
	@echo "    $(SRC_FILES)"
	@echo Dependency files:
	@echo "    $(SRC_DEPS)"
	@echo Object files:
	@echo "    $(SRC_OBJS)"
	@echo 
	@echo Tool Files
	@echo =============
	@echo Source files:
	@echo "    $(SPEC_FILES)"
	@echo Dependency files:
	@echo "    $(SPEC_DEPS)"
	@echo Object files:
	@echo "    $(SPEC_OBJS)"
	@echo 
	@echo Analysis Files
	@echo =============
	@echo Source files:
	@echo "    $(YARR_FILES)"
	@echo Dependency files:
	@echo "    $(YARR_DEPS)"
	@echo Object files:
	@echo "    $(YARR_OBJS)"
	@echo 
        
.PHONY: clean distclean mkinfo help
