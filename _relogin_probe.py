# -*- coding: utf-8 -*-
"""重上 / 通讯故障 probe.

The client's "通讯故障，暂时无法登录，请稍候重试。（原因：9030）" is
`logind.c::try_recover_data` case -2 (logind_v37.c:1964), i.e.
`read_content("runtime", account)` returned -2 - the DBA never answered
(`filed.c::do_operate` returns its initialised {-2,cookie} when the reply
callback never fires).

Sequence: log in + enter the world, kill the link like a crashing client,
immediately log in again and report every dialog the server sends back.

usage: python -X utf8 _relogin_probe.py [acct] [pwd] [char] [wait_seconds]
"""
import io
import importlib.util
import sys
import time

spec = importlib.util.spec_from_file_location('ll', '_login_lib.py')
ll = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ll)
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'awesrd'
WAIT = float(sys.argv[4]) if len(sys.argv) > 4 else 2.0


def main():
    print('=== session A: login + enter world ===', flush=True)
    a = ll.Session(ACCT, PWD, CHAR, tag='A')
    a.connect(window=20)
    print('    A: uid=0x%X pos=%s cmds=%d' % (a.uid or 0, a.pos, len(a.cmds)), flush=True)
    if a.pos:
        a.query_pos()
        print('    A: server pos=%s' % (a.pos,), flush=True)
    t = time.time()
    a.hard_close()
    print('    A: hard-closed (RST) after %.1fs in world' % (t - a.t0), flush=True)

    time.sleep(WAIT)

    print('=== session B: immediate re-login (%.1fs later) ===' % WAIT, flush=True)
    b = ll.Session(ACCT, PWD, CHAR, tag='B')
    b.connect(window=30, verbose=True)
    print('    B: uid=%s pos=%s' % (('0x%X' % b.uid) if b.uid else None, b.pos), flush=True)
    print('    B: dialogs=%s' % (b.dialogs,), flush=True)
    if b.g and b.g.fileno() >= 0:
        p = b.query_pos()
        print('    B: server pos=%s' % (p,), flush=True)
        print('=== session B: idle 15s (drop watch) ===', flush=True)
        b.pump(15)
        print('    B: cmds=%s' % [hex(c) for c in set(b.cmds)], flush=True)
    b.close()


if __name__ == '__main__':
    main()
