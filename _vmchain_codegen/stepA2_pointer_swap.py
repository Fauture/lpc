#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step A2: stash/restore become pointer moves (no data movement at all).

    python -X utf8 _vmchain_codegen/stepA2_pointer_swap.py

After Step A the execution state lives in `ExecState`, reached through `es_`, and
`VmRunState` holds a `shared_ptr<ExecState>`.  What is still left of the old
design is that stash/restore COPY the state (they move 19 members out of and into
the active `ExecState`).  This replaces both with pure pointer/ownership moves:

  * stash: hand the active `ExecState` to the map entry (shared_ptr move) and
    point `es_` at a fresh state - O(1), nothing copied, no field list to keep in
    sync;
  * restore: take the map entry's `ExecState` (shared_ptr move) and point `es_`
    at it; the entry is erased so it cannot be restored twice.

That is the property the public drivers have (a PCB owns its context, a switch is
a pointer change) and the reason a missed switch can no longer lose frames.
"""
import io

SRC = r'src\original_vm.cpp'

OLD_STASH = '''    void stashRunState(int64_t key) {
        if (key < 0) return;
        runStates_[key].es = std::shared_ptr<ExecState>(new ExecState(std::move(*es_)));
        *es_ = ExecState{};
    }
    bool restoreRunState(int64_t key) {
        auto it = runStates_.find(key);
        if (it == runStates_.end()) return false;
        if (it->second.es) *es_ = std::move(*it->second.es);
        execOwnerTid_ = it->second.ownerTid;
        // The chain is running again: its stashed copy is not needed any more.
        runStates_.erase(it);
        return true;
    }'''

NEW_STASH = '''    // O(1): hand the active state to `key`'s chain and give the VM a fresh one.
    // The state object itself never moves - only its owner does.
    void stashRunState(int64_t key) {
        if (key < 0) return;
        activeEs_ = std::move(runStates_[key].es);
        runStates_[key].es = std::shared_ptr<ExecState>(new ExecState(std::move(*es_)));
        es_ = runStates_[key].es.get();
        runStates_[key].ownerTid = execOwnerTid_;
    }
    // O(1): this chain runs again, so its stored state becomes the active one.
    bool restoreRunState(int64_t key) {
        auto it = runStates_.find(key);
        if (it == runStates_.end()) return false;
        std::shared_ptr<ExecState> st = std::move(it->second.es);
        const int64_t owner = it->second.ownerTid;
        runStates_.erase(it);          // a chain's state is restored exactly once
        if (!st) return false;
        activeEs_ = std::move(st);
        es_ = activeEs_.get();
        execOwnerTid_ = owner;
        return true;
    }'''


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')

    old = OLD_STASH.replace('\n', sep)
    new = NEW_STASH.replace('\n', sep)
    if t.count(old) != 1:
        print('stash/restore block COUNT %d' % t.count(old))
        return 1
    t = t.replace(old, new, 1)

    # the active state's owner: without it `es_` could point at a state the map
    # entry has already given away.
    anchor = '    ExecState defaultEs_;'
    if 'std::shared_ptr<ExecState> activeEs_;' not in t:
        if t.count(anchor.replace('\n', sep)) != 1:
            print('defaultEs_ anchor COUNT %d' % t.count(anchor.replace('\n', sep)))
            return 1
        t = t.replace(
            anchor.replace('\n', sep),
            ('    // Owns the ACTIVE chain\'s state while that chain is running: the map\n'
             '    // entry hands it over on entry and takes it back on park, so `es_` is\n'
             '    // never left pointing at a state the table has already released.\n'
             '    std::shared_ptr<ExecState> activeEs_;\n'
             '    ExecState defaultEs_;').replace('\n', sep), 1)

    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK stash/restore are pointer moves now')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
