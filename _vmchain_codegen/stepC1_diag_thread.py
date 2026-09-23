#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C1: the last diagnostic that ran LPC on its own OS thread -> scheduler.

    python -X utf8 _vmchain_codegen/stepC1_diag_thread.py

`src/original_vm.cpp` still spawned a detached `std::thread` to call
`callOriginalObjectFunction("/gs/daemons/taskd.c", "query_all_templates", ...)`
1.5 s later.  Running LPC off the scheduler thread is exactly what the
`execMutex_`/`execDepth_` machinery exists to survive; every such site removed is
one step closer to deleting that machinery (and with it the lock the VM still
takes on every entry).

The diagnostic becomes a scheduler process with a 1500 ms delay - same output,
but the LPC runs where all LPC now runs (LDMud's backend loop has no other way to
run a script either).
"""
import io

SRC = r'src\original_vm.cpp'

OLD_START = '''                                        std::string root = searchRoot();
                                        std::thread([root]() {
                                            std::this_thread::sleep_for(
                                                std::chrono::milliseconds(1500));'''
OLD_END = '''                                        }).detach();
                                    }
                                }'''

NEW = '''                                        // Run the diagnostic as a SCHEDULER process
                                        // with a 1500 ms delay: LPC only ever runs on
                                        // the scheduler thread (see
                                        // docs/scheduler_refactor_plan.md, Step C).
                                        std::string root = searchRoot();
                                        {
                                            auto tvm = std::make_shared<OriginalVm>(
                                                sharedObject(), root);
                                            tvm->shareObjVars(this);
                                            tvm->setThisObjectName(thisObjectName());
                                            scheduler().spawnDelayed(tvm, "__treg_dump",
                                                                     {}, nowMs() + 1500, nullptr);
                                        }
                                    }
                                }'''


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    i = t.find(OLD_START.replace('\n', sep))
    j = t.find(OLD_END.replace('\n', sep), i)
    if i < 0 or j < 0:
        print('anchors not found (%d/%d)' % (i, j))
        return 1
    j += len(OLD_END.replace('\n', sep))
    t = t[:i] + NEW.replace('\n', sep) + t[j:]
    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK diagnostic thread -> scheduler process')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
