import io, re, collections

lines = io.open('nb3/live_gs.txt', 'rb').read().decode('utf-8', 'replace').split('\n')
# window of the post_enter_world call
lo, hi = 123491, 124163
seg = lines[lo - 1:hi]
ent = collections.Counter()
for l in seg:
    m = re.search(r'\[mall\] ENTER(?:-inline)? obj=(\S+) .* fn=(\S+)', l)
    if m:
        ent[m.group(1) + '::' + m.group(2)] += 1
print('distinct ENTER targets in post_enter_world window:', len(ent))
for k, v in ent.most_common(30):
    print('%5d  %s' % (v, k))
print()
# how many apply lines and their owners, in order, first 45 and last 5
print('--- apply order around the boundary (indices 30..50) ---')
ap = []
for i, l in enumerate(seg):
    m = re.search(r'\[nb-apply\] (\S+) bound=(\d+) func=(\S+) owner=(\S+)', l)
    if m:
        ap.append((len(ap), i, m.group(1), m.group(4)))
for k, i, f, o in ap[28:52]:
    print('%4d line=%d %-28s %s' % (k, lo + i, f, o))
print('total applies:', len(ap))
print('tail owners:', collections.Counter(o for _, _, _, o in ap[38:]).most_common(5))
