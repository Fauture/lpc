# -*- coding: utf-8 -*-
"""Notify LPC when a client socket closes (the original driver's
vm_apply("cmd_disconnected", cno, where)).

start_gs::_cmd_disconnected(37938) -> user.o::disconnected(425)
  -> (no logout) set_timer(180, "time_expired") -> destruct
  -> userd::destroy_user(2645) -> me->save(0, get_save_done_func(me,"user/"))
Without it the character object is never torn down: nothing is ever written to
the ddb `user/<gid>` row after creation, so level/items/the "newbie" flag are
lost on every re-login (and the newbie story + newbie gift re-trigger each
time, while the gift's own dbase stays empty so opening it takes the rebuild
path instead of listing its items).
"""
import io

P = 'src/game_service.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


OLD = B("""                closesocket(c);
                {
                    std::lock_guard<std::mutex> lk(gameMutex());
                    gameClients().erase(clientNo);
                }
""")
NEW = B("""                closesocket(c);
                // Driver-side disconnect notification.  The original driver
                // applies cmd_disconnected(cno, where) when the socket goes
                // away; start_gs::_cmd_disconnected -> user.o::disconnected ->
                // (no logout) 180s timer -> time_expired -> destruct ->
                // userd::destroy_user -> me->save(0, cb), which is the ONLY
                // path that writes the character's dbase back to the ddb.  Our
                // port used to drop the client slot silently, so a character
                // was written once at creation and never again: level, items
                // and the "newbie" flag were lost on every re-login (so the
                // newbie story re-ran and the newbie gift looked empty, making
                // apply rebuild a NEW gift instead of listing its items).
                // 2 = "driver_callback" per start_gs::stat_disconnect(34691).
                // Called while the client slot is still registered: the LPC
                // looks the user up with find_user_by_client(cno).
                try {
                    callMasterProgramFunction("cmd_disconnected",
                                              {make_int(clientNo), make_int(2)},
                                              lpc::masterSearchRoot());
                } catch (...) {
                }
                {
                    std::lock_guard<std::mutex> lk(gameMutex());
                    gameClients().erase(clientNo);
                }
""")
assert d.count(OLD) == 1, 'anchor not found (%d)' % d.count(OLD)
d = d.replace(OLD, NEW)
io.open(P, 'wb').write(d)
print('patched; CRLF', d.count(b'\r\n'), 'bareLF', d.count(b'\n') - d.count(b'\r\n'))
