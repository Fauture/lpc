import io, re, sys

path = sys.argv[1] if len(sys.argv) > 1 else 'lat1/live_gs.txt'
lines = io.open(path, 'rb').read().decode('utf-8', 'replace').split('\n')

print('=== session 1 login: inbound 0x4124 -> first outbound packets (shared clock) ===')
ev = []
for i, l in enumerate(lines, 1):
    m = re.search(r'\[recv-accept\] t=(\d+)ms client=(\d+) n=(\d+) ([0-9A-F]+)', l)
    if m and '4124' in m.group(4):
        ev.append((i, int(m.group(1)), 'IN 4124', 'client=%s n=%s' % (m.group(2), m.group(3))))
    m = re.search(r'\[send\] t=(\d+)ms cno=(-?\d+) cmd=(0x[0-9a-f]+) body=(\d+) by=(\S+)', l)
    if m:
        ev.append((i, int(m.group(1)), 'OUT ' + m.group(3), 'cno=%s body=%s by=%s' % (m.group(2), m.group(4), m.group(5))))
    m = re.search(r'\[life\] ENTER(?:-inline)? .*fn=(\S+) passed=(\d+).*t=(\d+)ms', l)
    if m:
        ev.append((i, int(m.group(3)), 'life ' + m.group(1), 'passed=%s' % m.group(2)))

ev.sort()
logins = [e for e in ev if e[2] == 'IN 4124']
print('login packets found:', len(logins))
for k, lg in enumerate(logins):
    i0, t0 = lg[0], lg[1]
    print('\n--- login #%d at line %d t=%dms ---' % (k + 1, i0, t0))
    shown = 0
    for i, t, kind, extra in ev:
        if i < i0:
            continue
        if i > i0 + 400 or shown > 26:
            break
        print('  +%7dms  %-14s %s' % (t - t0, kind, extra))
        shown += 1
