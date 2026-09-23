# -*- coding: utf-8 -*-
"""Enter-game test, FAIR variant: ONE real submit, then the 19-ENTER hammer IN GAME.

Why a second harness: `_rapid2.py` fires all 20 ENTERs back to back while the
character form is still open, so the client submits create_new_char 8 times in
~800 ms.  The server's own timeline for such a run (vF13, account 20, cno=41):

    1061783 recv 0x0472  -> create_new_char      <- the FIRST submit succeeds
    1061890/1061952/1062031 recv 0x0472          <- duplicate submits
    1062240 DIALOG 请求处理中，请稍候！（原因：9101）
    1062250 DIALOG 这个角色名字已经存在。（原因：9113）
    1062317 recv 0x0472 n=100 (three ENTERs in one read)
    1062320 DIALOG 无法使用这个名字创建角色，请重新尝试
    1062370/377/381/384 DIALOG 9101 x4 ; 1062385 DIALOG 9113
    1062519 CHARTLIST body=112                   <- the character WAS created
    1062612 load_existed_char                    <- and the client entered the game

Every 9101/9113 in that run answers a duplicate submit the harness caused.  This
script therefore measures the two things separately:

  1. submit  - ONE ENTER: does the character get created and does the client
               reach the game?  (the real acceptance question)
  2. hammer  - the remaining `count-1` ENTERs fired IN GAME: is the session
               still alive and in game afterwards?  (anti-regression)

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
T0 = time.time()


def step(msg):
    print('[%7.1fs] %s' % (time.time() - T0, msg))
    sys.stdout.flush()


def log(msg):
    print('          %s' % msg)
    sys.stdout.flush()


def ingame(tag, wait=0.0):
    """Screenshot + reference-frame verdict; optionally poll until INGAME."""
    deadline = time.time() + wait
    v, d = 'not-ingame', -1.0
    while True:
        R.shot('ro_%s.jpg' % tag, 700)
        v, d = _ingame_sig.classify('ro_%s.jpg' % tag)
        if v == 'INGAME' or time.time() >= deadline:
            return v, d
        time.sleep(1.5)


subprocess.run(['taskkill', '/F', '/IM', 'asktao.mod'], capture_output=True)
time.sleep(3)
R.start_client()
time.sleep(5)

# ---- 1. the client's own re-login dialog, then a settled login form --------
for i in range(30):
    kind, d = R.screen_kind()
    if kind == 'dialog':
        R.click_frac(0.500, 0.476)       # 确定
        step('dismissed re-login dialog (d=%.1f)' % d)
        time.sleep(2.5)
        break
    time.sleep(1.0)
else:
    step('no re-login dialog')

kind, d = None, -1
for i in range(24):
    kind, d = R.screen_kind()
    if kind == 'login' and d < 18:
        break
    time.sleep(1.0)
step('login screen: %s (d=%.1f)' % (kind, d if kind is not None else -1))
R.focus()
R.shot('ro_0_login.jpg', 700)

# ---- 2. credentials --------------------------------------------------------
R.click_frac(*R.LOGIN_ACCOUNT)
time.sleep(0.5)
R.paste(acct)
time.sleep(0.4)
R.click_frac(*R.LOGIN_PASSWORD)
time.sleep(0.5)
# The 密码 box refuses Ctrl+V / ignores SendKeys (see _rdp.set_field_text); a
# silent failure here looks like "account=26 typed" with an empty password box.
ok = R.set_field_text(R.LOGIN_PASSWORD[0], R.LOGIN_PASSWORD[1], acct)
if not ok:
    R.paste(acct)
time.sleep(0.6)
R.shot('ro_1_filled.jpg', 700)
R.click_frac(*R.LOGIN_BUTTON)
step('account=%s typed + 登录 clicked' % acct)

# ---- 3. server line -> 进入 ------------------------------------------------
time.sleep(7.0)
R.click_frac(0.430, 0.410)          # 一线
time.sleep(0.8)
R.click_frac(0.393, 0.925)          # 进入
step('line + 进入 clicked')

# wait for the character list to be up (screen_kind knows it)
k, kd = None, -1
for i in range(16):
    k, kd = R.screen_kind()
    if k in ('charsel', 'createchar', 'ingame'):
        break
    time.sleep(1.0)
step('character page: %s (d=%.1f)' % (k, kd))
time.sleep(2.0)

# ---- 4. 创建角色 -> school -> name ----------------------------------------
R.click_frac(*R.CHAR_CREATE_OK)
time.sleep(3.0)
R.click_frac(0.370, 0.705)          # school
time.sleep(0.5)
R.click_frac(0.860, 0.720)          # name field
time.sleep(0.5)
R.paste(name)
time.sleep(0.6)
R.shot('ro_2_form.jpg', 700)

# ---- 5. ONE submit --------------------------------------------------------
t0 = time.time()
R.keys('{ENTER}')
step('submit: 1 ENTER fired')
v1, d1 = ingame('3_submit', wait=14.0)
step('after 1 submit: %s (distance %.1f, %.1fs)' % (v1, d1, time.time() - t0))
k1, kd1 = R.screen_kind()
log('screen_kind: %s (%.1f)' % (k1, kd1))

# ---- 6. the hammer, in game ----------------------------------------------
if count > 1:
    t1 = time.time()
    R.keys('{ENTER %d}' % (count - 1))
    step('hammer: {ENTER %d} fired in %.3fs' % (count - 1, time.time() - t1))
    time.sleep(6.0)
    v2, d2 = ingame('4_hammer')
    step('after hammer: %s (distance %.1f)' % (v2, d2))
    step('VERDICT: %s' % ('PASS' if (v1 == 'INGAME' and v2 == 'INGAME') else 'FAIL'))
else:
    step('VERDICT: %s' % ('PASS' if v1 == 'INGAME' else 'FAIL'))
