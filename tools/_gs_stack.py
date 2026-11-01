# -*- coding: utf-8 -*-
"""抓最大内存 lpcvm（gs）的 minidump 并解析各线程 EIP。"""
import ctypes, struct, subprocess, os, sys

out = subprocess.check_output(['tasklist', '/FI', 'IMAGENAME eq lpcvm.exe', '/FO', 'CSV'],
                              text=True, errors='replace')
rows = []
for l in out.splitlines():
    if 'lpcvm' not in l:
        continue
    parts = [p.strip('"') for p in l.split('","')]
    pid = int(parts[1])
    mem = int(parts[-1].replace(' K', '').replace(',', '').replace('.', '').strip())
    rows.append((pid, mem))
pid = max(rows, key=lambda x: x[1])[0]
print('gs pid', pid)

k32 = ctypes.windll.kernel32
h = k32.OpenProcess(0x1F0FFF, False, pid)
dmp = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tmp', 'gs_dump.dmp')
f = k32.CreateFileW(dmp, 0x40000000, 0, None, 2, 0, None)
ok = ctypes.windll.dbghelp.MiniDumpWriteDump(h, pid, f, 0, None, None, None)
k32.CloseHandle(f)
size = os.path.getsize(dmp)
print('dump ok', ok, 'size', size)

data = open(dmp, 'rb').read()
sig, ver, nstreams, dirrva, checksum, ts, flags = struct.unpack_from('<IIIIIIQ', data, 0)
assert sig == 0x504D444D, hex(sig)
streams = {}
for i in range(nstreams):
    t, dsz, drva = struct.unpack_from('<III', data, dirrva + 12 * i)
    streams[t] = (dsz, drva)
# ModuleListStream=4: 找 lpcvm.exe 基址
dsz, off = streams[4]
nmod = struct.unpack_from('<I', data, off)[0]
base = None
for i in range(nmod):
    mo = off + 4 + i * 108
    name_rva = struct.unpack_from('<I', data, mo + 20)[0]   # RVA ModuleNameRva @20
    mlen = struct.unpack_from('<I', data, name_rva)[0]
    nm = data[name_rva + 4:name_rva + 4 + mlen].decode('utf-16-le', 'replace')
    vb = struct.unpack_from('<Q', data, mo)[0]
    if 'lpcvm' in nm.lower():
        base = vb
        print('lpcvm base=0x%x' % base)
# ThreadListStream=3: 各线程的 Eip
dsz, off = streams[3]
nthr = struct.unpack_from('<I', data, off)[0]
print('threads:', nthr)
# 粗符号表: 从 lpcvm.map 取 "地址 名字"
syms = []
try:
    for ml in open(r'C:\Users\Administrator\Desktop\lpc\build\Release\lpcvm.map',
                   'r', errors='replace'):
        p = ml.split()
        if len(p) >= 2 and ':' in p[0] and p[0].split(':')[0] in ('0001','0002') and p[0].split(':')[0]=='0001':
            try:
                syms.append((int(p[0].split(':')[1], 16), p[1]))
            except ValueError:
                pass
    syms.sort()
except OSError:
    pass
import bisect
def sym(a):
    if not base or not (base <= a < base + 0x2000000):
        return ''
    rva = a - base
    i = bisect.bisect_right([s[0] for s in syms], rva) - 1
    if 0 <= i < len(syms):
        return ' %s+0x%x' % (syms[i][1], rva - syms[i][0])
    return ' lpcvm+0x%x' % rva
for i in range(nthr):
    to = off + 4 + i * 48
    tid, susp, prio_class, prio, teb, stk_start, stk_size, stk_rva, ctx_size, ctx_rva = \
        struct.unpack_from('<IiIiQQIIII', data, to)
    eip = struct.unpack_from('<Q', data, ctx_rva + 0xF8)[0]
    print('tid=%-6d eip=%s susp=%d' % (tid, sym(eip) or hex(eip), susp))
    fr = []
    for k in range(stk_size // 8):
        v = struct.unpack_from('<Q', data, stk_rva + 8 * k)[0]
        sc = sym(v)
        if sc:
            fr.append(sc.strip())
    print('    ', ' <- '.join(fr[:14]))
