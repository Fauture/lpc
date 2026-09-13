# -*- coding: utf-8 -*-
"""LPCVM_FNTRACE: accept a comma-separated list of substrings."""
import io

P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


OLD = B("""static bool fnTraceMatch(const std::string& name) {
    static const char* const flt = std::getenv("LPCVM_FNTRACE");
    if (!flt || !*flt || name.empty()) return false;
    return name.find(flt) != std::string::npos;
}""")
NEW = B("""static bool fnTraceMatch(const std::string& name) {
    static const char* const flt = std::getenv("LPCVM_FNTRACE");
    if (!flt || !*flt || name.empty()) return false;
    // Comma-separated list of substrings: any match traces the function.
    const char* p = flt;
    while (*p) {
        const char* comma = std::strchr(p, ',');
        size_t n = comma ? static_cast<size_t>(comma - p) : std::strlen(p);
        if (n && name.find(p, 0, n) != std::string::npos) return true;
        if (!comma) break;
        p = comma + 1;
    }
    return false;
}""")
assert d.count(OLD) == 1, 'anchor not found (%d)' % d.count(OLD)
d = d.replace(OLD, NEW)
io.open(P, 'wb').write(d)
print('patched; CRLF', d.count(b'\r\n'), 'bareLF', d.count(b'\n') - d.count(b'\r\n'))
