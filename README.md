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
