# -*- coding: utf-8 -*-
"""Newbie-gift probe: apply every bag slot and look for the gift UI (0x21B7).

`taskd->query_entry("新手礼包")->open_newgift_item(item)` sends 8631 (0x21B7)
with the item list when the gift still has its `gift_info`; when the gift was
built without `set_gift_item()`/`init_newbie_gift_items()` it is `is_empty()`,
so it is CONSUMED and replaced by the next-level gift instead (0x21B7 never
arrives) - exactly the "打开后生成一个新的新手礼包" symptom.

usage: python -X utf8 _gift_probe.py [acct] [pwd] [char] [maxpos]
"""
import importlib.util
import io
import struct
import sys
import time

spec = importlib.util.spec_from_file_location('ll', '_login_lib.py')
ll = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ll)
ep = ll.ep
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

ACCT = sys.argv[1] if len(sys.argv) > 1 else '1'
PWD = sys.argv[2] if len(sys.argv) > 2 else '1'
CHAR = sys.argv[3] if len(sys.argv) > 3 else 'gift1'
# Bag slots are numbered from carry.c::get_inv_start_pos() == 101 (not 0)!
POS0 = int(sys.argv[4]) if len(sys.argv) > 4 else 101
POS1 = int(sys.argv[5]) if len(sys.argv) > 5 else 145

# cmd numbers worth reporting when they show up
NAMES = {
    0x21B7: 'GIFT-UI(8631)',
    0x1FE5: 'dialog',
    0x3393: 'sysmsg',
    0x2603: 'task_log',
    0xF9FB: 'task_trace',
    0x5105: 'bag?',
    0xF05D: 'object',
    0x2131: 'objinfo',
    0x1155: 'pos',
}


def main():
    s = ll.Session(ACCT, PWD, CHAR, tag='G')
    s.connect(window=25)
    print('uid=%s pos=%s dialogs=%s' % (('0x%X' % s.uid) if s.uid else None, s.pos, s.dialogs), flush=True)
    if not s.g:
        return
    s.pump(2)

    print('--- applying bag slots %d..%d ---' % (POS0, POS1), flush=True)
    for pos in range(POS0, POS1 + 1):
        s.cmds = []
        s.g.sendall(ep.frame(0x220C, bytes([pos])))
        t0 = time.time()
        seen = []
        while time.time() - t0 < 0.7:
            r = ep.read_pkt(s.g, 0.25)
            if r is None:
                print('   connection closed at pos=%d' % pos, flush=True)
                return
            if r[0] == 'timeout':
                continue
            seen.append(r[0])
            if r[0] == 0x21B7:
                print('   *** pos=%d -> GIFT UI 0x21B7 len=%d hex=%s'
                      % (pos, len(r[1]), r[1].hex()[:200]), flush=True)
            elif r[0] in (0x1FE5, 0x3393) and len(r[1]) > 4:
                try:
                    txt = r[1][1:].split(b'\x00')[0].decode('gbk', 'replace')
                    print('   pos=%d dialog: %s' % (pos, txt[:120]), flush=True)
                except Exception:
                    pass
        interesting = sorted(set(c for c in seen if c in NAMES))
        if interesting:
            print('   pos=%-3d cmds=%s' % (pos, [NAMES[c] for c in interesting]), flush=True)
    print('--- idle 8s ---', flush=True)
    s.pump(8)
    print('done. dialogs=%s' % (s.dialogs,), flush=True)
    s.close()


if __name__ == '__main__':
    main()
