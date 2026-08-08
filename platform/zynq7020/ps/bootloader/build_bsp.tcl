if {$argc != 2} {
    error "usage: xsct build_bsp.tcl <hardware.hdf> <workspace>"
}

set hardware_definition [file normalize [lindex $argv 0]]
set workspace [file normalize [lindex $argv 1]]

if {![file exists $hardware_definition]} {
    error "hardware definition not found: $hardware_definition"
}

setws $workspace
createhw -name hw_platform -hwspec $hardware_definition
createbsp -name bootloader_bsp -hwproject hw_platform \
    -proc ps7_cortexa9_0 -os standalone
setlib -bsp bootloader_bsp -lib lwip202
configbsp -bsp bootloader_bsp \
    phy_link_speed CONFIG_LINKSPEED_AUTODETECT
set bsp_regenerated 0
set bsp_regen_error ""
for {set attempt 1} {$attempt <= 10} {incr attempt} {
    if {![catch {regenbsp -bsp bootloader_bsp} bsp_regen_error]} {
        set bsp_regenerated 1
        break
    }
    puts "BOOTLOADER_BSP_RETRY attempt=$attempt error=$bsp_regen_error"
    after 1000
}
if {!$bsp_regenerated} {
    error "BSP regeneration failed after 10 attempts: $bsp_regen_error"
}
projects -build -type bsp -name bootloader_bsp

set bsp_root [file join $workspace bootloader_bsp ps7_cortexa9_0]
set xil_library [file join $bsp_root lib libxil.a]
set lwip_library [file join $bsp_root lib liblwip4.a]
if {![file exists $xil_library] || ![file exists $lwip_library]} {
    error "BSP build did not produce libxil.a and liblwip4.a under: $bsp_root"
}
puts "BOOTLOADER_BSP_RESULT status=PASS root=$bsp_root"
