#!/usr/local/bin/tclsh8.2
set infile [lindex $argv 0]
set ip [lindex $argv 1]
set port [lindex $argv 2]
set outfile "$infile.hardcopy"
set logfile "$infile.log"

set input [open $infile]
set output [open $outfile w+]
set logit [open $logfile w+]
fconfigure $output -buffersize 32768
fconfigure $logit -buffersize 32768
set cnt 0

while {[gets $input line] > 0} {
incr cnt
set datein [format %-19s [clock format [clock seconds] -format %d-%m-%Y/%T]]
    set mysock [socket $ip $port]
    fconfigure $mysock -buffersize 32768 -eofchar {\x13 \x13}
    puts -nonewline $mysock $line\x13
    flush $mysock
    foreach line [split [read $mysock] \n] {
        flush $mysock
        puts $output $line\n
    }
close $mysock
}
set dateout [format %-19s [clock format [clock seconds] -format %d-%m-%Y/%T]]
puts $logit $cnt
puts $logit $datein
puts $logit $dateout