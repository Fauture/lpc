import io
import os
import re

# Harness audit: for the last few runs, check that
#   * every run really created its own character (create flow, not a load),
#   * the runs belong to DIFFERENT accounts (the user's rule),
#   * the dialogs counted are the ones that run produced.
LOGS = ['vC4', 'vF1', 'vTel', 'vC3', 'vLock', 'vA6c']

def load(d):
    p = os.path.join(d, 'live_gs.txt')
    return io.open(p, encoding='gbk', errors='replace').read().split('\n') if os.path.exists(p) else []

for d in LOGS:
    lines = load(d)
    if not lines:
        print('%-6s (no log)' % d)
        continue
    creates, order, seen = [], [], set()
    for i, l in enumerate(lines):
        m = re.search(r'args=\[create_new_char \| \(\{"([^"]+)"', l)
        if m:
            creates.append(m.group(1))
            if m.group(1) not in seen:
                seen.add(m.group(1))
                order.append(i)
    loads = re.findall(r'args=\[load_existed_char \| \(\{"([^"]+)"', '\n'.join(lines))
    # accounts seen on the wire (the client sends its account during login)
    print('%-6s runs=%-2d names=%s' % (d, len(order), [creates[i] for i in range(len(creates)) if i == 0 or creates[i] != creates[i-1]][:6]))
    print('        loads=%s' % (loads[:8]))
