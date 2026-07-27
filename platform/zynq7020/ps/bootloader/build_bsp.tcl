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
projects -build -type bsp -name bootloader_bsp

set bsp_library [file join $workspace bootloader_bsp ps7_cortexa9_0 lib libxil.a]
if {![file exists $bsp_library]} {
    error "BSP build did not produce: $bsp_library"
}
puts "BOOTLOADER_BSP_RESULT status=PASS library=$bsp_library"
