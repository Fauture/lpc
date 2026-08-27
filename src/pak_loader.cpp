#include "pak_loader.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "lzss.h"

namespace fs = std::filesystem;

namespace lpc {

namespace {

std::vector<uint8_t> readFileBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open pak: " + path);
    in.seekg(0, std::ios::end);
    std::streamoff len = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<size_t>(len));
    if (len > 0) in.read(reinterpret_cast<char*>(data.data()), len);
    return data;
}

uint32_t rd32(const std::vector<uint8_t>& d, size_t off) {
    return static_cast<uint32_t>(d[off]) |
           (static_cast<uint32_t>(d[off + 1]) << 8) |
           (static_cast<uint32_t>(d[off + 2]) << 16) |
           (static_cast<uint32_t>(d[off + 3]) << 24);
}

bool looksLikeNameAt(const std::vector<uint8_t>& data, size_t base) {
    size_t maxRead = base + 32;
    if (maxRead > data.size()) return false;
    bool any = false;
    for (size_t i = base; i < maxRead; ++i) {
        unsigned char c = data[i];
        if (c == 0) break;
        if (!std::isprint(c)) return false;
        any = true;
    }
    return any;
}

std::vector<PakEntry> parseTable(const std::vector<uint8_t>& data, size_t tableOff, bool rootFormat = false) {
    std::vector<PakEntry> entries;
    const size_t entrySize = 0x40;
    int consecutiveInvalid = 0;
    for (size_t off = tableOff; off + entrySize <= data.size(); off += entrySize) {
        uint32_t type, childOff, compSize, origSize;
        size_t nameBase;
        if (rootFormat) {
            // Root directory entries: name at +4, type at +0x30, child at +0x34.
            nameBase = off + 4;
            type = rd32(data, off + 0x30);
            childOff = rd32(data, off + 0x34);
            compSize = rd32(data, off + 0x38);
            origSize = rd32(data, off + 0x3C);
        } else {
            // Subdirectory/file entries: type at +0, child at +4, name at +0x14.
            nameBase = off + 0x14;
            type = rd32(data, off);
            childOff = rd32(data, off + 4);
            compSize = rd32(data, off + 8);
            origSize = rd32(data, off + 12);
        }
        bool valid = (type <= 1) && looksLikeNameAt(data, nameBase) && childOff < data.size();
        if (!valid && rootFormat) {
            // Some pak roots mix in file-format entries (e.g. start_aaa.o).
            type = rd32(data, off);
            childOff = rd32(data, off + 4);
            compSize = rd32(data, off + 8);
            origSize = rd32(data, off + 12);
            nameBase = off + 0x14;
            valid = (type <= 1) && looksLikeNameAt(data, nameBase) && childOff < data.size();
        }
        if (!valid) {
            if (++consecutiveInvalid >= 2) break;
            continue;
        }
        consecutiveInvalid = 0;
        PakEntry e;
        for (size_t i = nameBase; i < nameBase + 32 && i < data.size() && data[i]; ++i) e.name.push_back(static_cast<char>(data[i]));
        e.isDir = (type == 1);
        e.offset = childOff;
        e.compSize = compSize;
        e.origSize = origSize;
        if (e.isDir) {
            e.children = parseTable(data, childOff, false);
        }
        entries.push_back(std::move(e));
    }
    return entries;
}

void extractEntry(const std::vector<uint8_t>& pakData, const PakEntry& e, const fs::path& outDir) {
    fs::path outPath = outDir / e.name;
    if (e.isDir) {
        fs::create_directories(outPath);
        for (const auto& child : e.children) extractEntry(pakData, child, outPath);
        return;
    }
    if (e.offset + e.compSize > pakData.size()) throw std::runtime_error("pak entry out of range: " + e.name);
    std::vector<uint8_t> src(pakData.begin() + e.offset, pakData.begin() + e.offset + e.compSize);
    std::vector<uint8_t> out;
    if (!lzssDecompress(src, e.origSize, out)) {
        // Some entries may be stored uncompressed; fall back to raw copy.
        if (e.compSize == e.origSize) out = src;
        else throw std::runtime_error("failed to decompress pak entry: " + e.name);
    }
    fs::create_directories(outPath.parent_path());
    std::ofstream f(outPath, std::ios::binary);
    f.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
}

} // namespace

PakFile loadPak(const std::string& path) {
    std::vector<uint8_t> data = readFileBytes(path);
    if (data.size() < 0x190 || rd32(data, 0) != 0x11223344)
        throw std::runtime_error("bad pak magic");
    // Header is 16 bytes; root directory entries start at 0x10 in file format.
    PakFile pak;
    pak.root = parseTable(data, 0x10, false);
    return pak;
}

bool extractPak(const std::string& pakPath, const std::string& outputDir) {
    std::vector<uint8_t> data = readFileBytes(pakPath);
    if (data.size() < 0x190 || rd32(data, 0) != 0x11223344)
        throw std::runtime_error("bad pak magic");
    fs::create_directories(outputDir);
    auto entries = parseTable(data, 0x10, false);
    for (const auto& e : entries) extractEntry(data, e, outputDir);
    return true;
}

const PakEntry* findPakEntry(const PakFile& pak, const std::string& virtualPath) {
    const std::vector<PakEntry>* cur = &pak.root;
    const PakEntry* found = nullptr;
    size_t start = 0;
    while (start <= virtualPath.size()) {
        size_t slash = virtualPath.find('/', start);
        std::string part = virtualPath.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (part.empty()) break;
        const PakEntry* next = nullptr;
        for (const auto& e : *cur) {
            if (e.name == part) {
                next = &e;
                found = next;
                break;
            }
        }
        if (!next) return nullptr;
        if (slash == std::string::npos) return found;
        if (!next->isDir) return nullptr;
        cur = &next->children;
        start = slash + 1;
    }
    return found;
}

} // namespace lpc
