# -*- coding: utf-8 -*-
"""Make the [isfn] probe runtime-armable and unconditional on the caller.

Round 1 result: the probe never fired with LPCVM_ISFN_FN=query_task, so either
stubCallCtx().fn is not the LPC function name here, or the 200-line budget was
eaten before the login.  Fix both: arm the probe through the _probe_on file
switch (dynProbeFlag) right before the login, print EVERY is_function call with
its caller, and keep a 600-line budget.
"""
import io

p = 'src/builtins.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in d else '\n'

start = d.index('    if (name == "is_function") {')
end = d.index('    if (name == "is_ref")', start) if '    if (name == "is_ref")' in d[start:] else None
# replace up to the closing of this block: find the 'return make_int(!args.empty() && args[0].index() == 8 ? 1 : 0);\n    }' tail
tail = 'return make_int(!args.empty() && args[0].index() == 8 ? 1 : 0);'
ti = d.index(tail, start)
block_end = d.index('}', ti) + 1

new = '''    if (name == "is_function") {
        // [isfn]: runtime-armable (type nul > _probe_on\\LPCVM_ISFN) dump of
        // every is_function() argument.  Round 1 showed taskd::query_task_log
        // returns "" without ever entering the template's task_log() closure,
        // i.e. `is_function(funcs["log"])` is false - this says whether the
        // value read back out of all_templates is still a closure (type 8) and
        // who is asking.
        static const std::atomic<bool>& isfnFlag = lpc::dynProbeFlag("LPCVM_ISFN");
        if (isfnFlag.load(std::memory_order_relaxed)) {
            static int isfnBudget = 600;
            if (isfnBudget > 0 && !args.empty()) {
                --isfnBudget;
                std::string dump = to_string(args[0], false);
                if (dump.size() > 110) dump = dump.substr(0, 110) + "...";
                std::cerr << "[isfn] caller=" << stubCallCtx().obj << "::" << stubCallCtx().fn
                          << " argc=" << args.size()
                          << " argType=" << args[0].index()
                          << " val=" << dump << "\\n";
                std::cerr.flush();
            }
        }
        return make_int(!args.empty() && args[0].index() == 8 ? 1 : 0);
    }'''.replace('\n', nl)

d = d[:start] + new + d[block_end:]
io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('isfn probe rewritten (runtime armable, no caller filter)')
