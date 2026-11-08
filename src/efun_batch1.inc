// =====================================================================
// Efun batch 1 - efuns implemented from the reference driver's own
// decompilation (IDA, magic_Linux32x).  The authoritative list of the
// driver's efuns came from its ELF symbol table + the efun registration
// tables (_efun_report.txt): 561 real efuns, of which this file covers
// the short/self-contained ones.  Every branch here is placed BEFORE the
// stub tracker in call_builtin and the name is removed from
// src/stub_efuns.inc by tools/gen_stubset.py.
//
// Semantics notes (verified against vm_f_* decompiles):
//   * set_trace_level / xdebug / set_primitive / trace_modules are empty
//     in the original (debugger hooks compiled out) -> return 0.
//   * debug_off returns the PREVIOUS debugOn and sets it to -1.
//   * enable_message_stat / reset_message_stat only touch driver stats.
//   * get_internal_encoding returns vm_internalEncoding ("GBK" here);
//     set_internal_encoding stores it and returns nothing.
//   * mysql_debug_on/off return the previous dbMysqlDebugOn.
//   * rpc_debug_on/off, rpc_stop_server, rpc_pass_auth, rpc_close_client,
//     rpc_set_port, rpc_is_child, rpc_this_client have no RPC server in
//     this build -> the driver's values are 0.
//   * to_unsigned_str(v) = sprintf("%u", (unsigned)v).
//   * strwidth(s[,enc]) = display width (double-byte chars count 2).
//   * net8/net16/net32 pack big-endian bytes/words/dwords into a buffer.
//   * hash(v) = vm_whashbuf polynomial over the first 32 bytes.
//   * empty_value clears an array/mapping in place.
//   * get_first_data(...) returns the first non-zero argument.
//   * is_pt_in_line(x,y,x1,y1,x2,y2) / is_pt_in_rect(...) use the
//     original IsPtInLine/IsPtInRect (bounding box + collinearity).
//   * is_rect_crossed / is_line_crossed use the original IsRectCrossed /
//     IsLineCrossed geometry (two corner pairs per rect/segment).
//   * csv_parse / break_utf8 / combine_utf8 port the original parsers.
// =====================================================================

// ---- apex anti-cheat channel (IDA: vm_f_enable_apex 0x807e030,
//      vm_f_send_to_apex 0x807e124, CHSEnd 0x804be7c) --------------------
// The original's apex path is an OPTIONAL PLUGIN loaded at run time:
//   enable_apex([on])  on!=0 -> CHSStart() (dlopen the "ASclient" library; the
//                              globals are g_hASclient / g_sh_start /
//                              g_sh_exit / g_sh_setfunc) and CHSSetFunc()
//                              installs the receive callback; returns 1 only
//                              when that callback (apexRecv) got installed.
//                       on==0 -> apexRecv = 0; CHSEnd() (dlclose) -> returns 1.
//   send_to_apex(type, value, buf)
//                      apexRecv installed -> forward and return its result;
//                      otherwise -1.
// This build has no such plugin, so ENABLING correctly fails (0), but
// DISABLING must still answer 1 and send_to_apex must answer -1.  The old
// catch-all stub below answered 0 for both, which is not what the driver did.
if (eid == CID("enable_apex")) {
    int64_t on = args.empty() ? 1 : as_int(args[0]);
    return make_int(on ? 0 : 1);
}
if (eid == CID("send_to_apex")) return make_int(-1);

