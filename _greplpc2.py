import io, os, re, sys

root = r'linux\gs\pack_data\lib_gs32-c\gs'
pat = re.compile(r'^\s*(?:private\s+|static\s+|varargs\s+|nomask\s+)?(?:mixed|void|int|string|array|object|mapping)\s+(process_command|do_command|query_command|register_command|set_command)\s*\(', re.M)
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
        for m in pat.finditer(d):
            line = d[:m.start()].count('\n') + 1
            print(p, line, d.split('\n')[line - 1].strip()[:140])
            n += 1
print('total', n)
