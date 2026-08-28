// Verify the version field round-trip: des(":1.60.0905:", EN0, "abcdef")
// -> hex string; then des(restore(:hex:), DE1, "abcdef") -> ":1.60.0905:".
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include "d3des.h"
}

static std::string toHex(const std::string& b) {
    static const char* hx = "0123456789ABCDEF";
    std::string o;
    for (unsigned char c : b) { o += hx[c >> 4]; o += hx[c & 0xF]; }
    return o;
}
static std::string fromHex(const std::string& h) {
    std::string o;
    for (size_t i = 0; i + 1 < h.size(); i += 2) {
        auto hv = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };
        o += static_cast<char>((hv(h[i]) << 4) | hv(h[i + 1]));
    }
    return o;
}

static std::string desCipher(const std::string& data, int action, const std::string& key) {
    char keyBuf[128] = {0};
    memcpy(keyBuf, key.c_str(), key.size() < 127 ? key.size() : 127);
    unsigned char key8[8] = {0};
    makekey(keyBuf, key8);
    size_t blocks = (data.size() + 7) / 8;
    std::string padded = data;
    if (padded.size() % 8 != 0) {
        size_t rem = 8 - (padded.size() % 8);
        padded.append(rem, static_cast<char>(rem));
    }
    std::string out(blocks * 8, '\0');
    deskey(key8, action == 0 ? 0 : 1);
    for (size_t i = 0; i < blocks; ++i)
        des(reinterpret_cast<unsigned char*>(&padded[i * 8]),
            reinterpret_cast<unsigned char*>(&out[i * 8]));
    if (action != 0 && !out.empty()) {
        unsigned char last = static_cast<unsigned char>(out[out.size() - 1]);
        if (last >= 1 && last <= 8 && out.size() >= last) {
            bool ok = true;
            for (size_t i = out.size() - last; i < out.size(); ++i)
                if (static_cast<unsigned char>(out[i]) != last) { ok = false; break; }
            if (ok) out.resize(out.size() - last);
        }
    }
    return out;
}

int main() {
    // server-side: version = des(restore_value(":" + clientVersion + ":"), 1, "abcdef")
    // client-side: clientVersion = save_string(des(":1.60.0905:", 0, "abcdef"))[1..<2]
    std::string plain = ":1.60.0905:";
    std::string enc = desCipher(plain, 0, "abcdef");
    std::string clientVersion = toHex(enc);
    printf("plain=%s (%zu bytes)\n", plain.c_str(), plain.size());
    printf("cipher hex=%s (%zu)\n", clientVersion.c_str(), clientVersion.size());

    // server decrypt
    std::string dec = desCipher(fromHex(clientVersion), 1, "abcdef");
    printf("decrypted=%s (%zu bytes)\n", dec.c_str(), dec.size());
    printf("match=%s\n", dec == plain ? "YES" : "NO");

    // also test :hex: restore path used by restore_value
    std::string restoreInput = ":" + clientVersion + ":";
    std::string dec2 = desCipher(restoreInput.substr(1, restoreInput.size() - 2), 1, "abcdef");
    printf("decrypted via restore=%s match=%s\n", dec2.c_str(), dec2 == plain ? "YES" : "NO");
    return 0;
}
