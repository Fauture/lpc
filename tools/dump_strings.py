# -*- coding: utf-8 -*-
"""Dump string table entries 160-180 from start_ccs.o."""
import struct
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lpc_decompiler_v37 import LPCOFile

o = LPCOFile(r'C:\Users\Administrator\Desktop\lpc\linux\ccs\pack_data\lib_ccs32\ccs\start_ccs.o', encoding='gb2312').parse()
for i in range(160, 180):
    if i < len(o.str_table):
        s = o.str_table[i]
        print("[%d] %r" % (i, s))
