#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C1: the per-VM execution lock becomes a no-op on the scheduler thread.

    python -X utf8 _vmchain_codegen/stepC1_lock.py

MEASURED FIRST (see docs/scheduler_refactor_plan.md section 11): with the
`noteVmEntry()` probe on all five entry points, 40/40 VM entries during boot and
every entry under real client load reported `schedulerThread=1` - the lock is
taken on the wire path for nothing.

WHY NOT DELETE IT OUTRIGHT: `runDetached` releases the lock around a cross-object
call to avoid an A-B/B-A deadlock between two threads, and that release/relock pair
is written against `execDepth_`.  Deleting the mutex without rewiring that pair
would unlock an unlocked mutex (undefined behaviour) the first time some future
thread does enter a VM.

So the guard now takes the lock ONLY when it is not on the scheduler thread (where
all LPC runs), and records how many times it actually holds it in a thread-local.
`runDetached` releases/retakes exactly that recorded number.  Behaviour on the
scheduler thread is unchanged (no lock), behaviour off it is unchanged (locked).

Next step (C2) then deletes the mutex and the release/retake pair entirely.
"""
import io

SRC = r'src\original_vm.cpp'

GUARD_OLD = '''    struct ExecGuard {
        std::recursive_mutex& m;
        int& depth;
        std::string tag;
        ExecGuard(std::recursive_mutex& m_, int& d_, std::string tag_ = "")
            : m(m_), depth(d_), tag(std::move(tag_)) {
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[lock+] try " << tag << " t=" << std::this_thread::get_id() << "\\n";
            m.lock();
            ++depth;
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[lock+] got " << tag << " depth=" << depth
                          << " t=" << std::this_thread::get_id() << "\\n";
        }
        ~ExecGuard() {
            --depth;
            m.unlock();
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[lock-] rel " << tag << " depth=" << depth
                          << " t=" << std::this_thread::get_id() << "\\n";
        }
        ExecGuard(const ExecGuard&) = delete;
        ExecGuard& operator=(const ExecGuard&) = delete;
    };'''

GUARD_NEW = '''    // How many times THIS thread actually holds a VM execution lock.  All LPC
    // runs on the scheduler thread (measured: 0 non-scheduler entries), so on
    // that thread the count stays 0 and runDetached() has nothing to release.
    static int& tlsExecLocksHeld() {
        static thread_local int n = 0;
        return n;
    }
    struct ExecGuard {
        std::recursive_mutex& m;
        int& depth;
        std::string tag;
        bool locked = false;
        ExecGuard(std::recursive_mutex& m_, int& d_, std::string tag_ = "")
            : m(m_), depth(d_), tag(std::move(tag_)) {
            // The scheduler thread is the ONLY thread that runs LPC, so taking a
            // recursive mutex there is pure overhead (and the reason
            // runDetached needed its unlock/relock dance).  Off it - a telnet or
            // pending-thread entry - the lock is still taken, so nothing changes
            // for those paths.
            const bool needLock = !lpc::scheduler().isSchedulerThread();
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[lock+] try " << tag << " need=" << (needLock ? 1 : 0)
                          << " t=" << std::this_thread::get_id() << "\\n";
            if (needLock) {
                m.lock();
                locked = true;
                ++tlsExecLocksHeld();
            }
            ++depth;
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[lock+] got " << tag << " depth=" << depth
                          << " held=" << tlsExecLocksHeld()
                          << " t=" << std::this_thread::get_id() << "\\n";
        }
        ~ExecGuard() {
            --depth;
            if (locked) {
                --tlsExecLocksHeld();
                m.unlock();
            }
            if (std::getenv("LPCVM_LOCK_TRACE"))
                std::cerr << "[lock-] rel " << tag << " depth=" << depth
                          << " held=" << tlsExecLocksHeld()
                          << " t=" << std::this_thread::get_id() << "\\n";
        }
        ExecGuard(const ExecGuard&) = delete;
        ExecGuard& operator=(const ExecGuard&) = delete;
    };'''

UNLOCK_OLD = 'for (int i = 0; i < d; ++i) execMutex_.unlock();'
UNLOCK_NEW = ('for (int i = 0; i < d; ++i) { if (tlsExecLocksHeld() > 0) { --tlsExecLocksHeld(); '
              'execMutex_.unlock(); } }')
LOCK_OLD = 'for (int i = 0; i < d; ++i) execMutex_.lock();'
LOCK_NEW = ('for (int i = 0; i < d; ++i) { if (i < d && tlsExecLocksHeld() < i + 1) { '
            'execMutex_.lock(); ++tlsExecLocksHeld(); } }')


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')

    o, n = GUARD_OLD.replace('\n', sep), GUARD_NEW.replace('\n', sep)
    if t.count(o) != 1:
        print('ExecGuard COUNT %d' % t.count(o))
        return 1
    t = t.replace(o, n, 1)

    n_unlock = t.count(UNLOCK_OLD)
    n_lock = t.count(LOCK_OLD)
    t = t.replace(UNLOCK_OLD, UNLOCK_NEW)
    t = t.replace(LOCK_OLD, LOCK_NEW)

    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK guard is scheduler-aware; rewired %d unlock / %d relock loops'
          % (n_unlock, n_lock))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
