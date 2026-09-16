# -*- coding: utf-8 -*-
"""Verify the filedFns list now contains the task chain."""
import io

d = io.open('src/original_vm.cpp', encoding='utf-8', errors='replace').read()
i = d.find('filedFns[] = {')
j = d.find('};', i)
print(d[i:j + 2].replace('\r', ''))
