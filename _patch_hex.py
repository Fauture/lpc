# -*- coding: utf-8 -*-
"""Print the raw BYTES of the two strings that must match:

  A) the class registered at boot:  register_task_template(a0=...) from the .o
  B) the name looked up at login:   task_info["name"] out of the saved dbase

If they differ byte-wise the mapping lookups (task_name_list / all_templates)
can never hit - which is what the is_type probe shows (is_mapping on
valueType=0 = undefined).
"""
import io

p = 'src/original_vm.cpp'
d = io.open(p, encoding='utf-8', errors='replace').read()

# 1) FNARGS: add hex + size for string args
old = '''                    const Value& av = stack_[static_cast<size_t>(si)].value;
                    std::string s = to_string(av, false);
                    if (s.size() > 140) s = s.substr(0, 140) + "...";
                    std::cerr << " a" << ai << "=t" << av.index() << ":" << s;'''
new = '''                    const Value& av = stack_[static_cast<size_t>(si)].value;
                    std::string s = to_string(av, false);
                    if (s.size() > 140) s = s.substr(0, 140) + "...";
                    std::cerr << " a" << ai << "=t" << av.index() << ":" << s;
                    if (av.index() == 4) {
                        const std::string& sv = std::get<SharedString>(av).str();
                        std::cerr << " len=" << sv.size() << " hex=";
                        static const char* hx = "0123456789ABCDEF";
                        for (size_t q = 0; q < sv.size() && q < 40; ++q) {
                            std::cerr << hx[(static_cast<unsigned char>(sv[q]) >> 4) & 15]
                                      << hx[static_cast<unsigned char>(sv[q]) & 15];
                        }
                    }'''
o = old.replace('\n', '\r\n') if '\r\n' in d else old
n = new.replace('\n', '\r\n') if '\r\n' in d else new
assert o in d, 'FNARGS anchor not found'
d = d.replace(o, n, 1)

# 2) istype probe: hex of the tested value
old2 = '''                                std::string d = to_string(vv, false);
                                if (d.size() > 120) d = d.substr(0, 120) + "...";'''
new2 = '''                                std::string d = to_string(vv, false);
                                if (d.size() > 120) d = d.substr(0, 120) + "...";
                                if (vv.index() == 4) {
                                    const std::string& sv = std::get<SharedString>(vv).str();
                                    d += " len=" + std::to_string(sv.size()) + " hex=";
                                    static const char* hx2 = "0123456789ABCDEF";
                                    for (size_t q = 0; q < sv.size() && q < 40; ++q)
                                        d += std::string(1, hx2[(static_cast<unsigned char>(sv[q]) >> 4) & 15]) +
                                             hx2[static_cast<unsigned char>(sv[q]) & 15];
                                }'''
o2 = old2.replace('\n', '\r\n') if '\r\n' in d else old2
n2 = new2.replace('\n', '\r\n') if '\r\n' in d else new2
assert o2 in d, 'istype anchor not found'
d = d.replace(o2, n2, 1)

io.open(p, 'w', encoding='utf-8', newline='').write(d)
print('hex probes added')
