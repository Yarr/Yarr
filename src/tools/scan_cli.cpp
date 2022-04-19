//
// Created by wittgen on 3/29/22.
//
#include "ScanHelper.h"
#include "ScanConsole.h"

int main(int argc, char *argv[]) {
    ScanOpts scanOpts;
    int res=ScanHelper::parseOptions(argc,argv,scanOpts);
    if(res<=0) exit(res);
    ScanConsole con;
    con.init(scanOpts);
    res=con.loadConfig(); if(res!=0) exit(res);
    res=con.initHardware(); if(res!=0) exit(res);
    res=con.configure(); if(res!=0) exit(res);
    res=con.setupScan(); if(res!=0) exit(res);
    con.run();
    con.cleanup();
    con.plot();
    return 0;
}
