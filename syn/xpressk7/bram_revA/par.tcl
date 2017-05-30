open_project yarr.xpr

reset_run impl_1
launch_runs impl_1
wait_on_run impl_1
set result [get_property STATUS [get_runs impl_1]]
set keyword [lindex [split $result  ] end]
if { $keyword != "Complete!" } {
exit 1
}
exit
