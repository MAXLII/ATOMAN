set_property PACKAGE_PIN W15 [get_ports uart_rxd]
set_property IOSTANDARD LVCMOS33 [get_ports uart_rxd]
set_property PULLUP true [get_ports uart_rxd]

set_property PACKAGE_PIN U15 [get_ports uart_txd]
set_property IOSTANDARD LVCMOS33 [get_ports uart_txd]
set_property SLEW SLOW [get_ports uart_txd]

set_property PACKAGE_PIN M19 [get_ports {gpio_rtl_0_tri_i[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio_rtl_0_tri_i[0]}]

set_property PACKAGE_PIN T12 [get_ports {gpio_rtl_1_tri_o[3]}]
set_property PACKAGE_PIN U12 [get_ports {gpio_rtl_1_tri_o[2]}]
set_property PACKAGE_PIN V12 [get_ports {gpio_rtl_1_tri_o[1]}]
set_property PACKAGE_PIN W13 [get_ports {gpio_rtl_1_tri_o[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio_rtl_1_tri_o[*]}]
set_property SLEW SLOW [get_ports {gpio_rtl_1_tri_o[*]}]

set_property PACKAGE_PIN E18 [get_ports oled_clock]
set_property PACKAGE_PIN E19 [get_ports oled_data]
set_property PACKAGE_PIN F16 [get_ports oled_dc]
set_property PACKAGE_PIN F17 [get_ports oled_reset_n]
set_property IOSTANDARD LVCMOS33 \
    [get_ports {oled_clock oled_data oled_dc oled_reset_n}]
set_property SLEW SLOW \
    [get_ports {oled_clock oled_data oled_dc oled_reset_n}]
