proc add_axi_iir_register {address_block name offset access description} {
    set register [ipx::add_register $name $address_block]
    set_property address_offset $offset $register
    set_property size 32 $register
    set_property access $access $register
    set_property description $description $register
    return $register
}

proc package_axi_iir_ip {script_dir package_project_dir ip_repo_dir} {
    set repo_root [file normalize [file join $script_dir .. .. ..]]
    set rtl_dir [file join $repo_root verilog iir rtl]
    set ip_root [file join $ip_repo_dir axi_iir_3p3z_2_0]

    create_project -force axi_iir_3p3z_ip_package $package_project_dir \
        -part xc7z020clg400-2
    set_property target_language Verilog [current_project]
    add_files -norecurse [list \
        [file join $rtl_dir iir_3p3z_core.v] \
        [file join $rtl_dir axi_iir_3p3z.v]]
    set_property top axi_iir_3p3z [current_fileset]
    update_compile_order -fileset sources_1

    ipx::package_project -root_dir $ip_root \
        -vendor maxli.local \
        -library user \
        -taxonomy /UserIP \
        -import_files \
        -set_current true \
        -force

    set core [ipx::current_core]
    set_property name axi_iir_3p3z $core
    set_property display_name {AXI 3P3Z IIR Peripheral} $core
    set_property description \
        {Single-cycle signed Q2.30 3P3Z IIR filter with configurable output limits.} $core
    set_property vendor_display_name {Max.Li} $core
    set_property version 2.0 $core

    set slave_axi [ipx::get_bus_interfaces -quiet S_AXI -of_objects $core]
    if {[llength $slave_axi] != 1} {
        error "AXI IIR IP packaging failed: S_AXI slave interface not found"
    }

    set memory_map [ipx::add_memory_map S_AXI $core]
    set_property slave_memory_map_ref S_AXI $slave_axi
    set address_block [ipx::add_address_block Reg $memory_map]
    set_property base_address 0 $address_block
    set_property range 4096 $address_block
    set_property width 32 $address_block

    set control_reg [add_axi_iir_register $address_block CONTROL 0x00 \
                         write-only {Start, state reset, and done-clear pulses.}]
    foreach {field_name bit_offset field_description} {
        START 0 {Start one filter sample.}
        RESET_STATE 1 {Clear histories, output, and sample count.}
        CLEAR_DONE 2 {Clear the sticky completion flag.}
    } {
        set field [ipx::add_field $field_name $control_reg]
        set_property bit_offset $bit_offset $field
        set_property bit_width 1 $field
        set_property access write-only $field
        set_property description $field_description $field
    }

    set status_reg [add_axi_iir_register $address_block STATUS 0x04 \
                        read-only {Busy, done, saturation, and ready status.}]
    foreach {field_name bit_offset field_description} {
        BUSY 0 {Reserved compatibility status; single-cycle core is never busy.}
        DONE 1 {The output register contains a completed sample.}
        SATURATED 2 {The completed sample was clamped to configured limits.}
        READY 3 {A new start command can be accepted.}
    } {
        set field [ipx::add_field $field_name $status_reg]
        set_property bit_offset $bit_offset $field
        set_property bit_width 1 $field
        set_property access read-only $field
        set_property description $field_description $field
    }

    add_axi_iir_register $address_block INPUT 0x08 read-write \
        {Signed 32-bit input sample.}
    add_axi_iir_register $address_block OUTPUT 0x0C read-only \
        {Signed 32-bit saturated output sample.}
    add_axi_iir_register $address_block B0 0x10 read-write {Signed Q2.30 coefficient b0.}
    add_axi_iir_register $address_block B1 0x14 read-write {Signed Q2.30 coefficient b1.}
    add_axi_iir_register $address_block B2 0x18 read-write {Signed Q2.30 coefficient b2.}
    add_axi_iir_register $address_block B3 0x1C read-write {Signed Q2.30 coefficient b3.}
    add_axi_iir_register $address_block A1 0x20 read-write {Signed Q2.30 coefficient a1.}
    add_axi_iir_register $address_block A2 0x24 read-write {Signed Q2.30 coefficient a2.}
    add_axi_iir_register $address_block A3 0x28 read-write {Signed Q2.30 coefficient a3.}
    add_axi_iir_register $address_block SAMPLE_COUNT 0x2C read-only \
        {Completed sample count since state reset.}
    add_axi_iir_register $address_block VERSION 0x30 read-only {RTL peripheral version.}
    add_axi_iir_register $address_block FORMAT 0x34 read-only \
        {Sample width in bits 15:8 and coefficient fractional bits in bits 7:0.}
    add_axi_iir_register $address_block X1 0x38 read-only {Input history x[n-1].}
    add_axi_iir_register $address_block X2 0x3C read-only {Input history x[n-2].}
    add_axi_iir_register $address_block X3 0x40 read-only {Input history x[n-3].}
    add_axi_iir_register $address_block Y1 0x44 read-only {Output history y[n-1].}
    add_axi_iir_register $address_block Y2 0x48 read-only {Output history y[n-2].}
    add_axi_iir_register $address_block Y3 0x4C read-only {Output history y[n-3].}
    add_axi_iir_register $address_block LIMIT_LOWER 0x50 read-write \
        {Signed lower output limit; reset value is INT32_MIN.}
    add_axi_iir_register $address_block LIMIT_UPPER 0x54 read-write \
        {Signed upper output limit; reset value is INT32_MAX.}

    ipx::create_xgui_files $core
    ipx::update_checksums $core
    ipx::check_integrity $core
    ipx::save_core $core
    close_project
}
