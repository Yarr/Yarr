QMAKE=qmake-qt4
MAKE=make
BUILD=build
BIN=bin

all:
	mkdir -p $(BUILD)
	mkdir -p $(BIN)
	$(QMAKE) -makefile -o $(BUILD)/Makefile -unix CONFIG+=DEBUG CONFIG+=x86_64 YarrGui/YarrGui.pro
	$(MAKE) -C $(BUILD)
	mv $(BUILD)/YarrGui $(BIN)/YarrGui

clean:
	$(MAKE) -C $(BUILD) clean
	rm -f $(BUILD)/Makefile
	rm -f $(BIN)/YarrGui
