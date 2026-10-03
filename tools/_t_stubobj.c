// Which of the "unimplemented" efuns are actually dispatched?
// run: build\Release\lpcvm.exe run _t_stubobj.c
void create() {
    mixed o;
    string s;
    mixed r;

    o = this_object();
    printf("this=%O\n", o);

    // save_object(obj) -> "prog::var=value\n" text.  In stub_efuns.inc, so the
    // stub tracker (which returns 0 first) should win if it is really stubbed.
    r = save_object(o);
    printf("save_object -> type=%d val=%O\n", typeof(r), r);

    // restore_object(obj, text[, clear])
    r = restore_object(o, "x::y=1\n");
    printf("restore_object -> type=%d val=%O\n", typeof(r), r);

    // set_obj_var(obj, name, value)
    r = set_obj_var(o, "zzz", 4242);
    printf("set_obj_var -> type=%d val=%O\n", typeof(r), r);

    // control: an efun that is definitely implemented
    r = strlen("abcd");
    printf("strlen -> type=%d val=%O\n", typeof(r), r);

    // control: an efun that is definitely a stub
    r = get_uid(o);
    printf("get_uid -> type=%d val=%O\n", typeof(r), r);
}
