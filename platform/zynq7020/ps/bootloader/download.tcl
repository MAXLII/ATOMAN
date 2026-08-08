proc download_bootloader {script_dir} {
    set platform_dir [file dirname [file dirname $script_dir]]
    set pl_output_dir [file join $platform_dir pl build output]
    set bootloader_elf [file join $script_dir build zynq7020_bootloader.elf]

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
    dow $bootloader_elf
    mwr 0x0002FFF4 0x00000001
    mwr 0x0002FFF8 0xBDABB3BB
    mwr 0x0002FFF0 0x42544C44
    configparams force-mem-access 0
    con
    disconnect
}

set script_dir [file dirname [file normalize [info script]]]
set platform_dir [file dirname [file dirname $script_dir]]
set ps7_init_file [file join $platform_dir pl build output ps7_init.tcl]

if {[catch {
    source $ps7_init_file
    download_bootloader $script_dir
} download_error]} {
    puts stderr "BOOTLOADER_DOWNLOAD_RESULT status=FAIL error=$download_error"
    catch {configparams force-mem-access 0}
    catch {disconnect}
    exit 1
}

puts "BOOTLOADER_DOWNLOAD_RESULT status=PASS"
exit 0
