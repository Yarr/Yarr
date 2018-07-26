target = "xilinx"
action = "synthesis"

modules = {"local" : ["../../../rtl",
                    "../../../rtl/spartan6",
                    "../../../ip-cores/spartan6"],}

syn_device = "xc6slx45t"
syn_grade = "-3"
syn_package = "fgg484"
syn_top = "yarr"
syn_tool = "ise"
syn_project = "yarr_spec.xise"

files = ["itk_demo.ucf",
         "../top_yarr_spec.vhd",
         "board_pkg.vhd"]

fetchto = "../../../ip-cores/spartan6"
