import io, re, collections

lines = io.open('nb3/live_gs.txt', 'rb').read().decode('utf-8', 'replace').split('\n')
apre = re.compile(r'\[nb-apply\] (\S+) bound=(\d+) func=(\S+) owner=(\S+)')
seq = []
for i, l in enumerate(lines, 1):
    m = apre.search(l)
    if m:
        seq.append((i, m.group(3), m.group(4)))
print('total applies:', len(seq))
print('first line', seq[0][0], 'last line', seq[-1][0])
prev = None
for k, (i, f, o) in enumerate(seq):
    tag = o.split('/')[-1]
    if o != prev:
        print('%4d line=%-7d %-24s %s   <== change' % (k, i, f, tag))
        prev = o
print('--- histogram of run lengths by owner ---')
runs = []
cur = None
n = 0
for i, f, o in seq:
    if o == cur:
        n += 1
    else:
        if cur is not None:
            runs.append((cur, n))
        cur, n = o, 1
runs.append((cur, n))
for o, n in runs:
    print('%5d  %s' % (n, o))
