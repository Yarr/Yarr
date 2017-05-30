open_project yarr.xpr

reset_run synth_1
launch_runs synth_1
wait_on_run synth_1
set result [get_property STATUS [get_runs synth_1]]
set keyword [lindex [split $result  ] end]
if { $keyword != "Complete!" } {
exit 1
}
exit
