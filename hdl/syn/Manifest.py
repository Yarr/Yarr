target = "xilinx"
action = "synthesis"

modules = {"local" : ["../"],}

syn_device = "xc6slx45t"
syn_grade = "-3"
syn_package = "fgg484"
syn_top = "yarr"
syn_tool = "ise"
syn_project = "yarr_spec.xise"

files = ["../yarr_spec_quad_fei4_revB.ucf",
         "../top_yarr_spec.vhd"]

fetchto = "../ip_cores"
