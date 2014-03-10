QMAKE=qmake
MAKE=make
BUILD=build
BIN=bin
TARGET=YarrGui.app

all:
	mkdir -p $(BUILD)
	mkdir -p $(BIN)
	$(QMAKE) -o $(PWD)/$(BUILD)/Makefile YarrGui/YarrGui.pro
	$(MAKE) -C $(PWD)/$(BUILD)
	rm -rf $(PWD)/$(BIN)/$(TARGET)
	mv $(PWD)/$(BUILD)/$(TARGET) $(PWD)/$(BIN)/$(TARGET)
	rm -rf $(PWD)/$(BUILD)/$(TARGET)

clean:
	$(MAKE) -C $(PWD)/$(BUILD) clean
	rm -rf $(PWD)/$(BUILD)
	rm -rf $(PWD)/$(BIN)
