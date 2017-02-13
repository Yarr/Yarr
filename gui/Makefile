UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
	TARGET=YarrGui
	QMAKE=qmake-qt5
endif
ifeq ($(UNAME),Darwin)
	TARGET=YarrGui.app
	QMAKE=qmake
endif
MAKE=make
BUILD=build
BIN=bin

all:
	mkdir -p $(BUILD)
	mkdir -p $(BIN)
	$(QMAKE) -o $(PWD)/$(BUILD)/Makefile YarrGui/YarrGui.pro CONFIG+=debug CONFIG+=x86_64
	$(MAKE) -C $(PWD)/$(BUILD)
	rm -rf $(PWD)/$(BIN)/$(TARGET)
	mv $(PWD)/$(BUILD)/$(TARGET) $(PWD)/$(BIN)/$(TARGET)
	rm -rf $(PWD)/$(BUILD)/$(TARGET)

clean:
	$(MAKE) -C $(PWD)/$(BUILD) clean
	rm -rf $(PWD)/$(BUILD)
	rm -rf $(PWD)/$(BIN)
