This is a driver for multi-touch capable clickpads.

First instance is mainly the X driver, future versions will focus on wayland
etc. as well.

This driver requires:
* the Linux evdev kernel API
* a multitouch-capable clickpad

This driver provides:
* a "swapable" backend (making it usable for X, wayland compositors, etc.)
* tools for console debugging

Features (current):
* it builds

Features (eventually):
* tap-to-click
* two-finger scrolling
* software button emulation
