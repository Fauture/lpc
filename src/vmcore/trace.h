// ===========================================================================
// vmcore/trace.h — 语义级诊断 trace (全套默认开启, 排查期专用)
//
// 打点分两级:
//   * 指令级 (默认开): 每条执行步骤 — obj/fn/pc/op/栈深/栈顶值, 同一 pc 连续
//     循环 64 次后压制为每 4096 次报一次 (防死循环打爆磁盘)。
//   * 语义级 (默认开): 调用含实参/返回含值/比较/if 分支/map 读写删/存储失败/
//     队列收发/事件等待/构造步骤/各 miss, 数据全部打印。
// 控制环境变量: LPCVM_TRACE=0 全关; =1 只开语义级; 未设/=2 全开 (指令+语义)。
// 过滤(可选): LPCVM_TRACE_FN=fn1,fn2 / LPCVM_TRACE_OPS=7,42
// ===========================================================================
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>

namespace lpc {
namespace vmcore {

// 0=关(默认) 1=仅语义级 2=指令+语义。诊断一律显式 opt-in（LPCVM_TRACE=1/2），
// 默认输出只有原版语义的 mudlib printf 直出（Load:/Done of init/...）。
inline int traceLevel() {
    static const int lvl = [] {
        const char* e = std::getenv("LPCVM_TRACE");
        if (!e || !*e) return 0;
        return std::strcmp(e, "0") == 0 ? 0
             : std::strcmp(e, "1") == 0 ? 1 : 2;
    }();
    return lvl;
}

inline bool traceOn() { return traceLevel() >= 1; }
inline bool traceStepOn() { return traceLevel() >= 2; }

// fn: 当前函数名; 空 = 无函数上下文(如全局 init 段), 仅当未设过滤时输出
inline bool traceWant(const std::string& fn) {
    static const std::string filt = [] {
        const char* e = std::getenv("LPCVM_TRACE_FN");
        return e ? std::string(e) : std::string();
    }();
    if (filt.empty()) return true;
    if (fn.empty()) return false;
    size_t p = 0;
    while (p <= filt.size()) {
        size_t q = filt.find(',', p);
        if (q == std::string::npos) q = filt.size();
        if (q > p && filt.compare(p, q - p, fn) == 0) return true;
        p = q + 1;
    }
    return false;
}

// op: opcode 编号; -1 = 不按 op 过滤
inline bool traceWantOp(int op) {
    static const std::string filt = [] {
        const char* e = std::getenv("LPCVM_TRACE_OPS");
        return e ? std::string(e) : std::string();
    }();
    if (filt.empty() || op < 0) return true;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", op);
    size_t p = 0;
    while (p <= filt.size()) {
        size_t q = filt.find(',', p);
        if (q == std::string::npos) q = filt.size();
        if (q > p && filt.compare(p, q - p, buf) == 0) return true;
        p = q + 1;
    }
    return false;
}

} // namespace vmcore
} // namespace lpc
