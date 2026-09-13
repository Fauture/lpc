# -*- coding: utf-8 -*-
"""destructObjectByKey: run the LPC destructors BEFORE unregistering the object.

The destructor chain calls back INTO the object:
  user.o::destruct -> userd::destroy_user -> ... -> me->save(0, cb)   (userd:2711)
and call_other resolves the target object through reg.vms.  Erasing the
registry entry first (what this function did) made every such call a silent
no-op: the character's dbase was never written, so level / items / the
"newbie" flag were lost on every re-login.  A nested destruct_object(sameKey)
from inside the chain now returns immediately - the outer call owns teardown.
"""
import io

P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


OLD = B("""    std::shared_ptr<OriginalVm> vm;
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        auto it = reg.vms.find(key);
        if (it != reg.vms.end()) vm = it->second;
        reg.vms.erase(key);          // IndexedVmMap::erase(const std::string&)
        reg.objects.erase(key);      // IndexedObjMap::erase(const std::string&)
        reg.initialized.erase(key);
    }
    regIndexUnregister(key);
    // Run the object's LPC destructor (user.o declares `private void
    // destruct()`) BEFORE unregistering: that is what removes it from
    // logind/userd's account maps (set_name_user / all_users).  Without it the
    // next login still found the dead object via find_user_by_account,
    // concluded "previous login did not finish, residual USER_OB" and returned
    // WITHOUT sending the character list - the re-login then sat there with only
    // the echo reply (relog7: [life] destruct_object(user.o#16150) -> 0 by=
    // logind.o::do_login, then do_login RETURN with no 0xF061).
    if (vm && !vm->beingDestroyed()) {
        static const bool lifeOn = (std::getenv("LPCVM_LIFE") != nullptr);
        // EVERY inherited destructor must run (see callDestructors).
        vm->callDestructors(lifeOn);
    }
    {
        auto& park = destroyedVmPark();
        if (vm) park.push_back(vm);
        if (park.size() > 512) park.erase(park.begin(), park.begin() + 256);
    }
    return vm != nullptr;
""")

NEW = B("""    std::shared_ptr<OriginalVm> vm;
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        auto it = reg.vms.find(key);
        if (it != reg.vms.end()) vm = it->second;
    }
    if (!vm) return false;           // unknown / already destroyed
    if (vm->beingDestroyed()) {
        // Re-entrant destruct_object(<same key>) from inside the destructor
        // chain (a destructor destroying its own object): the OUTER call owns
        // the registry teardown and the parking, so leave the entry in place -
        // the chain keeps calling back into the object (me->save()).
        return true;
    }
    // Run the object's LPC destructors WHILE IT IS STILL REGISTERED - and that
    // ordering is load-bearing in BOTH directions:
    //  * the destructor chain resolves the object through reg.vms/objects, e.g.
    //    user.o::destruct -> userd::destroy_user -> me->save(0, cb)
    //    (userd:2711, the ONLY path that writes the character's dbase back to
    //    dl_ddb_1_test.`user`<gid>).  With the entry erased first that call was
    //    a silent no-op and the save never happened (level/items/"newbie" flag
    //    lost on every re-login);
    //  * user.o's destruct removes the char from logind/userd's account maps
    //    (set_name_user / all_users); without it the next login found the dead
    //    object via find_user_by_account, concluded "residual USER_OB" and
    //    returned WITHOUT the character list (relog7).
    {
        static const bool lifeOn = (std::getenv("LPCVM_LIFE") != nullptr);
        // EVERY inherited destructor must run (see callDestructors).
        vm->callDestructors(lifeOn);
    }
    {
        std::lock_guard<std::recursive_mutex> rl(registryMutex());
        reg.vms.erase(key);          // IndexedVmMap::erase(const std::string&)
        reg.objects.erase(key);      // IndexedObjMap::erase(const std::string&)
        reg.initialized.erase(key);
    }
    regIndexUnregister(key);
    {
        auto& park = destroyedVmPark();
        park.push_back(vm);
        if (park.size() > 512) park.erase(park.begin(), park.begin() + 256);
    }
    return true;
""")
assert d.count(OLD) == 1, 'anchor not found (%d)' % d.count(OLD)
d = d.replace(OLD, NEW)
io.open(P, 'wb').write(d)
print('patched; CRLF', d.count(b'\r\n'), 'bareLF', d.count(b'\n') - d.count(b'\r\n'))
