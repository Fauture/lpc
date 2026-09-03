# -*- coding: utf-8 -*-
# List pak catalog order for a virtual dir (e.g. zone/ or daemons/) so we can
# compare the original's load order source with our filesystem enumeration.
import sys, struct

def rd32(d, off):
    return d[off] | (d[off+1]<<8) | (d[off+2]<<16) | (d[off+3]<<24)

def parse_table(d, off, max_entries=0):
    out = []
    scanned = 0
    bad = 0
    while off + 0x40 <= len(d):
        if max_entries and scanned >= max_entries: break
        name_base = off + 0x14
        # name printable check
        nm = b''
        printable = True
        anyc = False
        for i in range(name_base, min(name_base+44, len(d))):
            c = d[i]
            if c == 0: break
            if not (32 <= c < 127): printable = False; break
            nm += bytes([c]); anyc = True
        typ = rd32(d, off); child = rd32(d, off+4)
        comp = rd32(d, off+8); orig = rd32(d, off+12)
        if not (printable and anyc and child < len(d)):
            bad += 1
            if bad >= 2: break
            off += 0x40; scanned += 1
            continue
        bad = 0
        isdir = (typ & 1) != 0
        children = []
        if isdir:
            children = parse_table(d, child, comp//0x40)
        out.append({'name': nm.decode('ascii', 'replace'), 'dir': isdir, 'children': children})
        off += 0x40; scanned += 1
    return out

def find_dir(entries, parts):
    cur = entries
    for p in parts:
        nxt = None
        for e in cur:
            if e['name'] == p and e['dir']:
                nxt = e['children']; break
        if nxt is None: return None
        cur = nxt
    return cur

def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'linux/gs/pack_data/lib_gs32.pak'
    vdir = sys.argv[2] if len(sys.argv) > 2 else 'zone'
    d = open(path, 'rb').read()
    root = parse_table(d, 0x10)
    parts = [p for p in vdir.split('/') if p]
    # strip optional gs/ leading if not present in pak root
    names0 = [e['name'] for e in root]
    print('root entries (%d): %s' % (len(root), names0[:20]))
    cur = root
    for p in parts:
        if p not in [e['name'] for e in cur]:
            # try without gs prefix: root may already be gs
            break
        cur = [e for e in cur if e['name'] == p][0]['children']
    # find partial
    cur2 = root
    for p in parts:
        hit = [e for e in cur2 if e['name'] == p and e['dir']]
        if not hit:
            # maybe last component is a dir we should list
            print('cannot descend: %s (have: %s)' % (p, [e['name'] for e in cur2][:15]))
            return
        cur2 = hit[0]['children']
    files = [e for e in cur2 if not e['dir']]
    dirs = [e for e in cur2 if e['dir']]
    print('dir %s: %d files, %d dirs' % (vdir, len(files), len(dirs)))
    print('files:', [e['name'] for e in files][:40])

main()
