#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lpc {

struct PakEntry {
    std::string name;
    bool isDir = false;
    uint32_t offset = 0;
    uint32_t compSize = 0;
    uint32_t origSize = 0;
    std::vector<PakEntry> children;
};

struct PakFile {
    std::vector<PakEntry> root;
};

PakFile loadPak(const std::string& path);
bool extractPak(const std::string& pakPath, const std::string& outputDir);
const PakEntry* findPakEntry(const PakFile& pak, const std::string& virtualPath);

} // namespace lpc
