import io, re, collections, sys

path = sys.argv[1] if len(sys.argv) > 1 else 'nb3/live_gs.txt'
lo = int(sys.argv[2]) if len(sys.argv) > 2 else 123490
hi = int(sys.argv[3]) if len(sys.argv) > 3 else 124163

lines = io.open(path, 'rb').read().decode('utf-8', 'replace').split('\n')
seg = lines[lo - 1:hi]

owners = collections.Counter()
funcs = collections.Counter()
pairs = []
pending = None
for l in seg:
    m = re.match(r'\[nb-isfn\] (.*)', l)
    if m:
        pending = m.group(1)
        continue
    m = re.match(r'\[nb-apply\] (\S+) bound=(\d+) func=(\S+) owner=(\S+)', l)
    if m:
        pairs.append((pending, m.group(1), m.group(3), m.group(4)))
        owners[m.group(4)] += 1
        funcs[m.group(1)] += 1

print('pairs:', len(pairs))
print('--- owner counts ---')
for o, c in owners.most_common(20):
    print('%5d  %s' % (c, o))
print('--- func counts ---')
for o, c in funcs.most_common(20):
    print('%5d  %s' % (c, o))
print('--- any newbied? ---')
for p in pairs:
    if 'newbie' in p[3] or 'newbie' in p[1] or (p[0] and 'newbie' in p[0]):
        print(p)
print('--- first 25 pairs ---')
for p in pairs[:25]:
    print(p)
print('--- isfn type histogram ---')
tc = collections.Counter((p[0] or '')[:2] for p in pairs)
print(tc)
