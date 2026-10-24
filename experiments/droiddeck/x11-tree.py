import ctypes as c

x = c.CDLL('libX11.so.6')
x.XOpenDisplay.restype = c.c_void_p
d = x.XOpenDisplay(None)
assert d
x.XDefaultRootWindow.argtypes = [c.c_void_p]
x.XDefaultRootWindow.restype = c.c_ulong
x.XQueryTree.argtypes = [c.c_void_p, c.c_ulong, c.POINTER(c.c_ulong), c.POINTER(c.c_ulong), c.POINTER(c.POINTER(c.c_ulong)), c.POINTER(c.c_uint)]
x.XGetGeometry.argtypes = [c.c_void_p, c.c_ulong, c.POINTER(c.c_ulong), c.POINTER(c.c_int), c.POINTER(c.c_int), c.POINTER(c.c_uint), c.POINTER(c.c_uint), c.POINTER(c.c_uint), c.POINTER(c.c_uint)]
x.XFetchName.argtypes = [c.c_void_p, c.c_ulong, c.POINTER(c.c_char_p)]
x.XFree.argtypes = [c.c_void_p]

def visit(window, depth=0):
    root, parent = c.c_ulong(), c.c_ulong()
    xx, yy, width, height, border, bits = c.c_int(), c.c_int(), c.c_uint(), c.c_uint(), c.c_uint(), c.c_uint()
    x.XGetGeometry(d, window, c.byref(root), c.byref(xx), c.byref(yy), c.byref(width), c.byref(height), c.byref(border), c.byref(bits))
    name = c.c_char_p()
    x.XFetchName(d, window, c.byref(name))
    print(' ' * depth, hex(window), xx.value, yy.value, width.value, height.value, name.value.decode(errors='replace')[:100] if name.value else '', flush=True)
    if name:
        x.XFree(name)
    children, count = c.POINTER(c.c_ulong)(), c.c_uint()
    x.XQueryTree(d, window, c.byref(root), c.byref(parent), c.byref(children), c.byref(count))
    if depth < 5:
        for child in children[:count.value]:
            visit(child, depth + 1)
    if children:
        x.XFree(children)

visit(x.XDefaultRootWindow(d))
