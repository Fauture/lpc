import io
import re

for d in ('vF1', 'vC4'):
    lines = io.open(d + '/live_gs.txt', encoding='gbk', errors='replace').read().split('\n')
    print('=== %s ===' % d)
    hits = re.findall(r'account[=:\s]+"?([0-9]{1,3})', '\n'.join(lines))
    print('  account mentions:', hits[:12])
    # names created, in order, with 40 lines of context scanned for an account
    for i, l in enumerate(lines):
        m = re.search(r'args=\[create_new_char \| \(\{"([^"]+)"', l)
        if not m:
            continue
        ctx = '\n'.join(lines[max(0, i - 60):i])
        acc = re.findall(r'account[=:\s]+"?([0-9]{1,3})', ctx)
        print('  create %-6s nearby accounts=%s' % (m.group(1), acc[-3:]))
