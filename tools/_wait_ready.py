# -*- coding: utf-8 -*-
"""Fast readiness poller: exits THE MOMENT gs 8160 is listening.
Usage: python -X utf8 tools/_wait_ready.py [timeout_sec] [port]
Polls every 2s; prints one line per check. Never sleeps a long fixed time.
"""
import socket
import sys
import time

timeout = int(sys.argv[1]) if len(sys.argv) > 1 else 540
port = int(sys.argv[2]) if len(sys.argv) > 2 else 8160

t0 = time.time()
while True:
    s = socket.socket()
    s.settimeout(1.0)
    try:
        s.connect(("127.0.0.1", port))
        s.close()
        print("READY port=%d after %.0fs" % (port, time.time() - t0))
        sys.exit(0)
    except OSError:
        s.close()
    if time.time() - t0 >= timeout:
        print("TIMEOUT port=%d after %.0fs" % (port, time.time() - t0))
        sys.exit(1)
    time.sleep(2)
