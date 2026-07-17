set script_dir [file dirname [file normalize [info script]]]
set project_dir [file join $script_dir build vivado]
set output_dir [file join $script_dir build output]
set package_project_dir [file join $script_dir build ip_packager]
set ip_repo_dir [file join $script_dir build ip_repo]

foreach build_dir [list $project_dir $output_dir $package_project_dir $ip_repo_dir] {
    if {[file exists $build_dir]} {
        file delete -force $build_dir
    }
}
file mkdir $output_dir

source [file join $script_dir package_axi_iir_ip.tcl]
package_axi_iir_ip $script_dir $package_project_dir $ip_repo_dir

create_project -force zynq7020_platform $project_dir -part xc7z020clg400-2
set_property ip_repo_paths $ip_repo_dir [current_project]
update_ip_catalog
set_msg_config -id {Synth 8-3331} -new_severity INFO
set_msg_config -id {Synth 8-3332} -new_severity INFO
set_msg_config -id {Synth 8-350} -new_severity INFO
set_msg_config -id {Synth 8-6014} -new_severity INFO
set_msg_config -id {Constraints 18-5210} -new_severity INFO
set_msg_config -id {Designutils 20-3303} -new_severity INFO
set_msg_config -id {IP_Flow 19-4994} -new_severity INFO
set_msg_config -id {IP_Flow 19-4995} -new_severity INFO

source [file join $script_dir reference_bd.tcl]

set_property CONFIG.PCW_PRESET_BANK1_VOLTAGE {LVCMOS 1.8V} \
    [get_bd_cells processing_system7_0]
set_property -dict [list \
    CONFIG.PCW_MIO_48_IOTYPE {LVCMOS 1.8V} \
    CONFIG.PCW_MIO_49_IOTYPE {LVCMOS 1.8V}] [get_bd_cells processing_system7_0]

set axi_uartlite_0 [create_bd_cell -type ip -vlnv xilinx.com:ip:axi_uartlite:2.0 axi_uartlite_0]
set_property -dict [list \
    CONFIG.C_BAUDRATE {115200} \
    CONFIG.C_DATA_BITS {8} \
    CONFIG.C_USE_PARITY {0} \
    CONFIG.C_ODD_PARITY {0}] $axi_uartlite_0

set axi_iir_3p3z_0 [create_bd_cell -type ip \
    -vlnv maxli.local:user:axi_iir_3p3z:1.0 axi_iir_3p3z_0]

set_property CONFIG.NUM_MI {4} [get_bd_cells ps7_0_axi_periph]
connect_bd_intf_net [get_bd_intf_pins axi_uartlite_0/S_AXI] \
                    [get_bd_intf_pins ps7_0_axi_periph/M02_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_iir_3p3z_0/S_AXI] \
                    [get_bd_intf_pins ps7_0_axi_periph/M03_AXI]
make_bd_intf_pins_external [get_bd_intf_pins axi_uartlite_0/UART]

connect_bd_net [get_bd_pins processing_system7_0/FCLK_CLK0] \
               [get_bd_pins axi_uartlite_0/s_axi_aclk] \
               [get_bd_pins axi_iir_3p3z_0/s_axi_aclk] \
               [get_bd_pins ps7_0_axi_periph/M02_ACLK]
connect_bd_net [get_bd_pins processing_system7_0/FCLK_CLK0] \
               [get_bd_pins ps7_0_axi_periph/M03_ACLK]
connect_bd_net [get_bd_pins rst_ps7_0_50M/peripheral_aresetn] \
               [get_bd_pins axi_uartlite_0/s_axi_aresetn] \
               [get_bd_pins axi_iir_3p3z_0/s_axi_aresetn] \
               [get_bd_pins ps7_0_axi_periph/M02_ARESETN]
connect_bd_net [get_bd_pins rst_ps7_0_50M/peripheral_aresetn] \
               [get_bd_pins ps7_0_axi_periph/M03_ARESETN]

create_bd_addr_seg -range 0x00010000 -offset 0x40600000 \
    [get_bd_addr_spaces processing_system7_0/Data] \
    [get_bd_addr_segs axi_uartlite_0/S_AXI/Reg] SEG_axi_uartlite_0_Reg
create_bd_addr_seg -range 0x00010000 -offset 0x43C00000 \
    [get_bd_addr_spaces processing_system7_0/Data] \
    [get_bd_addr_segs axi_iir_3p3z_0/S_AXI/Reg] SEG_axi_iir_3p3z_0_Reg

validate_bd_design
save_bd_design

set bd_file [get_files design_1.bd]
generate_target all $bd_file
make_wrapper -files $bd_file -top
add_files -norecurse [file join $project_dir zynq7020_platform.srcs sources_1 bd design_1 hdl design_1_wrapper.v]
add_files -fileset constrs_1 -norecurse [file join $script_dir zynq7020_platform.xdc]
set_property top design_1_wrapper [current_fileset]

launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

set impl_status [get_property STATUS [get_runs impl_1]]
set impl_progress [get_property PROGRESS [get_runs impl_1]]
if {$impl_progress ne "100%"} {
    error "PL implementation failed: status=$impl_status progress=$impl_progress"
}

open_run impl_1
report_drc -file [file join $output_dir drc.rpt]
report_timing_summary -file [file join $output_dir timing_summary.rpt]
report_utilization -file [file join $output_dir utilization.rpt]

set bit_file [file join $project_dir zynq7020_platform.runs impl_1 design_1_wrapper.bit]
if {![file exists $bit_file]} {
    error "PL bitstream not found: $bit_file"
}
file copy -force $bit_file [file join $output_dir zynq7020_platform.bit]
set ps7_init_file [file join $project_dir zynq7020_platform.srcs sources_1 bd design_1 ip \
                        design_1_processing_system7_0_0 ps7_init.tcl]
file copy -force $ps7_init_file [file join $output_dir ps7_init.tcl]
write_hwdef -force -file [file join $output_dir zynq7020_platform.hwdef]
write_sysdef -force \
    -hwdef [file join $output_dir zynq7020_platform.hwdef] \
    -bitfile [file join $output_dir zynq7020_platform.bit] \
    -file [file join $output_dir zynq7020_platform.hdf]

puts "PL_BUILD_RESULT status=PASS bit=$bit_file"
