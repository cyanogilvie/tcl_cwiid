Tcl Wrapper for libcwiid
========================

This Tcl package provides a low-level API for interfacing with Nintendo Wii
remotes and their extensions, based on (and closely conforming to) the
libcwiid library.

Example
-------

```tcl
package require cwiid

try {
	set wiimote	[cwiid::open 00:11:22:33:44:55]
} trap {CWIID OPEN} {errmsg} {
	puts stderr "Error connecting to Wiimote: $errmsg"
	exit 1
}

puts "Got id: [$wiimote get_id]"

# Close connection
rename $wiimote {}
```

API
---

cwiid::find_wiimote bdaddr timeout
cwiid::list_wiimotes
cwiid::get_bdinfo_array ?dev_id? ?timeout? ?flags?
cwiid::open bdaddr ?flags?
handle get_id
handle set_data data
handle get_data
handle enable flags
handle disable flags
handle set_mesg_callback cb
handle get_mesg
handle get_state
handle get_acc_cal ext_type
handle get_balance_cal
handle command command ?flags?
handle send_rpt flags remote data
handle request_status
handle set_led led
handle set_rumble active
handle set_rpt_mode mode
handle read flags offset len
handle write flags offset data

Status
------

This package is still in development, currently scanning, connecting and
disconnecting, and getting the connection id work, everything else is stubbed.

Documentation
-------------

See man (n) cwiid (in the project) for documentation, or the the libcwiid docs:
http://abstrakraft.org/cwiid/wiki/libcwiid

License
-------

This package is licensed under the GPL, to match libcwiid.
