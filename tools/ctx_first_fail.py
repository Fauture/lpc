# -*- coding: utf-8 -*-
import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
path = sys.argv[1]
f = open(path, 'rb')
size = 0
# find FIRST occurrence via streaming from start
import codecs
dec = codecs.getincrementaldecoder('utf-16-le')(errors='replace')
buf = ''
pos = 0
found = None
context = []
with open(path, 'rb') as fh:
    while True:
        chunk = fh.read(8 * 1024 * 1024)
        if not chunk:
            break
        buf += dec.decode(chunk)
        while True:
            nl = buf.find('\n')
            if nl < 0:
                break
            line = buf[:nl]
            buf = buf[nl + 1:]
            pos += 1
            context.append(line)
            if len(context) > 80:
                context.pop(0)
            if 'create dist start time failed' in line or 'load dist start time failed' in line:
                found = pos
                break
        if found:
            break
print('first fail print at line %d' % found)
for l in context[-70:]:
    print(l[:195])
