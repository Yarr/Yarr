# To list file
# ls -1 | xargs -I % echo \"%\",


library = "work"

modules = {
"local" : ["../../../rtl/common","../../../rtl/kintex7","../../../rtl/","../../../ip-cores/kintex7"],
}

files = [
#TOP
"../bram_yarr_fe65p2.vhd",
"../app_pkg.vhd",
"../app_fe65p2.vhd",
"../xpressk7.xdc",
#"../xpressk7-ddr3.xdc",
"../xpressk7-fmc-fe65p2.xdc",
"../xpressk7-timing.xdc",
]




target = "xilinx" 
action = "synthesis" 

syn_device = "xc7k325" 
syn_grade = "-2" 
syn_package = "tfbg676" 
syn_top = "top_level" 
syn_project = "yarr"
syn_tool = "vivado"
