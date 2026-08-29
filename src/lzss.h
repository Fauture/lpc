#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lpc {

// Decompresses the LZSS format used by magic_Linux32's .pak files.
// Returns false if the output size does not match expectedSize.
bool lzssDecompress(const std::vector<uint8_t>& src, size_t expectedSize, std::vector<uint8_t>& out);

// Compresses data in the same LZSS format (the game's vm_f_lzss_compress uses
// this via lzss_compress(0x80cba3c)).  Returns the compressed bytes.
std::vector<uint8_t> lzssCompress(const std::vector<uint8_t>& src);

} // namespace lpc