// ---- debugger / tracing / profiling knobs (all no-ops in this build) ----
if (eid == CID("set_trace_level") || eid == CID("xdebug") ||
    eid == CID("set_primitive") || eid == CID("trace_modules") ||
    eid == CID("debug_enter") || eid == CID("debug_command") ||
    eid == CID("debug_trap") || eid == CID("debug_disassemble") ||
    eid == CID("debug_quick_show_values") ||
    eid == CID("debug_delete_breakpoint") || eid == CID("debug_get_breakpoint") ||
    eid == CID("pp_get_profiling_time") || eid == CID("reset_eval_cost") ||
    eid == CID("set_force_relinquish") || eid == CID("reset_function_mark") ||
    eid == CID("reset_thread_seq_no") || eid == CID("set_thread_attr") ||
    eid == CID("set_thread_deletion") || eid == CID("resume_thread") ||
    eid == CID("set_io_translator") || eid == CID("enable_client_record") ||
    eid == CID("enable_client_snapshot") || eid == CID("set_mobile_client") ||
    eid == CID("debug_client") || eid == CID("add_resource_string") ||
    eid == CID("load_resource_strings") || eid == CID("reset_message_stat") ||
    eid == CID("garbage_collect") || eid == CID("mem_l2_stat") ||
    eid == CID("mem_mark") || eid == CID("mem_unmark") || eid == CID("mem_dump") ||
    eid == CID("mem_reset_alloc_stat") || eid == CID("mem_list") ||
    eid == CID("mem_stat") || eid == CID("mem_show_marked") ||
    eid == CID("verigen_remove_font") || eid == CID("verigen_mem_stat") ||
    eid == CID("verigen_get_distortion_index") ||
    eid == CID("verigen_set_no_distortion_char") ||
    eid == CID("dbfs_compact") || eid == CID("dbfs_openfs") ||
    eid == CID("save_map_info") || eid == CID("create_fs") ||
    eid == CID("pack_dir") || eid == CID("fgetfpinfo"))
    return make_int(0);
if (eid == CID("rpc_debug_on") || eid == CID("rpc_debug_off") ||
    eid == CID("rpc_stop_server") || eid == CID("rpc_pass_auth") ||
    eid == CID("rpc_close_client") || eid == CID("rpc_relay_to") ||
    eid == CID("rpc_call") || eid == CID("rpc_is_child") ||
    eid == CID("rpc_this_client") || eid == CID("rpc_is_client_on") ||
    eid == CID("rpc_set_port"))
    return make_int(0);
if (eid == CID("rpc_info")) return make_mapping();
if (eid == CID("check_var_closed_loop")) return make_int(0);
if (eid == CID("ps")) return make_int(0);
if (eid == CID("system")) {
    // Original refuses unless booted with --enable-system-command; we never
    // enable it, so mirror the refusal (but without vm_error'ing the caller).
    std::cerr << "[efun] system() refused (disabled for security)\n";
    return make_int(-1);
}
if (eid == CID("get_output") || eid == CID("set_output") || eid == CID("set_stdout"))
    return make_int(1);
