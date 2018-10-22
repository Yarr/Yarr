######
## Author: Timon Heim/Jan Kuechler
## Last edit: 17.01.2017
## Updates:
## * 17.01.2017 - K. Potamianos <karolos.potamianos@cern.ch>
##   Major refactoring to simplify adding features

UNAME_S := $(shell uname -s)
UNAME_P := $(shell uname -p)
MKDIR := mkdir -p

MODE = DEBUG

AR = ar

# In case it's not set, assuming local architecture
ARCH ?= $(UNAME_P)

CPP = g++
LD = g++

ifneq ($(CROSS_COMPILE),)
$(info Using: $(CPP))
$(foreach D,CPP LD,$(eval $(D) = $(addprefix $(CROSS_COMPILE),$($(D)))))
endif

#CC = gcc
ifeq ($(UNAME_S),Darwin)
CPP = clang++
LD = clang++
CPPFLAGS += -pthread
LDFLAGS += -Wl,-all_load
else
CPPFLAGS += -pthread -fPIC
LDFLAGS += -pthread -lrt -fPIC -Wl,--whole-archive
PLDFLAGS = -Wl,--no-whole-archive
endif

CPPFLAGS += -std=c++11 -Wall -Werror
INC_DIRS := $(wildcard */include)

ifneq (${NETIO_DIR},)
  INC_DIRS+= $(NETIO_DIR)/include
endif

INCFLAGS += $(addprefix -I,$(INC_DIRS))
CPPFLAGS += $(INCFLAGS)

CPPFLAGS_DEBUG += -g -O1 -DDEBUG
CPPFLAGS_RELEASE += -O2 -DNDEBUG

LDFLAGS_DEBUG +=
LDFLAGS_RELEASE +=
LIBS +=

BIN = bin
BUILD = build
LLIB = lib

ifneq ($(ARCH),)
$(foreach D,BIN BUILD LLIB,$(eval $(D) = $($(D))/$(ARCH)))
$(shell $(MKDIR) $(BIN) $(BUILD) $(LLIB))
endif

GCC_MAJOR = $(shell $(CPP) -dumpversion | cut -d'.' -f1)
GCC_MINOR = $(shell $(CPP) -dumpversion | cut -d'.' -f2)

ifneq ($(UNAME_S), Darwin)
ifeq ($(findstring $(GCC_MAJOR),0123456),$(GCC_MAJOR))
#ifeq ($(findstring $(GCC_MINOR),0),$(GCC_MINOR))
$(error $(CPP) version is too old, need atleast 7.0! Found $(GCC_MAJOR).$(GCC_MINOR))
#endif
endif
endif

# Set build mode specific variables
CPPFLAGS += $(CPPFLAGS_$(MODE))
LDFLAGS += $(LDFLAGS_$(MODE))

all:

######
##  File lists

