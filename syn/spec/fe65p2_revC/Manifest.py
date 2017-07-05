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

files = ["yarr_fe65p2_revC.ucf",
         "../top_yarr_spec_fe65p2.vhd"]

fetchto = "../../../ip-cores/spartan6"
