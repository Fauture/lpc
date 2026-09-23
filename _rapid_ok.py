# -*- coding: utf-8 -*-
"""Enter-game test, FAIR variant: one real login, ONE submit, then 19 ENTERs in game.

Why a second harness: `_rapid2.py` fires all 20 ENTERs back to back while the
character form is still open, so the client submits create_new_char 8 times in
~800 ms.  Measured on the healthy F4 baseline (vF13, account 20 -> cno 41):

    1061783/1061890/1061952/1062031  client 0x0472 (create)
    1062240 DIALOG 请求处理中，请稍候！（原因：9101）
    1062317 0x0472 n=100 (three ENTERs in one read)
    1062370/377/381/384 DIALOG 9101 x4
    1062385 DIALOG 这个角色名字已经存在。（原因：9113）
    1062519 CHARTLIST body=112      <- the character WAS created
    1062612 load_existed_char       <- and the client entered the game

So every 9101/9113 dialog in that run answers a DUPLICATE submit caused by the
harness; the character itself is created on the first ENTER.

This script keeps the click sequence of _rapid2.py byte for byte, but WAITS for
the login screen (via `_rdp.wait_kind`) instead of assuming a fixed delay - a
first attempt clicked into an empty desktop because the client had not drawn its
login form yet, which produced a "FAIL" that said nothing about the server.

usage: python -X utf8 _rapid_ok.py <acct> <name> [count]
"""
import io
import subprocess
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.path.insert(0, '.')
import _rdp as R        # noqa: E402
import _ingame_sig      # noqa: E402

acct = sys.argv[1] if len(sys.argv) > 1 else '1'
name = sys.argv[2] if len(sys.argv) > 2 else 'rapidok'
count = int(sys.argv[3]) if len(sys.argv) > 3 else 20


def step(msg):
    print('[%7.1fs] %s' % (time.time() - T0, msg))
    sys.stdout.flush()


T0 = time.time()
subprocess.run(['taskkill', '/F', '/IM', 'asktao.mod'], capture_output=True)
time.sleep(3)
R.start_client()
time.sleep(5)

# 1) the re-login warning must be dismissed FIRST, by its own 确定 button:
#    clicking the login form's field position while this dialog is up only
#    closes the dialog, and every later click then lands on the login screen
#    (measured: "account=27 typed" -> the run never left the login form).
for i in range(20):
    kind, d = R.screen_kind()
    if kind == 'dialog':
        R.click_frac(0.500, 0.476)       # 确定
        step('dismissed re-login dialog (d=%.1f)' % d)
        time.sleep(2.0)
        break
    time.sleep(1.0)
else:
    step('no re-login dialog')

# 2) wait until the LOGIN form is really on screen
kind, d = None, -1
for i in range(24):
    kind, d = R.screen_kind()
    if kind == 'login' and d < 18:
        break
    time.sleep(1.0)
step('login screen: %s (d=%.1f)' % (kind, d if kind else -1))
R.focus()
R.shot('ro_0_login.jpg', 700)
R.click_frac(*R.LOGIN_ACCOUNT)
time.sleep(0.5)
R.paste(acct)
time.sleep(0.4)
R.click_frac(*R.LOGIN_PASSWORD)
time.sleep(0.5)
# The 密码 box refuses Ctrl+V and ignores SendKeys at times (see _rdp
# .set_field_text's own note) - measured here as "账号=26, 密码 empty", which
# silently made the whole run fail at the login screen.
if not R.set_field_text(R.LOGIN_PASSWORD[0], R.LOGIN_PASSWORD[1], acct):
    R.paste(acct)
time.sleep(0.6)
R.shot('ro_1_filled.jpg', 700)
R.click_frac(*R.LOGIN_BUTTON)
step('account=%s typed, 登录 clicked' % acct)

# 2) server list -> 一线 -> 进入
time.sleep(7.0)
R.click_frac(0.430, 0.410)          # 一线
time.sleep(0.8)
R.click_frac(0.393, 0.925)          # 进入
step('line + 进入 clicked')
time.sleep(8.0)

# 3) character list -> 创建角色 -> form
R.click_frac(*R.CHAR_CREATE_OK)
time.sleep(3.0)
R.click_frac(0.370, 0.705)          # school
time.sleep(0.5)
R.click_frac(0.860, 0.720)          # name field
time.sleep(0.5)
R.paste(name)
time.sleep(0.6)
R.shot('ro_2_form.jpg', 700)

# 4) ONE submit, then look
R.keys('{ENTER}')
step('submit: 1 ENTER fired')
time.sleep(10.0)
R.shot('ro_3_submit.jpg', 700)
v1, d1 = _ingame_sig.classify('ro_3_submit.jpg')
step('after submit: %s (distance %.1f)' % (v1, d1))
k1, kd1 = R.screen_kind()
step('screen_kind: %s (%.1f)' % (k1, kd1))

# 5) the hammer, in game
if count > 1:
    t1 = time.time()
    R.keys('{ENTER %d}' % (count - 1))
    step('hammer: {ENTER %d} in %.3fs' % (count - 1, time.time() - t1))
    time.sleep(6.0)
    R.shot('ro_4_hammer.jpg', 700)
    v2, d2 = _ingame_sig.classify('ro_4_hammer.jpg')
    step('after hammer: %s (distance %.1f)' % (v2, d2))
    step('VERDICT: %s' % ('PASS' if (v1 == 'INGAME' and v2 == 'INGAME') else 'FAIL'))
else:
    step('VERDICT: %s' % ('PASS' if v1 == 'INGAME' else 'FAIL'))
