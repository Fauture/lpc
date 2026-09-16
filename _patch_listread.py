# -*- coding: utf-8 -*-
"""Probe every `.list` config read, so an empty `task_name.list` /
`task_alias.list` / shop-goods list is visible directly.

importd::create_data does `read_etc_file(file, "b")` (importd_v37.c:132) and
then explodes it into lines; an empty/failed read leaves the destination
mapping empty, which is what makes get_task_class() fall back to the task NAME
and every task template lookup miss.
"""
import io

p = 'src/builtins.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()
nl = '\r\n' if '\r\n' in d else '\n'

old = '''    if (name == "read_file") {
        if (args.empty()) return make_string("");
        std::string vpath = as_string(args[0]);'''
new = '''    if (name == "read_file") {
        if (args.empty()) return make_string("");
        std::string vpath = as_string(args[0]);
        // [listread] every ".list" config read (task_name.list / task_alias.list
        // / shop goods lists ...): a zero-length answer means the caller's
        // destination mapping stays empty and the whole feature silently
        // degrades (get_task_class -> task name -> no template -> empty quest
        // log; no shop goods).
        if (vpath.find(".list") != std::string::npos) {
            static int listBudget = 400;
            std::string rv = readVfsFile(vpath);
            if (listBudget > 0) {
                --listBudget;
                std::cerr << "[listread] path=" << vpath << " bytes=" << rv.size() << "\\n";
                std::cerr.flush();
            }
            if (args.size() >= 2 && args[1].index() == 4 &&
                std::get<SharedString>(args[1]).str() == "b")
                return make_buffer(rv);
            return make_string(rv);
        }'''
assert old.replace('\n', nl) in d, 'read_file anchor not found'
d = d.replace(old.replace('\n', nl), new.replace('\n', nl), 1)
io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('listread probe added')
