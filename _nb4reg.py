import io, re, collections

lines = io.open('nb4/live_gs.txt', 'rb').read().decode('utf-8', 'replace').split('\n')

regpat = re.compile(r'\[nb-reg\] obj=(\S+) this=(\S+) passed=(-?\d+) argc=(\d+) bp=(\d+) stack=(\d+) top=(.*)$')
cfvpat = re.compile(r'\[nb-cfv\] obj=(\S+) cur=(\S+) idx=(\d+) ptr=(\S+) f=(\S+) fo=(\S+) k=(\S+)$')

regs = []
cfvs = collections.defaultdict(list)
for i, l in enumerate(lines, 1):
    m = regpat.search(l)
    if m:
        regs.append((i,) + m.groups())
        continue
    m = cfvpat.search(l)
    if m:
        cfvs[m.group(1)].append((i,) + m.groups())

print('regs:', len(regs), 'objectsWithCfv:', len(cfvs))
print()
print('%-4s %-7s %-46s %-8s %-9s %-30s' % ('#', 'line', 'obj', 'passed', 'stack/bp', 'top'))
for k, r in enumerate(regs):
    i, obj, this, passed, argc, bp, stack, top = r
    top = top.strip()
    print('%-4d %-7d %-46s %-8s %-9s %-30s' % (k, i, obj, passed, stack + '/' + bp, top[:90]))
