import ctypes as c
import sys

x = c.CDLL('libX11.so.6')
x.XOpenDisplay.restype = c.c_void_p
d = x.XOpenDisplay(None)
if not d:
    raise SystemExit('XOpenDisplay failed')
x.XDefaultRootWindow.argtypes = [c.c_void_p]
x.XDefaultRootWindow.restype = c.c_ulong
root = x.XDefaultRootWindow(d)
x.XGetInputFocus.argtypes = [c.c_void_p,c.POINTER(c.c_ulong),c.POINTER(c.c_int)]
focused, revert = c.c_ulong(), c.c_int()
x.XGetInputFocus(d, c.byref(focused), c.byref(revert))
print('X11_FOCUS',hex(focused.value),'revert',revert.value,flush=True)
x.XQueryTree.argtypes = [c.c_void_p, c.c_ulong, c.POINTER(c.c_ulong), c.POINTER(c.c_ulong), c.POINTER(c.POINTER(c.c_ulong)), c.POINTER(c.c_uint)]
r, p, children, n = c.c_ulong(), c.c_ulong(), c.POINTER(c.c_ulong)(), c.c_uint()
x.XQueryTree(d, root, c.byref(r), c.byref(p), c.byref(children), c.byref(n))
x.XGetGeometry.argtypes = [c.c_void_p, c.c_ulong, c.POINTER(c.c_ulong), c.POINTER(c.c_int), c.POINTER(c.c_int), c.POINTER(c.c_uint), c.POINTER(c.c_uint), c.POINTER(c.c_uint), c.POINTER(c.c_uint)]
class Attributes(c.Structure):
    _fields_ = [('x',c.c_int),('y',c.c_int),('width',c.c_int),('height',c.c_int),('border_width',c.c_int),('depth',c.c_int),('visual',c.c_void_p),('root',c.c_ulong),('class_',c.c_int),('bit_gravity',c.c_int),('win_gravity',c.c_int),('backing_store',c.c_int),('backing_planes',c.c_ulong),('backing_pixel',c.c_ulong),('save_under',c.c_int),('colormap',c.c_ulong),('map_installed',c.c_int),('map_state',c.c_int),('all_event_masks',c.c_long),('your_event_mask',c.c_long),('do_not_propagate_mask',c.c_long),('override_redirect',c.c_int),('screen',c.c_void_p)]
x.XGetWindowAttributes.argtypes = [c.c_void_p,c.c_ulong,c.POINTER(Attributes)]
for window in [root] + list(children[:n.value]):
    xx, yy, w, h, border, depth = c.c_int(), c.c_int(), c.c_uint(), c.c_uint(), c.c_uint(), c.c_uint()
    x.XGetGeometry(d, window, c.byref(r), c.byref(xx), c.byref(yy), c.byref(w), c.byref(h), c.byref(border), c.byref(depth))
    attr = Attributes()
    x.XGetWindowAttributes(d, window, c.byref(attr))
    print('X11_WINDOW', hex(window), xx.value, yy.value, w.value, h.value, depth.value, 'map',attr.map_state, flush=True)
    if len(sys.argv)>2 and sys.argv[2]=='focus-largest' and window!=root and attr.map_state==2 and w.value>=640 and h.value>=400:
        x.XSetInputFocus.argtypes=[c.c_void_p,c.c_ulong,c.c_int,c.c_ulong]
        x.XSetInputFocus(d,window,2,0)
        x.XSync.argtypes=[c.c_void_p,c.c_int]
        x.XSync(d,0)
        print('X11_FOCUS_SET',hex(window),flush=True)
    if window == root:
        width, height = w.value, h.value

class Image(c.Structure):
    _fields_ = [('width', c.c_int), ('height', c.c_int), ('xoffset', c.c_int), ('format', c.c_int), ('data', c.c_void_p), ('byte_order', c.c_int), ('bitmap_unit', c.c_int), ('bitmap_bit_order', c.c_int), ('bitmap_pad', c.c_int), ('depth', c.c_int), ('bytes_per_line', c.c_int), ('bits_per_pixel', c.c_int), ('red_mask', c.c_ulong), ('green_mask', c.c_ulong), ('blue_mask', c.c_ulong)]

if len(sys.argv) > 2 and sys.argv[2].startswith('0x'):
    root = int(sys.argv[2], 16)
    if not x.XGetGeometry(d, root, c.byref(r), c.byref(xx), c.byref(yy), c.byref(w), c.byref(h), c.byref(border), c.byref(depth)):
        raise SystemExit('drawable geometry unavailable')
    width, height = w.value, h.value
    print('X11_CAPTURE_DRAWABLE', hex(root), width, height, flush=True)

x.XGetImage.argtypes = [c.c_void_p, c.c_ulong, c.c_int, c.c_int, c.c_uint, c.c_uint, c.c_ulong, c.c_int]
x.XGetImage.restype = c.POINTER(Image)
image = x.XGetImage(d, root, 0, 0, width, height, c.c_ulong(-1), 2)
if not image:
    raise SystemExit('XGetImage failed')
i = image.contents
print('X11_IMAGE', i.width, i.height, i.bits_per_pixel, i.bytes_per_line, hex(i.red_mask), flush=True)
if i.bits_per_pixel != 32 or i.byte_order != 0 or i.red_mask != 0xff0000:
    raise SystemExit('unhandled pixel layout')
data = c.string_at(i.data, i.bytes_per_line * i.height)
rgb = bytearray(i.width * i.height * 3)
for row in range(i.height):
    src = data[row*i.bytes_per_line:row*i.bytes_per_line+i.width*4]
    offset = row*i.width*3
    rgb[offset:offset+i.width*3:3] = src[2::4]
    rgb[offset+1:offset+i.width*3:3] = src[1::4]
    rgb[offset+2:offset+i.width*3:3] = src[0::4]
with open(sys.argv[1], 'wb') as f:
    f.write(f'P6\n{i.width} {i.height}\n255\n'.encode())
    f.write(rgb)
print('X11_RGB_RANGE', min(rgb), max(rgb), 'nonzero', sum(v != 0 for v in rgb), flush=True)
