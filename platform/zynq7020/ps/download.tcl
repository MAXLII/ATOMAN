proc download_platform {script_dir build_name firmware_name} {
    set build_dir [file join $script_dir $build_name]
    set pl_output_dir [file join $script_dir pl build output]

    connect -url tcp:127.0.0.1:3121
    targets -set -nocase -filter {name =~ "APU*"} -index 0
    rst -system
    after 3000
    targets -set -nocase -filter {name =~ "APU*"} -index 0
    loadhw -hw [file join $pl_output_dir zynq7020_platform.hdf] \
        -mem-ranges [list {0x40000000 0xbfffffff}]
    configparams force-mem-access 1
    ps7_init
    ps7_post_config
    fpga -file [file join $pl_output_dir zynq7020_platform.bit]
    targets -set -nocase -filter {name =~ "ARM*#0"} -index 0
    dow [file join $build_dir $firmware_name]
    configparams force-mem-access 0
    con
    disconnect
}

set script_dir [file dirname [file normalize [info script]]]
set ps7_init_file [file join $script_dir pl build output ps7_init.tcl]
set srtos_mode 0
if {[llength $argv] > 0} {
    set srtos_mode [lindex $argv 0]
}
if {$srtos_mode eq "1"} {
    set build_name build_srtos
    set firmware_name zynq7020_section_comm_srtos.elf
} else {
    set build_name build
    set firmware_name zynq7020_section_comm.elf
}
if {[catch {
    source $ps7_init_file
    download_platform $script_dir $build_name $firmware_name
} download_error]} {
    puts stderr "DOWNLOAD_RESULT status=FAIL error=$download_error"
    catch {configparams force-mem-access 0}
    catch {disconnect}
    exit 1
}

puts "DOWNLOAD_RESULT status=PASS srtos=$srtos_mode"
exit 0
