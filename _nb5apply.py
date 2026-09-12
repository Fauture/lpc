import io, re, collections

lines = io.open('nb5/live_gs.txt', 'rb').read().decode('utf-8', 'replace').split('\n')
apre = re.compile(r'\[nb-apply\] (\S+) bound=\d+ func=(\S+) owner=(\S+) ptr=(\S+) f=(\S+) fo=(\S+) k=(\S+)')
seq = []
for i, l in enumerate(lines, 1):
    m = apre.search(l)
    if m:
        seq.append((i, m.group(1), m.group(3), m.group(4), m.group(6)))
print('applies:', len(seq))
ptrs = collections.Counter(s[3] for s in seq)
print('distinct closure ptrs:', len(ptrs))
for p, c in ptrs.most_common(6):
    print('%5d  ptr=%s' % (c, p))
print('--- transitions (owner change) ---')
prev = None
for k, (i, f, o, p, fo) in enumerate(seq):
    if o != prev:
        print('%4d line=%-7d %-24s %-30s ptr=%s fo=%s' % (k, i, f, o, p, fo))
        prev = o
print('--- applies 30..48 with ptr ---')
for k, s in enumerate(seq[30:48]):
    print(k + 30, s[2], s[3], s[4])
