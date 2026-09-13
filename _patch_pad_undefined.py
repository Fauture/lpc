# -*- coding: utf-8 -*-
"""Pad omitted DECLARED parameters with UNDEFINED, not int 0.

The original driver zero-initializes the frame's parameter area, and in this VM
an all-zero value is type 0 = "undefined" (is_undefined() true, is_int() false).
Our padding used make_int(0) (an INT zero), which flips every `is_undefined()`
check on an omitted argument.  Concrete damage: menu.c::start_talk_to calls
    set_menu(ob, menu, menu_content);          // portrait/pic_no/... omitted
and menu.c::build_menu_buffer does
    else if (is_undefined(portrait)) { actor_name = ob->get_name();
                                       portrait_used = (int)ob->query("portrait"); }
    else { portrait_used = ((portrait == 0) ? -1 : portrait); actor_name = ""; }
so with the int-0 pad the NPC dialog packet carried portrait = 0xFFFFFFFF (-1) and
an EMPTY name (the client showed neither portrait nor name).
"""
import io

P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


OLD_SNIP = B("""for (int i = 0; i < pad; ++i) pushValue(make_int(0));""")
NEW_SNIP = B("""for (int i = 0; i < pad; ++i) pushValue(Value{});   // all-zero == undefined, NOT int 0""")
n = d.count(OLD_SNIP)
print('pad push sites:', n)
assert n >= 1, 'no pad sites found'
d = d.replace(OLD_SNIP, NEW_SNIP)

OLD_SNIP2 = B("""for (int i = 0; i < pad; ++i) pushValue(make_int(0));""")
io.open(P, 'wb').write(d)
print('patched; CRLF', d.count(b'\r\n'), 'bareLF', d.count(b'\n') - d.count(b'\r\n'))
print('make_int(0) pads left:', d.count(b'pushValue(make_int(0));'))