# Arguments: 1 - pkgName ; 2 - pkgDir
define addPkg
$(1)_FILES = $(wildcard $(2)/*.cpp)
ALL_FILES += $$($(1)_FILES)
$(1)_DEPS = $$(patsubst %.cpp,$(BUILD)/%.d,$$(notdir $$($(1)_FILES)))
ALL_DEPS += $$($(1)_DEPS)
$(1)_OBJS = $$(patsubst %.d,%.o,$$($(1)_DEPS))
ALL_OBJS += $$($(1)_OBJS)
STATLIBS += $(LLIB)/$(call lc,$(2).a)

$(BUILD)/%.o: $(2)/%.cpp

$(BUILD)/%.d: $(2)/%.cpp
	@$(CPP) $(CPPFLAGS) -MP -MM -MT $(BUILD)/$$*.o $$< -o $$@

endef

define lc
$(shell echo $(1) | tr 'A-Z' 'a-z')
endef

define uc
$(shell echo $(1) | tr 'a-z' 'A-Z')
endef

PKGS := tools libSpec libYarr libFei4 libFe65p2 libUtil libEmu libRce libBoc libKU040 libRd53a
$(foreach P,$(PKGS),$(eval $(call addPkg,$(patsubst LIB%,%,$(call uc,$(P))),$(P))))

ifeq (${NETIO_DIR},)
  TOOLS_FILES := $(filter-out tools/testNetioHw.cpp tools/testScan.cpp tools/testBitStream.cpp,$(TOOLS_FILES))
# $(info $(TOOLS_FILES))
endif

TARGETS=$(patsubst %.cpp,$(BIN)/%,$(notdir $(TOOLS_FILES)))
LIB_FILES := $(filter-out $(TOOLS_FILES),$(ALL_FILES))
LIB_OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(notdir $(LIB_FILES)))

ifneq (${NETIO_DIR},)
  PKGS+= libNetioHW
endif

######
##  Targets
all: $(TARGETS) $(LLIB)/libyarr.a

help:
	@echo # Use 'make' to create all programs
	@echo # Use 'make tools' to create the librce_tools.a
	@echo # Use 'make analysis' to create the librce_ana.a
	@echo # Special targets:
	@echo "#   list           - List all targets"
	@echo "#   mkinfo         - List makefile internals"
	@echo "#   help           - Show this help message"

# Object creation
$(BUILD)/%.o:
	@echo [Compiling] $@
	@$(CPP) $(CPPFLAGS) -c $< -o $@

# Target
$(BIN)/%: $(BUILD)/%.o $(LLIB)/libyarr.a
	@echo [Linking] $@
	@$(LD) $(LDFLAGS) $< $(LLIB)/libyarr.a $(PLDFLAGS) -o $@
ifeq ($(ARCH),$(UNAME_P))
	@ln -sf ../$@ bin/$*
endif

# Lib
$(LLIB)/libyarr.a: $(LIB_OBJS)
	@echo [Creating] $@
	@$(AR) rcs $@ $+
ifeq ($(ARCH),$(UNAME_P))
	ln -sf ../$@ lib/
endif

ifneq "$(MAKECMDGOALS)" "clean"
-include $(ALL_DEPS)
endif

clean:
	@$(RM) -f $(ALL_OBJS) $(ALL_DEPS) $(TARGETS) $(LLIB)/libyarr.a
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
	@echo "    $(TOOLS_FILES)"
	@echo Dependency files:
	@echo "    $(TOOLS_DEPS)"
	@echo Object files:
	@echo "    $(TOOLS_OBJS)"
	@echo
	@echo SPEC Files
	@echo =============
	@echo Source files:
	@echo "    $(SPEC_FILES)"
	@echo Dependency files:
	@echo "    $(SPEC_DEPS)"
	@echo Object files:
	@echo "    $(SPEC_OBJS)"
	@echo
	@echo YARR Files
	@echo =============
	@echo Source files:
	@echo "    $(YARR_FILES)"
	@echo Dependency files:
	@echo "    $(YARR_DEPS)"
	@echo Object files:
	@echo "    $(YARR_OBJS)"
	@echo
	@echo Fei4 Files
	@echo =============
	@echo Source files:
	@echo "    $(FEI4_FILES)"
	@echo Dependency files:
	@echo "    $(FEI4_DEPS)"
	@echo Object files:
	@echo "    $(FEI4_OBJS)"
	@echo
	@echo Fe65p2 Files
	@echo =============
	@echo Source files:
	@echo "    $(FE65P2_FILES)"
	@echo Dependency files:
	@echo "    $(FE65P2_DEPS)"
	@echo Object files:
	@echo "    $(FE65P2_OBJS)"
	@echo
	@echo Util Files
	@echo =============
	@echo Source files:
	@echo "    $(UTIL_FILES)"
	@echo Dependency files:
	@echo "    $(UTIL_DEPS)"
	@echo Object files:
	@echo "    $(UTIL_OBJS)"
	@echo
	@echo Emu Files
	@echo =============
	@echo Source files:
	@echo "    $(EMU_FILES)"
	@echo Dependency files:
	@echo "    $(EMU_DEPS)"
	@echo Object files:
	@echo "    $(EMU_OBJS)"
	@echo
	@echo

.PHONY: clean distclean mkinfo help
