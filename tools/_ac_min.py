import subprocess

CASES = [
    ('loop a+=', 'int challenge_server(int x, int y){ mixed i; mixed a; a = ({ }); loop (i = 0 upto 2) a += ({ i + 1 }); return sizeof(a); }', 3),
    ('braced',   'int challenge_server(int x, int y){ mixed i; mixed a; a = ({ }); loop (i = 0 upto 2) { a += ({ i + 1 }); } return sizeof(a); }', 3),
    ('map +=',   'int challenge_server(int x, int y){ mixed i; mixed m; m = allocate_mapping(4); m[1] = 10; loop (i = 0 upto 2) m[1] += i; return m[1]; }', 13),
    ('str +=',   'int challenge_server(int x, int y){ mixed i; string s; s = ""; loop (i = 0 upto 2) s += sprintf("%02X", i); return strlen(s); }', 6),
    ('idx str',  'int challenge_server(int x, int y){ string s; s = "abcdef"; return s[2]; }', 99),
]

for n, f, w in CASES:
    r = subprocess.run(['_clean.exe'], input=f + ' int get_x(){return 1;} int get_y(){return 2;}',
                       capture_output=True, text=True)
    print('%-10s cxx=%-16s want=%s' % (n, r.stdout.strip().replace('\n', '|'), w))
    if r.stderr.strip():
        print('   stderr:', r.stderr.strip()[:200])
