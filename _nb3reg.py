import io, re

lines = io.open('nb3/live_gs.txt', 'rb').read().decode('utf-8', 'replace').split('\n')
pat = re.compile(r'\[mall\] ENTER-inline obj=(\S+) this=\S+ fn=register_enter_world')
order = []
for i, l in enumerate(lines, 1):
    m = pat.search(l)
    if m:
        order.append((i, m.group(1)))
print('registrations:', len(order))
print('--- registration order (index, obj) ---')
for k, (i, o) in enumerate(order):
    print('%4d  %s' % (k, o))
