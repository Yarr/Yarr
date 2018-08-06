# To list file
# ls -1 | xargs -I % echo \"%\",

target = "xilinx"
action = "synthesis"

syn_device = "xc7k160"
syn_grade = "-2"
syn_package = "tfbg676"
syn_top = "top_level"
syn_project = "yarr"
syn_tool = "vivado"

#library = "work"

modules = {
"local" : ["../../../rtl/common","../../../rtl/kintex7","../../../rtl/","../../../ip-cores/kintex7"],
#"git" : ["https://bitbucket.org/levkurilenko/aurora_rx.git"]
}

fetchto = "../../../ip-cores"

files = [
#TOP
"board_pkg.vhd",
"../bram_yarr_trenz.vhd",
"../app_pkg.vhd",
"../app.vhd",
"../trenz.xdc",
"../trenz-fmc-quad-dp-ohio.xdc",
"../xpressk7-timing.xdc",
]

