# -*- coding: utf-8 -*-
"""Shim for _login_lib.py: it imports the packet primitives from _enter_probe.py,
which is no longer in the tree.  They live in _login_full.py; recv_until() is added
here.  Local diagnostic helper, never committed.
"""
import importlib.util
import time


def _load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


_lf = _load('_lf', '_login_full.py')

frame = _lf.frame
s8 = _lf.s8
s16 = _lf.s16
read_pkt = _lf.read_pkt
des_encrypt = _lf.des_encrypt


def recv_until(s, cmd, deadline):
    """Read packets until `cmd` arrives or the deadline passes."""
    while time.time() < deadline:
        r = read_pkt(s, 0.5)
        if r is None:
            return None
        if r[0] == 'timeout':
            continue
        if r[0] == cmd:
            return r
    return None
