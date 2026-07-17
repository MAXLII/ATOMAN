set axi_uartlite_base 0x40600000
set axi_gpio_input_base 0x41200000
set axi_gpio_output_base 0x41210000
set axi_iir_base 0x43C00000

proc iir_process_sample {base input_sample} {
    mwr [expr {$base + 0x08}] $input_sample
    mwr [expr {$base + 0x00}] 0x00000001

    set status 0
    for {set poll 0} {$poll < 100} {incr poll} {
        set status [mrd -value [expr {$base + 0x04}]]
        if {($status & 0x00000002) != 0} {
            break
        }
    }
    if {($status & 0x00000002) == 0} {
        error "IIR completion timeout"
    }

    set output_sample [mrd -value [expr {$base + 0x0C}]]
    mwr [expr {$base + 0x00}] 0x00000004
    return [list $output_sample $status]
}

proc run_pl_selftest {axi_uartlite_base axi_gpio_input_base axi_gpio_output_base axi_iir_base} {
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~ "ARM*#0"} -index 0
stop
configparams force-mem-access 1

set gpio_input [mrd -value $axi_gpio_input_base]
set uart_status [mrd -value [expr {$axi_uartlite_base + 8}]]

mwr $axi_gpio_output_base 0x00000005
set gpio_output [mrd -value $axi_gpio_output_base]
mwr $axi_gpio_output_base 0x00000000

mwr [expr {$axi_iir_base + 0x10}] 0x20000000
mwr [expr {$axi_iir_base + 0x14}] 0x10000000
mwr [expr {$axi_iir_base + 0x18}] 0x08000000
mwr [expr {$axi_iir_base + 0x1C}] 0x04000000
mwr [expr {$axi_iir_base + 0x20}] 0x10000000
mwr [expr {$axi_iir_base + 0x24}] 0x08000000
mwr [expr {$axi_iir_base + 0x28}] 0x04000000
mwr [expr {$axi_iir_base + 0x00}] 0x00000002

set impulse_inputs {0x00100000 0 0 0 0 0 0 0}
set impulse_expected {0x00080000 0x00020000 0x00008000 0x00002000 0xFFFFC800 0x00000200 0x00000480 0x00000220}
set impulse_actual {}
set iir_status 0
for {set index 0} {$index < 8} {incr index} {
    set sample_result [iir_process_sample $axi_iir_base [lindex $impulse_inputs $index]]
    set sample_output [lindex $sample_result 0]
    set iir_status [lindex $sample_result 1]
    lappend impulse_actual $sample_output
}

set iir_count [mrd -value [expr {$axi_iir_base + 0x2C}]]
set iir_version [mrd -value [expr {$axi_iir_base + 0x30}]]
set iir_format [mrd -value [expr {$axi_iir_base + 0x34}]]
set iir_y1 [mrd -value [expr {$axi_iir_base + 0x44}]]
set iir_y2 [mrd -value [expr {$axi_iir_base + 0x48}]]
set iir_y3 [mrd -value [expr {$axi_iir_base + 0x4C}]]

mwr [expr {$axi_iir_base + 0x00}] 0x00000002
mwr [expr {$axi_iir_base + 0x10}] 0x7FFFFFFF
mwr [expr {$axi_iir_base + 0x14}] 0x00000000
mwr [expr {$axi_iir_base + 0x18}] 0x00000000
mwr [expr {$axi_iir_base + 0x1C}] 0x00000000
mwr [expr {$axi_iir_base + 0x20}] 0x00000000
mwr [expr {$axi_iir_base + 0x24}] 0x00000000
mwr [expr {$axi_iir_base + 0x28}] 0x00000000
set positive_result [iir_process_sample $axi_iir_base 0x7FFFFFFF]
mwr [expr {$axi_iir_base + 0x00}] 0x00000002
set negative_result [iir_process_sample $axi_iir_base 0x80000000]
set positive_output [lindex $positive_result 0]
set positive_status [lindex $positive_result 1]
set negative_output [lindex $negative_result 0]
set negative_status [lindex $negative_result 1]

mwr [expr {$axi_iir_base + 0x00}] 0x00000002
mwr [expr {$axi_iir_base + 0x10}] 0x40000000

set result PASS
if {($gpio_output & 0x0000000F) != 0x00000005} {
    set result FAIL
}
for {set index 0} {$index < 8} {incr index} {
    if {[lindex $impulse_actual $index] != [lindex $impulse_expected $index]} {
        set result FAIL
    }
}
if {$iir_count != 8 || $iir_version != 0x00010000 || $iir_format != 0x0000201E} {
    set result FAIL
}
if {$iir_y1 != 0x00000220 || $iir_y2 != 0x00000480 || $iir_y3 != 0x00000200} {
    set result FAIL
}
if {$positive_output != 0x7FFFFFFF || ($positive_status & 0x00000004) == 0} {
    set result FAIL
}
if {$negative_output != 0x80000000 || ($negative_status & 0x00000004) == 0} {
    set result FAIL
}

configparams force-mem-access 0
con
disconnect

puts [format "PL_SELFTEST result=%s gpio_input=0x%08X gpio_output=0x%08X uart_status=0x%08X iir_impulse=%s iir_count=%u iir_history=0x%08X/0x%08X/0x%08X iir_saturation=0x%08X/0x%08X iir_version=0x%08X iir_format=0x%08X" \
      $result $gpio_input $gpio_output $uart_status $impulse_actual $iir_count \
      $iir_y1 $iir_y2 $iir_y3 $positive_output $negative_output $iir_version $iir_format]

if {$result ne "PASS"} {
    error "PL AXI self-test failed"
}
}

if {[catch {
    run_pl_selftest $axi_uartlite_base $axi_gpio_input_base $axi_gpio_output_base $axi_iir_base
} selftest_error]} {
    puts stderr "PL_SELFTEST result=FAIL error=$selftest_error"
    catch {configparams force-mem-access 0}
    catch {disconnect}
    exit 1
}

exit 0
