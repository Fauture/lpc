#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step A3 (simple form): the chain table owns each chain's state; `es_` is a view.

    python -X utf8 _vmchain_codegen/stepA3b_simple.py

Replaces the last copies in the chain machinery.  After Step A a park/resume
still did:

    stash   : runStates_[k].es = new ExecState(std::move(*es_));  *es_ = ExecState{}
    restore : *es_ = std::move(*runStates_[k].es)

i.e. the whole execution state was copied twice per park (and a third time by
clearRunMembers).  Now:

  * `runStates_[key].es` is allocated ONCE per chain and never moves;
  * `switchToChain(key)` points `es_` at that object (a pointer change, no data);
  * `clearRunMembers()` resets the active chain's state IN PLACE;
  * `finishChain(key)` resets in place (if active) and drops the entry;
  * `key < 0` means "no chain": reset the default state and point `es_` at it.

The state object's address is therefore stable for the chain's lifetime, which is
what makes the old stash/restore pair unnecessary rather than merely cheaper.
"""
import io

SRC = r'src\original_vm.cpp'

OLD_BLOCK_START = '    void clearRunMembers() {'
OLD_BLOCK_END = '    int64_t activeChainKey() const { return activeKey_; }'

NEW = '''    // Reset the active chain's execution state IN PLACE.  The state object is
    // owned by the chain table (or is `defaultEs_`) and must not be replaced,
    // because `es_` points into it.
    void clearRunMembers() {
        if (es_ == &defaultEs_) defaultEs_ = ExecState{};
        else *es_ = ExecState{};
        execOwnerTid_ = -1;
    }
    // Diagnostic (LPCVM_TAIL): who moves this object's chain around.
    void chainDbg(const char* what, int64_t key) {
        if (!std::getenv("LPCVM_TAIL")) return;
        if (objName_.find("login.o") == std::string::npos) return;
        std::cerr << "[chn] " << what << " key=" << key << " active=" << activeKey_
                  << " frames=" << es_->frames.size() << " parked=" << (es_->parked ? 1 : 0)
                  << " cond=" << es_->cond.kind << " stash=" << runStates_.size() << "\\n";
        std::cerr.flush();
    }
    // Make `key`'s chain current.  `key < 0` means "no chain": the default state
    // is reset and used.  Otherwise the chain's own state object is allocated on
    // first use and simply pointed at from then on - nothing is copied or moved,
    // so a park/resume cycle cannot lose or duplicate a frame stack.
    void switchToChain(int64_t key) {
        if (key == activeKey_) return;
        if (key < 0) {
            chainDbg("switch-default", key);
            clearRunMembers();
            activeKey_ = -1;
            es_ = &defaultEs_;
            return;
        }
        chainDbg("switch-in", key);
        auto it = runStates_.find(key);
        if (it == runStates_.end()) {
            runStates_[key].es = std::shared_ptr<ExecState>(new ExecState());
            runStates_[key].ownerTid = (lpc::scheduler().chainTid() >= 0)
                                           ? lpc::scheduler().chainTid()
                                           : lpc::scheduler().currentProcessTid();
            it = runStates_.find(key);
            activeKey_ = key;
            es_ = it->second.es.get();
            execOwnerTid_ = it->second.ownerTid;
            chainDbg("switch-fresh", key);
            return;
        }
        activeKey_ = key;
        es_ = it->second.es.get();
        execOwnerTid_ = it->second.ownerTid;
        chainDbg("switch-resume", key);
    }
'''


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')

    i = t.find(OLD_BLOCK_START.replace('\n', sep))
    j = t.find(OLD_BLOCK_END.replace('\n', sep), i)
    if i < 0 or j < 0:
        print('block anchors not found (%d/%d)' % (i, j))
        return 1
    j += len(OLD_BLOCK_END.replace('\n', sep))
    t = t[:i] + NEW.replace('\n', sep) + t[j:]

    # finishChain: reset in place, then drop the entry
    i = t.find('    void finishChain(int64_t key) {')
    if i < 0:
        print('finishChain not found')
        return 1
    j = t.find('    }', t.find('finish-cleared', i))
    j += len('    }')
    t = t[:i] + '''    // The chain's process finished: its state must not linger (a later chain
    // with the same recycled key would otherwise resume into it).
    void finishChain(int64_t key) {
        if (key < 0) return;
        chainDbg("finish", key);
        if (activeKey_ == key) {
            if (es_ != &defaultEs_) *es_ = ExecState{};
            es_ = &defaultEs_;
            activeKey_ = -1;
            execOwnerTid_ = -1;
        }
        runStates_.erase(key);
        chainDbg("finish-cleared", key);
    }'''.replace('\n', sep) + t[j:]

    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK A3b applied')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
