import io, os, re, sys

needle = sys.argv[1] if len(sys.argv) > 1 else 'cmd_enter_room'
root = sys.argv[2] if len(sys.argv) > 2 else r'linux\gs\pack_data\lib_gs32-c\gs'
n = 0
for dirpath, dirnames, filenames in os.walk(root):
    for f in filenames:
        if not f.endswith('.c'):
            continue
        p = os.path.join(dirpath, f)
        try:
            d = io.open(p, 'rb').read().decode('gbk', 'replace')
        except Exception:
            continue
        for line_no, line in enumerate(d.split('\n'), 1):
            if needle in line and ('register' in line or 'cmd_map' in line or '=>' in line
                                   or ':' in line and '"' in line and 'void ' not in line):
                print(p, line_no, line.strip()[:150])
                n += 1
print('total', n)
