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

// Driver-style pak mount: the original driver treats any path containing
// ".pak/" as a packed volume (vm_isInPackedVolume) and lists directories
// through the pak's catalog in catalog order (vm_getDirInPackedFs).
// Mount a pak file at a virtual prefix, e.g. "/aaa/pack_data/lib_aaa32.pak".
void setPakMount(const std::string& pakRealPath, const std::string& virtualPrefix);
bool pakMounted();
std::string pakVirtualPrefix();

// Read a file below the mounted prefix:
// "/aaa/pack_data/lib_aaa32.pak/feature/common/daemon.o" -> entry bytes.
bool pakReadFile(const std::string& vpath, std::vector<uint8_t>& out);

// Directory listing below the mounted prefix, in CATALOG order (the order
// the original driver's get_dir returns for packed volumes).  `vdir` must
// not contain the pattern.  Returns false when the directory is unknown.
bool pakListDir(const std::string& vdir, std::vector<const PakEntry*>& out);

} // namespace lpc
