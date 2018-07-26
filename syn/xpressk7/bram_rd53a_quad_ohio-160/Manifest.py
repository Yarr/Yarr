# To list file
# ls -1 | xargs -I % echo \"%\",


library = "work"

modules = {
"local" : ["../../../rtl/common","../../../rtl/kintex7","../../../rtl/","../../../ip-cores/kintex7"],
#"git" : ["https://bitbucket.org/levkurilenko/aurora_rx.git"]
}

fetchto = "../../../ip-cores"

files = [
#TOP
"../bram_yarr.vhd",
"../app_pkg.vhd",
"board_pkg.vhd",
"../app.vhd",
"../xpressk7.xdc",
#"../xpressk7-ddr3.xdc",
"../xpressk7-fmc-quad-dp-ohio.xdc",
#"../TEF1001-fmc-octa.xdc",
"../xpressk7-timing.xdc",
]




target = "xilinx" 
action = "synthesis" 

syn_device = "xc7k160" 
syn_grade = "-2" 
syn_package = "tfbg676" 
syn_top = "top_level" 
syn_project = "yarr"
syn_tool = "vivado"
