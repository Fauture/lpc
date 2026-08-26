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

} // namespace lpc
