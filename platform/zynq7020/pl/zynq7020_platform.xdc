set_property PACKAGE_PIN U15 [get_ports UART_0_rxd]
set_property IOSTANDARD LVCMOS33 [get_ports UART_0_rxd]
set_property PULLUP true [get_ports UART_0_rxd]

set_property PACKAGE_PIN W15 [get_ports UART_0_txd]
set_property IOSTANDARD LVCMOS33 [get_ports UART_0_txd]
set_property SLEW SLOW [get_ports UART_0_txd]

set_property PACKAGE_PIN M19 [get_ports {gpio_rtl_0_tri_i[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio_rtl_0_tri_i[0]}]

set_property PACKAGE_PIN T12 [get_ports {gpio_rtl_1_tri_o[3]}]
set_property PACKAGE_PIN U12 [get_ports {gpio_rtl_1_tri_o[2]}]
set_property PACKAGE_PIN V12 [get_ports {gpio_rtl_1_tri_o[1]}]
set_property PACKAGE_PIN W13 [get_ports {gpio_rtl_1_tri_o[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio_rtl_1_tri_o[*]}]
set_property SLEW SLOW [get_ports {gpio_rtl_1_tri_o[*]}]
