// =====================================================================
// Efun batch 2 - verified against the reference driver's vm_f_* decompiles
// (magic_Linux32x; see _ida_missing/).  Same rules as batch 1: placed
// before the stub tracker, names removed from src/stub_efuns.inc.
//
//   * padd/psub(a, b[, sat][, bits]): element-wise add/subtract of two
//     ints or two string/buffers in units of bits/8 bytes.  Strings and
//     buffers are read/written BIG-endian (the decompile byte-swaps every
//     unit), ints are read native.  With a third argument the result is
//     saturated at that value (padd: upper bound, psub: lower bound).
//   * psum(v[, bits]): sum of all units of v (big-endian for string/buffer).
//   * pcmp(a, b[, bits]): buffer of one byte per unit, 0xFF when the units
//     are equal, 0x00 otherwise.
//   * sig(v): sign of an int/float -> -1 / 0 / 1 (vm_f_sig, not a signal).
//   * rmdir(path) / rename_file(old, new): 1 on success, 0 on failure.
//   * set_internal_comm(cno): mark the game client as an internal comm peer
//     (attrib |= 0x302, the driver's INTERNAL_COMM bits) -> 1.
//   * mysql_get_connection_info() / mysql_get_view_info(): the driver's
//     connection/view pool listing; this engine has no pool table, so the
//     no-argument form returns an empty array and the indexed form 0.
//   * peek_io_buffer(): no io buffers are wired up -> 0.
// =====================================================================

