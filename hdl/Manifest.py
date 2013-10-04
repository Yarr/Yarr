files = ["ip_cores/clk_gen.vhd",
         "ip_cores/ila.ngc",
         "ip_cores/ila_icon.ngc",
         "ip_cores/fifo_64x512.ngc",
         "ip_cores/fifo_32x512.ngc"]

modules = { "local" : ["common",
            "gn4124-core",
            "ddr3-sp6-core"],
            "git" : "git://ohwr.org/hdl-core-lib/general-cores.git"}

