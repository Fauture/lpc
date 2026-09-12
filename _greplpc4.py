import io, os, re, sys

needle = sys.argv[1]
root = r'linux\gs\pack_data\lib_gs32-c\gs'
pat = re.compile(r'\b' + re.escape(needle) + r'\s*\(')
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
            if pat.search(line):
                print(p, line_no, line.strip()[:140])
                n += 1
                if n > 25:
                    print('...')
                    sys.exit(0)
print('total', n)
