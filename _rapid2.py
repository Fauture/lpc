# -*- coding: utf-8 -*-
"""TRUE rapid-ENTER repro: log in as a normal test account, type the name, then
fire 5 ENTERs in ONE SendKeys call ({ENTER 5} - no per-press process spawn).

usage: python -X utf8 _rapid2.py <acct> <name> [count]
"""
import io
import subprocess
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, '.')
import _rdp as R  # noqa: E402

acct = sys.argv[1] if len(sys.argv) > 1 else '1'
name = sys.argv[2] if len(sys.argv) > 2 else 'rapidx1'
count = sys.argv[3] if len(sys.argv) > 3 else '5'

# fresh client so the login/anti-cheat state is clean
subprocess.run(['taskkill', '/F', '/IM', 'asktao.mod'], capture_output=True)
time.sleep(3)
R.start_client()
time.sleep(12)
R.focus()
R.shot('r2_0_start.jpg', 700)

# the re-login warning dialog may be up first
R.click_frac(0.500, 0.476)
time.sleep(2.0)
k, d = R.screen_kind()
print('after dialog: %s(%d)' % (k, int(d)))

R.click_frac(*R.LOGIN_ACCOUNT)
time.sleep(0.4)
R.paste(acct)
time.sleep(0.4)
R.click_frac(*R.LOGIN_PASSWORD)
time.sleep(0.5)
subprocess.run([sys.executable, '-X', 'utf8', '_postchar.py', acct], check=False)
time.sleep(0.5)
R.click_frac(*R.LOGIN_BUTTON)
time.sleep(7.0)
R.shot('r2_1_server.jpg', 700)

R.click_frac(0.430, 0.410)   # 一线 entry
time.sleep(0.8)
R.click_frac(0.393, 0.925)   # 进入
time.sleep(8.0)
R.shot('r2_2_charlist.jpg', 700)

R.click_frac(*R.CHAR_CREATE_OK)   # 创建角色
time.sleep(3.0)
R.shot('r2_3_form.jpg', 700)
R.click_frac(0.370, 0.705)        # 金 school
time.sleep(0.5)
R.click_frac(0.860, 0.720)        # 角色名字
time.sleep(0.5)
subprocess.run([sys.executable, '-X', 'utf8', '_postchar.py', name], check=False)
time.sleep(0.6)
R.shot('r2_4_name.jpg', 700)

t0 = time.time()
R.keys('{ENTER %s}' % count)      # ONE call: ENTERs back to back
print('fired {ENTER %s} in %.3fs' % (count, time.time() - t0))
# GROUND TRUTH: dialogs take a moment to appear; screen_kind() called this
# "ingame" while the screenshot showed the creation form + 9101 dialogs, so the
# verdict now comes from a reference-frame comparison (_ingame_sig.py).
time.sleep(10.0)
R.shot('r2_5_after.jpg', 700)
try:
    import _ingame_sig
    verdict, d = _ingame_sig.classify('r2_5_after.jpg')
    print('GROUND TRUTH: %s (distance %.1f)' % (verdict, d))
except Exception as e:
    print('GROUND TRUTH: classifier failed: %s' % e)
k, d = R.screen_kind()
print('screen after: %s(%d)' % (k, int(d)))
