package require cwiid

puts "Press 1 and 2 simultaneously"

set wiimotes	[cwiid::get_bdinfo_array -1 4]
if {[llength $wiimotes] == 0} {
	puts "No wiimotes found"
	exit 1
}

set bdaddr	[dict get [lindex $wiimotes 0] bdaddr]

set wiimote	[cwiid::open $bdaddr {CONTINUOUS MESG_IFC}]
puts "connected, id: [$wiimote get_id]"
$wiimote set_mesg_callback [list apply {
	info {
		puts "in mesg callback: $info"
	}
}]

#$wiimote set_rpt_mode {
#	STATUS
#	BTN
#	ACC
#	IR
#	NUNCHUK
#	CLASSIC
#	BALANCE
#	MOTIONPLUS
#	EXT
#}
$wiimote set_rpt_mode {EXT ACC BTN STATUS}
puts "set rpt_mode"
$wiimote enable MOTIONPLUS
puts "enabled MOTIONPLUS"

proc read_mesgs wiimote {
	puts "waiting for a message"
	set info	[$wiimote get_mesg]
	puts "----------------------------------------------"
	puts "Timestamp: [dict get $info sec]:[dict get $info nsec]"
	foreach mesg [dict get $info mesgs] {
		puts "===================="
		puts "Type: [dict get $mesg type]"
		puts "\t$mesg"
	}
}

while {1} {
	read_mesgs $wiimote
}
rename $wiimote {}
