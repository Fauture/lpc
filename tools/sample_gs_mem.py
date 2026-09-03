# -*- coding: utf-8 -*-
"""Sample gs process memory + log progress over startup."""
import subprocess, sys, time, os, io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

env = dict(os.environ)
env['LPCVM_TRACE'] = '1'
env['LPCVM_INHINIT'] = '1'
log = open('logs_gsAX_err.txt', 'wb')
p = subprocess.Popen(['build/Release/lpcvm.exe', 'run', 'linux/gs/pack_data/lib_gs32.pak'],
                     stdout=log, stderr=log, env=env)
print('pid', p.pid)
import ctypes
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
h = ctypes.windll.kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, p.pid)
class PMC(ctypes.Structure):
    _fields_ = [('cb', ctypes.c_ulong), ('PageFaultCount', ctypes.c_ulong),
                ('PeakWorkingSetSize', ctypes.c_size_t), ('WorkingSetSize', ctypes.c_size_t),
                ('QuotaPeakPagedPoolUsage', ctypes.c_size_t), ('QuotaPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t), ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
                ('PagefileUsage', ctypes.c_size_t), ('PeakPagefileUsage', ctypes.c_size_t)]
pmc = PMC()
last_line = -1
for i in range(120):
    time.sleep(2)
    ctypes.windll.psapi.GetProcessMemoryInfo(h, ctypes.byref(pmc), ctypes.sizeof(pmc))
    mb = pmc.WorkingSetSize / (1024*1024)
    pf = pmc.PagefileUsage / (1024*1024)
    try:
        with open('logs_gsAX_err.txt', 'rb') as f:
            f.seek(0, 2)
            n = f.tell()
    except Exception:
        n = 0
    print('[%3ds] WS=%.0fMB PF=%.0fMB log=%d' % (i*2, mb, pf, n), flush=True)
    if p.poll() is not None:
        print('process exited', p.poll(), flush=True)
        break
ctypes.windll.kernel32.CloseHandle(h)
