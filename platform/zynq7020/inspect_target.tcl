proc inspect_target {} {
    connect -url tcp:127.0.0.1:3121
    targets -set -nocase -filter {name =~ "ARM*#0"} -index 0
    stop
    puts "PC=[rrd pc]"
    puts "CPSR=[rrd cpsr]"
    puts "SP=[rrd sp]"
    con
    disconnect
}

if {[catch {inspect_target} inspect_error]} {
    puts stderr "TARGET_INSPECT result=FAIL error=$inspect_error"
    catch {disconnect}
    exit 1
}

puts "TARGET_INSPECT result=PASS"
exit 0
