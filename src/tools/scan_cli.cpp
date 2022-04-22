//
// Created by wittgen on 3/29/22.
//
#include "ScanConsole.h"

int main(int argc, char *argv[]) {
    ScanConsole con;
    int res=con.init(argc, argv);
    if(res<=0) return res;
    res=con.loadConfig(); if(res!=0) exit(res);
    res=con.initHardware(); if(res!=0) exit(res);
    res=con.configure(); if(res!=0) exit(res);
    res=con.setupScan(); if(res!=0) exit(res);
    con.run();
    con.cleanup();
    con.plot();
    return 0;
}
