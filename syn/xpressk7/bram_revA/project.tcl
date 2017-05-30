create_project yarr ./
set_property part xc7k160tfbg676-2 [current_project]
set_property target_language VHDL [current_project]
set_property top top_level [get_property srcset [current_run]]
source files.tcl
update_compile_order -fileset sources_1
update_compile_order -fileset sim_1
exit
