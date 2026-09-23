#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C2 (precise): telnet daemons become scheduler processes.

    python -X utf8 _vmchain_codegen/stepC2_telnet2.py

Each insertion is anchored on a UNIQUE multi-line context (the first attempt used
`int64_t clientNo = -1;`, which also matches a local in `create_thread`, so three
of the four parts were reported "already present" and one landed in the wrong
scope).

Parts:
  1. `Process::telnetCno`            - the client id travels with the process
  2. `Scheduler::spawnTelnet` decl   - next to `spawn()`
  3. `Scheduler::spawnTelnet` def    - next to the other spawn definitions
  4. `runProcess` publishes the id   - thread-local slot for the run, restored on exit
"""
import io

SRC = r'src\original_vm.cpp'

PARTS = [
    # 1. Process field (8-space indent: `Process` lives in this file's anonymous
    #    namespace, not nested in Scheduler, so the first attempt's 4-space anchor
    #    matched nothing)
    ('''        int64_t clientNo = -1;
        // True while this process sits in ready_.''',
     '''        int64_t clientNo = -1;
        // Telnet client this process serves (-1 = none).  The telnet efuns route
        // through a thread-local client id; with every LPC on the scheduler thread
        // that id must travel with the PROCESS, and runProcess publishes it around
        // this process's run.
        int telnetCno = -1;
        // True while this process sits in ready_.'''),
    # 2. declaration
    ('''    // Spawn a new process (create_thread / set_timer / post_init / network
    // callback).  Called from any OS thread; the process runs on the
    // scheduler thread via run().
    void spawn(std::shared_ptr<OriginalVm> vm, const std::string& entry,
               std::vector<Value> args);''',
     '''    // Spawn a new process (create_thread / set_timer / post_init / network
    // callback).  Called from any OS thread; the process runs on the
    // scheduler thread via run().
    void spawn(std::shared_ptr<OriginalVm> vm, const std::string& entry,
               std::vector<Value> args);
    // Same, but the process serves a telnet client: the id is pinned for the run
    // (see Process::telnetCno) so the LPC telnet efuns keep routing to its socket.
    void spawnTelnet(std::shared_ptr<OriginalVm> vm, const std::string& entry,
                     std::vector<Value> args, int telnetCno);'''),
    # 3. definition
    ('''void Scheduler::spawnConstruction(std::shared_ptr<OriginalVm> vm, std::vector<Value> args,
                                  std::string doneEvent) {
    auto p = std::make_shared<Process>();''',
     '''// A telnet daemon used to be its own OS thread; it is an ordinary scheduler
// process now (Step C2), with its client id pinned for the run.
void Scheduler::spawnTelnet(std::shared_ptr<OriginalVm> vm, const std::string& entry,
                           std::vector<Value> args, int telnetCno) {
    auto p = std::make_shared<Process>();
    p->vm = std::move(vm);
    p->homeVm = p->vm;
    p->entry = entry;
    p->args = std::move(args);
    p->telnetCno = telnetCno;
    p->clientNo = -1;
    p->tid = allocTid();
    {
        std::lock_guard<std::mutex> lk(mtx_);
        ready_.push_back(p);
    }
    cv_.notify_one();
}

void Scheduler::spawnConstruction(std::shared_ptr<OriginalVm> vm, std::vector<Value> args,
                                  std::string doneEvent) {
    auto p = std::make_shared<Process>();'''),
    # 4. publish for the run
    ('''    // Pin the chain-owner tid for this run: get_thread_id() must return the same
    // tid for the process that owns the executing frame chain (see chainTid()).
    if (p->tid >= 0) setChainTid(p->tid);''',
     '''    // Pin the chain-owner tid for this run: get_thread_id() must return the same
    // tid for the process that owns the executing frame chain (see chainTid()).
    if (p->tid >= 0) setChainTid(p->tid);
    // Publish this process's telnet client id for its whole run and restore the
    // previous value on every exit path: the telnet efuns read a thread-local
    // slot, and with all LPC on one thread that slot must follow the PROCESS.
    struct TelnetCnoScope {
        int prev;
        bool armed;
        TelnetCnoScope() : prev(lpc::threadTelnetClient()), armed(false) {}
        ~TelnetCnoScope() {
            if (armed) lpc::setThreadTelnetClient(prev);
        }
    } telnetCnoScope;
    if (p->telnetCno >= 0) {
        lpc::setThreadTelnetClient(p->telnetCno);
        telnetCnoScope.armed = true;
    }'''),
]


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    done = []
    for idx, (old, new) in enumerate(PARTS, 1):
        o, n = old.replace('\n', sep), new.replace('\n', sep)
        c = t.count(o)
        if c != 1:
            print('part %d: anchor COUNT %d (need 1)' % (idx, c))
            return 1
        t = t.replace(o, n, 1)
        done.append(str(idx))
    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK parts applied: %s' % ','.join(done))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
