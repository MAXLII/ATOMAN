proc add_oled_dma_register {address_block name offset access description} {
    set register [ipx::add_register $name $address_block]
    set_property address_offset $offset $register
    set_property size 32 $register
    set_property access $access $register
    set_property description $description $register
    return $register
}

proc package_axi_oled_dma_ip {script_dir package_project_dir ip_repo_dir} {
    set repo_root [file normalize [file join $script_dir .. .. ..]]
    set rtl_dir [file join $repo_root verilog oled_dma src]
    set project_dir [file join $package_project_dir axi_oled_dma]
    set ip_root [file join $ip_repo_dir axi_oled_dma_1_0]

    create_project -force axi_oled_dma_ip_package $project_dir \
        -part xc7z020clg400-2
    set_property target_language Verilog [current_project]
    add_files -norecurse [list \
        [file join $rtl_dir oled_serial_phy.v] \
        [file join $rtl_dir oled_frame_ram.v] \
        [file join $rtl_dir oled_frame_dma.v] \
        [file join $rtl_dir ssd1306_protocol.v] \
        [file join $rtl_dir axi_oled_dma.v]]
    set_property top axi_oled_dma [current_fileset]
    update_compile_order -fileset sources_1

    ipx::package_project -root_dir $ip_root \
        -vendor maxli.local \
        -library user \
        -taxonomy /UserIP \
        -import_files \
        -set_current true \
        -force

    set core [ipx::current_core]
    set_property name axi_oled_dma $core
    set_property display_name {AXI OLED Framebuffer DMA} $core
    set_property description \
        {Decoupled OLED controller with DDR framebuffer DMA and serial protocol engine.} $core
    set_property vendor_display_name {Max.Li} $core
    set_property version 1.0 $core

    foreach {name mode} {s_axi slave m_axi master} {
        set interface [ipx::get_bus_interfaces -quiet $name -of_objects $core]
        if {[llength $interface] != 1} {
            error "AXI OLED DMA packaging failed: $name $mode interface not found"
        }
    }

    set slave_axi [ipx::get_bus_interfaces s_axi -of_objects $core]
    set memory_map [ipx::add_memory_map s_axi $core]
    set_property slave_memory_map_ref s_axi $slave_axi
    set clock_interface [ipx::get_bus_interfaces aclk -of_objects $core]
    set associated_busif [ipx::get_bus_parameters ASSOCIATED_BUSIF \
                              -of_objects $clock_interface]
    set_property value {s_axi:m_axi} $associated_busif
    set address_block [ipx::add_address_block Reg $memory_map]
    set_property base_address 0 $address_block
    set_property range 4096 $address_block
    set_property width 32 $address_block

    add_oled_dma_register $address_block CONTROL 0x00 read-write \
        {Enable, reset, refresh, clear, and display control.}
    add_oled_dma_register $address_block STATUS 0x04 read-only \
        {Initialization, DMA, protocol, display, and error status.}
    add_oled_dma_register $address_block FB_BASE 0x08 read-write \
        {1024-byte-aligned DDR framebuffer address.}
    add_oled_dma_register $address_block SPI_DIV 0x0C read-write \
        {Serial physical-layer half-period divider.}
    add_oled_dma_register $address_block REFRESH_PERIOD 0x10 read-write \
        {Automatic refresh period in peripheral clock cycles.}
    add_oled_dma_register $address_block CONTRAST 0x14 read-write \
        {SSD1306 contrast value.}
    add_oled_dma_register $address_block IRQ_STATUS 0x18 read-write \
        {Write-one-to-clear error interrupt status.}
    add_oled_dma_register $address_block IRQ_ENABLE 0x1C read-write \
        {Error interrupt enable mask.}
    add_oled_dma_register $address_block FRAME_COUNT 0x20 read-only \
        {Successfully transmitted framebuffer count.}
    add_oled_dma_register $address_block CLEAR_COUNT 0x24 read-only \
        {Successfully transmitted clear-frame count.}
    add_oled_dma_register $address_block AXI_ERRORS 0x28 read-only \
        {AXI read error count.}
    add_oled_dma_register $address_block COMMAND_ERRORS 0x2C read-only \
        {Configuration, command, and protocol error count.}
    add_oled_dma_register $address_block DMA_STOP 0x30 read-only \
        {Latched DMA stop reason.}
    add_oled_dma_register $address_block VERSION 0x34 read-only \
        {RTL peripheral version.}
    add_oled_dma_register $address_block GEOMETRY 0x38 read-only \
        {Framebuffer height and width.}
    add_oled_dma_register $address_block FRAME_BYTES 0x3C read-only \
        {Framebuffer byte count.}

    ipx::create_xgui_files $core
    ipx::update_checksums $core
    ipx::check_integrity $core
    ipx::save_core $core
    close_project
}
