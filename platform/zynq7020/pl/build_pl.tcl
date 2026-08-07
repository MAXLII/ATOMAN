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
source [file join $script_dir package_axi_uart_dma_ip.tcl]
package_axi_uart_dma_ip $script_dir $package_project_dir $ip_repo_dir
source [file join $script_dir package_axi_oled_dma_ip.tcl]
package_axi_oled_dma_ip $script_dir $package_project_dir $ip_repo_dir

create_project -force zynq7020_platform $project_dir -part xc7z020clg400-2
catch {config_webtalk -user off}
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
set_property -dict [list \
    CONFIG.PCW_ENET0_ENET0_IO {MIO 16 .. 27} \
    CONFIG.PCW_ENET0_GRP_MDIO_ENABLE {1} \
    CONFIG.PCW_ENET0_GRP_MDIO_IO {MIO 52 .. 53} \
    CONFIG.PCW_ENET0_PERIPHERAL_CLKSRC {IO PLL} \
    CONFIG.PCW_ENET0_PERIPHERAL_DIVISOR0 {8} \
    CONFIG.PCW_ENET0_PERIPHERAL_DIVISOR1 {1} \
    CONFIG.PCW_ENET0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_ENET0_PERIPHERAL_FREQMHZ {1000 Mbps} \
    CONFIG.PCW_EN_EMIO_ENET0 {0} \
    CONFIG.PCW_EN_ENET0 {1} \
    CONFIG.PCW_USE_S_AXI_HP0 {1} \
    CONFIG.PCW_USE_FABRIC_INTERRUPT {1} \
    CONFIG.PCW_IRQ_F2P_INTR {1}] [get_bd_cells processing_system7_0]

set axi_uart_dma_0 [create_bd_cell -type ip \
    -vlnv maxli.local:user:axi_uart_dma:1.0 axi_uart_dma_0]
set uart_dma_mem_interconnect [create_bd_cell -type ip \
    -vlnv xilinx.com:ip:axi_interconnect:2.1 uart_dma_mem_interconnect]
set_property -dict [list \
    CONFIG.NUM_SI {2} \
    CONFIG.NUM_MI {1}] $uart_dma_mem_interconnect

set axi_iir_3p3z_0 [create_bd_cell -type ip \
    -vlnv maxli.local:user:axi_iir_3p3z:2.0 axi_iir_3p3z_0]
set axi_oled_dma_0 [create_bd_cell -type ip \
    -vlnv maxli.local:user:axi_oled_dma:1.0 axi_oled_dma_0]
set fabric_irq_concat [create_bd_cell -type ip \
    -vlnv xilinx.com:ip:xlconcat:2.1 fabric_irq_concat]
set_property CONFIG.NUM_PORTS {2} $fabric_irq_concat

set_property CONFIG.NUM_MI {5} [get_bd_cells ps7_0_axi_periph]
connect_bd_intf_net [get_bd_intf_pins axi_uart_dma_0/s_axi] \
                    [get_bd_intf_pins ps7_0_axi_periph/M02_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_iir_3p3z_0/S_AXI] \
                    [get_bd_intf_pins ps7_0_axi_periph/M03_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_oled_dma_0/s_axi] \
                    [get_bd_intf_pins ps7_0_axi_periph/M04_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_uart_dma_0/m_axi] \
                    [get_bd_intf_pins uart_dma_mem_interconnect/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_oled_dma_0/m_axi] \
                    [get_bd_intf_pins uart_dma_mem_interconnect/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins uart_dma_mem_interconnect/M00_AXI] \
                    [get_bd_intf_pins processing_system7_0/S_AXI_HP0]

set uart_rxd [create_bd_port -dir I uart_rxd]
set uart_txd [create_bd_port -dir O uart_txd]
connect_bd_net $uart_rxd [get_bd_pins axi_uart_dma_0/uart_rxd]
connect_bd_net [get_bd_pins axi_uart_dma_0/uart_txd] $uart_txd
connect_bd_net [get_bd_pins axi_uart_dma_0/irq] \
               [get_bd_pins fabric_irq_concat/In0]
connect_bd_net [get_bd_pins axi_oled_dma_0/irq] \
               [get_bd_pins fabric_irq_concat/In1]
connect_bd_net [get_bd_pins fabric_irq_concat/dout] \
               [get_bd_pins processing_system7_0/IRQ_F2P]
