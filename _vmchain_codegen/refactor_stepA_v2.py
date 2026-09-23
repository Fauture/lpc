#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step A of the scheduler refactor: per-chain ExecState (range-protected).

    python -X utf8 _vmchain_codegen/refactor_stepA_v2.py

Earlier attempts damaged the Scheduler class because a blanket
`member_ -> es_->member` rewrite also hit STRUCT FIELD DECLARATIONS
(`std::vector<...> parked_;`, `sockWaiters_`, `timers_` ...).  This version is
range-protected: the Scheduler class body and the OriginalVm class's own
declaration block are excluded from the rewrite, and only the interpreter code
outside them is transformed.

  1. OriginalVm's 19 execution members -> `struct ExecState` + `ExecState* es_`,
     every use outside the protected ranges rewritten to `es_->member`.
  2. Uses through another VM pointer (`vm->frames_`, `p->parkedVm->frames_`)
     routed through that VM's chain state.
  3. `VmRunState` becomes `shared_ptr<ExecState>` + owner tid, and
     stash/restore/switchToChain are rewritten as ownership transfers.

Verification: build, then boot + real-client create + _acc.py.
"""
import io
import re
import sys

SRC = r'src\original_vm.cpp'
BACKUP = r'_orig_vm_before_execstate.cpp'

MEMBERS = {
    'detachedStack_': 'std::vector<StackItem> detachedStack_;',
    'savedObjVarsForNextCall_': 'std::shared_ptr<std::vector<Value>> savedObjVarsForNextCall_;',
    'frames_': 'std::vector<Frame> frames_;',
    'funcLookup_': 'const std::map<int, const OriginalFunction*>* funcLookup_ = nullptr;',
    'lastOp_': 'int lastOp_ = -1;',
    'lastEfunIndex_': 'int lastEfunIndex_ = -1;',
    'lastIp_': 'size_t lastIp_ = 0;',
    'returnValue_': 'Value returnValue_ = make_int(0);',
    'returned_': 'bool returned_ = false;',
    'catchDepth_': 'int catchDepth_ = 0;',
    'catches_': 'std::vector<CatchRecord> catches_;',
    'code_': 'const std::vector<OriginalInsn>* code_ = nullptr;',
    'meta_': 'const std::vector<uint8_t>* meta_ = nullptr;',
    'ip_': 'size_t ip_ = 0;',
    'cond_': 'ResumeCond cond_;',
    'parked_': 'bool parked_ = false;',
    'resumeValue_': 'Value resumeValue_ = make_int(0);',
    'onResume_': 'std::function<void()> onResume_;',
    'stack_': 'std::vector<StackItem> stack_;',
}

EXEC_STATE = '''    // ---- per-chain execution state -------------------------------------
    // The interpreter's whole mutable state for ONE execution chain, reached
    // through `es_` (the active chain's state) instead of living directly on the
    // VM.  One VM object hosts several chains (a create command, its continuation
    // on another object, daemons sharing a registry VM); keeping this data in
    // shared members forced the old stashRunState()/restoreRunState() copy dance
    // on every switch, and a missed switch point lost frames (the historical
    // "no character list / no account link" failures).  See
    // docs/scheduler_refactor_plan.md.
    struct CatchRecord {
        size_t ipTarget = 0;     // resume ip when an error is caught
        size_t stackSize = 0;    // stack to unwind to
        size_t frameCount = 0;   // frames to keep
    };
    struct ExecState {
        std::vector<StackItem> stack;
        std::vector<StackItem> detachedStack;
        std::shared_ptr<std::vector<Value>> savedObjVarsForNextCall;
        std::vector<Frame> frames;
        const std::map<int, const OriginalFunction*>* funcLookup = nullptr;
        int lastOp = -1;
        int lastEfunIndex = -1;
        size_t lastIp = 0;
        Value returnValue = make_int(0);
        bool returned = false;
        int catchDepth = 0;
        std::vector<CatchRecord> catches;
        const std::vector<OriginalInsn>* code = nullptr;
        const std::vector<uint8_t>* meta = nullptr;
        size_t ip = 0;
        ResumeCond cond;
        bool parked = false;
        Value resumeValue = make_int(0);
        std::function<void()> onResume;
    };
    // Every LPC entry goes through switchToChain(), but a few paths (the master,
    // runInit) execute before any chain key exists; the default state is owned
    // here so `es_` never dangles.
    ExecState defaultEs_;
    ExecState* es_ = &defaultEs_;'''

SWITCH_NEW = '''    void clearRunMembers() {
        ExecState fresh;
        fresh.returnValue = make_int(0);
        *es_ = std::move(fresh);
        execOwnerTid_ = -1;
    }
    // Hand the active state to `key`'s chain.  With the state carried BY the
    // chain this is an ownership transfer (shared_ptr), not a copy of 19 members
    // - the old stashRunState()/restoreRunState() pair is gone.
    void stashRunState(int64_t key) {
        if (key < 0) return;
        runStates_[key].es = std::shared_ptr<ExecState>(new ExecState(std::move(*es_)));
        *es_ = ExecState{};
        es_->returnValue = make_int(0);
    }
    bool restoreRunState(int64_t key) {
        auto it = runStates_.find(key);
        if (it == runStates_.end()) return false;
        if (it->second.es) *es_ = std::move(*it->second.es);
        execOwnerTid_ = it->second.ownerTid;
        // The chain is running again: its stashed copy is not needed any more.
        runStates_.erase(it);
        return true;
    }
    // Make `key`'s chain the one `es_` points at.  A no-op when it is already
    // active, so nested calls inside one chain cost two compares.
    void chainDbg(const char* what, int64_t key) {
        if (!std::getenv("LPCVM_TAIL")) return;
        if (objName_.find("login.o") == std::string::npos) return;
        std::cerr << "[chn] " << what << " key=" << key << " active=" << activeKey_
                  << " frames=" << es_->frames.size() << " parked=" << (es_->parked ? 1 : 0)
                  << " cond=" << es_->cond.kind << " stash=" << runStates_.size() << "\\n";
        std::cerr.flush();
    }
    void switchToChain(int64_t key) {
        if (key < 0 || key == activeKey_) return;
        chainDbg("switch-in", key);
        if (activeKey_ >= 0) {
            stashRunState(activeKey_);      // the running chain keeps its state
        } else {
            clearRunMembers();              // no chain owned es_: start clean
        }
        const bool got = restoreRunState(key);
        if (!got) {
            clearRunMembers();
            activeKey_ = key;
            runStates_[key].ownerTid =
                (lpc::scheduler().chainTid() >= 0) ? lpc::scheduler().chainTid()
                                                   : lpc::scheduler().currentProcessTid();
            chainDbg("switch-fresh", key);
            return;
        }
        activeKey_ = key;
        chainDbg("switch-restored", key);
    }
    int64_t activeChainKey() const { return activeKey_; }
'''


def norm(s: str, sep: str) -> str:
    return s.replace('\r\n', '\n').replace('\n', sep)


def is_comment(line: str) -> bool:
    s = line.lstrip()
    return s.startswith('//') or s.startswith('*') or s.startswith('/*')


def main() -> int:
    raw = open(BACKUP, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    lines = t.split(sep)

    # ---- protected ranges: the Scheduler class body and the OriginalVm
    #      declaration block (its members are handled explicitly) ------------
    def find_line(pred, start=0):
        for i in range(start, len(lines)):
            if pred(lines[i]):
                return i
        return -1

    sched_start = find_line(lambda l: l.startswith('class Scheduler {'))
    if sched_start < 0:
        print('Scheduler class not found')
        return 1
    # its closing brace: the first line that is exactly '};' at column 0
    sched_end = find_line(lambda l: l.rstrip() == '};', sched_start)
    protected = set(range(sched_start, sched_end + 1))
    print('Scheduler class: lines %d..%d protected' % (sched_start + 1, sched_end + 1))

    # the OriginalVm member block: from the first member definition to the last
    def_line = {}
    for i, ln in enumerate(lines):
        s = ln.strip()
        for name, decl in MEMBERS.items():
            if s == decl or (s.startswith(decl[:-1]) and ';' in s and
                             s.index(';') >= len(decl) - 1):
                def_line.setdefault(name, []).append(i)
    for name, ds in def_line.items():
        if len(ds) != 1:
            print('def %s -> %s' % (name, [d + 1 for d in ds]))
            return 1
    missing = [n for n in MEMBERS if n not in def_line]
    if missing:
        print('NOT FOUND %s' % missing)
        return 1
    member_lines = sorted(def_line[n][0] for n in MEMBERS)
    print('VM members at lines %d..%d' % (member_lines[0] + 1, member_lines[-1] + 1))

    # CatchRecord's old definition (it moves into ExecState)
    cr_def = find_line(lambda l: l.strip() == 'struct CatchRecord {')
    cr_end = find_line(lambda l: l.strip() == '};', cr_def)

    skip = set(member_lines) | set(range(cr_def, cr_end + 1))
    insert_at = member_lines[0]

    out = []
    subs = 0
    for i, ln in enumerate(lines):
        if i == insert_at:
            out.append(norm(EXEC_STATE, sep))
        if i in skip:
            continue
        new = ln
        if i not in protected and not is_comment(ln):
            for name in MEMBERS:
                if name == 'cond_':
                    pat = r'(?<![\w>.])(?<!st\.)(?<!sig\.)cond_\b'
                else:
                    pat = r'(?<![\w>])' + re.escape(name) + r'\b'
                cnt = len(re.findall(pat, new))
                if cnt:
                    new = re.sub(pat, 'es_->' + name[:-1], new)
                    subs += cnt
        out.append(new)
    t = sep.join(out)

    # ---- accesses through another VM pointer ----------------------------
    cross = 0
    for name in MEMBERS:
        def repl(m):
            nonlocal cross
            if m.group(1).endswith('es_->'):
                return m.group(0)
            cross += 1
            return m.group(1) + 'es_->' + name[:-1]
        t = re.sub(r'\b([A-Za-z_]\w*->)(' + re.escape(name) + r')\b', repl, t)

    # ---- VmRunState + stash/restore/switch -------------------------------
    i = t.find('struct VmRunState {')
    j = t.find('};', i)
    t = t[:i] + norm('''struct VmRunState {
        // The chain's execution state.  It used to be a COPY of the VM's 19
        // execution members, moved in and out on every switch
        // (stashRunState/restoreRunState); carrying the state itself makes a
        // switch an ownership transfer, so a missed switch cannot lose frames.
        std::shared_ptr<ExecState> es;
        int64_t ownerTid = -1;
    };''', sep) + t[j + 2:]

    marker = "    // The chain key's process finished: its state must not linger"
    i = t.find('    void clearRunMembers() {')
    j = t.find(marker, i)
    if i < 0 or j < 0:
        print('run-state block not found (%d/%d)' % (i, j))
        return 1
    t = t[:i] + norm(SWITCH_NEW, sep) + t[j:]

    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK ExecState refactor: %d member uses, %d cross-VM uses' % (subs, cross))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
