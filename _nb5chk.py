import io, re

lines = io.open('nb5/live_gs.txt', 'rb').read().decode('utf-8', 'replace').split('\n')
appre = re.compile(r'\[nb-app\] obj=(\S+) lhsN=(\d+) lhsUse=(\d+) rhsN=(\d+) rhs0=(\S+?) f=(\S+) fo=(\S+) k=(\S+?) resN=(\d+) resLast=ptr=(\S+) f=(\S+) fo=(\S+)')
gwre = re.compile(r'\[nb-gw\] via-ref obj=(\S+) fn=(\S+) v=(.*) v0=(.*)$')
gsre = re.compile(r'\[nb-gs\] obj=(\S+) fn=(\S+)')

apps = []
for i, l in enumerate(lines, 1):
    m = appre.search(l)
    if m:
        apps.append((i, m.group(1), int(m.group(2)), int(m.group(9)), m.group(6), m.group(12)))

gws = []
for i, l in enumerate(lines, 1):
    if '[nb-gw]' in l or '[nb-gs]' in l:
        gws.append((i, l[:170]))

print('appends:', len(apps), ' global writes logged:', len(gws))
print('--- lhsN sequence check ---')
bad = 0
for k, (i, obj, lhs, res, rf, rfo) in enumerate(apps):
    if k < 45 or k % 20 == 0 or k > len(apps) - 4:
        print('%4d %-46s lhs=%3d res=%3d rhs=%-24s rhsOwner=%s' % (k, obj, lhs, res, rf, rfo))
    if res != lhs + 1:
        bad += 1
        if bad < 10:
            print('   !! res != lhs+1 at', k, obj, lhs, res)
print('bad res!=lhs+1:', bad)
print('---- last 25 global writes ----')
for i, l in gws[-25:]:
    print(i, l)
print('---- global writes after the last append ----')
lastapp = apps[-1][0]
for i, l in gws:
    if i > lastapp:
        print(i, l)