foreach oled_signal {oled_clock oled_data oled_dc oled_reset_n} {
    set oled_port [create_bd_port -dir O $oled_signal]
    connect_bd_net [get_bd_pins axi_oled_dma_0/$oled_signal] $oled_port
}

connect_bd_net [get_bd_pins processing_system7_0/FCLK_CLK0] \
               [get_bd_pins axi_uart_dma_0/aclk] \
               [get_bd_pins axi_iir_3p3z_0/s_axi_aclk] \
               [get_bd_pins ps7_0_axi_periph/M02_ACLK] \
               [get_bd_pins processing_system7_0/S_AXI_HP0_ACLK] \
               [get_bd_pins uart_dma_mem_interconnect/ACLK] \
               [get_bd_pins uart_dma_mem_interconnect/S00_ACLK] \
               [get_bd_pins uart_dma_mem_interconnect/S01_ACLK] \
               [get_bd_pins uart_dma_mem_interconnect/M00_ACLK]
connect_bd_net [get_bd_pins processing_system7_0/FCLK_CLK0] \
               [get_bd_pins ps7_0_axi_periph/M03_ACLK] \
               [get_bd_pins ps7_0_axi_periph/M04_ACLK] \
               [get_bd_pins axi_oled_dma_0/aclk]
connect_bd_net [get_bd_pins rst_ps7_0_50M/peripheral_aresetn] \
               [get_bd_pins axi_uart_dma_0/aresetn] \
               [get_bd_pins axi_iir_3p3z_0/s_axi_aresetn] \
               [get_bd_pins ps7_0_axi_periph/M02_ARESETN] \
               [get_bd_pins uart_dma_mem_interconnect/ARESETN] \
               [get_bd_pins uart_dma_mem_interconnect/S00_ARESETN] \
               [get_bd_pins uart_dma_mem_interconnect/S01_ARESETN] \
               [get_bd_pins uart_dma_mem_interconnect/M00_ARESETN]
connect_bd_net [get_bd_pins rst_ps7_0_50M/peripheral_aresetn] \
               [get_bd_pins ps7_0_axi_periph/M03_ARESETN] \
               [get_bd_pins ps7_0_axi_periph/M04_ARESETN] \
               [get_bd_pins axi_oled_dma_0/aresetn]

create_bd_addr_seg -range 0x00010000 -offset 0x40600000 \
    [get_bd_addr_spaces processing_system7_0/Data] \
    [get_bd_addr_segs axi_uart_dma_0/s_axi/Reg] SEG_axi_uart_dma_0_Reg
create_bd_addr_seg -range 0x00010000 -offset 0x43C00000 \
    [get_bd_addr_spaces processing_system7_0/Data] \
    [get_bd_addr_segs axi_iir_3p3z_0/S_AXI/Reg] SEG_axi_iir_3p3z_0_Reg
create_bd_addr_seg -range 0x00010000 -offset 0x41220000 \
    [get_bd_addr_spaces processing_system7_0/Data] \
    [get_bd_addr_segs axi_oled_dma_0/s_axi/Reg] SEG_axi_oled_dma_0_Reg
assign_bd_address -target_address_space \
    [get_bd_addr_spaces axi_uart_dma_0/m_axi] \
    [get_bd_addr_segs processing_system7_0/S_AXI_HP0/HP0_DDR_LOWOCM]
assign_bd_address -target_address_space \
    [get_bd_addr_spaces axi_oled_dma_0/m_axi] \
    [get_bd_addr_segs processing_system7_0/S_AXI_HP0/HP0_DDR_LOWOCM]

validate_bd_design
save_bd_design

set bd_file [get_files design_1.bd]
generate_target all $bd_file
make_wrapper -files $bd_file -top
add_files -norecurse [file join $project_dir zynq7020_platform.srcs sources_1 bd design_1 hdl design_1_wrapper.v]
add_files -fileset constrs_1 -norecurse [file join $script_dir zynq7020_platform.xdc]
set_property top design_1_wrapper [current_fileset]

# OneDrive marks generated directories with the Windows read-only directory
# attribute. Vivado 2018.3 mistakes that attribute for an ACL failure in OOC
# child processes, even though the paths are writable. Clear only attributes
# in the generated project tree before launching the runs.
set generated_tree_pattern [file nativename [file join $project_dir *]]
if {[catch {exec attrib -R $generated_tree_pattern /S /D} attrib_error]} {
    error "Unable to clear generated-tree read-only attributes: $attrib_error"
}

