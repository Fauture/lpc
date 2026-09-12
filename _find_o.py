import os, sys

needle = (sys.argv[1] if len(sys.argv) > 1 else 'register_enter_world').encode()
root = sys.argv[2] if len(sys.argv) > 2 else r'linux\gs\pack_data\lib_gs32'
hits = []
for dirpath, dirnames, filenames in os.walk(root):
    for f in filenames:
        if not f.endswith('.o'):
            continue
        p = os.path.join(dirpath, f)
        try:
            d = open(p, 'rb').read()
        except Exception:
            continue
        if needle in d:
            hits.append((p, d.count(needle)))
hits.sort(key=lambda x: -x[1])
for p, c in hits[:25]:
    print(c, p)
print('total', len(hits))
