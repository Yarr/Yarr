files = ["ip_cores/clk_gen.vhd",
         "ip_cores/ila.vhd",
         "ip_cores/ila_icon.vhd",
         "ip_cores/fifo_64x512.vhd",
         "ip_cores/fifo_32x512.vhd"]

modules = { "local" : ["common",
            "gn4124-core",
            "ddr3-sp6-core"],
            "git" : "git://ohwr.org/hdl-core-lib/general-cores.git"}