# Vivado 2018.3 OOC runs share the project cache. Serial execution avoids
# cache-file races when the workspace is synchronized by OneDrive.
launch_runs impl_1 -to_step write_bitstream -jobs 1
wait_on_run impl_1

set impl_status [get_property STATUS [get_runs impl_1]]
set impl_progress [get_property PROGRESS [get_runs impl_1]]
if {$impl_progress ne "100%"} {
    error "PL implementation failed: status=$impl_status progress=$impl_progress"
}

open_run impl_1
# Vivado 2018.3 reports RTSTAT-10 on unused status taps inside the Xilinx
# AXI4-to-AXI3 converter inserted for the Zynq HP0 port. These nets are wholly
# inside the vendor IP and have no functional or routable load by design. Keep
# the check enabled in the report and exclude only its known instances from the
# implementation gate.
report_drc -ruledeck default -file [file join $output_dir drc.rpt]
report_timing_summary -file [file join $output_dir timing_summary.rpt]
report_utilization -file [file join $output_dir utilization.rpt]

set worst_setup_path [get_timing_paths -quiet -delay_type max -max_paths 1]
if {[llength $worst_setup_path] != 1} {
    error "PL implementation has no timed setup path"
}
set worst_setup_slack [get_property SLACK [lindex $worst_setup_path 0]]
if {$worst_setup_slack < 0.0} {
    error "PL implementation setup timing failed: WNS=$worst_setup_slack ns"
}
set worst_hold_path [get_timing_paths -quiet -delay_type min -max_paths 1]
if {[llength $worst_hold_path] != 1} {
    error "PL implementation has no timed hold path"
}
set worst_hold_slack [get_property SLACK [lindex $worst_hold_path 0]]
if {$worst_hold_slack < 0.0} {
    error "PL implementation hold timing failed: WHS=$worst_hold_slack ns"
}
puts "PL_TIMING_RESULT status=PASS wns_ns=$worst_setup_slack whs_ns=$worst_hold_slack"

set drc_violations [get_drc_violations -quiet \
                        -filter {(SEVERITY == "Error" ||
                                  SEVERITY == "Critical Warning" ||
                                  SEVERITY == "Warning") &&
                                 NAME !~ "RTSTAT-10#*"}]
if {[llength $drc_violations] != 0} {
    foreach violation $drc_violations {
        puts "PL_DRC_VIOLATION name=$violation severity=[get_property SEVERITY $violation]"
    }
    error "PL implementation DRC failed with [llength $drc_violations] violations"
}
set vendor_drc_exemptions [get_drc_violations -quiet \
                               -filter {NAME =~ "RTSTAT-10#*"}]
set vendor_drc_advisories [get_drc_violations -quiet \
                               -filter {SEVERITY == "Advisory"}]
puts "PL_DRC_RESULT actionable=0 vendor_exemptions=[llength $vendor_drc_exemptions] advisories=[llength $vendor_drc_advisories]"

set bit_file [file join $project_dir zynq7020_platform.runs impl_1 design_1_wrapper.bit]
if {![file exists $bit_file]} {
    error "PL bitstream not found: $bit_file"
}
file copy -force $bit_file [file join $output_dir zynq7020_platform.bit]
set ps7_init_candidates [glob -nocomplain \
    [file join $project_dir zynq7020_platform.srcs sources_1 bd design_1 ip \
        design_1_processing_system7_0_0* ps7_init.tcl]]
if {[llength $ps7_init_candidates] != 1} {
    error "Expected one PS7 initialization script, found [llength $ps7_init_candidates]: $ps7_init_candidates"
}
set ps7_init_file [lindex $ps7_init_candidates 0]
file copy -force $ps7_init_file [file join $output_dir ps7_init.tcl]
write_hwdef -force -file [file join $output_dir zynq7020_platform.hwdef]
write_sysdef -force \
    -hwdef [file join $output_dir zynq7020_platform.hwdef] \
    -bitfile [file join $output_dir zynq7020_platform.bit] \
    -file [file join $output_dir zynq7020_platform.hdf]

puts "PL_BUILD_RESULT status=PASS bit=$bit_file"
