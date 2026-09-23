# -*- coding: utf-8 -*-
"""Fair enter-game test: submit ONCE, enter the game ONCE, then hammer IN GAME.

This is the corrected shape of `_rapid2.py`'s 20-ENTER burst.  What the client
actually needs (read off the screenshots and the server log):

  1 ENTER  - submit the create-character form
             (screenshot: the character LIST with 名称 rfCa / 门派 无)
  1 ENTER  - enter the game from that list
  18 ENTER- the hammer, delivered IN GAME

Evidence that this is the real flow, and that the extra dialogs in the 20-burst
runs are a harness artifact (vF13):

  rfBa/rfBb/rfCa  creates=1 loads=1 dialogs[9101=0 9113=0 9112=0]   <- 1 submit
  rf8a/rf8b/rf8c  creates=11 loads=1 dialogs[9101=6 9113=3]        <- 20 burst

i.e. ONE submit produces ZERO dialogs; the burst makes the client submit the
form 8 times because the ENTERs arrive while the form is still up.

usage: python -X utf8 _rapid_fair.py <acct> <name> [count]
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
name = sys.argv[2] if len(sys.argv) > 2 else 'rapidfair'
count = int(sys.argv[3]) if len(sys.argv) > 3 else 20
T0 = time.time()


def step(msg):
    print('[%7.1fs] %s' % (time.time() - T0, msg))
    sys.stdout.flush()


def look(tag, wait=0.0, want='INGAME'):
    """Screenshot and classify; with `wait` keep polling until `want` or timeout."""
    deadline = time.time() + wait
    v, d = 'not-ingame', -1.0
    while True:
        R.shot('rf_%s.jpg' % tag, 700)
        v, d = _ingame_sig.classify('rf_%s.jpg' % tag)
        if v == want or time.time() >= deadline:
            return v, d
        time.sleep(2.0)


# pass 1: the proven login/creation sequence, ONE submit (count=1)
rc = subprocess.run([sys.executable, '-X', 'utf8', '_rapid2.py', acct, name, '1'],
                    capture_output=True, text=True, encoding='utf-8', errors='replace')
for l in [l for l in (rc.stdout or '').split('\n') if l.strip()][-3:]:
    print('   rap2| %s' % l)
v0, d0 = _ingame_sig.classify('r2_5_after.jpg')
step('after form submit: %s (distance %.1f)' % (v0, d0))

R.focus()

# pass 2: ONE ENTER to enter the game from the character list
R.keys('{ENTER}')
step('enter-game: 1 ENTER fired')
v1, d1 = look('1_ingame', wait=16.0)
step('after enter: %s (distance %.1f)' % (v1, d1))

# pass 3: the hammer, IN GAME
v2, d2 = v1, d1
if count > 2:
    t1 = time.time()
    R.keys('{ENTER %d}' % (count - 2))
    step('hammer: {ENTER %d} fired in %.3fs' % (count - 2, time.time() - t1))
    time.sleep(6.0)
    v2, d2 = look('2_hammer')
    step('after hammer: %s (distance %.1f)' % (v2, d2))

step('VERDICT: %s' % ('PASS' if (v1 == 'INGAME' and v2 == 'INGAME') else 'FAIL'))
