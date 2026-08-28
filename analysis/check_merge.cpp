// Verify inherit-chain merging for a template object: does process_command
// resolve, and via which chain member?
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "o_loader.h"
#include "pak_loader.h"
#include "value.h"

using namespace lpc;

static bool hasSuffix2(const std::string& s, const std::string& suf) {
    return s.size() >= suf.size() &&
           s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

// Minimal re-implementation of the merge walk to print what gets merged.
static void printMerged(const OriginalObject& root) {
    std::map<int, std::string> merged;  // merged index -> name
    std::map<int, std::string> mergedSrc;

    auto loadRel = [](const std::string& vpath) -> std::shared_ptr<OriginalObject> {
        std::string rel = vpath;
        if (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
        if (hasSuffix2(rel, ".c")) rel.replace(rel.size() - 2, 2, ".o");
        // try out_chk tree (mounted dir)
        std::string cand1 = "linux/aaa - bak/" + rel;
        std::string cand2 = "out_chk/" + rel;
        std::string cand3 = "linux/aaa/aaa/" + rel;
        for (const auto& c : {cand1, cand2, cand3}) {
            FILE* f = std::fopen(c.c_str(), "rb");
            if (f) { std::fclose(f); return std::make_shared<OriginalObject>(loadOriginalObject(c)); }
        }
        return nullptr;
    };

    std::map<std::string, std::shared_ptr<OriginalObject>> loaded;
    std::function<void(const OriginalObject&, int, int, const std::string&)> walk;
    walk = [&](const OriginalObject& parent, int accFun, int accVar, const std::string& chain) {
        for (const auto& inh : parent.inherits) {
            std::shared_ptr<OriginalObject> parentObj;
            auto lit = loaded.find(inh.path);
            if (lit != loaded.end()) parentObj = lit->second;
            else {
                parentObj = loadRel(inh.path);
                if (!parentObj) {
                    std::printf("  !! FAILED to load inherit %s (chain %s)\n",
                                inh.path.c_str(), chain.c_str());
                    continue;
                }
                loaded[inh.path] = parentObj;
            }
            int funOff = accFun + inh.baseFunOffset;
            int varOff = accVar + inh.baseVarOffset;
            std::string subChain = chain + " -> " + inh.path + "(funOff=" + std::to_string(funOff) + ")";
            walk(*parentObj, funOff, varOff, subChain);
            for (const auto& fn : parentObj->functions) {
                if (fn.index < 0) continue;
                int childIndex = funOff + fn.index;
                if (merged.find(childIndex) == merged.end()) {
                    merged[childIndex] = fn.name;
                    mergedSrc[childIndex] = inh.path + "#" + std::to_string(fn.index);
                }
            }
        }
    };
    walk(root, 0, 0, "ROOT");
    std::printf("  merged functions=%zu\n", merged.size());
    for (const auto& kv : merged) {
        const char* marker = kv.second == "process_command" ? "  <===" : "";
        std::printf("    idx=%4d name=%-32s src=%s%s\n", kv.first, kv.second.c_str(),
                    mergedSrc[kv.first].c_str(), marker);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: check_merge <file.o>\n"); return 1; }
    OriginalObject obj;
    try {
        obj = loadOriginalObject(argv[1]);
    } catch (const std::exception& e) {
        std::printf("ERROR loading %s: %s\n", argv[1], e.what());
        return 1;
    }
    std::printf("=== %s ===\n", argv[1]);
    std::printf("  own functions=%zu inherits=%zu\n", obj.functions.size(), obj.inherits.size());
    for (const auto& inh : obj.inherits)
        std::printf("    inherit path=%s baseFun=%d baseVar=%d\n",
                    inh.path.c_str(), inh.baseFunOffset, inh.baseVarOffset);
    printMerged(obj);
    return 0;
}
