if {$argc != 2} {
    error "usage: xsct build_fsbl.tcl <hardware.hdf> <workspace>"
}

set hardware_definition [file normalize [lindex $argv 0]]
set workspace [file normalize [lindex $argv 1]]

if {![file exists $hardware_definition]} {
    error "hardware definition not found: $hardware_definition"
}

setws $workspace
createhw -name hw_platform -hwspec $hardware_definition
createbsp -name fsbl_bsp -hwproject hw_platform -proc ps7_cortexa9_0 -os standalone
setlib -bsp fsbl_bsp -lib xilffs
regenbsp -bsp fsbl_bsp
createapp -name fsbl -hwproject hw_platform -proc ps7_cortexa9_0 \
    -os standalone -lang C -app {Zynq FSBL} -bsp fsbl_bsp
projects -build -type all

set fsbl_elf [file join $workspace fsbl Debug fsbl.elf]
if {![file exists $fsbl_elf]} {
    error "FSBL build did not produce: $fsbl_elf"
}
puts "FSBL_BUILD_RESULT status=PASS elf=$fsbl_elf"
