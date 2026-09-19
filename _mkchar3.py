# -*- coding: utf-8 -*-
"""Create a character with a legal Chinese name through the standard probe
login path (HANDOVER: test data must be produced by the server itself; a probe
login is allowed for that).  The real client is then used to enter the game."""
import sys
import time

sys.path.insert(0, '.')
import _login_lib as ll  # noqa: E402

ACCT = sys.argv[1] if len(sys.argv) > 1 else 'a'
PWD = sys.argv[2] if len(sys.argv) > 2 else 'a'
NAME = sys.argv[3] if len(sys.argv) > 3 else '测试剑客'

s = ll.Session(ACCT, PWD, NAME, tag='MK')
try:
    ok = s.connect()
    print('connect ->', ok, 'created =', s.created)
    s.pump(12)
    print('created =', s.created, 'selected =', s.selected)
    for d in s.dialogs[-5:]:
        print('  dialog:', d)
finally:
    try:
        s.close()
    except Exception as e:
        print('close:', e)
print('done')