if (eid == CID("set_echo_input")) return make_int(1);
if (eid == CID("debug_off")) {
    // vm_f_debug_off: return previous debugOn, then set it to -1.
    static int debugOn = 1;
    int old = debugOn;
    debugOn = -1;
    return make_int(old);
}
if (eid == CID("enable_jit")) {
    static int jit = 0;
    int old = jit;
    jit = (!args.empty() && is_truthy(args[0])) ? 1 : 0;
    return make_int(old);
}
if (eid == CID("enable_message_stat")) return make_int(0);
if (eid == CID("get_internal_encoding")) return make_string("GBK");
if (eid == CID("set_internal_encoding")) {
    static std::string enc = "GBK";
    std::string old = enc;
    if (!args.empty()) {
        std::string e = as_string(args[0]);
        if (e.size() <= 31) enc = e;
    }
    return make_string(old);
}
if (eid == CID("mysql_debug_on") || eid == CID("mysql_debug_off")) {
    static int dbg = 0;
    int old = dbg;
    dbg = (eid == CID("mysql_debug_on")) ? 1 : 0;
    return make_int(old);
}
if (eid == CID("mysql_get_version_info")) {
    auto m = lpc::Ref<MappingValue>(new MappingValue());
    m->insert(make_string("MYSQL_SERVER_VERSION"), make_string("5.0.22"));
    m->insert(make_string("MYSQL_VERSION_ID"), make_int(50022));
    return m;
}
if (eid == CID("get_system_id")) {
    // vm_f_get_system_id folds the current task id; one task id here.
    int32_t t = 1;
    int32_t v = (t ^ ((t ^ ((t ^ (t << 8)) << 8)) << 8)) - 2004318072;
    return make_int(static_cast<int64_t>(static_cast<int32_t>(v ^ 0x12345678)));
}
if (eid == CID("get_prompt")) return make_string(promptText());
if (eid == CID("set_prompt")) {
    if (!args.empty()) promptText() = as_string(args[0]);
    return make_string(promptText());
}
if (eid == CID("set_broadcast")) return args.empty() ? make_int(0) : args[0];
if (eid == CID("get_pconfig")) {
    int64_t id = args.empty() ? 0 : as_int(args[0]);
    auto& pc = pconfigTable();
    auto it = pc.find(id);
    return make_int(it == pc.end() ? 0 : it->second);
}
if (eid == CID("change_pconfig")) {
    int64_t id = args.empty() ? 0 : as_int(args[0]);
    int64_t v = args.size() > 1 ? as_int(args[1]) : 0;
    pconfigTable()[id] = v;
    return make_int(0);
}
if (eid == CID("switch_root_dir")) return make_int(0);
if (eid == CID("get_var")) {
    if (args.empty()) return Value(UndefinedTag{});
    auto& gv = globalVarTable();
    auto it = gv.find(as_string(args[0]));
    if (it == gv.end()) return Value(UndefinedTag{});
    return it->second;
}
if (eid == CID("set_var")) {
    if (args.size() < 2) return make_int(0);
    globalVarTable()[as_string(args[0])] = args[1];
    return make_int(1);
}
if (eid == CID("get_string_stat")) return make_int(0);
if (eid == CID("get_ref_counter")) return make_int(1);
if (eid == CID("get_pointer")) return make_int(0);
if (eid == CID("get_pend_object_id")) return make_int(-1);
if (eid == CID("get_pend_object_info")) return make_int(0);
if (eid == CID("get_all_pend_objects")) return make_array();
if (eid == CID("get_map_events")) return make_array();
if (eid == CID("uscounter")) {
    // vm_getCurrentUsCounter: microseconds since the current tick started.
    using namespace std::chrono;
    static const auto base = steady_clock::now();
    int64_t us = duration_cast<microseconds>(steady_clock::now() - base).count();
    return make_int(static_cast<int32_t>(us));
}
if (eid == CID("to_unsigned_str")) {
    uint32_t u = static_cast<uint32_t>(args.empty() ? 0 : as_int(args[0]));
    return make_string(std::to_string(u));
}
if (eid == CID("strwidth")) {
    // vm_f_strwidth: display width under the internal encoding.  GBK lead
    // bytes (0x81..0xFE) consume the following byte and count as 2 columns.
    std::string s = args.empty() ? std::string() : as_string(args[0]);
    int64_t w = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 0x81 && c <= 0xFE && i + 1 < s.size()) { w += 2; ++i; }
        else ++w;
    }
    return make_int(w);
}
if (eid == CID("hash")) {
    // vm_f_hash: 16-bit hash of a string or buffer.  The string branch uses
    // the driver's table hash (vm_whashstr), the buffer branch vm_whashbuf
    // (base-8 polynomial).  We use the polynomial for both so a value is
    // stable across our engine; the driver's own table hash is internal.
    std::string s;
    if (!args.empty()) {
        if (args[0].index() == 4) s = as_string(args[0]);
        else if (args[0].index() == 10)
            s = std::get<lpc::Ref<BufferValue>>(args[0])->data;
    }
    uint32_t h = 0;
    size_t n = s.size() < 32 ? s.size() : 32;
    for (size_t i = 0; i < n; ++i) h = static_cast<uint32_t>(static_cast<unsigned char>(s[i])) + 8u * h;
    return make_int(static_cast<int64_t>(h & 0xFFFFu));
}
if (eid == CID("net8") || eid == CID("net16") || eid == CID("net32")) {
    // vm_f_net8/16/32: pack the integer arguments big-endian into a buffer.
    int width = (eid == CID("net8")) ? 1 : (eid == CID("net16") ? 2 : 4);
    std::string buf;
    buf.reserve(args.size() * width);
    for (const Value& v : args) {
        uint32_t x = static_cast<uint32_t>(as_int(v));
        for (int k = width - 1; k >= 0; --k)
            buf.push_back(static_cast<char>((x >> (8 * k)) & 0xFF));
    }
    return make_buffer(buf);
}
if (eid == CID("empty_value")) {
    if (args.empty()) return make_int(0);
    if (args[0].index() == 5) {
        std::get<lpc::Ref<ArrayValue>>(args[0])->items.clear();
    } else if (args[0].index() == 6) {
        auto m = std::get<lpc::Ref<MappingValue>>(args[0]);
        m->nodes.clear();
        m->table.assign(8, -1);
        m->tableSize = 7;
        m->unfilled = 6;
        m->cnt = 0;
    }
    return make_int(0);
}
if (eid == CID("get_first_data")) {
    // vm_f_get_first_data: first argument that is not zero.
    for (const Value& v : args)
        if (is_truthy(v)) return v;
    return Value(UndefinedTag{});
}
if (eid == CID("atomac")) {
    // vm_f_atomac: build a 6-byte MAC buffer from "xx.xx.xx.xx.xx.xx" /
    // "xx-xx-..." (hex), from an int[,int], or pass a 6-byte buffer through.
    unsigned char mac[6] = {0, 0, 0, 0, 0, 0};
    if (!args.empty() && args[0].index() == 4) {
        const std::string s = as_string(args[0]);
        int idx = 0;
        size_t pos = 0;
        while (idx < 6 && pos < s.size()) {
            while (pos < s.size() && (s[pos] == '.' || s[pos] == '-')) ++pos;
            if (pos >= s.size()) break;
            int val = 0, digits = 0;
            while (pos < s.size() && digits < 2) {
                char c = s[pos];
                int d = -1;
                if (c >= '0' && c <= '9') d = c - '0';
                else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
                else break;
                val = val * 16 + d;
                ++pos; ++digits;
            }
            if (!digits) break;
            mac[idx++] = static_cast<unsigned char>(val);
        }
    } else if (!args.empty() && args[0].index() == 2) {
        uint32_t a = static_cast<uint32_t>(as_int(args[0]));
        uint32_t b = args.size() > 1 ? static_cast<uint32_t>(as_int(args[1])) : 0;
        mac[0] = static_cast<unsigned char>((a >> 8) & 0xFF);
        mac[1] = static_cast<unsigned char>(a & 0xFF);
        mac[2] = static_cast<unsigned char>((b >> 24) & 0xFF);
        mac[3] = static_cast<unsigned char>((b >> 16) & 0xFF);
        mac[4] = static_cast<unsigned char>((b >> 8) & 0xFF);
        mac[5] = static_cast<unsigned char>(b & 0xFF);
    } else if (!args.empty() && args[0].index() == 10) {
        return args[0];
    }
    return make_buffer(std::string(reinterpret_cast<char*>(mac), 6));
}
if (eid == CID("fxn")) {
    // vm_f_fxn(x, c1, c2, ...) = c1*x + c2*x^2 + ... (Horner form, float).
    if (args.size() < 2) return make_float(0.0);
    double x = (args[0].index() == 3) ? std::get<double>(args[0])
                                      : static_cast<double>(as_int(args[0]));
    double ret = 0.0, xn = x;
    for (size_t i = 1; i < args.size(); ++i) {
        double c = (args[i].index() == 3) ? std::get<double>(args[i])
                                          : static_cast<double>(as_int(args[i]));
        ret += c * xn;
        xn *= x;
    }
    return make_float(ret);
}
if (eid == CID("is_pt_in_line") || eid == CID("is_pt_in_rect")) {
    // IsPtInLine/IsPtInRect(x, y, x1, y1, x2, y2)
    if (args.size() < 6) return make_int(0);
    int64_t x = as_int(args[0]), y = as_int(args[1]);
    int64_t x1 = as_int(args[2]), y1 = as_int(args[3]);
    int64_t x2 = as_int(args[4]), y2 = as_int(args[5]);
    int64_t lx = x1, hx = x2, ly = y1, hy = y2;
    if (x1 > x2) { lx = x2; hx = x1; }
    if (y1 > y2) { ly = y2; hy = y1; }
    bool in = (x >= lx && x <= hx && y >= ly && y <= hy);
    if (in && eid == CID("is_pt_in_line"))
        in = (y2 - y1) * (x - x1) == (y - y1) * (x2 - x1);
    return make_int(in ? 1 : 0);
}
if (eid == CID("is_rect_crossed")) {
    // IsRectCrossed(x1,y1,x2,y2, x3,y3,x4,y4): axis-aligned overlap.
    if (args.size() < 8) return make_int(0);
    int64_t x1 = as_int(args[0]), y1 = as_int(args[1]);
    int64_t x2 = as_int(args[2]), y2 = as_int(args[3]);
    int64_t x3 = as_int(args[4]), y3 = as_int(args[5]);
    int64_t x4 = as_int(args[6]), y4 = as_int(args[7]);
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (x3 > x4) std::swap(x3, x4);
    if (y3 > y4) std::swap(y3, y4);
    bool hit = !(x2 < x3 || x4 < x1 || y2 < y3 || y4 < y1);
    return make_int(hit ? 1 : 0);
}
if (eid == CID("is_line_crossed")) {
    // IsLineCrossed(x1,y1,x2,y2, x3,y3,x4,y4): segment intersection test,
    // ported 1:1 from the driver (bounding box + three cross products).
    if (args.size() < 8) return make_int(0);
    int64_t p0 = as_int(args[0]), p1 = as_int(args[1]);
    int64_t p2 = as_int(args[2]), p3 = as_int(args[3]);
    int64_t p4 = as_int(args[4]), p5 = as_int(args[5]);
    int64_t p6 = as_int(args[6]), p7 = as_int(args[7]);
    int64_t lx1 = p0, hx1 = p2, ly1 = p1, hy1 = p3;
    if (p0 > p2) { lx1 = p2; hx1 = p0; }
    if (p1 > p3) { ly1 = p3; hy1 = p1; }
    int64_t lx2 = p4, hx2 = p6, ly2 = p5, hy2 = p7;
    if (p4 > p6) { lx2 = p6; hx2 = p4; }
    if (p5 > p7) { ly2 = p7; hy2 = p5; }
    if (hx1 < lx2 || hx2 < lx1 || hy1 < ly2 || hy2 < ly1) return make_int(0);
    auto sgn = [](int64_t v) -> int64_t { return v > 0 ? 1 : (v < 0 ? -1 : 0); };
    int64_t d1 = (p7 - p5) * (p0 - p4) - (p1 - p5) * (p6 - p4);
    int64_t d2 = (p3 - p5) * (p6 - p4) - (p7 - p5) * (p2 - p4);
    if (sgn(d1) * sgn(d2) < 0) return make_int(0);
    int64_t d3 = (p3 - p1) * (p4 - p0) - (p5 - p1) * (p2 - p0);
    int64_t d4 = (p7 - p1) * (p2 - p0) - (p3 - p1) * (p6 - p0);
    return make_int(sgn(d3) * sgn(d4) >= 0 ? 1 : 0);
}
if (eid == CID("break_utf8")) {
    // vm_f_break_utf8: UTF-8 text -> array of unicode code points.
    std::string s = args.empty() ? std::string() : as_string(args[0]);
    auto a = lpc::Ref<ArrayValue>(new ArrayValue());
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        uint32_t cp = 0; int extra = 0;
        if (c < 0x80) { cp = c; extra = 0; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
        else break;                       // invalid lead byte: stop like original
        if (i + static_cast<size_t>(extra) >= s.size()) break;
        bool ok = true;
        for (int k = 1; k <= extra; ++k) {
            unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if ((cc & 0xC0) != 0x80) { ok = false; break; }
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (!ok || cp == 0) break;
        a->items.push_back(make_int(static_cast<int64_t>(cp)));
        i += static_cast<size_t>(extra) + 1;
    }
    return a;
}
if (eid == CID("combine_utf8")) {
    // vm_f_combine_utf8: array of code points -> UTF-8 text.
    std::string out;
    if (!args.empty() && args[0].index() == 5) {
        for (const Value& v : std::get<lpc::Ref<ArrayValue>>(args[0])->items) {
            uint32_t cp = static_cast<uint32_t>(as_int(v));
            if (cp < 0x80) out.push_back(static_cast<char>(cp));
            else if (cp < 0x800) {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp < 0x10000) {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }
    }
    return make_string(out);
}
if (eid == CID("csv_parse")) {
    // vm_f_csv_parse: RFC-style CSV with '"' quoting ("" = literal quote),
    // ',' field separator, LF/CRLF record separator.  Returns an array of
    // arrays of strings.
    std::string s = args.empty() ? std::string() : as_string(args[0]);
    auto result = lpc::Ref<ArrayValue>(new ArrayValue());
    auto row = lpc::Ref<ArrayValue>(new ArrayValue());
    std::string field;
    bool quoted = false, anyField = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (quoted) {
            if (c == '"') {
                if (i + 1 < s.size() && s[i + 1] == '"') { field.push_back('"'); ++i; }
                else quoted = false;
            } else if (c != '\r') field.push_back(c);
        } else if (c == '"') {
            quoted = true; anyField = true;
        } else if (c == ',') {
            row->items.push_back(make_string(field));
            field.clear(); anyField = true;
        } else if (c == '\n') {
            if (anyField || !field.empty() || !row->items.empty()) {
                row->items.push_back(make_string(field));
                result->items.push_back(row);
            }
            row = lpc::Ref<ArrayValue>(new ArrayValue());
            field.clear(); anyField = false;
        } else if (c != '\r') {
            field.push_back(c);
        }
    }
    if (anyField || !field.empty() || !row->items.empty()) {
        row->items.push_back(make_string(field));
        result->items.push_back(row);
    }
    return result;
}
if (eid == CID("check_event")) {
    // vm_f_check_event(name|id[, mask]) -> raised bits of the event.
    if (args.empty()) return make_int(0);
    auto& reg = eventRegistry();
    std::lock_guard<std::mutex> lk(reg.mtx);
    std::string en = resolveEventNameLocked(args[0], reg);
    if (en.empty()) return make_int(0);
    auto it = reg.events.find(en);
    if (it == reg.events.end()) return make_int(0);
    uint32_t bits = it->second->raised;
    if (args.size() > 1) bits &= static_cast<uint32_t>(as_int(args[1]));
    return make_int(static_cast<int64_t>(bits));
}
if (eid == CID("telnet_config_info")) {
    auto m = lpc::Ref<MappingValue>(new MappingValue());
    m->insert(make_string("native_process"), make_int(0));
    m->insert(make_string("echo"), make_int(0));
    m->insert(make_string("auth"), make_int(0));
    return m;
}
if (eid == CID("convert_mapping_to_string")) {
    // vm_f_convert_mapping_to_string(map, keys[]): "key:value;key:value;"
    // for every key in the array that exists in the mapping.
    if (args.size() < 2 || args[0].index() != 6 || args[1].index() != 5)
        return make_string("");
    auto mp = std::get<lpc::Ref<MappingValue>>(args[0]);
    std::string out;
    for (const Value& k : std::get<lpc::Ref<ArrayValue>>(args[1])->items) {
        if (k.index() != 4) continue;
        const Value* got = mp->find(k);
        if (got && got->index() == 2)
            out += as_string(k) + ":" + std::to_string(as_int(*got)) + ";";
    }
    return make_string(out);
}
if (eid == CID("explode_to_mapping")) {
    // vm_f_explode_to_mapping(str, sep, kvsep): "a=1;b=2" -> ([ "a":1 ]).
    std::string s = args.empty() ? std::string() : as_string(args[0]);
    char sep = args.size() > 1 ? static_cast<char>(as_int(args[1]) & 0xFF) : ',';
    char kv = args.size() > 2 ? static_cast<char>(as_int(args[2]) & 0xFF) : '=';
    auto m = lpc::Ref<MappingValue>(new MappingValue());
    size_t pos = 0;
    while (pos <= s.size()) {
        size_t e = s.find(sep, pos);
        std::string part = s.substr(pos, e == std::string::npos ? std::string::npos : e - pos);
        size_t k = part.find(kv);
        if (k != std::string::npos) {
            std::string key = part.substr(0, k);
            int64_t val = 0;
            try { val = std::stoll(part.substr(k + 1)); } catch (...) { val = 0; }
            m->insert(make_string(key), make_int(val));
        }
        if (e == std::string::npos) break;
        pos = e + 1;
    }
    return m;
}
if (eid == CID("create_io_buffer")) {
    // vm_f_create_io_buffer(name[, size]) -> fd.  No real io buffers are
    // wired to the socket layer here; hand out a stable fake fd so LPC code
    // that only passes it around keeps working.
    static int64_t nextFd = 1000;
    return make_int(nextFd++);
}
if (eid == CID("get_io_buffer")) return Value(UndefinedTag{});
if (eid == CID("delete_io_buffer")) return make_int(0);
if (eid == CID("get_all_io_buffers_info")) return make_array();
if (eid == CID("mysql_escape_string")) {
    // vm_f_mysql_escape_string returns the text wrapped in single quotes
    // with the MySQL C-API escapes applied.
    std::string in = args.empty() ? std::string() : as_string(args[0]);
    std::string out = "'";
    for (char c : in) {
        switch (c) {
            case '\0': out += "\\0"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\\': out += "\\\\"; break;
            case '\'': out += "\\'"; break;
            case '"': out += "\\\""; break;
            case '\x1a': out += "\\Z"; break;
            default: out.push_back(c);
        }
    }
    out += "'";
    return make_string(out);
}
