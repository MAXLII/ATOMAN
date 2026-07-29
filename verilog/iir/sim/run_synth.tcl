set script_dir [file dirname [file normalize [info script]]]
set verilog_dir [file dirname $script_dir]
set repo_root [file dirname [file dirname $verilog_dir]]
set output_dir [file join $repo_root build verilog_synth]

if {[file exists $output_dir]} {
    file delete -force $output_dir
}
file mkdir $output_dir

create_project -in_memory -part xc7z020clg400-2
read_verilog [file join $verilog_dir rtl iir_3p3z_core.v]
synth_design -mode out_of_context -top iir_3p3z_core \
    -part xc7z020clg400-2 -flatten_hierarchy rebuilt
create_clock -name pl_clk -period 20.000 [get_ports clk]
set_property HD.CLK_SRC BUFGCTRL_X0Y0 [get_ports clk]
set_false_path -from [get_ports resetn]
opt_design

report_drc -file [file join $output_dir iir_3p3z_core_drc.rpt]
report_timing_summary -delay_type max -max_paths 10 \
    -file [file join $output_dir iir_3p3z_core_timing.rpt]
report_utilization -file [file join $output_dir iir_3p3z_core_utilization.rpt]

set actionable_violations [list]
foreach violation [get_drc_violations -quiet] {
    # A Zynq out-of-context PL core deliberately has no PS7 instance. The
    # integrated platform build checks ZPS7-1 later; no other DRC is exempt.
    if {![string match "ZPS7-1#*" $violation]} {
        lappend actionable_violations $violation
    }
}
if {[llength $actionable_violations] != 0} {
    foreach violation $actionable_violations {
        puts "PL_CORE_DRC_VIOLATION name=$violation severity=[get_property SEVERITY $violation]"
    }
    error "Standalone IIR core DRC failed with [llength $actionable_violations] actionable violations"
}

set worst_path [get_timing_paths -quiet -delay_type max -max_paths 1]
if {[llength $worst_path] != 1} {
    error "Standalone IIR core has no timed setup path"
}
set worst_slack [get_property SLACK [lindex $worst_path 0]]
if {$worst_slack < 0.0} {
    error "Standalone IIR core setup timing failed: WNS=$worst_slack ns"
}

set dsp_count [llength [get_cells -hierarchical -quiet \
                           -filter {REF_NAME =~ DSP48*}]]
puts "PL_CORE_SYNTH_RESULT status=PASS actionable_drc=0 ooc_exempt=ZPS7-1 period_ns=20.000 wns_ns=$worst_slack dsp=$dsp_count"
exit
