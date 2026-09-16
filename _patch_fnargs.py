# -*- coding: utf-8 -*-
"""Add LPCVM_FNARGS=<substr,...>: make the [fn] ENTER probe also print the
argument VALUES (up to 4) for the named functions.

Needed because the task-template question is "was register_task_template called
with which class and what does the template mapping contain" - the current
ENTER line only shows argc.

LPCVM_FNARGS is NOT filtered by LPCVM_FNOBJ: the whole point is to see which
OBJECT registers what.
"""
import io

p = 'src/original_vm.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in d else '\n'


def sub(old, new):
    global d
    o, n = old.replace('\n', nl), new.replace('\n', nl)
    assert o in d, 'anchor not found: %r' % old[:70]
    d = d.replace(o, n, 1)


sub('''static bool fnObjMatch(const std::string& objName) {''',
    '''// LPCVM_FNARGS=<substr>[,<substr>...]: also dump the first few ARGUMENT
// VALUES on the [fn] ENTER line for those functions (LPCVM_FNTRACE only prints
// argc).  Deliberately independent of LPCVM_FNOBJ - the question it answers
// ("which object registered which task class / template fields") needs the
// caller's identity too.
static bool fnArgsMatch(const std::string& name) {
    static const char* const flt = std::getenv("LPCVM_FNARGS");
    if (!flt || !*flt || name.empty()) return false;
    const char* p = flt;
    while (*p) {
        const char* comma = std::strchr(p, ',');
        size_t n = comma ? static_cast<size_t>(comma - p) : std::strlen(p);
        if (n && name.find(p, 0, n) != std::string::npos) return true;
        if (!comma) break;
        p = comma + 1;
    }
    return false;
}

static bool fnObjMatch(const std::string& objName) {''')

# patch BOTH [fn] ENTER sites identically
old_enter = '''        if (fnTraceMatch(fn.name) && fnObjMatch(objName_))
            std::cerr << "[fn] ENTER " << fn.name << " obj=" << objName_
                      << " args=" << ((passedArgs >= 0) ? passedArgs : fn.argCount) << "\\n";'''
new_enter = '''        if (fnTraceMatch(fn.name) && fnObjMatch(objName_)) {
            std::cerr << "[fn] ENTER " << fn.name << " obj=" << objName_
                      << " args=" << ((passedArgs >= 0) ? passedArgs : fn.argCount);
            if (fnArgsMatch(fn.name)) {
                int acA = (passedArgs >= 0) ? passedArgs : fn.argCount;
                for (int ai = 0; ai < acA && ai < 4; ++ai) {
                    int si = bp - ai;   // param0 is on TOP of the stack
                    if (si < 0 || si >= static_cast<int>(stack_.size())) break;
                    const Value& av = stack_[static_cast<size_t>(si)].value;
                    std::string s = to_string(av, false);
                    if (s.size() > 140) s = s.substr(0, 140) + "...";
                    std::cerr << " a" << ai << "=t" << av.index() << ":" << s;
                }
            }
            std::cerr << "\\n";
            std::cerr.flush();
        }'''

o = old_enter.replace('\n', nl)
n = new_enter.replace('\n', nl)
cnt = d.count(o)
assert cnt == 2, 'expected 2 [fn] ENTER sites, found %d' % cnt
d = d.replace(o, n)

io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('FNARGS probe added to %d sites' % cnt)
