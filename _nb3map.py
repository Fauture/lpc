import io, re, collections

lines = io.open('nb3/live_gs.txt', 'rb').read().decode('utf-8', 'replace').split('\n')

apre = re.compile(r'\[nb-apply\] (\S+) bound=(\d+) func=(\S+) owner=(\S+)')
enre = re.compile(r'\[mall\] ENTER obj=(\S+) this=\S+ fn=(\S+)')

seq = []
for i, l in enumerate(lines, 1):
    m = apre.search(l)
    if m:
        seq.append([i, m.group(3), m.group(4), None])
        continue
    if seq and seq[-1][3] is None:
        m = enre.search(l)
        if m:
            seq[-1][3] = m.group(1) + '::' + m.group(2)

print('apply lines:', len(seq))
first, last = seq[0][0], seq[-1][0]
print('line span:', first, last)
owners = collections.Counter(s[2] for s in seq)
funcs = collections.Counter(s[1] for s in seq)
enters = collections.Counter(s[3] for s in seq)
print('distinct owners:', len(owners), 'distinct enter targets:', len(enters))
print('--- enter targets ---')
for k, v in enters.most_common(60):
    print('%5d  %s' % (v, k))
print()
print('--- applies with NO enter (skipped?) ---')
n = 0
for i, f, o, e in seq:
    if e is None:
        n += 1
        if n <= 40:
            print('%7d %-28s owner=%s' % (i, f, o))
print('total no-enter:', n)
