GIT_VERS = $(shell git rev-parse --short HEAD)
SUBDIRS = $(patsubst %/,%,$(shell ls -d */))
$(info $(SUBDIRS))

all: vers $(SUBDIRS)

vers:
	./make_version.sh

$(SUBDIRS): vers 
	-@echo "Building $@/tef1001_R1" && cd $@/tef1001_R1 && $(MAKE) > /dev/null 2>&1
	-@echo "Building $@/tef1001_R2" && cd $@/tef1001_R2 && $(MAKE) > /dev/null 2>&1
	-@echo "Building $@/xpressk7_160" && cd $@/xpressk7_160 && $(MAKE) > /dev/null 2>&1
	-@echo "Building $@/xpressk7_325" && cd $@/xpressk7_325 && $(MAKE) > /dev/null 2>&1
	-@echo "Finished $@/tef1001_R1 ... moving bit file" && cp $@/tef1001_R1/yarr.runs/impl_1/top_level.bit $@/$@_tef1001_R1.bit
	-@echo "Finished $@/tef1001_R2 ... moving bit file" && cp $@/tef1001_R2/yarr.runs/impl_1/top_level.bit $@/$@_tef1001_R2.bit
	-@echo "Finished $@/xpressk7_160 ... moving bit file" && cp $@/xpressk7_160/yarr.runs/impl_1/top_level.bit $@/$@_xpressk7_160.bit
	-@echo "Finished $@/xpressk7_325 ... moving bit file" && cp $@/xpressk7_325/yarr.runs/impl_1/top_level.bit $@/$@_xpressk7_325.bit

hdl:
	for d in $(SUBDIRS); do cd $$d/tef1001_R1 && hdlmake && cd -; done
	for d in $(SUBDIRS); do cd $$d/tef1001_R2 && hdlmake && cd -; done
	for d in $(SUBDIRS); do cd $$d/xpressk7_160 && hdlmake && cd -; done
	for d in $(SUBDIRS); do cd $$d/xpressk7_325 && hdlmake && cd -; done

clean:
	for d in $(SUBDIRS); do make -C $$d/tef1001_R1 clean; done
	for d in $(SUBDIRS); do make -C $$d/tef1001_R2 clean; done
	for d in $(SUBDIRS); do make -C $$d/xpressk7_160 clean; done
	for d in $(SUBDIRS); do make -C $$d/xpressk7_325 clean; done

deploy:
	ssh lxplus.cern.ch "mkdir -p ~/eos/www/yarr/firmware/$(GIT_VERS) &&\
		rm -f ~/eos/www/yarr/firmware/latest &&\
		ln -s ~/eos/www/yarr/firmware/$(GIT_VERS) ~/eos/www/yarr/firmware/latest"
	rsync -av --include '*/' --include '*_*_*.bit' --exclude '*' --prune-empty-dirs ./ lxplus.cern.ch:~/eos/www/yarr/firmware/$(GIT_VERS)

.PHONY: all clean hdl vers $(SUBDIRS)
