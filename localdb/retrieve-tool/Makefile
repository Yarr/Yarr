#################################
# Author: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for YARR
# Description: Makefile for retrieve tool
#################################

TOOLNAME="LocalDB Tool - Retrieve"
TARGETDIR?=/
SRC=src/
BINFILE=usr/local/bin/localdbtool-retrieve
LIBFILE=usr/local/lib/localdb-tools/modules/
LIBS=$(wildcard ${SRC}${LIBFILE}*.py)
BASHCOMPLETION=usr/local/lib/localdb-tools/bash-completion/completions/localdbtool-retrieve
ENABLEFILE=usr/local/lib/localdb-tools/enable

## DO NOT TOUCH THESE
BINFILEDIR = $(patsubst %/,%,$(dir ${BINFILE}))
LIBFILEDIR = $(patsubst %/,%,$(dir ${LIBFILE}))
BASHCOMPLETIONDIR = $(patsubst %/,%,$(dir ${BASHCOMPLETION}))
ENABLEDIR = $(patsubst %/,%,$(dir ${ENABLEFILE}))

all:
	@echo "Makefile usage"
	@echo ""
	@echo " Configuration is done using environment variable"
	@echo ""
	@echo " TARGETDIR defines where to install ${TOOLNAME}"
	@echo ""
	@echo " The current values are:"
	@echo "  TARGETDIR=${TARGETDIR}"
	@echo ""
	@echo " Once environment variable are set, execute: make install"
	@echo ""

install: check
	@echo "Install ${TOOLNAME} ..."
	@mkdir -p ${TARGETDIR}${BINFILEDIR}
	@cp ${SRC}${BINFILE} ${TARGETDIR}${BINFILE}
	@mkdir -p ${TARGETDIR}${LIBFILEDIR}
	@cp ${LIBS} ${TARGETDIR}${LIBFILE}
	@mkdir -p ${TARGETDIR}${BASHCOMPLETIONDIR}
	@cp ${SRC}${BASHCOMPLETION} ${TARGETDIR}${BASHCOMPLETION}
	@mkdir -p ${TARGETDIR}${ENABLEFILEDIR}
	@cp ${SRC}${ENABLEFILE} ${TARGETDIR}${ENABLEFILE}
	@echo ""

check:
	@/usr/bin/env python3 ../check_python_modules.py

clean:
	@echo "USE 'make uninstall'!"

uninstall:
	@echo "Uninstall ${TOOLNAME} ..."
	@rm ${TARGETDIR}${BINFILE}
	@rm $(subst ${SRC}${LIBFILE},${TARGETDIR}${LIBFILE},$(LIBS))
	@rm ${TARGETDIR}${BASHCOMPLETION}
	@rm ${TARGETDIR}${ENABLEFILE}
	@echo ""

.PHONY: clean check uninstall
