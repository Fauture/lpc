# -*- coding: utf-8 -*-
"""Add the [keyt] probe: mapping key TYPES at restore and save time.

Question it answers: after restore_value() parses the saved dbase, are the
`task` / `carry` mapping keys INT (as vm_restoreMixedString does: a bare 729
becomes vm_pushNumber) or STRING?  express_set/express_query only ever match
STRING keys (vm_addMappingValue sets lv.type=4; vm_findStringInMappingByCStr
requires node key type 4), so an int key is unreachable by a path query - which
is exactly the "task panel empty / bag not drawn after re-login" symptom.
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


# 1) helper: dump key/value types of a mapping (capped)
sub('''Value simpleDbSelect(const std::string& table) {''',
    '''// [keyt] probe: print the key and value TYPE INDEX of every entry of a
// mapping.  Value index: 0 undefined, 2 int, 4 string, 5 array, 6 mapping,
// 7 object, 10 buffer.  A path query (express_query / db->query("a/b")) only
// ever matches STRING keys, so an int key is invisible to the whole LPC data
// layer even though the value is still there.
void dumpKeyTypes(const Value& v, const char* tag) {
    static int budget = 400;
    if (budget <= 0) return;
    if (v.index() != 6) {
        std::cerr << "[keyt] " << tag << " not-a-mapping(t" << v.index() << ")\\n";
        return;
    }
    auto mp = std::get<std::shared_ptr<MappingValue>>(v);
    std::cerr << "[keyt] " << tag << " entries=" << mp->items().size() << "\\n";
    for (auto& kv : mp->items()) {
        std::cerr << "[keyt]   key=" << to_string(kv.first, false).substr(0, 20)
                  << " keyType=" << kv.first.index()
                  << " valType=" << kv.second.index() << "\\n";
        if (--budget <= 0) { std::cerr << "[keyt]   ...budget\\n"; break; }
    }
    std::cerr.flush();
}

Value simpleDbSelect(const std::string& table) {''')

# 2) hook restore_value
sub('''            if (ok) return out;
        }
        return args.empty() ? make_int(0) : args[0];
    }''',
    '''            if (ok && out.index() == 6 && std::getenv("LPCVM_KEYT")) {
                auto mp = std::get<std::shared_ptr<MappingValue>>(out);
                for (const char* k : {"task", "carry", "achieve"}) {
                    const Value* sub = mp->find(make_string(k));
                    if (sub) {
                        std::string tg = std::string("restore/") + k;
                        dumpKeyTypes(*sub, tg.c_str());
                    }
                }
            }
            if (ok) return out;
        }
        return args.empty() ? make_int(0) : args[0];
    }''')

# 3) hook save_string
sub('''    if (name == "save_string") {''',
    '''    if (name == "save_string" && !args.empty() && args[0].index() == 6 &&
        std::getenv("LPCVM_KEYT")) {
        auto mp = std::get<std::shared_ptr<MappingValue>>(args[0]);
        for (const char* k : {"task", "carry", "achieve"}) {
            const Value* sub = mp->find(make_string(k));
            if (sub) {
                std::string tg = std::string("save/") + k;
                dumpKeyTypes(*sub, tg.c_str());
            }
        }
    }
    if (name == "save_string") {''')

io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('keyt probe added')
