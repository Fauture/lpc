import subprocess
for img in ('mspdbsrv.exe', 'cl.exe', 'MSBuild.exe'):
    r = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq ' + img], capture_output=True)
    out = r.stdout.decode('gbk', errors='replace')
    hits = [l for l in out.splitlines() if img.split('.')[0] in l.lower()]
    print(img, '->', hits if hits else 'none')
