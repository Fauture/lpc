#include "lzss.h"

#include <array>
#include <cstdint>

namespace lpc {

bool lzssDecompress(const std::vector<uint8_t>& src, size_t expectedSize, std::vector<uint8_t>& out) {
    std::array<uint8_t, 4096> buffer{};
    buffer.fill(32);

    size_t inPos = 0;
    out.clear();
    out.reserve(expectedSize);

    uint16_t flags = 0;
    int writePos = 4078; // 0xFEE

    auto readByte = [&]() -> int {
        if (inPos >= src.size()) return -1;
        return src[inPos++];
    };

    while (true) {
        flags >>= 1;
        if ((flags & 0x100) == 0) {
            int c = readByte();
            if (c < 0) break;
            flags = static_cast<uint16_t>(c | 0xFF00);
        }
        if ((flags & 1) == 0) {
            // Match: two-byte token, followed by back to flag stream.
            int lo = readByte();
            int hi = readByte();
            if (lo < 0 || hi < 0) break;
            uint16_t offset = static_cast<uint16_t>(((hi & 0xF0) << 4) | (lo & 0xFF));
            int len = (hi & 0x0F) + 3;
            for (int i = 0; i < len; ++i) {
                uint8_t b = buffer[(offset + i) & 0xFFF];
                if (out.size() < expectedSize) out.push_back(b);
                buffer[writePos] = b;
                writePos = (writePos + 1) & 0xFFF;
            }
            continue;
        }

        // Literal byte
        int c = readByte();
        if (c < 0) break;
        uint8_t b = static_cast<uint8_t>(c);
        if (out.size() < expectedSize) out.push_back(b);
        buffer[writePos] = b;
        writePos = (writePos + 1) & 0xFFF;
    }

    return out.size() == expectedSize;
}

std::vector<uint8_t> lzssCompress(const std::vector<uint8_t>& src) {
    // Classic LZSS (the driver's lzss_compress, 0x80cba3c): 4096-byte
    // circular window, 12-bit offset, 4-bit length (3..18).  Flag byte: bit
    // set = literal byte follows; bit clear = 2-byte match token
    // ((offset>>4)<<8 | ...).  This produces a stream the lzssDecompress above
    // (and the original decoder) can read back.
    std::vector<uint8_t> out;
    std::array<uint8_t, 4096> win{};
    win.fill(32);
    size_t winPos = 4078;   // 0xFEE
    size_t ip = 0;
    const size_t n = src.size();

    uint16_t flags = 0;
    int flagBits = 0;
    size_t flagPos = 0;

    auto putByte = [&](uint8_t b) { out.push_back(b); };
    auto putFlag = [&](bool literal) {
        if (flagBits == 8) { out[flagPos] = static_cast<uint8_t>(flags); flagBits = 0; }
        if (flagBits == 0) { flagPos = out.size(); out.push_back(0); flags = 0; }
        if (literal) flags |= static_cast<uint16_t>(1 << flagBits);
        ++flagBits;
    };

    auto findMatch = [&](size_t pos, size_t& off, size_t& len) -> bool {
        // Search back up to the window for the longest match (cap 18).
        size_t maxOff = (pos < 4096) ? pos : 4096;
        size_t bestLen = 2;   // need >= 3 to emit a match
        size_t bestOff = 0;
        size_t limit = n - pos;
        if (limit > 18) limit = 18;
        for (size_t d = 1; d <= maxOff && d <= 4096; ++d) {
            size_t cand = pos - d;
            size_t l = 0;
            while (l < limit && src[cand + l] == src[pos + l]) ++l;
            if (l > bestLen) { bestLen = l; bestOff = d; if (bestLen == limit) break; }
        }
        if (bestLen >= 3) { off = bestOff; len = bestLen; return true; }
        return false;
    };

    while (ip < n) {
        size_t off = 0, len = 0;
        if (findMatch(ip, off, len)) {
            putFlag(false);
            // Original layout (lzss_compress 0x80cba3c): match token is
            // lo byte first (window position low 8 bits), then hi byte
            // ((pos >> 8) << 4 | (len - 3)).  The 12-bit offset is the
            // ABSOLUTE 4096-window position, not a distance.
            uint16_t offAbs = static_cast<uint16_t>((winPos - off) & 0xFFF);
            uint16_t token = static_cast<uint16_t>(((offAbs >> 8) << 12) |
                                                   ((len - 3) << 8) | (offAbs & 0xFF));
            putByte(static_cast<uint8_t>(token & 0xFF));
            putByte(static_cast<uint8_t>(token >> 8));
            for (size_t i = 0; i < len; ++i) {
                win[winPos] = src[ip];
                winPos = (winPos + 1) & 0xFFF;
                ++ip;
            }
        } else {
            putFlag(true);
            putByte(src[ip]);
            win[winPos] = src[ip];
            winPos = (winPos + 1) & 0xFFF;
            ++ip;
        }
    }
    if (flagBits) out[flagPos] = static_cast<uint8_t>(flags);
    return out;
}

} // namespace lpc
