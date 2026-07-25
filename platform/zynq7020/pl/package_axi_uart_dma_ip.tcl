proc add_uart_dma_register {address_block name offset access description} {
    set register [ipx::add_register $name $address_block]
    set_property address_offset $offset $register
    set_property size 32 $register
    set_property access $access $register
    set_property description $description $register
    return $register
}

proc package_axi_uart_dma_ip {script_dir package_project_dir ip_repo_dir} {
    set repo_root [file normalize [file join $script_dir .. .. ..]]
    set rtl_dir [file join $repo_root verilog uart_dma src]
    set project_dir [file join $package_project_dir axi_uart_dma]
    set ip_root [file join $ip_repo_dir axi_uart_dma_1_0]

    create_project -force axi_uart_dma_ip_package $project_dir \
        -part xc7z020clg400-2
    set_property target_language Verilog [current_project]
    add_files -norecurse [list \
        [file join $rtl_dir uart_sync_fifo.v] \
        [file join $rtl_dir uart_serial_core.v] \
        [file join $rtl_dir axi_uart_dma.v]]
    set_property top axi_uart_dma [current_fileset]
    update_compile_order -fileset sources_1

    ipx::package_project -root_dir $ip_root \
        -vendor maxli.local \
        -library user \
        -taxonomy /UserIP \
        -import_files \
        -set_current true \
        -force

    set core [ipx::current_core]
    set_property name axi_uart_dma $core
    set_property display_name {AXI UART DDR Ring DMA} $core
    set_property description \
        {Configurable UART with autonomous RX and TX DDR ring DMA.} $core
    set_property vendor_display_name {Max.Li} $core
    set_property version 1.0 $core

    foreach {name mode} {s_axi slave m_axi master} {
        set interface [ipx::get_bus_interfaces -quiet $name -of_objects $core]
        if {[llength $interface] != 1} {
            error "AXI UART DMA packaging failed: $name $mode interface not found"
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

    add_uart_dma_register $address_block CONTROL 0x00 read-write \
        {Enable, soft reset, and internal loopback control.}
    add_uart_dma_register $address_block STATUS 0x04 read-only \
        {UART, FIFO, ring, DMA, and error status.}
    add_uart_dma_register $address_block UART_CFG 0x08 read-write \
        {Data bits, parity, and stop-bit configuration.}
    add_uart_dma_register $address_block BAUD_INC 0x0C read-write \
        {Fractional 16x baud phase increment.}
    add_uart_dma_register $address_block RX_BASE 0x10 read-write {RX DDR ring base.}
    add_uart_dma_register $address_block RX_SIZE 0x14 read-write {RX DDR ring size.}
    add_uart_dma_register $address_block RX_PRODUCED 0x18 read-only {RX hardware producer count.}
    add_uart_dma_register $address_block RX_CONSUMED 0x1C read-write {RX software consumer count.}
    add_uart_dma_register $address_block TX_BASE 0x20 read-write {TX DDR ring base.}
    add_uart_dma_register $address_block TX_SIZE 0x24 read-write {TX DDR ring size.}
    add_uart_dma_register $address_block TX_PRODUCED 0x28 read-write {TX software producer count.}
    add_uart_dma_register $address_block TX_CONSUMED 0x2C read-only {TX hardware consumer count.}
    add_uart_dma_register $address_block IRQ_STATUS 0x30 read-write {Write-one-to-clear error IRQ status.}
    add_uart_dma_register $address_block IRQ_ENABLE 0x34 read-write {Error interrupt enable mask.}
    add_uart_dma_register $address_block UART_ERRORS 0x38 read-only {Parity and frame error counts.}
    add_uart_dma_register $address_block DMA_ERRORS 0x3C read-only {Ring overflow and AXI error counts.}
    add_uart_dma_register $address_block DMA_STOP 0x40 read-only {Latched DMA stop reasons.}
    add_uart_dma_register $address_block VERSION 0x44 read-only {RTL peripheral version.}

    ipx::create_xgui_files $core
    ipx::update_checksums $core
    ipx::check_integrity $core
    ipx::save_core $core
    close_project
}
