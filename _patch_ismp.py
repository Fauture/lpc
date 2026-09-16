# -*- coding: utf-8 -*-
"""Probe is_mapping() when called from taskd::query_task_log / query_task_tip.

Round 2 finding: [isfn] never fired, which is consistent with query_task_log
returning "" EARLIER than the is_function call - i.e. at

    funcs = all_templates[task_class];
    if (!is_mapping(funcs)) return "";

So this logs the argument of is_mapping (and is_string) for callers whose name
contains "query_task", which distinguishes "all_templates has no entry for this
class" from "the entry exists but its log field is not a closure".
"""
import io

p = 'src/builtins.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in d else '\n'

old = '''    if (name == "is_mapping") return make_int(!args.empty() && args[0].index() == 6 ? 1 : 0);'''
new = '''    if (name == "is_mapping") {
        // [ismp] runtime-armed (type nul > _probe_on\\LPCVM_ISFN): what did
        // query_task_log/query_task_tip actually get back from all_templates?
        static const std::atomic<bool>& ismpFlag = lpc::dynProbeFlag("LPCVM_ISFN");
        if (ismpFlag.load(std::memory_order_relaxed)) {
            static int ismpBudget = 600;
            const std::string& cf = stubCallCtx().fn;
            if (ismpBudget > 0 && cf.find("query_task") != std::string::npos) {
                --ismpBudget;
                std::string dump = args.empty() ? std::string("<noargs>") : to_string(args[0], false);
                if (dump.size() > 130) dump = dump.substr(0, 130) + "...";
                std::cerr << "[ismp] caller=" << stubCallCtx().obj << "::" << cf
                          << " argType=" << (args.empty() ? -1 : args[0].index())
                          << " val=" << dump
                          << " -> " << (!args.empty() && args[0].index() == 6 ? 1 : 0) << "\\n";
                std::cerr.flush();
            }
        }
        return make_int(!args.empty() && args[0].index() == 6 ? 1 : 0);
    }'''
assert old.replace('\n', nl) in d, 'is_mapping anchor not found'
d = d.replace(old.replace('\n', nl), new.replace('\n', nl), 1)
io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('ismp probe added')
