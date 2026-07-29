set script_dir [file dirname [file normalize [info script]]]
set verilog_dir [file dirname $script_dir]
set repo_root [file dirname [file dirname $verilog_dir]]
set output_dir [file join $repo_root build uart_dma_synth]

if {[file exists $output_dir]} {
    file delete -force $output_dir
}
file mkdir $output_dir

create_project -in_memory -part xc7z020clg400-2
read_verilog [file join $verilog_dir rtl uart_sync_fifo.v]
read_verilog [file join $verilog_dir rtl uart_serial_core.v]
read_verilog [file join $verilog_dir rtl axi_uart_dma.v]
synth_design -mode out_of_context -top axi_uart_dma \
    -part xc7z020clg400-2 -flatten_hierarchy rebuilt
create_clock -name pl_clk -period 20.000 [get_ports aclk]
set_property HD.CLK_SRC BUFGCTRL_X0Y0 [get_ports aclk]
set_false_path -from [get_ports aresetn]
opt_design

report_drc -file [file join $output_dir uart_dma_drc.rpt]
report_timing_summary -delay_type max -max_paths 10 \
    -file [file join $output_dir uart_dma_timing.rpt]
report_utilization -file [file join $output_dir uart_dma_utilization.rpt]

set actionable_violations [list]
foreach violation [get_drc_violations -quiet] {
    if {![string match "ZPS7-1#*" $violation]} {
        lappend actionable_violations $violation
    }
}
if {[llength $actionable_violations] != 0} {
    foreach violation $actionable_violations {
        puts "UART_DMA_DRC_VIOLATION name=$violation severity=[get_property SEVERITY $violation]"
    }
    error "UART DMA DRC failed with [llength $actionable_violations] actionable violations"
}

set worst_path [get_timing_paths -quiet -delay_type max -max_paths 1]
if {[llength $worst_path] != 1} {
    error "UART DMA has no timed setup path"
}
set worst_slack [get_property SLACK [lindex $worst_path 0]]
if {$worst_slack < 0.0} {
    error "UART DMA setup timing failed: WNS=$worst_slack ns"
}

set lut_count [llength [get_cells -hierarchical -quiet \
                           -filter {REF_NAME =~ LUT*}]]
set register_count [llength [get_cells -hierarchical -quiet \
                                -filter {REF_NAME =~ FD*}]]
puts "UART_DMA_OOC_SYNTH_RESULT status=PASS actionable_drc=0 ooc_exempt=ZPS7-1 period_ns=20.000 wns_ns=$worst_slack lut=$lut_count registers=$register_count"
exit
