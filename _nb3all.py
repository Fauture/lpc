import io, re, collections

lines = io.open('nb3/live_gs.txt', 'rb').read().decode('utf-8', 'replace').split('\n')
pat = re.compile(r'\[nb-apply\] (\S+) bound=(\d+) func=(\S+) owner=(\S+)')
owners = collections.Counter()
funcs = collections.Counter()
n = 0
seq = []
for i, l in enumerate(lines, 1):
    m = pat.search(l)
    if m:
        n += 1
        seq.append((i, m.group(1), m.group(3), m.group(4)))
        owners[m.group(4)] += 1
        funcs[m.group(1)] += 1
print('total nb-apply lines:', n)
print('--- owners ---')
for o, c in owners.most_common(60):
    print('%5d  %s' % (c, o))
print('--- funcs ---')
for o, c in funcs.most_common(60):
    print('%5d  %s' % (c, o))
print('--- first 5 / last 3 with line numbers ---')
for s in seq[:5]:
    print(s)
for s in seq[-3:]:
    print(s)
print('--- non-vipd sequence in order ---')
for s in seq:
    if 'vipd.o' not in s[3]:
        print(s)
