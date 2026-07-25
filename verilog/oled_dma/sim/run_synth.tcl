set script_dir [file dirname [file normalize [info script]]]
set verilog_dir [file dirname $script_dir]
set repo_root [file dirname [file dirname $verilog_dir]]
set output_dir [file join $repo_root build oled_dma_synth]

if {[file exists $output_dir]} {
    file delete -force $output_dir
}
file mkdir $output_dir

create_project -in_memory -part xc7z020clg400-2
read_verilog [file join $verilog_dir src oled_serial_phy.v]
read_verilog [file join $verilog_dir src oled_frame_ram.v]
read_verilog [file join $verilog_dir src oled_frame_dma.v]
read_verilog [file join $verilog_dir src ssd1306_protocol.v]
read_verilog [file join $verilog_dir src axi_oled_dma.v]
synth_design -mode out_of_context -top axi_oled_dma \
    -part xc7z020clg400-2 -flatten_hierarchy rebuilt
create_clock -name pl_clk -period 20.000 [get_ports aclk]
set_property HD.CLK_SRC BUFGCTRL_X0Y0 [get_ports aclk]
set_false_path -from [get_ports aresetn]
opt_design

report_drc -file [file join $output_dir oled_dma_drc.rpt]
report_timing_summary -delay_type max -max_paths 10 \
    -file [file join $output_dir oled_dma_timing.rpt]
report_utilization -file [file join $output_dir oled_dma_utilization.rpt]

set actionable_violations [list]
foreach violation [get_drc_violations -quiet] {
    if {![string match "ZPS7-1#*" $violation]} {
        lappend actionable_violations $violation
    }
}
if {[llength $actionable_violations] != 0} {
    foreach violation $actionable_violations {
        puts "OLED_DMA_DRC_VIOLATION name=$violation severity=[get_property SEVERITY $violation]"
    }
    error "OLED DMA DRC failed with [llength $actionable_violations] violations"
}

set worst_path [get_timing_paths -quiet -delay_type max -max_paths 1]
if {[llength $worst_path] != 1} {
    error "OLED DMA has no timed setup path"
}
set worst_slack [get_property SLACK [lindex $worst_path 0]]
if {$worst_slack < 0.0} {
    error "OLED DMA setup timing failed: WNS=$worst_slack ns"
}

set lut_count [llength [get_cells -hierarchical -quiet -filter {REF_NAME =~ LUT*}]]
set register_count [llength [get_cells -hierarchical -quiet -filter {REF_NAME =~ FD*}]]
set bram_count [llength [get_cells -hierarchical -quiet -filter {REF_NAME =~ RAMB*}]]
puts "OLED_DMA_OOC_SYNTH_RESULT status=PASS actionable_drc=0 ooc_exempt=ZPS7-1 period_ns=20.000 wns_ns=$worst_slack lut=$lut_count registers=$register_count bram=$bram_count"
exit
