#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Step C measurement: is the VM ever entered from a non-scheduler thread?

    python -X utf8 _vmchain_codegen/stepC_measure.py

`execMutex_` / `execDepth_` / `runDetached` exist ONLY to serialise a VM that can
be entered by several threads (their own comment says so).  But every LPC entry
should now go through the scheduler:

  * `create_thread` -> `scheduler().spawn()` (3 sites)
  * timers / constructions / service replies -> `spawn*`
  * network packets -> `execOnMain`

If the six guard sites never run off the scheduler thread, the lock and the
detach machinery can be deleted (Step C) instead of guessed at.  This adds a
bounded diagnostic to the guard sites that records the entering thread, so the
next real-client run answers the question with evidence.
"""
import io
import re

SRC = r'src\original_vm.cpp'

PROBE = '''
// ---- Step C diagnostic: who enters the VM? -------------------------------
// `execMutex_`/`execDepth_`/`runDetached` only exist because a VM can be entered
// from several threads.  Every entry should go through the scheduler now, so
// this records any entry that does NOT - the evidence needed to delete the lock
// machinery (see docs/scheduler_refactor_plan.md, Step C).
inline void noteVmEntry(const char* site, const char* obj) {
    static const std::atomic<bool>& on = dynProbeFlag("LPCVM_ENTRYDBG");
    if (!on.load(std::memory_order_relaxed)) return;
    const bool sched = lpc::scheduler().isSchedulerThread();
    static std::atomic<int> shown{0};
    if (sched) {
        if (shown.fetch_add(1) >= 40) return;
    }
    std::cerr << "[entry] site=" << site << " obj=" << obj
              << " schedulerThread=" << (sched ? 1 : 0)
              << " tid=" << std::this_thread::get_id() << "\\n";
    std::cerr.flush();
}
'''


def main() -> int:
    raw = open(SRC, 'rb').read()
    crlf = raw.count(b'\r\n')
    lf = raw.count(b'\n')
    sep = '\r\n' if crlf * 2 >= lf else '\n'
    t = raw.decode('utf-8')
    if 'noteVmEntry' in t:
        print('probe already installed')
        return 0

    # insert the helper right before the class so every guard site can call it
    anchor = 'class OriginalVm {'
    a = anchor.replace('\n', sep)
    if t.count(a) != 1:
        print('class anchor COUNT %d' % t.count(a))
        return 1
    t = t.replace(a, PROBE.replace('\n', sep) + sep + a, 1)

    # call it at the guard sites
    subs = 0
    def add_call(text, pattern, site):
        nonlocal subs
        def repl(m):
            nonlocal subs
            subs += 1
            return m.group(0) + sep + m.group(1) + 'noteVmEntry("%s", objName_.c_str());' % site
        new, n = re.subn(pattern, repl, text)
        return new

    t = add_call(t, r'([ \t]*)ExecGuard lock\(execMutex_, execDepth_, "fromOutside[^\n]*\);',
                 "fromOutside")
    t = add_call(t, r'([ \t]*)ExecGuard lock\(execMutex_, execDepth_, "onStack[^\n]*\);',
                 "onStack")
    t = add_call(t, r'([ \t]*)ExecGuard lock\(execMutex_, execDepth_, "resume "[^\n]*\);',
                 "resume")
    t = add_call(t, r'([ \t]*)ExecGuard lock\(execMutex_, execDepth_, "resumeRunLoop[^\n]*\);',
                 "resumeRunLoop")
    t = add_call(t, r'([ \t]*)ExecGuard lock\(execMutex_, execDepth_, "resumeCaller[^\n]*\);',
                 "resumeCaller")

    open(SRC, 'wb').write(t.encode('utf-8'))
    print('OK probe installed, %d guard sites instrumented' % subs)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
