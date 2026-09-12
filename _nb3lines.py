import io, re

lines = io.open('nb3/live_gs.txt', 'rb').read().decode('utf-8', 'replace').split('\n')
pat = re.compile(r'\[mall\] ENTER-inline obj=(\S+) this=\S+ fn=register_enter_world')
prev = None
out = []
for i, l in enumerate(lines, 1):
    m = pat.search(l)
    if m:
        out.append((i, m.group(1)))
print('regs:', len(out))
print('idx  line   delta  obj')
for k, (i, o) in enumerate(out):
    d = '' if prev is None else str(i - prev)
    prev = i
    print('%4d %7d %6s  %s' % (k, i, d, o))
