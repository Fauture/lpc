# -*- coding: utf-8 -*-
"""callDestructors: run the MOST-DERIVED destructors first.

user.o inherits /gs/feature/char/carry.c, whose destruct() does
    lite_set_temp("destructing", 1);
and user.o::save() starts with
    if (lite_query_temp("destructing") || lite_query_temp("destructing_store")) return 1;
user.o::destruct -> userd::destroy_user -> me->save(0, cb) (userd:2711) is the
ONLY path that writes the character's dbase back to dl_ddb_1_test.`user`<gid>.
Running the inherited destructors first set "destructing" before that save, so
every character save silently returned 1 without writing anything: level, items
and the "newbie" flag were lost on re-login, the account stayed in
stun_account[] (next login rejected with 9026/9029) and the 新手礼包 looked
empty (apply rebuilt a new gift instead of listing its items).

The object's OWN program functions carry funOffset 0; inherited programs get
increasing baseFunOffset in inheritance order - so sorting by funOffset puts the
object's own destruct first, then its parents, matching the engine's chain
order.
"""
import io

P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


OLD = B("""        std::vector<const OriginalFunction*> fns;
        for (const auto& kv : funcsByIndex_) {
            const OriginalFunction* f = kv.second;
            if (!f) continue;
            if (f->name != "destruct" && f->name != "delete") continue;
            bool dup = false;
            for (auto* x : fns) if (x == f) { dup = true; break; }
            if (!dup) fns.push_back(f);
        }
""")
NEW = B("""        std::vector<const OriginalFunction*> fns;
        for (const auto& kv : funcsByIndex_) {
            const OriginalFunction* f = kv.second;
            if (!f) continue;
            if (f->name != "destruct" && f->name != "delete") continue;
            bool dup = false;
            for (auto* x : fns) if (x == f) { dup = true; break; }
            if (!dup) fns.push_back(f);
        }
        // MOST-DERIVED FIRST.  user.o inherits /gs/feature/char/carry.c, whose
        // destruct() sets lite_set_temp("destructing", 1) - and user.o::save()
        // bails out with `if (destructing || destructing_store) return 1;`.
        // user.o::destruct -> userd::destroy_user -> me->save(0, cb)
        // (userd:2711) is the ONLY writer of the character's dbase, so with an
        // inherited destructor running first the flag was already set and the
        // save silently did nothing (level/items/"newbie" flag lost, the
        // account stayed in stun_account[] and the next login was rejected with
        // 9026/9029).  The object's OWN program has funOffset 0; inherited
        // programs get increasing baseFunOffset in inheritance order.
        std::stable_sort(fns.begin(), fns.end(),
                         [](const OriginalFunction* a, const OriginalFunction* b) {
                             if (a->funOffset != b->funOffset) return a->funOffset < b->funOffset;
                             return a->index < b->index;
                         });
""")
assert d.count(OLD) == 1, 'anchor not found (%d)' % d.count(OLD)
d = d.replace(OLD, NEW)
io.open(P, 'wb').write(d)
print('patched; CRLF', d.count(b'\r\n'), 'bareLF', d.count(b'\n') - d.count(b'\r\n'))
