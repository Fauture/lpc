# -*- coding: utf-8 -*-
"""Extend the [keyt] probe so a restore/save of the character dbase prints the
TOP-LEVEL key set (name + type), not just the task/carry sub-maps.

Why: `[keyt] restore/task` never printed while `restore/carry` did, so the row
that carries "task" is restored through a different call.  Printing every
top-level key of any restored/saved mapping that has a "name" or "account" key
identifies WHICH restore carries the task map and whether its key is a string.
"""
import io

p = 'src/builtins.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in d else '\n'


def sub(old, new):
    global d
    o, n = old.replace('\n', nl), new.replace('\n', nl)
    assert o in d, 'anchor not found: %r' % old[:60]
    d = d.replace(o, n, 1)


# add a dbase-level dump helper right after dumpKeyTypes
sub('''Value simpleDbSelect(const std::string& table) {''',
    '''// [keyt] dbase-level dump: what top-level fields does this mapping have and
// what TYPE are their keys?  "\u4efb\u52a1" lives under the key "task"; if that
// key were an int the whole LPC data layer (express_query) could never see it.
void dumpDbaseKeys(const Value& v, const char* tag) {
    static int budget = 60;
    if (budget <= 0 || v.index() != 6) return;
    --budget;
    auto mp = std::get<std::shared_ptr<MappingValue>>(v);
    std::cerr << "[keyt] " << tag << " topEntries=" << mp->items().size() << "\\n";
    int shown = 0;
    for (auto& kv : mp->items()) {
        if (kv.first.index() != 4) {
            std::cerr << "[keyt]   <nonstring key type=" << kv.first.index() << ">\\n";
            continue;
        }
        const std::string& k = std::get<SharedString>(kv.first).str();
        if (k == "task" || k == "carry" || k == "achieve" || k == "name" ||
            k == "account" || k == "patch" || k == "guide" || k == "newbie" ||
            k == "trace_task")
            std::cerr << "[keyt]   key=" << k << " valType=" << kv.second.index() << "\\n";
        if (++shown >= 60) break;
    }
    std::cerr.flush();
}

Value simpleDbSelect(const std::string& table) {''')

# restore_value hook: also dump the dbase-level key set
sub('''            if (ok && out.index() == 6 && std::getenv("LPCVM_KEYT")) {
                auto mp = std::get<std::shared_ptr<MappingValue>>(out);
                for (const char* k : {"task", "carry", "achieve"}) {''',
    '''            if (ok && out.index() == 6 && std::getenv("LPCVM_KEYT")) {
                auto mp = std::get<std::shared_ptr<MappingValue>>(out);
                dumpDbaseKeys(out, "restore");
                for (const char* k : {"task", "carry", "achieve"}) {''')

# save_string hook: same
sub('''    if (name == "save_string" && !args.empty() && args[0].index() == 6 &&
        std::getenv("LPCVM_KEYT")) {
        auto mp = std::get<std::shared_ptr<MappingValue>>(args[0]);''',
    '''    if (name == "save_string" && !args.empty() && args[0].index() == 6 &&
        std::getenv("LPCVM_KEYT")) {
        auto mp = std::get<std::shared_ptr<MappingValue>>(args[0]);
        dumpDbaseKeys(args[0], "save");''')

io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('keyt dbase dump added')
