# -*- coding: utf-8 -*-
"""监视 warwatch1_run 各 live 日志的新增行，发现战斗探针/反作弊/真错误立即打印。
不启动服务、不改文件，只读。每 60s 增量扫描一次，运行 N 秒后退出。"""
import sys, time, os, io

RUNDIR = sys.argv[1] if len(sys.argv) > 1 else "tmp/warwatch1_run"
DURATION = int(sys.argv[2]) if len(sys.argv) > 2 else 14400
INTERVAL = 60

PATS = ("[war-entry]", "[war-call]", "[war-timer]", "[war]", "[ac]",
        "[pos]", "challenge_dump", "Too depth", "fatal",
        "Access violation", "0xc0000005")

def open_log(p):
    return io.open(p, "r", encoding="utf-8", errors="replace")

logs = {}
deadline = time.time() + DURATION
while time.time() < deadline:
    try:
        names = [f for f in os.listdir(RUNDIR) if f.startswith("live_") and f.endswith(".txt")]
    except OSError:
        names = []
    for name in names:
        path = os.path.join(RUNDIR, name)
        try:
            sz = os.path.getsize(path)
        except OSError:
            continue
        if name not in logs:
            logs[name] = {"pos": max(0, sz - 4096), "size": sz}
            continue
        st = logs[name]
        if sz > st["size"]:
            with open_log(path) as fh:
                fh.seek(st["size"])
                new = fh.read()
            for line in new.splitlines():
                if any(p in line for p in PATS):
                    ts = time.strftime("%H:%M:%S")
                    print(f"[{ts}][{name}] {line[:400]}", flush=True)
            st["size"] = sz
        elif sz < st["size"]:
            st["size"] = sz  # log rotated/truncated
    time.sleep(INTERVAL)
print("[watch-done]", flush=True)
