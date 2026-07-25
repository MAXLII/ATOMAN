set script_dir [file dirname [file normalize [info script]]]
set project_dir [file join $script_dir build vivado]
set output_dir [file join $script_dir build output]
set project_file [file join $project_dir zynq7020_platform.xpr]

open_project $project_file
set impl_status [get_property STATUS [get_runs impl_1]]
set impl_progress [get_property PROGRESS [get_runs impl_1]]
if {$impl_progress ne "100%"} {
    error "PL implementation is incomplete: status=$impl_status progress=$impl_progress"
}

open_run impl_1
report_drc -ruledeck default -file [file join $output_dir drc.rpt]
report_timing_summary -file [file join $output_dir timing_summary.rpt]
report_utilization -file [file join $output_dir utilization.rpt]

set worst_path [get_timing_paths -quiet -delay_type max -max_paths 1]
if {[llength $worst_path] != 1} {
    error "PL implementation has no timed setup path"
}
set worst_slack [get_property SLACK [lindex $worst_path 0]]
if {$worst_slack < 0.0} {
    error "PL implementation setup timing failed: WNS=$worst_slack ns"
}

set drc_violations [get_drc_violations -quiet \
                        -filter {NAME !~ "RTSTAT-10#*"}]
if {[llength $drc_violations] != 0} {
    foreach violation $drc_violations {
        puts "PL_DRC_VIOLATION name=$violation severity=[get_property SEVERITY $violation]"
    }
    error "PL implementation DRC failed with [llength $drc_violations] violations"
}
set vendor_drc_exemptions [get_drc_violations -quiet \
                               -filter {NAME =~ "RTSTAT-10#*"}]

set bit_file [file join $project_dir zynq7020_platform.runs impl_1 design_1_wrapper.bit]
if {![file exists $bit_file]} {
    error "PL bitstream not found: $bit_file"
}
file copy -force $bit_file [file join $output_dir zynq7020_platform.bit]
set ps7_init_candidates [glob -nocomplain \
    [file join $project_dir zynq7020_platform.srcs sources_1 bd design_1 ip \
        design_1_processing_system7_0_0* ps7_init.tcl]]
if {[llength $ps7_init_candidates] != 1} {
    error "Expected one PS7 initialization script, found [llength $ps7_init_candidates]"
}
file copy -force [lindex $ps7_init_candidates 0] \
    [file join $output_dir ps7_init.tcl]
write_hwdef -force -file [file join $output_dir zynq7020_platform.hwdef]
write_sysdef -force \
    -hwdef [file join $output_dir zynq7020_platform.hwdef] \
    -bitfile [file join $output_dir zynq7020_platform.bit] \
    -file [file join $output_dir zynq7020_platform.hdf]

puts "PL_FINALIZE_RESULT status=PASS actionable_drc=0 vendor_exemptions=[llength $vendor_drc_exemptions] wns_ns=$worst_slack"
exit
