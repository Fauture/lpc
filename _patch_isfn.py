# -*- coding: utf-8 -*-
"""Probe is_function() when called from taskd::query_task_log.

query_task_log does:
    task_log = funcs["log"];
    if (is_function(task_log)) task_log = (*task_log)(me, task_info);
    else if (!is_string(task_log)) task_log = task_info["log"];
    if (!is_string(task_log)) return "";
and our trace shows task_log() (the template closure) is NEVER entered while
query_task_log returns "" - so is_function(funcs["log"]) must be false, i.e. the
closure read back out of the all_templates mapping is not a closure Value any
more (or the key lookup misses).  This prints the argument's type index and a
dump, tagged with the calling LPC function.
"""
import io

p = 'src/builtins.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in d else '\n'

old = '''    if (name == "is_function") return make_int(!args.empty() && args[0].index() == 8 ? 1 : 0);'''
new = '''    if (name == "is_function") {
        // [isfn] what does the caller actually hold?  A template closure that
        // comes back out of the all_templates mapping as type != 8 explains
        // "task_log() is never entered, query_task_log returns ''".
        if (std::getenv("LPCVM_KEYT") && !args.empty()) {
            static const char* f = std::getenv("LPCVM_ISFN_FN");
            const std::string& cf = stubCallCtx().fn;
            if (!f || cf.find(f) != std::string::npos) {
                static int budget = 200;
                if (budget > 0) {
                    --budget;
                    std::string dump = to_string(args[0], false);
                    if (dump.size() > 120) dump = dump.substr(0, 120) + "...";
                    std::cerr << "[isfn] caller=" << stubCallCtx().obj << "::" << cf
                              << " argType=" << args[0].index()
                              << " argc=" << args.size()
                              << " val=" << dump << "\\n";
                    std::cerr.flush();
                }
            }
        }
        return make_int(!args.empty() && args[0].index() == 8 ? 1 : 0);
    }'''
assert old.replace('\n', nl) in d, 'is_function anchor not found'
d = d.replace(old.replace('\n', nl), new.replace('\n', nl), 1)
io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('isfn probe added')
