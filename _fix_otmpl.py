# -*- coding: utf-8 -*-
"""Bound the [otmpl] trace to gift/newbie objects (avoid a global flood)."""
p = 'src/original_vm.cpp'
s = open(p, 'rb').read().decode('utf-8', 'replace')
old1 = "                if (maybeEvDbgGift() && tvm)"
new1 = ("                const bool otmplTrace = vmEnvEvDbg() &&\n"
        "                    (objName_.find(\"newbie\") != std::string::npos ||\n"
        "                     objName_.find(\"gift\") != std::string::npos);\n"
        "                if (otmplTrace && tvm)")
old2 = "                if (maybeEvDbgGift())"
new2 = "                if (otmplTrace)"
n1 = s.count(old1)
n2 = s.count(old2)
s = s.replace(old1, new1).replace(old2, new2)
open(p, 'wb').write(s.encode('utf-8'))
print('replaced1=%d replaced2=%d otmplTrace=%d' % (n1, n2, s.count('otmplTrace')))
