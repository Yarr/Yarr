SUBDIRS = $(patsubst %/,%,$(shell ls -d */))
$(info $(SUBDIRS))

all: vers $(SUBDIRS)

vers:
	./make_version.sh

$(SUBDIRS): 
	-@echo "Building $@/tef1001_R1" && cd $@/tef1001_R1 && $(MAKE) 1> buildlog.txt
	-@echo "Building $@/tef1001_R2" && cd $@/tef1001_R2 && $(MAKE) 1> buildlog.txt
	-@echo "Building $@/xpressk7_160" && cd $@/xpressk7_160 && $(MAKE) 1> buildlog.txt
	-@echo "Building $@/xpressk7_325" && cd $@/xpressk7_325 && $(MAKE) 1> buildlog.txt
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

.PHONY: all clean hdl $(SUBDIRS)
