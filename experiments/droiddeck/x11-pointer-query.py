"""Read the real X server pointer position without sending input."""
import ctypes as c
import json
x = c.CDLL('libX11.so.6')
x.XOpenDisplay.restype = c.c_void_p
display = x.XOpenDisplay(None)
assert display, 'X display unavailable'
x.XDefaultRootWindow.argtypes = [c.c_void_p]
x.XDefaultRootWindow.restype = c.c_ulong
x.XQueryPointer.argtypes = [c.c_void_p,c.c_ulong,c.POINTER(c.c_ulong),c.POINTER(c.c_ulong),c.POINTER(c.c_int),c.POINTER(c.c_int),c.POINTER(c.c_int),c.POINTER(c.c_int),c.POINTER(c.c_uint)]
root, child = c.c_ulong(), c.c_ulong()
rx, ry, wx, wy, mask = c.c_int(), c.c_int(), c.c_int(), c.c_int(), c.c_uint()
assert x.XQueryPointer(display,x.XDefaultRootWindow(display),c.byref(root),c.byref(child),c.byref(rx),c.byref(ry),c.byref(wx),c.byref(wy),c.byref(mask))
print(json.dumps(dict(root_x=rx.value,root_y=ry.value,mask=mask.value)))
x.XCloseDisplay.argtypes = [c.c_void_p]
x.XCloseDisplay(display)
