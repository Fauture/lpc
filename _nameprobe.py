# -*- coding: utf-8 -*-
"""Probe the server's character-name rule: try several names, print the verdict.

usage: python -X utf8 _nameprobe.py <acct> <pwd> <name> [<name> ...]
"""
import sys
import time

sys.path.insert(0, '.')
import _login_lib as ll  # noqa: E402

ACCT = sys.argv[1] if len(sys.argv) > 1 else '11'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
NAMES = sys.argv[3:] or ['测试', '测试剑客', 'abcdef', '123456', 'ab12', '张三']

for nm in NAMES:
    s = ll.Session(ACCT, PWD, nm, tag='NP')
    try:
        s.connect()
        s.pump(6)
        verdict = 'CREATED' if s.created else 'rejected'
        msgs = [d for d in s.dialogs if '名字' in d or '9111' in d]
        print('%-10s len=%2d -> %-8s %s' % (nm, len(nm.encode('gbk')), verdict,
                                            (msgs[-1][:70] if msgs else '')))
    except Exception as e:
        print('%-10s EXC %s' % (nm, e))
    finally:
        try:
            s.close()
        except Exception:
            pass
    time.sleep(0.5)
