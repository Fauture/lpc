# -*- coding: utf-8 -*-
"""Check the newbie/story chain + the char's saved position.

  python -X utf8 _check_newbie.py [logdir] [charid]

Greps the gs log for the story chain
  enter_world -> post_enter_world -> newbied::do_when_enter_world ->
  do_when_char_born / do_task_instruction -> try_create_task ->
  raise_event_ex("初进游戏") -> scenario -> do_stage_move(S0) ->
  moved->enter_room(me, 揽仙镇外, 102, 157, dir)
and then reads dl_ddb_1_test.data (path='user') to print the character's
saved `position` (room/x/y/dir) - the teleport target must be
/lanxianzhen/lanxianzhenwai.
"""
import io
import os
import re
import sys

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

LOG = sys.argv[1] if len(sys.argv) > 1 else 'nb6'
CID = sys.argv[2] if len(sys.argv) > 2 else '0000000000000015'

path = os.path.join(LOG, 'live_gs.txt')
data = io.open(path, 'rb').read().decode('utf-8', 'replace').splitlines()
print('gs log lines: %d' % len(data))

KEYS = [
    ('post_enter_world', 'enter-world hook loop'),
    ('fn=do_when_enter_world', 'enter-world hooks invoked'),
    ('newbied', 'newbied object'),
    ('do_when_char_born', 'task: char born'),
    ('do_task_instruction', 'task: instruction'),
    ('try_create_task', 'task created'),
    ('raise_event_ex', 'event raised'),
    ('初进游戏', 'event 初进游戏'),
    ('do_stage_move', 'stage move'),
    ('enter_room', 'room enter'),
    ('揽仙镇外', 'target room name'),
    ('lanxianzhenwai', 'target room path'),
    ('scenariod', 'scenario daemon'),
    ('gift_for_newbie', 'newbie gift'),
    ('set_newbie_task', 'logind set_newbie_task'),
    ('newbie', 'any other newbie mention'),
]
for k, what in KEYS:
    hits = [l for l in data if k in l]
    print('%-28s %-24s %6d' % (k, what, len(hits)))

print()
print('=== enter-world hook applications (distinct closures) ===')
apre = re.compile(r'\[nb-apply\] (\S+) bound=\d+ func=(\S+) owner=(\S+) ptr=(\S+)')
seq = []
for i, l in enumerate(data, 1):
    m = apre.search(l)
    if m:
        seq.append((i, m.group(2), m.group(4)))
print('applies: %d  distinct closures: %d' % (len(seq), len(set(s[2] for s in seq))))
if seq:
    import collections
    ptrs = collections.Counter(s[2] for s in seq)
    print('most repeated closure ptr: %s x%d' % ptrs.most_common(1)[0][::1])

print()
print('=== story-chain lines (last 60) ===')
STORY = ('do_when_char_born', 'do_task_instruction', 'try_create_task', 'raise_event_ex',
         'do_stage_move', 'enter_room', 'gift_for_newbie', '初进游戏', 'scenariod',
         'newbie_task', 'newbie-gift-task')
hits = [l for l in data if any(k in l for k in STORY)]
print('matching lines: %d' % len(hits))
for l in hits[-60:]:
    print('  ' + l[:240])

print()
print('=== saved char position (dl_ddb_1_test.data path=user name=%s) ===' % CID)
try:
    import pymysql
    conn = pymysql.connect(host='192.168.200.128', port=3306, user='root',
                           password='123456', charset='utf8', database='dl_ddb_1_test')
    cur = conn.cursor()
    cur.execute("SELECT content FROM `data` WHERE path='user' AND `name`=%s", (CID,))
    row = cur.fetchone()
    if row:
        c = row[0]
        s = bytes(c).decode('latin-1') if isinstance(c, (bytes, bytearray)) else str(c)
        m = re.search(r'"position":\((.*?)\),', s)
        print('position:', m.group(1) if m else '(not found)')
        for key in ('"newbie"', '"task"', '"first_login"'):
            print('%s present: %s' % (key, key in s))
    else:
        print('no user row for', CID)
    conn.close()
except Exception as e:
    print('db check failed:', e)
