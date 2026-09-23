#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C2b: the telnet branch of create_thread stops using an OS thread.

    python -X utf8 _vmchain_codegen/stepC2_telnet3.py

`create_thread` keeps its `isTelnetThread` special case only because the telnet
daemon used to need its own thread; with `Scheduler::spawnTelnet` there is no
reason left, so the branch becomes an ordinary scheduler spawn and the
`std::thread` fallback (the last one on the game/telnet path) disappears.

The shared-objVars setup is exactly what the previous `spawn` branch already does,
so the telnet branch reuses it (a telnet daemon must see the same instance
variables as its creator).
"""
import io

SRC = r'src\original_vm.cpp'

OLD = '''                std::thread([objPtr, owner, funName, threadArgs, root, isTelnetThread, telnetCno,
                             objVarsSnapshot = std::move(objVarsSnapshot)]() {'''

def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    o = OLD.replace('\n', sep)
    if t.count(o) != 1:
        print('anchor COUNT %d' % t.count(o))
        return 1

    # find the whole statement: from the anchor line to the matching `}).detach();`
    lines = t.split(sep)
    start = next(i for i, l in enumerate(lines) if l.strip().startswith('std::thread([objPtr, owner, funName, threadArgs, root, isTelnetThread'))
    depth = 0
    end = -1
    for i in range(start, len(lines)):
        depth += lines[i].count('(') - lines[i].count(')')
        if depth <= 0 and '.detach()' in lines[i]:
            end = i
            break
    if end < 0:
        print('end not found')
        return 1
    removed = sep.join(lines[start:end + 1])
    # the lambda counts braces; the statement is a `std::thread(...)` call
    if removed.count('std::thread(') != 1 or removed.count('.detach()') != 1:
        print('refusing: %d spawns / %d detaches' % (removed.count('std::thread('),
                                                     removed.count('.detach()')))
        return 1
    if removed.count('}') != removed.count('{'):
        print('refusing: unbalanced braces in the removed statement (%d/%d)'
              % (removed.count('{'), removed.count('}')))
        return 1

    indent = lines[start][:len(lines[start]) - len(lines[start].lstrip())]
    replacement = [
        indent + '// Telnet daemons are ordinary scheduler processes now (Step C2):',
        indent + '// the client id travels with the process, so LPC keeps running on',
        indent + '// the one scheduler thread and no OS thread is created here.',
        indent + '{',
        indent + '    auto vm = std::make_shared<OriginalVm>(objPtr, root);',
        indent + '    {',
        indent + '        auto& reg = objectRegistry();',
        indent + '        std::lock_guard<std::recursive_mutex> rl(registryMutex());',
        indent + '        for (auto& kv : reg.vms) {',
        indent + '            if (&kv.second->object() == owner) {',
        indent + '                vm->shareObjVars(kv.second.get());',
        indent + '                if (vm->thisObjectName() == "<this>")',
        indent + '                    vm->setThisObjectName(kv.second->thisObjectName());',
        indent + '                break;',
        indent + '            }',
        indent + '        }',
        indent + '    }',
        indent + '    if (vmEnvTrace())',
        indent + '        std::cerr << "[sched] spawn telnet " << funName',
        indent + '                  << " cno=" << telnetCno << "\\n";',
        indent + '    scheduler().spawnTelnet(vm, funName, threadArgs, telnetCno);',
        indent + '    return make_int(1);',
        indent + '}',
    ]
    lines[start:end + 1] = replacement
    out = sep.join(lines)
    open(SRC, 'wb').write(out.encode('utf-8'))
    print('OK telnet OS thread replaced by scheduler().spawnTelnet (lines %d..%d)'
          % (start + 1, end + 1))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