if (eid == CID("padd") || eid == CID("psub")) {
    if (args.size() < 2) return make_int(0);
    bool add = (eid == CID("padd"));
    int bits = 8;
    if (args.size() > 3) bits = static_cast<int>(as_int(args[3]));
    if (bits != 8 && bits != 16 && bits != 32) bits = 8;
    const int unit = bits / 8;
    bool isInt = (args[0].index() == 2 && args[1].index() == 2);
    std::string a, b;
    if (isInt) {
        uint32_t x = static_cast<uint32_t>(as_int(args[0]));
        uint32_t y = static_cast<uint32_t>(as_int(args[1]));
        a.assign(reinterpret_cast<char*>(&x), 4);
        b.assign(reinterpret_cast<char*>(&y), 4);
    } else {
        auto bytesOf = [](const Value& v) -> std::string {
            if (v.index() == 4) return as_string(v);
            if (v.index() == 10) return std::get<lpc::Ref<BufferValue>>(v)->data;
            return std::string();
        };
        a = bytesOf(args[0]);
        b = bytesOf(args[1]);
        if (a.size() != b.size()) return make_int(0);
    }
    const size_t len = a.size();
    if (unit == 0 || len % static_cast<size_t>(unit) != 0) return make_int(0);
    const bool be = !isInt;                 // strings/buffers: big-endian units
    bool sat = args.size() > 2;
    int64_t satVal = sat ? as_int(args[2]) : 0;
    if (sat) {
        int64_t lo = (bits == 8) ? -128 : (bits == 16 ? -32768 : -2147483647LL - 1);
        int64_t hi = (bits == 8) ? 255 : (bits == 16 ? 65535 : 0x7FFFFFFFLL);
        if (add) { if (satVal < 0) satVal = 0; if (satVal > hi) satVal = hi; }
        else { if (satVal < lo) satVal = lo; if (satVal > hi) satVal = hi; }
    }
    std::string out(len, '\0');
    for (size_t off = 0; off < len; off += static_cast<size_t>(unit)) {
        auto rd = [&](const std::string& s) -> uint32_t {
            uint32_t v = 0;
            for (int k = 0; k < unit; ++k) {
                unsigned char c = static_cast<unsigned char>(s[off + k]);
                v |= be ? (static_cast<uint32_t>(c) << (8 * (unit - 1 - k)))
                        : (static_cast<uint32_t>(c) << (8 * k));
            }
            return v;
        };
        int64_t x = static_cast<int64_t>(rd(a));
        int64_t y = static_cast<int64_t>(rd(b));
        int64_t r = add ? (x + y) : (x - y);
        if (sat) {
            if (add && r > satVal) r = satVal;
            if (!add && r < satVal) r = satVal;
        }
        uint32_t ur = static_cast<uint32_t>(r);
        for (int k = 0; k < unit; ++k) {
            unsigned char c = be ? static_cast<unsigned char>((ur >> (8 * (unit - 1 - k))) & 0xFF)
                                 : static_cast<unsigned char>((ur >> (8 * k)) & 0xFF);
            out[off + k] = static_cast<char>(c);
        }
    }
    if (isInt) {
        uint32_t r = 0;
        for (int k = 0; k < 4; ++k) r |= static_cast<uint32_t>(static_cast<unsigned char>(out[k])) << (8 * k);
        return make_int(static_cast<int32_t>(r));
    }
    return (args[0].index() == 4) ? make_string(out) : make_buffer(out);
}
if (eid == CID("psum")) {
    if (args.empty()) return make_int(0);
    int bits = 8;
    if (args.size() > 1) bits = static_cast<int>(as_int(args[1]));
    if (bits != 8 && bits != 16 && bits != 32) bits = 8;
    const int unit = bits / 8;
    bool isInt = (args[0].index() == 2);
    std::string s;
    if (isInt) {
        uint32_t x = static_cast<uint32_t>(as_int(args[0]));
        s.assign(reinterpret_cast<char*>(&x), 4);
    } else if (args[0].index() == 4) {
        s = as_string(args[0]);
    } else if (args[0].index() == 10) {
        s = std::get<lpc::Ref<BufferValue>>(args[0])->data;
    } else {
        return make_int(0);
    }
    if (s.size() % static_cast<size_t>(unit) != 0) return make_int(0);
    int64_t sum = 0;
    for (size_t off = 0; off < s.size(); off += static_cast<size_t>(unit)) {
        uint32_t v = 0;
        for (int k = 0; k < unit; ++k) {
            unsigned char c = static_cast<unsigned char>(s[off + k]);
            v |= isInt ? (static_cast<uint32_t>(c) << (8 * k))
                       : (static_cast<uint32_t>(c) << (8 * (unit - 1 - k)));
        }
        sum += static_cast<int64_t>(v);
    }
    return make_int(sum);
}
if (eid == CID("pcmp")) {
    if (args.size() < 2) return make_int(0);
    int bits = 8;
    if (args.size() > 2) bits = static_cast<int>(as_int(args[2]));
    if (bits != 8 && bits != 16 && bits != 32) bits = 8;
    const int unit = bits / 8;
    bool isInt = (args[0].index() == 2 && args[1].index() == 2);
    std::string a, b;
    if (isInt) {
        uint32_t x = static_cast<uint32_t>(as_int(args[0]));
        uint32_t y = static_cast<uint32_t>(as_int(args[1]));
        a.assign(reinterpret_cast<char*>(&x), 4);
        b.assign(reinterpret_cast<char*>(&y), 4);
    } else {
        auto bytesOf = [](const Value& v) -> std::string {
            if (v.index() == 4) return as_string(v);
            if (v.index() == 10) return std::get<lpc::Ref<BufferValue>>(v)->data;
            return std::string();
        };
        a = bytesOf(args[0]);
        b = bytesOf(args[1]);
        if (a.size() != b.size()) return make_buffer(std::string());
    }
    if (a.size() % static_cast<size_t>(unit) != 0) return make_buffer(std::string());
    std::string out;
    out.reserve(a.size() / static_cast<size_t>(unit));
    for (size_t off = 0; off + static_cast<size_t>(unit) <= a.size(); off += static_cast<size_t>(unit))
        out.push_back(a.compare(off, static_cast<size_t>(unit), b, off, static_cast<size_t>(unit)) == 0
                          ? static_cast<char>(0xFF) : static_cast<char>(0x00));
    return make_buffer(out);
}
if (eid == CID("sig")) {
    // vm_f_sig: sign of the argument (-1 / 0 / 1).
    if (args.empty()) return make_int(0);
    if (args[0].index() == 3) {
        double d = std::get<double>(args[0]);
        return make_int(d < 0.0 ? -1 : (d > 0.0 ? 1 : 0));
    }
    int64_t n = as_int(args[0]);
    return make_int(n < 0 ? -1 : (n > 0 ? 1 : 0));
}
if (eid == CID("rmdir")) {
    // vm_f_rmdir: 1 on success, 0 on failure.
    if (args.empty()) return make_int(0);
    std::error_code ec;
    fs::path p = resolveVfsPath(as_string(args[0]));
    bool ok = fs::is_directory(p, ec) && fs::remove(p, ec) && !ec;
    return make_int(ok ? 1 : 0);
}
if (eid == CID("rename_file")) {
    // vm_f_rename_file: 1 on success, 0 on failure.
    if (args.size() < 2) return make_int(0);
    std::error_code ec;
    fs::path from = resolveVfsPath(as_string(args[0]));
    fs::path to = resolveVfsPath(as_string(args[1]));
    if (!fs::exists(from, ec)) return make_int(0);
    fs::rename(from, to, ec);
    return make_int(ec ? 0 : 1);
}
if (eid == CID("set_internal_comm")) {
    // vm_f_set_internal_comm: mark the client as an internal comm peer.
    if (args.empty()) return make_int(0);
    int cno = static_cast<int>(as_int(args[0]));
    gameServiceSetAttrib(cno, 0x302u, 0, -1);
    return make_int(1);
}
if (eid == CID("mysql_get_connection_info") || eid == CID("mysql_get_view_info")) {
    if (args.empty()) return make_array();
    return make_int(0);
}
if (eid == CID("peek_io_buffer")) return make_int(0);
if (eid == CID("var_info")) {
    // vm_f_var_info (0x80f97f0): {type, type_desc, address} of a value.
    auto m = lpc::Ref<MappingValue>(new MappingValue());
    int ti = args.empty() ? 0 : args[0].index();
    m->insert(make_string("type"), make_int(ti));
    m->insert(make_string("type_desc"), make_string(args.empty() ? std::string("0") : value_type_name(args[0])));
    m->insert(make_string("address"), make_int(0));
    return m;
}
