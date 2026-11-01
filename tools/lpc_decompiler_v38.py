#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""lpc_decompiler_v38.py —— LPC .o 反编译器（全新实现 v38）

参考: lpc_decompiler_v37.py（渲染经验+缺陷教训）、restore_vm_o_v2.py（.o 格式与指令语义）、
      tools/lndiff.py / tools/lineinfo.py（行号块 schema）、docs/ 方法论文档。

与 v37/v2 的关键差异（设计目标）:
  1. 行号信息: 解析 .o 内嵌行号块（lineinfo schema: 03 <slen> <路径\\0> <起始行> (02 <语句行>)* <trailer>），
     函数头注释源起始行；语句级按指令序线性估计标注 `/* L~N */`（--lines，默认开）。
  2. 括号/前后缀敏感渲染:
     - 前缀/后缀自增自减: 语句级按 nop 签名判定（律#4/#90: `x++;`=nop×2+ptr+inc+nop, `++x;`=ptr+inc+nop×3，
       `x--;`=nop×2+stmt+nop, `--x;`无前导 nop → 前导 nop>=2 判后缀）；
       表达式级按 to_value 模式判定（`push_ptr(X); to_value; ...; push_ptr(X); inc` = X++）。
     - 括号化下标: to_ptr 作用于 member/range 结果 → 基址加括号 `(m[i])["k"]`（tc17/律#24）。
  3. 验证独立于反编译: --cmp <ref.o> <cmp.o> 本地逐函数归一化指令流比对;
     编译验证(上传服务器单编/全树 bygs)不属本脚本, 用直接命令另行执行。
  4. push_0 → undefined（非 0）; push_real 整数值保留 .0; 实参右→左压栈顺序=源码顺序;
     数组字面量=压栈逆序; 默认参数值同时写入原型与定义（bygs 规则）。
  5. 随机取样: --random [N] 从 lib_gs32 随机取 .o 反编译留存到 lizi/ 查看还原程度。

用法:
  python lpc_decompiler_v38.py <file.o> [-o out.c] [--no-lines]
  python lpc_decompiler_v38.py <dir> [-k lizi]          # 目录批量（递归 *.o, 镜像结构）
  python lpc_decompiler_v38.py --disasm <file.o> [函数名]
  python lpc_decompiler_v38.py --random [N] [--seed S] [-k lizi]
  python lpc_decompiler_v38.py --cmp <ref.o> <cmp.o> [函数名]
  -g/--global-file <启动文件.o>   # 指定启动文件（全局程序）; 不指定则不加载全局表
"""
import argparse
import json
import os
import random
import re
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REF_ROOT = ROOT / "lib_gs32"
DEFAULT_OUT_DIR = ROOT / "lizi"

# ===========================================================================
# §0 常量: 类型表 / 操作码表 / efun 名表
# ===========================================================================
TYPE_MAP = {0: "void", 1: "void", 2: "int", 3: "float", 4: "string", 5: "buffer",
            6: "array", 7: "mapping", 8: "mixed", 9: "function", 10: "object",
            255: "mixed"}
NOP_OPS = (0, 29, 46)

OP_NAMES = {
    0: "nop", 1: "add", 2: "and", 3: "assign", 4: "assign_rev", 5: "call_ptr",
    6: "call_efun", 7: "call_user_fn", 8: "call_other", 9: "call_os", 10: "case_num",
    11: "case_str", 12: "default", 13: "cast", 14: "catch", 15: "checkargs",
    16: "checkreftype", 17: "checkreftype_i", 18: "for_cond", 19: "to_ptr",
    20: "to_value", 21: "arg_array", 22: "new_array", 23: "new_func", 24: "new_mapping",
    25: "div", 26: "end_catch", 27: "enter", 28: "eq", 29: "nop", 30: "ge",
    31: "goto", 32: "gt", 33: "if", 34: "inc", 35: "for_init", 36: "typeof",
    37: "le", 38: "lsh", 39: "lt", 40: "mod", 41: "mul", 42: "ne", 43: "neg",
    44: "try", 45: "not", 46: "nop", 47: "op_eq", 48: "or", 49: "pickup_os",
    50: "pop_n", 51: "pop", 52: "push_argnum", 53: "push_global", 54: "push_global_ptr",
    55: "push_local", 56: "push_local_i", 57: "push_local_ptr", 58: "push_local_ptr_i",
    59: "push_i", 60: "push_num", 61: "push_ovar", 62: "push_ovar_ptr", 63: "push_real",
    64: "push_s", 65: "push_this", 66: "push_0", 67: "member", 68: "member_ptr",
    69: "range", 70: "range_ptr", 71: "resort", 72: "ret_fake", 73: "ret",
    74: "rev", 75: "rsh", 76: "set_global", 77: "set_local", 78: "set_ovar",
    79: "sizeof", 80: "for_step", 81: "sub", 82: "reserve", 83: "unreserve",
    84: "trace", 85: "xor",
}
JUMP_OPS_PAR16 = (12, 18, 31, 33, 80)     # 跳转 offset 直接在 par16（有符号）
JUMP_OPS_META = (10, 11)                  # 跳转 offset 在 metadata
BINOPS = {1: "+", 2: "&", 25: "/", 28: "==", 30: ">=", 32: ">", 37: "<=",
          38: "<<", 39: "<", 40: "%", 41: "*", 42: "!=", 48: "|", 75: ">>",
          81: "-", 85: "^"}
OP_EQ_MAP = {2: "+", 3: "-", 4: "*", 5: "/", 6: "&", 7: "|", 8: "^", 9: ">>",
             10: "<<", 11: "%"}
IS_TYPE_EFUN = {0: "is_undefined", 1: "is_void", 2: "is_int", 3: "is_real",
                4: "is_string", 5: "is_buffer", 6: "is_array", 7: "is_mapping",
                9: "is_function", 10: "is_object"}
OS_FUN_MAP = {0: ("+", True), 1: ("-", True), 2: ("*", True), 3: ("/", True),
              4: ("%", True), 5: ("==", True), 6: ("!=", True), 7: ("<", True),
              8: (">", True), 9: ("<=", True), 10: (">=", True), 11: ("&&", True),
              12: ("||", True), 13: ("&", True), 14: ("|", True), 15: ("^", True),
              16: ("<<", True), 17: (">>", True), 18: ("!", False), 19: ("~", False),
              20: ("++", False), 21: ("--", False)}

_EFUN_NAMES = {}


def _load_efun_names():
    global _EFUN_NAMES
    if _EFUN_NAMES:
        return _EFUN_NAMES
    for p in (ROOT / "efun_names_full.json", Path("efun_names_full.json")):
        if p.exists():
            try:
                head = p.read_bytes()[:3]
                enc = "utf-8-sig" if head == b"\xef\xbb\xbf" else "gbk"
                data = json.loads(p.read_text(encoding=enc))
                names = data.get("names", data) if isinstance(data, dict) else data
                _EFUN_NAMES = {int(k): v for k, v in names.items()}
            except (OSError, ValueError, UnicodeError):
                pass
            break
    return _EFUN_NAMES


def efun_name(idx):
    return _load_efun_names().get(idx, "efun_%d" % idx)


# ===========================================================================
# §1 .o 文件解析（格式经 restore_vm_o_v2.py 全树 6266 文件验证）
# ===========================================================================
class ParseError(Exception):
    pass


DLT_FLAG_GLOBAL_VAR = 0x80000
DLT_FLAG_OBJECT_VAR = 0x40000
DLT_FLAG_FUNCTION = 0x8000000


class Reader:
    __slots__ = ("data", "pos")

    def __init__(self, data, pos=0):
        self.data, self.pos = data, pos

    def take(self, n):
        if n < 0 or self.pos + n > len(self.data):
            raise ParseError("truncated at 0x%X (need %d)" % (self.pos, n))
        out = self.data[self.pos:self.pos + n]
        self.pos += n
        return out

    def u8(self):
        return self.take(1)[0]

    def u16(self):
        return struct.unpack("<H", self.take(2))[0]

    def i16(self):
        return struct.unpack("<h", self.take(2))[0]

    def u32(self):
        return struct.unpack("<I", self.take(4))[0]

    def string(self):
        size = self.u32()
        if size == 0:
            return ""
        if size > len(self.data) - self.pos:
            raise ParseError("bad string len %d at 0x%X" % (size, self.pos - 4))
        raw = self.take(size)
        if raw.endswith(b"\0"):
            raw = raw[:-1]
        return raw.decode("gbk", "surrogateescape")   # 串池字节透传(全库 gbk 无损往返实证)


def _safe_name(value, fallback):
    value = value.strip().replace("\0", "")
    return value or fallback


def _parse_symbol(r):
    flags = r.u8()
    source = r.string() if flags & 1 else None
    line = r.u32() if flags & 2 else None
    return {"flags": flags, "source": source, "line": line}


def parse_function(r, origin):
    start = r.pos
    raw = r.take(64)
    token = struct.unpack_from("<I", raw, 4)[0]
    attrib, locals_no = struct.unpack_from("<hh", raw, 8)
    ret_attrib, ret_type, max_arg, min_arg = raw[12], raw[13], raw[14], raw[15]
    name = r.string()
    if len(name) > 512 or any(ord(c) < 32 for c in name):
        raise ParseError("implausible function name at 0x%X" % (start + 64))
    arg_count = r.i16()
    if not 0 <= arg_count <= 4096:
        raise ParseError("implausible arg count %d" % arg_count)
    args = []
    for i in range(arg_count):
        rec = r.take(20)
        args.append((_safe_name(r.string(), "arg%d" % i), rec[5], rec[4] != 0, rec[10]))
    local_count = r.i16()
    if not 0 <= local_count <= 4096:
        raise ParseError("implausible local count %d" % local_count)
    locals_ = []
    for i in range(local_count):
        rec = r.take(20)
        locals_.append((_safe_name(r.string(), "local%d" % i), rec[5]))
    symbol_count = r.u32()
    if symbol_count > 100000:
        raise ParseError("implausible symbol count %d" % symbol_count)
    symbols = [_parse_symbol(r) for _ in range(symbol_count)]
    node_count = r.u32()
    if node_count > 1000000:
        raise ParseError("implausible node count %d" % node_count)
    if not name and node_count == 0 and arg_count == 0 and local_count == 0:
        raise ParseError("empty function at 0x%X" % start)
    nodes = [(r.u16(), r.u8(), r.u8()) for _ in range(node_count)]
    meta_len = r.u32()
    if meta_len > len(r.data) - r.pos:
        raise ParseError("invalid meta len %d" % meta_len)
    meta = r.take(meta_len)
    runtime_count = r.u32()
    if runtime_count > 1000000:
        raise ParseError("implausible runtime count %d" % runtime_count)
    r.take(runtime_count * 4)
    return {"offset": start, "end": r.pos, "origin": origin, "token": token,
            "attrib": attrib,
            "local_no": locals_no, "ret_attrib": ret_attrib, "ret_type": ret_type,
            "max_arg": max_arg, "min_arg": min_arg,
            "name": _safe_name(name, "func_%X" % start),
            "args": args, "locals": locals_, "symbols": symbols,
            "nodes": nodes, "meta": meta}


def _try_function(r, origin):
    saved = r.pos
    try:
        return parse_function(r, origin)
    except (ParseError, struct.error, UnicodeError) as exc:
        failed_at = r.pos
        raw = r.data[saved:failed_at] if failed_at > saved else b""
        r.pos = saved
        return {"_failed": True, "offset": saved, "end_offset": failed_at,
                "origin": origin, "raw_bytes": raw, "error": str(exc)}


def _parse_global_code(r):
    start = r.pos
    symbol_count = r.u32()
    if symbol_count > 100000:
        raise ParseError("implausible global symbol count %d" % symbol_count)
    symbols = [_parse_symbol(r) for _ in range(symbol_count)]
    node_count = r.u32()
    if node_count > 1000000:
        raise ParseError("implausible global node count %d" % node_count)
    nodes = [(r.u16(), r.u8(), r.u8()) for _ in range(node_count)] if node_count else []
    meta_len = r.u32()
    if meta_len > len(r.data) - r.pos:
        raise ParseError("invalid global meta len %d" % meta_len)
    meta = r.take(meta_len) if meta_len else b""
    runtime_count = r.u32()
    if runtime_count > 1000000:
        raise ParseError("implausible global runtime count %d" % runtime_count)
    r.take(runtime_count * 4)
    return {"offset": start, "end": r.pos, "origin": "global", "token": 0, "attrib": 0,
            "local_no": 0, "ret_attrib": 0, "ret_type": 0, "max_arg": 0, "min_arg": 0,
            "name": "__global_code__", "args": [], "locals": [], "symbols": symbols,
            "nodes": nodes, "meta": meta}


def parse_file(path):
    """解析 .o → dict。字段与 restore_vm_o_v2 兼容（经全树验证的格式）。"""
    path = Path(path)
    r = Reader(path.read_bytes())
    version, _reserved, function_bc, define_count = r.u32(), r.u32(), r.u32(), r.u32()
    defines = [(r.u32(), r.string(), r.string()) for _ in range(define_count)]
    program_strings = [r.string() for _ in range(r.u32())]
    name_count, names_blob_len = r.u32(), r.u32()
    names_blob_start = r.pos
    names_blob = r.take(names_blob_len)
    names = [x.decode("gbk", "surrogateescape") for x in names_blob.split(b"\0") if x]
    max_variable_no, prog_var_count = r.i16(), r.i16()
    prog_var_decls = []
    prog_vars_start = r.pos
    is_global_program = False
    try:
        for index in range(prog_var_count):
            rec = r.take(16)
            vtype = rec[8]
            vidx = struct.unpack_from("<H", rec, 10)[0]
            vname = _safe_name(r.string(), "object_var%d" % index)
            prog_var_decls.append((vname, vtype, vidx))
    except ParseError:
        r.pos = prog_vars_start
        is_global_program = True
    efun_count, max_function_no = r.i16(), r.i16()
    hidden_count = r.u32()
    functions = []
    for _ in range(hidden_count):
        functions.append(_try_function(r, "hidden"))
    inherits = []
    max_constitution_no = 0
    if not is_global_program:
        inherits_start = r.pos
        try:
            max_constitution_no, inherit_count = r.i16(), r.u32()
            if inherit_count > 10000:
                raise ParseError("implausible inherit_count %d" % inherit_count)
            for _ in range(inherit_count):
                inherits.append((r.string(), tuple(r.i16() for _ in range(6))))
        except ParseError:
            r.pos = inherits_start
            is_global_program = True
    dlt_count = r.u32()
    dlt_offsets, failures, unparsed_dlt = [], [], 0
    dlt_ovar_entries, dlt_gvar_entries = [], []
    func_index_map = {}
    for _ in range(dlt_count):
        dlt_offsets.append(r.pos)
        entry = r.take(20)
        flags = struct.unpack_from("<I", entry, 4)[0]
        if flags & DLT_FLAG_FUNCTION:
            func_idx = struct.unpack_from("<I", entry, 16)[0] & 0x7FFF
            fun = _try_function(r, "dlt")
            if fun.get("_failed"):
                unparsed_dlt += 1
                failures.append({"kind": "dlt_function", "dlt_offset": dlt_offsets[-1],
                                 "func_offset": fun["offset"], "end_offset": fun["end_offset"],
                                 "raw_bytes": fun["raw_bytes"], "error": fun["error"]})
            else:
                functions.append(fun)
                func_index_map[func_idx] = fun["name"]
        elif flags & (DLT_FLAG_OBJECT_VAR | DLT_FLAG_GLOBAL_VAR):
            name_offset = struct.unpack_from("<I", entry, 0)[0]
            var_offset = struct.unpack_from("<I", entry, 12)[0]
            if 0 <= name_offset < len(names_blob):
                end = names_blob.find(b"\0", name_offset)
                if end < 0:
                    end = len(names_blob)
                var_name = names_blob[name_offset:end].decode("gbk", "surrogateescape")
            else:
                var_name = "<bad %d>" % name_offset
            if flags & DLT_FLAG_OBJECT_VAR:
                dlt_ovar_entries.append((var_name, var_offset))
            if flags & DLT_FLAG_GLOBAL_VAR:
                dlt_gvar_entries.append((var_name, var_offset))
        else:
            unparsed_dlt += 1
            failures.append({"kind": "unknown_dlt", "dlt_offset": dlt_offsets[-1],
                             "raw_bytes": entry, "flags": flags})
    # object_vars: runtime index 顺序（DLT OVAR 权威 / 反转 PROG_VARS 兜底）
    object_vars, object_var_types = [], []
    if prog_var_decls:
        n = len(prog_var_decls)
        rt_type_map = {vidx: vtype for _n, vtype, vidx in prog_var_decls}
        if dlt_ovar_entries and len(dlt_ovar_entries) == n:
            runtime_names = [None] * n
            base_var_offset = max_variable_no - n
            for nm, off in dlt_ovar_entries:
                li = off - base_var_offset
                if 0 <= li < n:
                    runtime_names[li] = nm
            if all(x is not None for x in runtime_names):
                object_vars = runtime_names
                object_var_types = [rt_type_map.get(i, 255) for i in range(n)]
        if not object_vars:
            decl = list(reversed(prog_var_decls))
            object_vars = [d[0] for d in decl]
            object_var_types = [d[1] for d in decl]
    global_code = None
    if r.pos < len(r.data):
        gc_start = r.pos
        try:
            global_code = _parse_global_code(r)
        except (ParseError, struct.error, UnicodeError) as exc:
            failures.append({"kind": "global_code", "offset": gc_start,
                             "end_offset": r.pos, "raw_bytes": r.data[gc_start:],
                             "error": str(exc)})
            global_code = None
    if global_code is not None and global_code["nodes"]:
        functions.append(global_code)
    return {"path": path, "version": version, "function_bc": function_bc,
            "defines": defines, "program_strings": program_strings,
            "name_count": name_count, "names": names,
            "max_variable_no": max_variable_no, "object_vars": object_vars,
            "object_var_types": object_var_types,
            "efun_count": efun_count, "max_function_no": max_function_no,
            "inherits": inherits, "dlt_count": dlt_count,
            "unparsed_dlt": unparsed_dlt,
            "functions": [f for f in functions if not f.get("_failed")],
            "dlt_gvar_entries": dlt_gvar_entries, "failures": failures,
            "func_index_map": func_index_map,
            "is_global_program": is_global_program,
            "raw_data": r.data, "end": r.pos, "size": len(r.data)}


# ===========================================================================
# §2 全局程序映射 + 继承链函数名解析
# ===========================================================================
GLOBAL_VAR_MAP = {}
GLOBAL_FUNC_MAP = {}
GLOBAL_EFUN_COUNT = 0
GLOBAL_FUNCTION_BC = 0
_PARENT_PROG_CACHE = {}
GLOBAL_FILE_ARG = None   # CLI -g/--global-file 指定的启动文件（对齐 v37 -q/--sefun、v2 -g/--global-file）


def init_global_maps(global_file=None, force=False):
    """从全局程序（启动文件）构建全局变量/函数名映射（action 53/54/76/7bit7/23bit7 用）。

    启动文件只来自显式指定（global_file 参数 > CLI -g，对齐 v37 -q / v2 -g）。
    不指定 = 不加载全局表（无硬编码默认、无扫描推断）——
    未加载时全局名回落 global_%d / gvar_%d 渲染。force=True 清空重建。
    """
    global GLOBAL_VAR_MAP, GLOBAL_FUNC_MAP, GLOBAL_EFUN_COUNT, GLOBAL_FUNCTION_BC
    if force:
        GLOBAL_VAR_MAP.clear()
        GLOBAL_FUNC_MAP.clear()
        GLOBAL_EFUN_COUNT = 0
        GLOBAL_FUNCTION_BC = 0
    if GLOBAL_VAR_MAP or GLOBAL_FUNC_MAP:
        return
    src = global_file or GLOBAL_FILE_ARG
    if not src:
        return
    gp = Path(src)
    if not gp.exists():
        print("warn: global program not found: %s" % gp, file=sys.stderr)
        return
    try:
        info = parse_file(gp)
    except (ParseError, OSError, struct.error, UnicodeError) as exc:
        print("warn: global program parse failed: %s" % exc, file=sys.stderr)
        return
    if not info.get("is_global_program"):
        print("warn: %s is not a global program" % gp, file=sys.stderr)
        return
    for var_name, var_offset in info.get("dlt_gvar_entries", []):
        GLOBAL_VAR_MAP[var_offset] = var_name
    GLOBAL_EFUN_COUNT = info["efun_count"]
    GLOBAL_FUNCTION_BC = info["function_bc"]
    for func_idx, func_name in info.get("func_index_map", {}).items():
        if func_idx >= GLOBAL_FUNCTION_BC:
            GLOBAL_FUNC_MAP[func_idx] = func_name
    print("global program: %s (GVAR=%d, global_funcs=%d, efun=%d, funcBC=%d)" % (
        gp.name, len(GLOBAL_VAR_MAP), len(GLOBAL_FUNC_MAP),
        GLOBAL_EFUN_COUNT, GLOBAL_FUNCTION_BC), file=sys.stderr)


def _resolve_inherit_o(inherit_name, source_path):
    """继承路径('/a/b.c') → 本地 .o 路径。优先 REF 根相对，其次自 source 向上逐级。"""
    rel = inherit_name.lstrip("/")
    cand = REF_ROOT / (rel[:-2] + ".o" if rel.endswith(".c") else rel)
    if cand.exists():
        return cand
    p = Path(source_path).resolve()
    for _ in range(8):
        cand = p.parent / (rel[:-2] + ".o" if rel.endswith(".c") else rel)
        if cand.exists():
            return cand
        if p.parent == p:
            break
        p = p.parent
    return None


def _load_parent_prog(inherit_name, source_path):
    """加载父 .o 解析结果（带缓存）。

    缓存键 = 解析后的 .o 路径（父 .o 内容与谁继承它无关）——
    旧键 (inherit_name, source_path) 会让同一父文件被每个子文件重复解析
    （如 /feature/char.c 被全库上千文件继承），批量 6270 文件时是主要热点。
    """
    p = _resolve_inherit_o(inherit_name, source_path)
    key = str(p) if p is not None else "missing:" + inherit_name
    if key in _PARENT_PROG_CACHE:
        return _PARENT_PROG_CACHE[key]
    prog = None
    if p is not None:
        try:
            prog = parse_file(p)
        except (ParseError, struct.error, UnicodeError):
            prog = None
    # 有界缓存: 批量全树时防止内存膨胀（逐出最旧; 批量按序处理目录局部性好）
    if len(_PARENT_PROG_CACHE) >= 512:
        _PARENT_PROG_CACHE.pop(next(iter(_PARENT_PROG_CACHE)))
    _PARENT_PROG_CACHE[key] = prog
    return prog


def resolve_inherited_func_name(prog, par16, visited=None):
    """递归解析 call_user_fn/new_func 的 par16 → 函数名（本地表 → 继承链）。"""
    if visited is None:
        visited = set()
    source_path = prog.get("path")
    if source_path is None:
        return None
    key = str(source_path)
    if key in visited:
        return None
    visited.add(key)
    fim = prog.get("func_index_map", {})
    if par16 in fim:
        return fim[par16]
    for inh_name, inh_data in prog.get("inherits", []):
        if len(inh_data) < 6:
            continue
        base_fun, exp_fun = inh_data[1], inh_data[4]
        if base_fun <= par16 < base_fun + exp_fun:
            parent_prog = _load_parent_prog(inh_name, source_path)
            if parent_prog is not None:
                nm = resolve_inherited_func_name(parent_prog, par16 - base_fun, visited)
                if nm is not None:
                    return nm
            return None
    return None


def resolve_global_func_name(par16):
    """par16 (bit7=1) → 全局函数名（efun/simul_efun 区间走 efun 表）。"""
    if par16 < GLOBAL_FUNCTION_BC and par16 in _load_efun_names():
        return _load_efun_names()[par16]
    return GLOBAL_FUNC_MAP.get(par16)


# ===========================================================================
# §3 行号信息（lineinfo schema，探针实证）
# ===========================================================================
def line_blocks(path):
    """扫描 .o 内嵌行号块: [(pos, fname, start_line, [stmt_lines], [trailer])]。

    schema: 03 <u32 slen> <源文件路径\\0> <u32 起始行> (02 <u32 语句行>)* <u32 trailer=nc>
    """
    d = Path(path).read_bytes()
    out = []
    for m in re.finditer(rb"\x03", d):
        pos = m.start()
        if pos + 5 > len(d):
            continue
        slen = struct.unpack_from("<I", d, pos + 1)[0]
        if not (4 <= slen <= 200) or pos + 5 + slen > len(d):
            continue
        raw = d[pos + 5:pos + 5 + slen]
        if not raw.endswith(b"\x00"):
            continue
        name = raw[:-1]
        if b"/" not in name or not name.endswith(b".c"):
            continue
        try:
            name.decode("gbk")
        except Exception:
            continue
        i = pos + 5 + slen
        if i + 4 > len(d):
            continue
        start_line = struct.unpack_from("<I", d, i)[0]
        if not (0 < start_line < 100000):
            continue
        i += 4
        lines = []
        while i + 5 <= len(d) and d[i] == 0x02:
            v = struct.unpack_from("<I", d, i + 1)[0]
            if not (0 < v < 100000):
                break
            lines.append(v)
            i += 5
        out.append((pos, name.decode("gbk", "surrogateescape"), start_line, lines))
    return out


def attach_line_info(info):
    """行号信息直接取自函数记录的 symbols 区（探针实证 schema）:
    首符号 flags=3 (bit0=file, bit1=line) = 源文件 + 函数起始行;
    其后 flags=2 符号 = 各语句源行（含循环回跳形态），计数 = nstmts。

    多文件混合（#include 展开编译, 如 start_gs.o 混 40 文件）:
    __global_code__ 的 symbols 区含多个 flags=3 条目, 每个 = 一个源文件
    全局代码的起始（= include 展开序的直接内嵌证据, 主文件天然殿后,
    start_gs.o 实证: /xdbc/xdbc_comm.c → ... → /gs/start_gs.c:131）。
    ① info["src_file_order"]: 该序列 → 函数排序的文件维（律#99）;
    ② stmt_segs: stmt_lines 按 flags=3 边界分段 —— 行号组归并/前瞻
       禁止跨段, 否则跨文件行号跳变被误判为组内回落, 单条语句吞掉
       整张行号表（卡 L4 实证: _xdbc_output_file=([]) 从 33 前瞻吞到
       const.c:4, min(vals)=4 之后游标永卡 4）。
    """
    nblks = 0
    seq = []
    for f in info["functions"]:
        if f["name"] == "__global_code__":
            for s in (f.get("symbols") or []):
                if s.get("flags") == 3 and s.get("source") and s["source"] not in seq:
                    seq.append(s["source"])
    info["src_file_order"] = {sf: k for k, sf in enumerate(seq)}
    for f in info["functions"]:
        syms = f.get("symbols") or []
        if not syms:
            continue
        first = syms[0]
        if first.get("source") and first.get("line"):
            f["src_file"] = first["source"]
            f["src_line"] = first["line"]
            segs, cur_file, cur_lines = [], first["source"], []
            for s in syms[1:]:
                if not s.get("line"):
                    continue
                if s.get("flags") == 3 and s.get("source") and s["source"] != cur_file:
                    segs.append((cur_file, cur_lines))
                    cur_file, cur_lines = s["source"], []
                cur_lines.append(s["line"])
            segs.append((cur_file, cur_lines))
            f["stmt_lines"] = [ln for _fl, ls in segs for ln in ls]
            if len(segs) > 1:
                f["stmt_segs"] = segs
            f["src_file_order"] = info["src_file_order"].get(first["source"])
            nblks += 1
    return nblks


# ===========================================================================
# §4 反汇编（--disasm 与差异定位共用）
# ===========================================================================
def fmt_insn(pc, par16, par8, op, fun, info):
    """单条指令 → 可读文本（名字尽量解析）。"""
    signed = par16 if par16 < 0x8000 else par16 - 0x10000
    tgt = pc + 1 + signed

    def vn(idx):
        if idx >= 0x8000:
            idx -= 0x10000
        return local_name(idx, fun)

    n = OP_NAMES.get(op, "op%d" % op)
    if op in BINOPS:
        return n
    if op == 3:
        return "assign"
    if op == 5:
        return "%s(nargs=%d)" % (n, par8)
    if op == 6:
        return "%s(%s, nargs=%d)" % (n, efun_name(par16), par8 & 0x3F)
    if op == 7:
        who = resolve_global_func_name(par16) if (par8 & 0x80) else \
            resolve_inherited_func_name(info, par16)
        return "%s(%s, nargs=%d)" % (n, who or ("#%d" % par16), par8 & 0x3F)
    if op == 8:
        return "%s(flags=%d, nargs=%d)" % (n, par16, par8)
    if op == 9:
        return "%s(#%d, nargs=%d)" % (n, par16, par8 & 0x3F)
    if op == 10:
        off = par16 | (par8 << 16)
        meta = fun.get("meta", b"")
        val = struct.unpack_from("<i", meta, off + 4)[0] if off + 8 <= len(meta) else 0
        return "case_num(%d)" % val
    if op == 11:
        off = par16 | (par8 << 16)
        meta = fun.get("meta", b"")
        idx = struct.unpack_from("<I", meta, off + 4)[0] if off + 8 <= len(meta) else -1
        s = info["program_strings"][idx] if 0 <= idx < len(info["program_strings"]) else "?"
        return 'case_str("%s")' % s[:40]
    if op == 12:
        return "default(->%d)" % tgt
    if op == 13:
        return "cast(%s)" % TYPE_MAP.get(par16, str(par16))
    if op in (18, 80):
        return "%s(->%d)" % (n, tgt)
    if op == 31:
        return "goto %d" % tgt
    if op == 33:
        jtype = "jz" if (par8 & 1) else "jnz"
        pop = ",pop" if (par8 & 2) else ""
        return "if_%s%s -> %d" % (jtype, pop, tgt)
    if op == 34:
        return "inc(%d)" % signed
    if op == 35:
        return "for_init(var=%s)" % vn(signed)
    if op == 36:
        return IS_TYPE_EFUN.get(par16, "typeof(T%d)" % par16)
    if op == 44:
        return "try(catch->%d)" % tgt
    if op == 47:
        return "op_eq(%s)" % OP_EQ_MAP.get(par16, str(par16))
    if op == 49:
        return "pickup_os(%d,%d)" % (par16, par8)
    if op == 50:
        return "pop_n(%d)" % par16
    if op == 53 or op == 54:
        return "%s(%s)" % (n, GLOBAL_VAR_MAP.get(signed, "g%d" % signed))
    if op in (55, 56, 57, 58, 77):
        return "%s(%s)" % (n, vn(par16))
    if op == 59:
        return "push_i(%d)" % signed
    if op == 60:
        off = par16 | (par8 << 16)
        meta = fun.get("meta", b"")
        v = struct.unpack_from("<i", meta, off)[0] if off + 4 <= len(meta) else 0
        return "push_num(%d)" % v
    if op in (61, 62, 78):
        return "%s(%s)" % (n, object_var_name(signed, info))
    if op == 63:
        off = par16 | (par8 << 16)
        meta = fun.get("meta", b"")
        if off + 4 <= len(meta):
            v = struct.unpack("<f", struct.pack("<i", struct.unpack_from("<i", meta, off)[0]))[0]
            return "push_real(%s)" % fmt_float(v)
        return "push_real(?)"
    if op == 64:
        strs = info["program_strings"]
        s = strs[par16] if 0 <= par16 < len(strs) else "?"
        s = s.replace("\\", "\\\\").replace("\n", "\\n").replace('"', '\\"')[:40]
        return 'push_s("%s")' % s
    if op == 22:
        return "new_array(%d)" % par16
    if op == 23:
        who = resolve_global_func_name(par16) if (par8 & 0x80) else \
            resolve_inherited_func_name(info, par16)
        return "new_func(%s, cap=%d)" % (who or ("#%d" % par16), par8 & 0x3F)
    if op == 24:
        return "new_mapping(%d)" % par16
    if op in (67, 68):
        return "%s(#%d)" % (n, par16)
    if op in (69, 70):
        return "%s(flag=%d)" % (n, par16)
    return "%s(%d,%d)" % (n, par16, par8)


# ===========================================================================
# §5 表达式/语句生成器（带 provenance 的栈机）
# ===========================================================================
class V:
    """求值栈上的值。t=文本; ptr=是地址; var_id=底层变量标识(后缀检测);
    fresh=由 to_value 产生(postfix 候选); sub=由 member/range 产生;
    bracket=作为下标基址时需加括号; pn=所在语句前导 nop 数(前后缀判定)。"""
    __slots__ = ("t", "ptr", "var_id", "fresh", "sub", "bracket", "pn", "pc0", "caught")

    def __init__(self, t, ptr=False, var_id=None, fresh=False, sub=False,
                 bracket=False, pn=None, pc0=None, caught=False):
        self.t, self.ptr, self.var_id = t, ptr, var_id
        self.fresh, self.sub, self.bracket, self.pn = fresh, sub, bracket, pn
        self.pc0, self.caught = pc0, caught

    def base(self):
        """作为下标/成员基址的文本（bracket → 加括号）。"""
        return "(%s)" % self.t if (self.bracket and self.sub) else self.t


def local_name(index, fun):
    """VM 本地变量索引 → 名。>=0: 先 args 后 locals；<0: locals 尾部反向。"""
    args, locals_ = fun["args"], fun["locals"]
    n_args, n_locals = len(args), len(locals_)
    if index >= 0:
        if index < n_args:
            return args[index][0]
        li = index - n_args
        if li < n_locals:
            return locals_[li][0]
        return "local_%d" % index
    ri = -index - 1
    li = n_locals - 1 - ri
    if 0 <= li < n_locals:
        return locals_[li][0]
    ai = n_args - 1 - ri
    if 0 <= ai < n_args:
        return args[ai][0]
    return "var_%d" % index


def object_var_name(index, info):
    base_var_offset = info["max_variable_no"] - len(info["object_vars"])
    li = index - base_var_offset
    if 0 <= li < len(info["object_vars"]):
        return info["object_vars"][li]
    if 0 <= index < base_var_offset:
        return "inherited_var_%d" % index
    return "object_var_%d" % index


def fmt_float(v):
    """浮点渲染: 最短 float32 往返表示(字节码存原始 4 字节位型) + 必带 ≥1 位小数(用户规则)。"""
    if v != v:
        return "nan"
    if v == float("inf"):
        return "inf"
    if v == float("-inf"):
        return "-inf"
    best = None
    for prec in range(1, 10):
        s = "%.*g" % (prec, v)
        try:
            if struct.pack("<f", float(s)) == struct.pack("<f", v):
                best = s
                break
        except (OverflowError, ValueError):
            continue
    if best is None:
        best = repr(v)
    # 强制至少 1 位小数: "123456"→"123456.0"; "1e+30"→"1.0e+30"
    if "e" in best:
        mant, exp = best.split("e", 1)
        if "." not in mant:
            mant += ".0"
        best = mant + "e" + exp
    elif "." not in best:
        best += ".0"
    return best


def quote(value):
    # 转义对齐 v37（lib_gs32_c 实证）: \ " \n \r 均转义, 字符串内不保留裸换行
    # （源码 "\n" 编译成池内真实 0x0A 字节, 反编译须还原为 \n 转义）。
    return '"' + (value.replace("\\", "\\\\").replace("\n", "\\n")
                  .replace("\r", "\\r").replace('"', '\\"')
                  .replace("\x00", "").replace("\t", "\\t")) + '"'


def _pop(stack):
    return stack.pop() if stack else V("0")


def _pop_args(stack, count):
    # 实参右→左压栈 ⇒ 弹出顺序即源码顺序
    return [_pop(stack) for _ in range(count)]


def _arg_text(a):
    """call 实参文本: ptr 型局部/全局变量 → 引用传参 &x
    （taskd.load_task_alias 实证, ground truth: create_data("...", &form_index, &m)）。"""
    if a.ptr and a.var_id and not a.t.startswith("&"):
        return "&" + a.t
    return a.t


def _find_top_level_binop(expr):
    """(X op Y) 最外层运算符（跳过字符串字面量）。"""
    if not (expr.startswith("(") and expr.endswith(")")):
        return None
    inner = expr[1:-1]
    depth = in_string = escape = 0
    i = 0
    ops_long = ["==", "!=", "<<", ">>", "<=", ">=", "&&", "||"]
    ops_short = ["+", "-", "*", "/", "%", "&", "|", "^", "<", ">"]
    while i < len(inner):
        c = inner[i]
        if escape:
            escape = False
        elif c == "\\":
            escape = True
        elif c == '"':
            in_string = not in_string
        elif in_string:
            pass
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif depth == 0 and c == " ":
            matched = None
            for op in ops_long + ops_short:
                end = i + 1 + len(op)
                if inner[i + 1:end] == op and end < len(inner) and inner[end] == " ":
                    matched = op
                    break
            if matched is not None:
                return matched
        i += 1
    return None


def _has_top_level_assign(s):
    depth = i = 0
    n = len(s)
    while i < n:
        c = s[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == '"':
            i += 1
            while i < n:
                if s[i] == "\\":
                    i += 2
                    continue
                if s[i] == '"':
                    break
                i += 1
        elif c == "=" and depth == 0:
            prev = s[i - 1] if i > 0 else ""
            nxt = s[i + 1] if i + 1 < n else ""
            if prev not in "=!<>+-*/%&|^" and nxt != "=":
                return True
        i += 1
    return False


def _wrap_assign(v):
    """赋值表达式作操作数时加括号（LPC 优先级）。"""
    return "(%s)" % v if _has_top_level_assign(v) else v


def _negate_cond(cond):
    cond = cond.strip()
    if cond.startswith("!(") and cond.endswith(")"):
        return cond[2:-1]
    for op, neg in [("==", "!="), ("!=", "=="), ("<=", ">"), (">=", "<"),
                    ("<", ">="), (">", "<=")]:
        depth = in_string = 0
        str_ch = None
        i = 0
        while i < len(cond):
            c = cond[i]
            if in_string:
                if c == str_ch and cond[i - 1] != "\\":
                    in_string = False
                i += 1
                continue
            if c in "\"'":
                in_string, str_ch = True, c
                i += 1
                continue
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            elif depth == 0 and cond[i:i + len(op)] == op:
                if op == ">" and i > 0 and cond[i - 1] == "-":
                    i += 1
                    continue
                return "%s %s %s" % (cond[:i].rstrip(), neg, cond[i + len(op):].lstrip())
            i += 1
        break
    return "!(%s)" % cond


class FnCtx:
    def __init__(self, fun, info, with_lines=False):
        self.fun, self.info = fun, info
        self.stack = []          # list[V]
        self.lines = []          # list[(pc, text)]
        self.targets = set()
        self.pc = 0
        self.signed = 0
        self.pre_nops = 0
        self.next_action = -1
        self.last_assigned_value = ""
        self.override_names = {f["name"] for f in info.get("functions", [])
                                if f.get("attrib", 0) & 8}
        self.pending_sc = []     # [(left, op, target, popped)]
        self.pending_ternary = None
        self.suppress_pops = 0   # 融合后缀时吞掉后续 pop
        self.skip_pcs = set()    # 融合后缀时跳过的 push_ptr/inc 指令
        self.tc_fuse = None      # 律#111 三元+catch 逗号融合状态:
                                 # {cond, else_pc, end_pc, catch_body, a, in_catch}
        self.catch_expr = None   # 律#112 catch 值短路表达式状态: {if_pc, body}
        self.stmt_lines = fun.get("stmt_lines") if with_lines else None
        self.li = 0               # stmt_lines 顺序消费指针（精确行号模型）
        self.line_cursor = fun.get("src_line")  # 行游标（耗尽时沿用——同行多语句）
        # 多文件混合（律#99）: stmt_lines 按 flags=3 边界分段, 每段=一个源文件。
        # 行号消费（组归并/前瞻/尖峰扫描）禁止跨段 —— 跨文件行号跳变不是组内回落。
        segs = fun.get("stmt_segs")
        if segs and self.stmt_lines:
            self.seg_bounds, self.seg_files = [], []
            n = 0
            for fl, ls in segs:
                n += len(ls)
                self.seg_bounds.append(n)     # 各段在 flat 表中的累计边界
                self.seg_files.append(fl)
            # 段标记 = 每段首条目（flags=3 行 = 该文件第一条全局语句行）。
            # 只允许 take_stmt_line 入口（跨段首语句）消费; take_line 系与
            # 组归并一律不得吞（否则张冠李戴: INI_FILE 三元吞 agents/init
            # 标记, client_agents/trim 连锁平移 —— start_gs 实证）。
            self.marker_idxs = set(self.seg_bounds[:-1])
        else:
            self.seg_bounds = self.seg_files = None
            self.marker_idxs = set()
        self._last_seg = -1
        self._pending_file = None             # 跨段待发射的 `// ---- from 文件 ----` 标记
        self.case_landings = {}   # 非链首 case 的目标 pc -> 条目数（label 处消费）
        self.saw_literal = False  # 语句 pc 区间内出现 create_mapping/array/function_value（字面量组前瞻用）
        self.literal_pairs = 0    # 字面量元素对数(new_mapping par16/2 + new_array par16) → 递增段上限
        self.nc = len(fun["nodes"])

    def _seg_end(self):
        """li 所在段的条目边界; 跨段时登记文件标记（发射为注释行）。"""
        if not self.seg_bounds:
            return len(self.stmt_lines) if self.stmt_lines else 0
        for k, b in enumerate(self.seg_bounds):
            if self.li < b:
                if k != self._last_seg:
                    self._last_seg = k
                    self._pending_file = self.seg_files[k]
                return b
        return len(self.stmt_lines)

    def emit(self, text, pc=None):
        pc = self.pc if pc is None else pc
        if self.lines and pc < self.lines[-1][0]:
            # 融合语句(如 return --x)的值起点 pc 早于已发射的标签 → 有序插入
            item = (pc, text)
            k = len(self.lines) - 1
            while k > 0 and self.lines[k - 1][0] > pc:
                k -= 1
            self.lines.insert(k, item)
        else:
            self.lines.append((pc, text))

    def take_line(self):
        """按序消费一条行号条目（探针实证的消费模型, 见 decompile_function 注释）。
        消费后同步行游标（耗尽沿用模型的游标源）。
        段标记保护: 标记只能被 take_stmt_line 入口消费（律#99）。"""
        if self.stmt_lines and self.li < len(self.stmt_lines):
            if self.li in self.marker_idxs:
                return None
            v = self.stmt_lines[self.li]
            self.li += 1
            self.line_cursor = v
            return v
        return None

    def take_lines(self, n):
        self.saw_literal = False
        self.literal_pairs = 0
        for _ in range(n):
            self.take_line()

    def take_ifgoto(self, pc=None):
        """if 语句符号组消费（律#101, 探针 p2-p5/q1-q7/r1-r3 全实证）:

        组 = [语句起始行, 条件折行行..., 各门落点标签行...]，渲染行 = 起始行。
        落点标签行的值与折行值天然重复（门 i 的落点 = 门 i+1 所在行 = 下一折行行）,
        相邻同值在 .o 侧已折叠 → 消费用值驱动规则（r3 三门三元组实证）:
          逐条消费; 下降到"起始之后已消费过的值" = 门落点区重复折行值 → 继续;
          下降到新值 = then 首条目 → 组结束。
        例: [313,317|315..] 单门→[313,317]; [23,24,25,24,25,27|26..] 三门→整组;
            [317,321|319..] 单行双门(patch1 折叠)→[317,321]。
        """
        if self.stmt_lines is None:
            return
        lines = self.stmt_lines
        seg_end = self._seg_end()
        self.saw_literal = False
        self.literal_pairs = 0
        if self.li >= len(lines) or self.li >= seg_end \
                or self.li in self.marker_idxs:
            return
        ln0 = lines[self.li]
        self.li += 1
        # 门数（链式短路门回扫）: 组长上限 1 + 2k（折行 ≤ k-1 + 门 k, 律#101）
        k = 1 + self._count_sc_gates(pc)
        cap = self.li + max(0, 2 * k - 1)
        consumed = []            # 起始之后的组内值（下降归属判断）
        prev = ln0
        max_seen = ln0
        while self.li < len(lines) and self.li < seg_end \
                and self.li < cap and self.li not in self.marker_idxs:
            v = lines[self.li]
            if v >= prev:
                if v > max_seen:
                    max_seen = v
                consumed.append(v)
                prev = v
                self.li += 1
                continue
            if v in consumed:
                # 门落点区重复折行值 → 继续与否决胜: 下降目标之后 (k-1) 条内
                # 出现超过下降前最大值的新值 = 门落点区仍在延续（r3 实证 24→25..27）;
                # 无 = then 首条目（p4#2 实证: then 行与折行值重合, 39 不再创新高）
                look = self.li + 1
                cont = False
                for j in range(look, min(look + max(0, k - 1), len(lines),
                                         seg_end)):
                    if lines[j] > max_seen:
                        cont = True
                        break
                if cont:
                    consumed.append(v)
                    prev = v
                    self.li += 1
                    continue
            break                # 新低值 = then 首条目 → 组结束
        self.line_cursor = ln0   # 游标 = 语句起始行

    def _count_sc_gates(self, pc):
        """回扫链式短路门数: op33 非弹栈(bit1=0) 且跳转目标链到 pc 的门。"""
        if pc is None:
            return 0
        nodes = self.fun["nodes"]
        n = 0
        k = pc
        for _ in range(16):
            found = None
            lo = max(0, k - 80)
            for j in range(k - 1, lo, -1):
                p16, p8, op = nodes[j]
                if op == 33 and not (p8 & 2):
                    s = p16 if p16 < 0x8000 else p16 - 0x10000
                    if j + 1 + s == k:
                        found = j
                        break
            if found is None:
                break
            n += 1
            k = found
        return n

    def take_stmt_line(self):
        """语句行号: 消费该语句的全部条目, 返回最小值 = 语句起始行。

        行变化模型(探针 m1/m3/m4/c1/c3/create 实证): 条目=代码生成时行游标变化,
        多行语句产生一组值域=语句行范围的条目(折行调用 [末行,首行,末行] /
        折行二元 [左行,右行,实参行,call行] 等, 可递增开头)。边界判据:
        下一事件(锚/goto/下条语句)的条目值恒在本语句行范围之外 —— 即
        v>max 且其后一条也>max(或无) 时为止; v>max 但其后一条<=max
        说明是组内振荡回折(如 [21,22,21,22]), 继续消费。
        组归并与前瞻禁止跨文件段（律#99: 语句不跨文件, 跨段行号跳变≠回落）。
        """
        if self.stmt_lines is None or self.li >= len(self.stmt_lines):
            return None
        lines = self.stmt_lines
        seg_end = self._seg_end()
        li_start = self.li
        vals = [lines[self.li]]
        mx = lines[self.li]
        self.li += 1
        literal = self.saw_literal
        had_drop = False
        while self.li < seg_end and self.li not in self.marker_idxs:
            v = lines[self.li]
            if v <= mx:
                vals.append(v)
                self.li += 1
                continue
            nxt2 = lines[self.li + 1] if self.li + 1 < seg_end else None
            if nxt2 is not None and nxt2 <= mx:
                vals.append(v)
                if v > mx:
                    mx = v
                self.li += 1
                continue
            if literal:
                # 字面量组(mp/ar/mp3/wawa/xiqing 实证): mapping 元素行按词法序递增。
                # 归组判据(满足其一):
                #   ① 前瞻窗口内存在回落条目(<= 首条目) —— 组尾 LHS 行(wawa/mp3);
                #   ② 已消费数 < pairs+1 —— 无回落形态(LHS 与首元素同行, xiqing)。
                # 行距钳制仅在混合文件启用（律#99, start_gs xdbc_func_map 实证:
                # 组吞段尾真语句条目 396 → 后续整串平移）。单文件不钳制 ——
                # 表尾杂散条目（bonusd 实证: 165/2865 等指向函数区的编译器
                # 内部行游标记录）必须被组吸收, 否则污染后续语句行号。
                j = self.li
                drop = False
                while j < seg_end and j - self.li <= 64:
                    if lines[j] <= vals[0]:
                        drop = True
                        break
                    j += 1
                if drop:
                    had_drop = True
                    vals.append(v)
                    mx = v
                    self.li += 1
                    continue
                if len(vals) < self.literal_pairs + 1 \
                        and (self.seg_bounds is None or v <= vals[0] + 128):
                    vals.append(v)
                    mx = v
                    self.li += 1
                    continue
            break
        # 无回落 literal 组连续性检查（律#100, accountd try_to_connect_aaa 实证）:
        # 真字面量组元素行连续（跨度 <= 条目数-1）; 组内跳行有两种形态:
        #   a) 表尾截断（组后剩余 <= 1 条）: 行内 mapping 完成后跟别的语句,
        #      pairs 上限截断 → 回退仅保留首条, 条目还给后续语句
        #      （accountd: 组 [380,381,383] 后剩 384 = printf/return 真行,
        #      不回退则尾部语句全卡 LHS 行）;
        #   b) 组后仍有后续条目: 元素同行共享/表尾杂散吸收的正常形态, 信任组
        #      （combatd message_map 30 对缺 66, 组后 92/95 = init/join 真行;
        #       bonusd team_bonus 组 [78,165] 后 169/1052/2865 = 表尾杂散,
        #       回退会让杂散污染后续语句行号）。
        if literal and not had_drop and len(vals) > 1 \
                and len(lines) - self.li <= 1 \
                and max(vals) - min(vals) > len(vals) - 1:
            self.li = li_start + 1
            vals = [lines[li_start]]
        self.saw_literal = False
        self.literal_pairs = 0
        self.line_cursor = ln0 = min(vals)
        return ln0

    def emit_stmt(self, text, pc=None):
        """普通语句: 消费该语句条目组, 注释最小值 = 语句起始行(探针 m1-m4 实证)。"""
        # 律#112: catch 表达式体 —— 语句文本挂起为 catch(...) 部件
        # （真身 if (catch(A) || Y): A 是表达式, 不是独立语句）
        if self.catch_expr and self.catch_expr.get("body") is None:
            self.take_stmt_line()
            self.catch_expr["body"] = text.strip().rstrip(";").strip()
            return
        ln = self.take_stmt_line()
        if ln is None:
            # 行游标模型: 条目耗尽 = 行未变化(同行多语句共享一条, 如
            # `int a = 0, b = 0;`) —— 沿用游标当前值, 不编 None。
            ln = self.line_cursor
        if self._pending_file:
            # 多文件混合: 跨段处发文件标记（全局代码区可读性, 律#99）
            self.emit("// ---- from %s ----" % self._pending_file, pc=pc)
            self._pending_file = None
        if ln is not None and text.rstrip().endswith(";"):
            text = "%s  /* L%d */" % (text, ln)
        self.emit(text, pc=pc)

    def last_texts(self, n):
        return [t.strip() for _pc, t in self.lines[-n:]]


# ---- 指令处理器（op 号语义均经 IDA/探针实证，注释从简） ----

def _h_binop(op_sym):
    def h(ctx, par16, par8):
        right = _pop(ctx.stack)
        left = _pop(ctx.stack)
        lt, rt = left.t, right.t
        if _find_top_level_binop(lt) == op_sym:   # 左结合链扁平化
            lt = lt[1:-1]
        lt, rt = _wrap_assign(lt), _wrap_assign(rt)
        ctx.stack.append(V("(%s %s %s)" % (lt, op_sym, rt)))
    return h


def _h_neg(ctx, par16, par8):
    ctx.stack.append(V("-(%s)" % _pop(ctx.stack).t))


def _h_not(ctx, par16, par8):
    ctx.stack.append(V("!(%s)" % _pop(ctx.stack).t))


def _h_rev(ctx, par16, par8):
    ctx.stack.append(V("~(%s)" % _pop(ctx.stack).t))


def _push(ctx, v):
    # pn 哨兵（律#102, start_gs update_today_zero_time 实证）: member/range/to_ptr
    # 链显式传播 obj.pn（语句首 push 的前导 nop 数, 律#4 后缀成员判据依赖）,
    # 链中段 push 的 pre_nops=0 不得覆盖 —— 否则 `m[k]--;` 后缀判据失效,
    # 值残留栈底在 ret 处成幻影 return。
    if v.pn is None:
        v.pn = ctx.pre_nops
    if v.pc0 is None:
        v.pc0 = ctx.pc
    ctx.stack.append(v)
    return v


def _h_global(ctx, par16, par8):
    _push(ctx, V(GLOBAL_VAR_MAP.get(ctx.signed, "global_%d" % ctx.signed),
                 var_id=GLOBAL_VAR_MAP.get(ctx.signed)))


def _h_global_ptr(ctx, par16, par8):
    nm = GLOBAL_VAR_MAP.get(ctx.signed, "global_%d" % ctx.signed)
    _push(ctx, V(nm, ptr=True, var_id=nm))


def _h_local(ctx, par16, par8):
    _push(ctx, V(local_name(ctx.signed, ctx.fun)))


def _h_local_ptr(ctx, par16, par8):
    nm = local_name(ctx.signed, ctx.fun)
    _push(ctx, V(nm, ptr=True, var_id=nm))


def _h_local_i(ctx, par16, par8):
    _push(ctx, V("local_i[%s]" % _pop(ctx.stack).t))


def _h_local_ptr_i(ctx, par16, par8):
    _push(ctx, V("local_i[%s]" % _pop(ctx.stack).t, ptr=True))


def _h_small_int(ctx, par16, par8):
    _push(ctx, V(str(ctx.signed)))


def _h_long_int(ctx, par16, par8):
    off = par16 | (par8 << 16)
    meta = ctx.fun.get("meta", b"")
    v = struct.unpack_from("<i", meta, off)[0] if off + 4 <= len(meta) else 0
    _push(ctx, V(str(v)))


def _h_obj_var(ctx, par16, par8):
    _push(ctx, V(object_var_name(ctx.signed, ctx.info)))


def _h_obj_var_ptr(ctx, par16, par8):
    nm = object_var_name(ctx.signed, ctx.info)
    _push(ctx, V(nm, ptr=True, var_id=nm))


def _h_real(ctx, par16, par8):
    off = par16 | (par8 << 16)
    meta = ctx.fun.get("meta", b"")
    if off + 4 <= len(meta):
        v = struct.unpack("<f", struct.pack("<i", struct.unpack_from("<i", meta, off)[0]))[0]
        _push(ctx, V(fmt_float(v)))
    else:
        _push(ctx, V("<real:meta+0x%X>" % off))


def _h_string(ctx, par16, par8):
    strs = ctx.info["program_strings"]
    s = strs[par16] if 0 <= par16 < len(strs) else "<s%d>" % par16
    _push(ctx, V(quote(s)))


def _h_this_object(ctx, par16, par8):
    _push(ctx, V("this_object()"))


def _h_undefined(ctx, par16, par8):
    _push(ctx, V("undefined"))        # push_0 = T_UNDEFINED（缺陷④）


def _h_member(ctx, par16, par8):
    member = _pop(ctx.stack)
    obj = _pop(ctx.stack)
    _push(ctx, V("%s[%s]" % (obj.base(), member.t), sub=True,
                 pn=obj.pn, var_id=None))


def _h_member_ptr(ctx, par16, par8):
    member = _pop(ctx.stack)
    obj = _pop(ctx.stack)
    _push(ctx, V("%s[%s]" % (obj.base(), member.t), ptr=True, sub=True, pn=obj.pn))


def _mk_range(ctx, par16, ptr_mode):
    end = _pop(ctx.stack)
    start = _pop(ctx.stack)
    value = _pop(ctx.stack)
    st, en = start.t, end.t
    if par16 & 2:
        st = "<%s" % st
    if par16 & 1:
        en = "<%s" % en
    _push(ctx, V("%s[%s..%s]" % (value.base(), st, en),
                 ptr=ptr_mode, sub=True, pn=value.pn))


def _h_range(ctx, par16, par8):
    _mk_range(ctx, par16, False)


def _h_range_ptr(ctx, par16, par8):
    _mk_range(ctx, par16, True)


def _h_to_ptr(ctx, par16, par8):
    v = _pop(ctx.stack)
    # 括号化基址: to_ptr 作用于 member/range 值 → 基址加括号 (m[i])["k"]（tc17）
    _push(ctx, V(v.t, ptr=True, var_id=v.var_id, sub=v.sub, bracket=v.sub, pn=v.pn))


def _h_to_value(ctx, par16, par8):
    v = _pop(ctx.stack)
    _push(ctx, V(v.t, var_id=v.var_id, fresh=v.ptr and v.var_id is not None, pn=v.pn))


def _h_arg_count(ctx, par16, par8):
    _push(ctx, V("arg_count"))


def _h_arg_array(ctx, par16, par8):
    _push(ctx, V("$<"))


def _h_create_array(ctx, par16, par8):
    pass  # array 元素右→左求值=条目递减(ar 探针), 振荡规则已覆盖, 无需递增许可
    elems = _pop_args(ctx.stack, par16)
    _push(ctx, V("({%s})" % ", ".join(e.t for e in elems)))


def _h_create_mapping(ctx, par16, par8):
    ctx.saw_literal = True
    ctx.literal_pairs += par16 // 2
    items = _pop_args(ctx.stack, par16)
    pairs = []
    for i in range(0, len(items) - 1, 2):
        pairs.append("%s: %s" % (items[i].t, items[i + 1].t))
    if len(items) % 2:
        pairs.append("%s: <missing>" % items[-1].t)
    pairs.reverse()                    # 压栈为逆序
    _push(ctx, V("([%s])" % ", ".join(pairs)))


def _h_create_function_value(ctx, par16, par8):
    pass  # 闭包不产生行号条目(cl 探针实证)
    n_args = par8 & 0x3F
    args = _pop_args(ctx.stack, n_args)
    if par8 & 0x80:
        nm = resolve_global_func_name(par16) or "global_func_%d" % par16
    else:
        nm = resolve_inherited_func_name(ctx.info, par16) or "local_fn_%d" % par16
        # 同 _h_funcall: 继承链槽位 + override 自调用 → (: ::name :), 全库 0 例（保守）
        if nm in ctx.override_names and nm == ctx.fun.get("name") \
                and par16 not in ctx.info.get("func_index_map", {}):
            nm = "::%s" % nm
    if args:
        _push(ctx, V("(: %s, %s :)" % (nm, ", ".join(a.t for a in args))))
    else:
        _push(ctx, V("(: %s :)" % nm))


def _h_sizeof(ctx, par16, par8):
    _push(ctx, V("sizeof(%s)" % _pop(ctx.stack).t))


def _h_call_fp(ctx, par16, par8):
    items = _pop_args(ctx.stack, par8)
    fp = items[0].t if items else "0"
    _push(ctx, V("(*%s)(%s)" % (fp, ", ".join(a.t for a in items[1:]))))


def _h_efun(ctx, par16, par8):
    nm = efun_name(par16)
    n_pop = par8 & 0x3F
    args = _pop_args(ctx.stack, n_pop)
    _push(ctx, V("%s(%s)" % (nm, ", ".join(_arg_text(a) for a in args))))


def _h_funcall(ctx, par16, par8):
    if par8 & 0x80:
        nm = resolve_global_func_name(par16) or "global_func_%d" % par16
    else:
        nm = resolve_inherited_func_name(ctx.info, par16) or "local_fn_%d" % par16
        # `::name()` 调用判据（guard.o 实证, 2026-10-20）:
        #   override 函数自调用父类版本时 par16 指向继承链槽位 —— 不在本文件
        #   func_index_map（guard.o: ::receive_damage 的 par16=442, 自有槽位=919）。
        #   par16 落在本文件 fim = 普通调用（含 override 自身槽位递归）, 不加 ::。
        #   全库 v2 树 814 处 :: 均为此形态, 无闭包/非自调用变体。
        if nm in ctx.override_names and nm == ctx.fun.get("name") \
                and par16 not in ctx.info.get("func_index_map", {}):
            nm = "::%s" % nm
    args = _pop_args(ctx.stack, par8 & 0x3F)
    _push(ctx, V("%s(%s)" % (nm, ", ".join(_arg_text(a) for a in args))))


def _h_call_other(ctx, par16, par8):
    args = _pop_args(ctx.stack, 2 + (par8 & 0xFF))
    obj, func = args[0].t, args[1].t
    if func.startswith('"') and func.endswith('"'):
        func = func[1:-1]
    call_args = [_arg_text(a) for a in args[2:]]
    _push(ctx, V("%s->%s(%s)" % (obj, func, ", ".join(call_args))))


def _h_os_fun(ctx, par16, par8):
    n_args = par8 & 0x3F
    args = _pop_args(ctx.stack, n_args)
    if par16 in OS_FUN_MAP:
        op, is_binop = OS_FUN_MAP[par16]
        if is_binop and n_args >= 2:
            lt, rt = _wrap_assign(args[0].t), _wrap_assign(args[1].t)
            _push(ctx, V("(%s %s %s)" % (lt, op, rt)))
        elif not is_binop and n_args >= 1:
            _push(ctx, V("%s(%s)" % (op, args[0].t)))
        else:
            _push(ctx, V("os_fun_%d(%s)" % (par16, ", ".join(a.t for a in args))))
    else:
        _push(ctx, V("os_fun_%d(%s)" % (par16, ", ".join(a.t for a in args))))


def _h_cast_type(ctx, par16, par8):
    tt = TYPE_MAP.get(par16, "type_%d" % par16)
    _push(ctx, V("(%s)%s" % (tt, _pop(ctx.stack).t)))


def _h_is_type(ctx, par16, par8):
    val = _pop(ctx.stack)
    if par16 in IS_TYPE_EFUN:
        _push(ctx, V("%s(%s)" % (IS_TYPE_EFUN[par16], val.t)))
    else:
        _push(ctx, V("is_type(%s, T%d)" % (val.t, par16)))


def _h_check_is_ref_type(ctx, par16, par8):
    _push(ctx, V("1"))


def _h_check_is_ref_type_i(ctx, par16, par8):
    _pop(ctx.stack)
    _push(ctx, V("1"))


def _h_caught_error(ctx, par16, par8):
    # 律#112: catch 表达式模式 —— 压入 catch(体) 表达式值（供短路 if 作左枝）
    if ctx.catch_expr:
        ce = ctx.catch_expr
        ctx.catch_expr = None
        if ctx.stack and ctx.stack[-1].caught:
            ctx.stack.pop()
        body = ce.get("body")
        if body:
            _push(ctx, V("catch(%s)" % body))
            return
    # 异常路径判定值 1: 替换正常路径 0（运行时二选一, 线性解释器防泄漏）
    if ctx.stack and ctx.stack[-1].caught:
        ctx.stack.pop()
    _push(ctx, V("1", caught=True))   # catch 判定机器码值, pop 时静默吞


def _h_check_args(ctx, par16, par8):
    pass


def _h_end_thread_catch(ctx, par16, par8):
    # 正常路径判定值 0（catch 机器码: 与 caught_error 的 1 同一运行时槽位）
    if ctx.stack and ctx.stack[-1].t == "0":
        ctx.stack[-1] = V("0", caught=True)


def _h_finish_action(ctx, par16, par8):
    pass


def _h_null_action(ctx, par16, par8):
    pass


def _h_resort_stack(ctx, par16, par8):
    pass


def _h_to_reserve(ctx, par16, par8):
    pass


def _h_to_unreserve(ctx, par16, par8):
    pass


def _h_trace(ctx, par16, par8):
    ctx.emit("    /* trace(%d) */" % par16)


def _h_enter_function(ctx, par16, par8):
    ctx.stack.clear()


def _h_return(ctx, par16, par8, ret_op=73):
    if not ctx.stack:
        ctx.emit_stmt("    return;")
    else:
        v = _pop(ctx.stack)
        pc0 = v.pc0 if (v.pc0 is not None and v.pc0 < ctx.pc) else None
        ctx.emit_stmt("    return %s;" % v.t, pc=pc0)
    ctx.stack.clear()


def _h_return_fake(ctx, par16, par8):
    _h_return(ctx, par16, par8, 72)


def _emit_switch_marker_if_first(ctx):
    if ctx.lines:
        last = ctx.lines[-1][1].strip()
        if last.startswith("case ") or last.startswith("default:"):
            return
    if ctx.stack:
        ctx.emit("    /* switch: %s */" % ctx.stack[-1].t)
        ctx.take_lines(2)   # 行号模型: switch = [switch 行, 出口行]


def _h_case_num(ctx, par16, par8):
    off = par16 | (par8 << 16)
    meta = ctx.fun.get("meta", b"")
    offset = struct.unpack_from("<i", meta, off)[0] if off + 4 <= len(meta) else 0
    number = struct.unpack_from("<i", meta, off + 4)[0] if off + 8 <= len(meta) else 0
    target = ctx.pc + 1 + offset
    ctx.targets.add(target)
    _emit_switch_marker_if_first(ctx)
    ctx.emit("    case %d: goto @%d;" % (number, target))


def _h_case_str(ctx, par16, par8):
    off = par16 | (par8 << 16)
    meta = ctx.fun.get("meta", b"")
    offset = struct.unpack_from("<i", meta, off)[0] if off + 4 <= len(meta) else 0
    stridx = struct.unpack_from("<I", meta, off + 4)[0] if off + 8 <= len(meta) else -1
    target = ctx.pc + 1 + offset
    ctx.targets.add(target)
    _emit_switch_marker_if_first(ctx)
    strs = ctx.info["program_strings"]
    text = strs[stridx] if 0 <= stridx < len(strs) else "<string:%d>" % stridx
    ctx.emit('    case %s: goto @%d;' % (quote(text), target))


def _h_case_default(ctx, par16, par8):
    target = ctx.pc + 1 + ctx.signed
    ctx.targets.add(target)
    _emit_switch_marker_if_first(ctx)
    _pop(ctx.stack)
    ctx.emit("    default: goto @%d;" % target)


def _h_goto(ctx, par16, par8):
    target = ctx.pc + 1 + ctx.signed
    # 律#111 三元+catch 融合:
    #   - catch 内部回合跳转（→catch merge）: 纯构造件, 静默
    #   - then 收尾 goto（→汇合点）: 弹 A 分支值挂起, 静默
    if ctx.tc_fuse:
        tf = ctx.tc_fuse
        if tf.get("in_catch") and target != tf["end_pc"]:
            return
        if target == tf["end_pc"] and tf.get("a") is None:
            tf["a"] = _pop(ctx.stack).t
            tf["in_catch"] = False
            ctx.take_lines(1)   # 律#101: goto = then 退出符号
            return
    # 律#112: catch 表达式构造内的回合 goto 静默
    if ctx.catch_expr and target == ctx.catch_expr.get("if_pc"):
        return
    ctx.emit("    goto @%d;" % (ctx.pc + 1 + ctx.signed))
    # 行号模型（律#101, 探针 q1/q2/p3#1/p3#2 实证）: then 退出 goto 的符号
    # = else 路径末指令行。前向且前一指令为体锚 nop → 锚单独 1 条
    # （} else { 行）。1-stmt else 时 goto 符号与 else 体首条目同值 →
    # .o 侧已折叠 → goto 消费 0; 多语句 else → goto 符号独立 1 条。
    n = 0
    if ctx.signed > 0:
        nodes = ctx.fun["nodes"]
        if ctx.pc > 0 and nodes[ctx.pc - 1][2] in NOP_OPS:
            n += 1           # then 体锚
        t = ctx.pc + 1 + ctx.signed
        multi = 0 < t <= len(nodes) and nodes[t - 1][2] in NOP_OPS
        if not multi:
            # else 末锚不存在时, else 区域含跳转指令(else-if 链) → 多语句,
            # goto 符号独立（q4 实证）; 纯 1-stmt else → 与体首条目折叠, 消费 0
            lo = ctx.pc + 1
            for j in range(lo, min(t, len(nodes))):
                if nodes[j][2] in (33, 31):
                    multi = True
                    break
        if multi:
            n += 1
    else:
        n = 1                # 回边: 锚+goto 同值折叠 → 1 条
    ctx.take_lines(n)


def _h_if(ctx, par16, par8):
    cond = ctx.stack[-1].t if ctx.stack else "0"
    if not (par8 & 2):
        # 短路求值（bit1=0 不弹条件）: pending_sc 在 label 处合并
        target = ctx.pc + 1 + ctx.signed
        op = "||" if (par8 & 1) else "&&"
        ctx.pending_sc.append((cond, op, target, False))
        ctx.last_assigned_value = cond
        return
    _pop(ctx.stack)
    target = ctx.pc + 1 + ctx.signed
    # 律#111 三元+catch 逗号融合（前向假跳形态）: 命中则不渲染 if 行,
    # 状态机在 else 汇合点产出 `(C ? (catch(body), A) : B)` 表达式。
    if ctx.signed >= 0 and not (par8 & 1) and ctx.tc_fuse is None:
        tc = _try_fuse_catch_ternary(ctx, target)
        if tc:
            E, M, _try_pc = tc
            ctx.tc_fuse = {"cond": cond, "else_pc": E, "end_pc": M,
                           "catch_body": None, "a": None, "in_catch": False}
            ctx.take_ifgoto(ctx.pc)
            return
    if par8 & 1:
        ctx.emit("    if (%s) goto @%d;" % (cond, target))
        ctx.take_lines(2)   # 行号模型: 真跳(回边/do-while 族)固定 [条件, 落点]
    else:
        if cond.startswith("!(") and cond.endswith(")"):
            ctx.emit("    if (%s) goto @%d;" % (cond[2:-1], target))
        else:
            ctx.emit("    if (!(%s)) goto @%d;" % (cond, target))
        if ctx.signed >= 0:
            # 前向假跳: 值驱动组消费（律#101, 探针全实证）
            ctx.take_ifgoto(ctx.pc)
        else:
            ctx.take_lines(2)   # 回跳: 固定 [条件, 落点]


def _h_cond_jmp_for_loop(ctx, par16, par8):
    target = ctx.pc + 1 + ctx.signed
    ctx.emit("    if (loop_ctrl_%d) goto @%d;  /* loop continue */" % (par8, target))
    ctx.take_lines(1)   # 行号模型: 循环条件/出口条目


def _h_step_jmp_for_loop(ctx, par16, par8):
    target = ctx.pc + 1 + ctx.signed
    ctx.emit("    /* step loop_ctrl_%d */ goto @%d;  /* loop step */" % (par8, target))
    ctx.take_lines(1)   # 行号模型: 回边条目


def _h_init_for_loop(ctx, par16, par8):
    step = _pop(ctx.stack)
    end = _pop(ctx.stack)
    iterator = _pop(ctx.stack)
    var = local_name(ctx.signed, ctx.fun)
    try:
        step_val = int(step.t)
        loop_type = "foreach" if step_val == 997 else "for_range"
    except (ValueError, TypeError):
        loop_type = "for_loop"
    # 机内注释必须单行（taskd 实证: end 表达式含真实换行的字符串 → 注释断行
    # → for 匹配正则失配 → 整个循环退化泄漏）。渲染时 \n 转义。
    _cmt = lambda s: s.replace("\r", "\\r").replace("\n", "\\n")
    ctx.emit("    /* for_loop_init(%s): var=%s iter=%s end=%s step=%s */" % (
        loop_type, var, _cmt(iterator.t), _cmt(end.t), _cmt(step.t)))
    ctx.take_lines(1)   # 行号模型: loop 语句行


def _h_pop_n_stack(ctx, par16, par8):
    for _ in range(par16):
        if not ctx.stack:
            break
        val = _pop(ctx.stack)
        if val.caught:  # caught_error 判定值: 机器码, 不渲染不消费行号
            continue
        if val.t == ctx.last_assigned_value:
            ctx.last_assigned_value = ""
            continue
        ctx.emit_stmt("    %s;" % val.t)


def _h_pop_stack(ctx, par16, par8):
    if ctx.suppress_pops > 0:
        ctx.suppress_pops -= 1
        return
    val = _pop(ctx.stack)
    if val.caught:      # caught_error 判定值: 机器码, 不渲染不消费行号
        return
    # 律#111 三元+catch: catch 体语句（如 error(...)）作为表达式部件挂起,
    # 不渲染为独立语句（真身 = (catch(error(...)), A) 逗号左枝）
    if ctx.tc_fuse and ctx.tc_fuse.get("in_catch") \
            and ctx.tc_fuse.get("catch_body") is None:
        ctx.tc_fuse["catch_body"] = val.t
        ctx.take_lines(1)   # 律#101: catch 符号由内层语句消费
        return
    if ctx.pending_sc:
        left, op, target, popped = ctx.pending_sc[-1]
        if val.t == left and not popped:
            ctx.pending_sc[-1] = (left, op, target, True)
            ctx.last_assigned_value = ""
            return
    if val.t == ctx.last_assigned_value:
        ctx.last_assigned_value = ""
        return
    ctx.emit_stmt("    %s;" % val.t)


def _inc_stmt(ctx, target, signed):
    """独立自增/自减语句: 前导 nop>=2 → 后缀（律#4/#90），否则前缀。"""
    amt = abs(signed) if signed else 1
    if target.bracket and target.sub:
        base = "(%s)" % target.t
    else:
        base = target.t
    postfix = ((target.pn or 0) >= 2)
    if signed == 1:
        txt = "%s++" if postfix else "++%s"
        txt = txt % base
    elif signed == -1:
        txt = "%s--" if postfix else "--%s"
        txt = txt % base
    else:
        txt = "%s += %d" % (base, signed)
    ctx.emit_stmt("    %s;" % txt)
    ctx.last_assigned_value = target.t


def _h_inc_value(ctx, par16, par8):
    target = _pop(ctx.stack)
    signed = ctx.signed
    if not target.ptr and target.sub:
        pass  # member_ptr 结果也是 ptr=True；非 ptr 的情形罕见，继续
    base_t = target.t
    var_id = target.var_id
    # 后缀语句形态（律#4 实证）: nop×2+ptr+inc+尾 nop, 无 pop, 值不消费
    #   例 cur_waiting_login_num++; —— 前导 nop≥2 为判据
    if signed in (1, -1) and (target.pn or 0) >= 2 and ctx.next_action not in (50, 51):
        txt = "%s++" if signed == 1 else "%s--"
        ctx.emit_stmt("    %s;" % (txt % base_t))
        ctx.last_assigned_value = base_t
        return
    # 表达式级后缀: 栈顶为同变量的旧值(经 to_value) → X++ / X--
    if signed in (1, -1) and ctx.stack and ctx.stack[-1].fresh and \
            ctx.stack[-1].var_id is not None and ctx.stack[-1].var_id == var_id:
        if signed == 1:
            ctx.stack[-1] = V("%s++" % ctx.stack[-1].t)
        else:
            ctx.stack[-1] = V("%s--" % ctx.stack[-1].t)
        ctx.last_assigned_value = ctx.stack[-1].t
        return
    # 语句级（后随 pop）
    if ctx.next_action in (50, 51):
        _inc_stmt(ctx, target, signed)
        return
    # 语句级前缀形态（律#90/#102, 探针 r1 实证）: `--x;` 前缀自减语句
    # 前导 nop 仅 1 条（不满足 pn>=2 后缀判据），且值悬空不消费 ——
    # 表达式级 inc 的值必被后续指令立即消费（assign/算术/实参…），
    # 后随 nop ⟺ 值悬空 ⟺ 语句（否则残留栈底在 ret 处成幻影 return）。
    if signed in (1, -1) and ctx.next_action in NOP_OPS:
        _inc_stmt(ctx, target, signed)
        return
    # 表达式级前缀: 产生新值 ++X / --X
    # 前缀自增 + 立即重取同变量（律#109, task_monster_refreshd.place_monster
    # 实证: [ptr][inc][ptr(X)][to_value] —— 编译器自增后重取当前值作比较
    # （do{}while(--times > 0)）。不融合则残留 --X 悬空被丢 → 自减语义丢失,
    # 循环条件失真。跳过重取两条指令, 让 inc 压入的 --X 直接充当表达式值。
    if signed in (1, -1) and ctx.next_action in (57, 58, 54, 62):
        _nodes = ctx.fun["nodes"]
        _pc1, _pc2 = ctx.pc + 1, ctx.pc + 2
        if _pc2 < len(_nodes) and _nodes[_pc2][2] == 20:   # to_value
            _nm = _ptr_var_name(ctx, _nodes[_pc1][0], _nodes[_pc1][2])
            if _nm == var_id:
                ctx.skip_pcs.add(_pc1)
                ctx.skip_pcs.add(_pc2)
    if signed == 1:
        ctx.stack.append(V("++%s" % base_t, var_id=var_id, pn=target.pn,
                           pc0=target.pc0))
    elif signed == -1:
        ctx.stack.append(V("--%s" % base_t, var_id=var_id, pn=target.pn,
                           pc0=target.pc0))
    else:
        ctx.stack.append(V("%s += %d" % (base_t, signed), var_id=var_id, pn=target.pn,
                           pc0=target.pc0))


def _fuse_postfix_lookahead(ctx, value_v):
    """赋值语句值 X 为 to_value 产生的新值时，前瞻 [pc+1, pop) 内是否有
    push_*_ptr(X); inc —— 有则融合为 `target = X++`（tc22）。
    返回 (push_pc, inc_pc, inc_signed) 或 None。"""
    if not (value_v.fresh and value_v.var_id):
        return None
    nodes = ctx.fun["nodes"]
    k = ctx.pc + 1
    while k < len(nodes) and k <= ctx.pc + 12:
        p16, p8, op = nodes[k]
        if op in (50, 51):
            return None
        if op in (57, 58, 54, 62):  # push_*_ptr
            nm = _ptr_var_name(ctx, p16, op)
            if nm == value_v.var_id and k + 1 < len(nodes) and nodes[k + 1][2] == 34:
                inc_p16 = nodes[k + 1][0]
                inc_signed = inc_p16 if inc_p16 < 0x8000 else inc_p16 - 0x10000
                return (k, k + 1, inc_signed)
        if op in (3, 4):  # 嵌套赋值：放弃
            return None
        k += 1
    return None


def _ptr_var_name(ctx, p16, op):
    signed = p16 if p16 < 0x8000 else p16 - 0x10000
    if op in (57, 58):
        return local_name(signed, ctx.fun)
    if op == 54:
        return GLOBAL_VAR_MAP.get(signed)
    if op == 62:
        return object_var_name(signed, ctx.info)
    return None


def _h_push_assign(ctx, par16, par8):
    """assign: ... value target => target = value，value 留栈。"""
    target = _pop(ctx.stack)
    value_v = ctx.stack[-1] if ctx.stack else V("0")
    # 三元表达式模式: if(C) goto @L1; push v1; goto @L2; @L1: push v2; @L2: [target]
    if len(ctx.lines) >= 4:
        ls = ctx.last_texts(4)
        m_if = IF_GOTO_RE.match(ls[0])
        m_goto = GOTO_RE.match(ls[1])
        m_l1 = LABEL_RE.match(ls[2])
        m_l2 = LABEL_RE.match(ls[3])
        if m_if and m_goto and m_l1 and m_l2 and m_if.group(2) == m_l1.group(1) \
                and m_goto.group(1) == m_l2.group(1):
            cond = m_if.group(1)
            val2 = ctx.stack.pop()   # else 值（栈顶）
            val1 = ctx.stack.pop()   # then 值
            tgt_t = target.t[1:] if target.t.startswith("&") else target.t
            if cond.startswith("!(") and cond.endswith(")"):
                inner = cond[2:-1]
                if inner.startswith("(") and inner.endswith(")") and _balanced(inner):
                    inner = inner[1:-1]
                expr = "%s = (%s) ? %s : %s" % (tgt_t, inner, val1.t, val2.t)
            else:
                expr = "%s = (%s) ? %s : %s" % (tgt_t, cond, val2.t, val1.t)
            del ctx.lines[-4:]
            if ctx.next_action in (50, 51):
                ctx.emit_stmt("    %s;" % expr)
                ctx.last_assigned_value = val2.t
            else:
                ctx.stack.append(V(expr))
            return
    tgt_t = target.t[1:] if target.t.startswith("&") else target.t
    if ctx.next_action in (50, 51):
        fused = _fuse_postfix_lookahead(ctx, value_v)
        if fused and fused[2] in (1, -1):
            k_push, k_inc, inc_signed = fused
            ctx.stack.pop()
            suffix = "++" if inc_signed == 1 else "--"
            ctx.emit_stmt("    %s = %s%s;" % (tgt_t, value_v.t, suffix))
            ctx.skip_pcs.update((k_push, k_inc))
            ctx.suppress_pops = 1
            ctx.last_assigned_value = ""
            return
        ctx.emit_stmt("    %s = %s;" % (tgt_t, value_v.t))
        ctx.last_assigned_value = value_v.t
    else:
        ctx.stack[-1] = V("%s = %s" % (tgt_t, value_v.t))


def _h_push_assign_rev(ctx, par16, par8):
    """assign_rev: ... target value => target = value，value 副本留栈。"""
    value_v = _pop(ctx.stack)
    target = _pop(ctx.stack)
    tgt_t = target.t[1:] if target.t.startswith("&") else target.t
    if ctx.next_action in (50, 51):
        ctx.emit_stmt("    %s = %s;" % (tgt_t, value_v.t))
        ctx.stack.append(value_v)
        ctx.last_assigned_value = value_v.t
    else:
        ctx.stack.append(V("%s = %s" % (tgt_t, value_v.t)))


def _h_op_eq(ctx, par16, par8):
    op = OP_EQ_MAP.get(par16, "?")
    value = _pop(ctx.stack)
    target = _pop(ctx.stack)
    tgt_t = target.t[1:] if target.t.startswith("&") else target.t
    ctx.emit_stmt("    %s %s= %s;" % (tgt_t, op, value.t))
    ctx.stack.append(V(tgt_t))
    ctx.last_assigned_value = tgt_t


def _var_assign(ctx, name, var_id=None):
    value_v = ctx.stack[-1] if ctx.stack else V("0")
    if ctx.next_action in (50, 51):
        ctx.emit_stmt("    %s = %s;" % (name, value_v.t))
        ctx.last_assigned_value = value_v.t
    else:
        ctx.stack[-1] = V("%s = %s" % (name, value_v.t), var_id=var_id)


def _h_global_assign(ctx, par16, par8):
    nm = GLOBAL_VAR_MAP.get(ctx.signed, "global_%d" % ctx.signed)
    _var_assign(ctx, nm)


def _h_local_assign(ctx, par16, par8):
    _var_assign(ctx, local_name(ctx.signed, ctx.fun))


def _h_obj_var_assign(ctx, par16, par8):
    _var_assign(ctx, object_var_name(ctx.signed, ctx.info))


def _detect_catch_expr(ctx):
    """律#112: catch 值被短路 if 消费（encryptd.decrypt 实证）:
       [try(44)][body: calc+恰1 pop][10/26 end_catch][31 goto→m][14 catch][33 短路if]
       → 真身 `if (catch(A) || Y)` —— catch 判定值参与 ||/&& 短路,
       不是独立 catch 语句。返回 op14 的 pc（构造终点）。"""
    nodes = ctx.fun["nodes"]
    pc = ctx.pc
    k = pc + 1
    pops = 0
    end26 = None
    while k < len(nodes) and k <= pc + 32:
        op = nodes[k][2]
        if op == 26:
            end26 = k
            break
        if op in (31, 33, 18, 80, 44, 14):
            return None
        if op in (50, 51):
            pops += 1
        k += 1
    if end26 is None or pops != 1:
        return None
    k = end26 + 1
    if k >= len(nodes) or nodes[k][2] != 31:
        return None
    k += 1
    while k < len(nodes) and k <= pc + 40:
        op = nodes[k][2]
        if op == 14:
            j = k + 1
            while j < len(nodes) and nodes[j][2] in NOP_OPS:
                j += 1
            if j < len(nodes) and nodes[j][2] == 33 and not (nodes[j][1] & 2):
                return k
            return None
        if op in (31, 33, 18, 80, 44, 26):
            return None
        k += 1
    return None


def _h_new_thread_catch(ctx, par16, par8):
    catch_target = ctx.pc + 1 + ctx.signed
    ctx.targets.add(catch_target)
    # 律#111 三元融合: catch 作为表达式部件, 不发标记注释
    if ctx.tc_fuse:
        ctx.tc_fuse["in_catch"] = True
        ctx.take_lines(0)
        return
    # 律#112: catch 值被短路 if 消费 → catch 表达式模式
    ce_if = _detect_catch_expr(ctx)
    if ce_if is not None:
        ctx.catch_expr = {"if_pc": ce_if, "body": None}
        ctx.take_lines(0)
        return
    ctx.emit("    /* new_thread_catch -> @%d */" % catch_target)
    # 行号模型（律#101, 探针 r2 实证）: catch 语句的符号由内层语句的
    # emit_stmt 消费（catch(x=strlen(s)) → [14] 恰 1 条）, try 构造本身不发。
    ctx.take_lines(0)


# 律#111: 三元+catch 逗号融合 —— 纯计算类操作码白名单（A/B 分支值算式允许集）
_TC_CALC_OK = frozenset(
    [5, 6, 7, 8, 9, 13, 19, 20, 21, 22, 24, 43, 45, 47, 49, 52, 53, 54, 55,
     56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 79]
    + list(BINOPS.keys()))


def _try_fuse_catch_ternary(ctx, target):
    """SKILL_OB 型三元+catch 逗号融合（用户真身定案, skill.c/mengqingsan 实证）:
       [if C→ELSE][A算式][try 形B: body→26→31→(14)→(50)][31→MERGE]
       [ELSE: B算式][MERGE: 消费者]
       真身 `(C ? (catch(body), A) : B)` —— 线性 leftovers 使 call_other 的
       obj/fun 槽吃到两分支残值（渲染 `A->B()` 垃圾）。匹配成功返回
       (else_pc, end_pc, try_pc)，调用方跳过 if 行发射。"""
    nodes = ctx.fun["nodes"]
    pc = ctx.pc
    E = target
    if ctx.pending_sc or not (pc + 4 <= E <= pc + 48):
        return None
    # phase 1: then 区前段 [pc+1, try_pc) 只许 calc ops, 恰一个 try(44)
    try_pc = None
    for k in range(pc + 1, E):
        op = nodes[k][2]
        if op == 44:
            try_pc = k
            break
        if op not in _TC_CALC_OK and op not in NOP_OPS:
            return None
    if try_pc is None:
        return False
    # phase 2: 形B [try][body(calc/pop 恰 1 次)][26][31→m2][14][50][31→M]
    k = try_pc + 1
    body_pops = 0
    while k < E and nodes[k][2] != 26:
        op = nodes[k][2]
        if op in (31, 33, 18, 80, 44):
            return None
        if op in (50, 51):
            body_pops += 1
        k += 1
    if k >= E or body_pops != 1:
        return None
    k += 1                                   # end_catch(26)
    if k >= E or nodes[k][2] != 31:
        return None
    k += 1                                   # goto m2
    seen14 = seen50 = False
    M = None
    while k < len(nodes) and k < E + 32:
        op = nodes[k][2]
        if op == 14 and not seen14 and not seen50:
            seen14 = True
        elif op == 50 and seen14 and not seen50:
            seen50 = True
        elif op == 31 and seen50:
            s = nodes[k][0]
            M = k + 1 + (s if s < 0x8000 else s - 0x10000)
            break
        elif op in (31, 33, 18, 80, 44, 26):
            return None
        k += 1
    if M is None or not (E < M <= E + 40):
        return None
    # phase 3: else 区 [E, M) 只许 calc ops
    for k2 in range(E, M):
        op = nodes[k2][2]
        if op not in _TC_CALC_OK and op not in NOP_OPS:
            return None
    return (E, M, try_pc)


def _h_pickup_os_value(ctx, par16, par8):
    off = par16 | (par8 << 16)
    meta = ctx.fun.get("meta", b"")
    length = struct.unpack_from("<I", meta, off + 4)[0] if off + 8 <= len(meta) else 0
    _push(ctx, V("pickup_os_value(meta+0x%X, %d)" % (off, length)))


def _balanced(s):
    depth = 0
    for i, ch in enumerate(s):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0 and i < len(s) - 1:
                return False
    return depth == 0


DT = {}
for _opn, _ops in BINOPS.items():
    DT[_opn] = _h_binop(_ops)
DT.update({
    3: _h_push_assign, 4: _h_push_assign_rev, 47: _h_op_eq, 34: _h_inc_value,
    43: _h_neg, 45: _h_not, 74: _h_rev,
    5: _h_call_fp, 6: _h_efun, 7: _h_funcall, 8: _h_call_other, 9: _h_os_fun,
    10: _h_case_num, 11: _h_case_str, 12: _h_case_default,
    13: _h_cast_type, 16: _h_check_is_ref_type, 17: _h_check_is_ref_type_i,
    36: _h_is_type,
    18: _h_cond_jmp_for_loop, 31: _h_goto, 33: _h_if, 80: _h_step_jmp_for_loop,
    21: _h_arg_array, 22: _h_create_array, 23: _h_create_function_value,
    24: _h_create_mapping, 35: _h_init_for_loop,
    27: _h_enter_function, 72: _h_return_fake, 73: _h_return,
    19: _h_to_ptr, 20: _h_to_value, 49: _h_pickup_os_value,
    50: _h_pop_n_stack, 51: _h_pop_stack, 71: _h_resort_stack,
    82: _h_to_reserve, 83: _h_to_unreserve,
    52: _h_arg_count, 59: _h_small_int, 60: _h_long_int, 63: _h_real,
    64: _h_string, 65: _h_this_object, 66: _h_undefined,
    53: _h_global, 54: _h_global_ptr, 55: _h_local, 56: _h_local_i,
    57: _h_local_ptr, 58: _h_local_ptr_i,
    61: _h_obj_var, 62: _h_obj_var_ptr,
    67: _h_member, 68: _h_member_ptr, 69: _h_range, 70: _h_range_ptr,
    76: _h_global_assign, 77: _h_local_assign, 78: _h_obj_var_assign,
    14: _h_caught_error, 15: _h_check_args, 26: _h_end_thread_catch,
    29: _h_finish_action, 44: _h_new_thread_catch, 46: _h_null_action,
    79: _h_sizeof, 84: _h_trace,
})


def decompile_function(fun, info, with_lines=False):
    """单函数字节码 → 结构化行（goto/label 中间形态 → 控制流恢复）。"""
    nodes = fun["nodes"]
    ctx = FnCtx(fun, info, with_lines=with_lines)
    # 行号模型（探针 q/r/g1/g2/g3/h/while/f6 实证, 见文件头注释）:
    # 行号模型（律#101, 三轮探针 p/q/r 全实证, 见 take_ifgoto 注释）:
    #   stmt=条目组取min; ifgoto=值驱动组消费[起始行, 折行..., 门落点...];
    #   goto=then退出符号(else路径末行; 1-stmt else 折叠不消费); catch=内层语句消费;
    #   goto=1(跳转所属行); label: case落点=每case恰1条(链首0/default1,无锚),
    #   非case label 前指令为 nop → 锚1条; switch=2(switch行+出口行);
    #   &&短路=0; for_init/for_cond/for_step=1; try=1; 行内 return 记所在行。
    meta_l = fun.get("meta", b"")
    prev_op = -1
    for _pc, (_p16, _p8, _op) in enumerate(nodes):
        if _op in (10, 11, 12) and prev_op in (10, 11, 12):
            if _op == 12:
                s = _p16 if _p16 < 0x8000 else _p16 - 0x10000
                t = _pc + 1 + s
            else:
                off = _p16 | (_p8 << 16)
                offset = struct.unpack_from("<i", meta_l, off)[0] if off + 4 <= len(meta_l) else 0
                t = _pc + 1 + offset
            ctx.case_landings[t] = ctx.case_landings.get(t, 0) + 1
        prev_op = _op
    # 第一遍: 收集跳转目标
    meta = fun.get("meta", b"")
    for pc, (par16, par8, action) in enumerate(nodes):
        if action in JUMP_OPS_PAR16:
            s = par16 if par16 < 0x8000 else par16 - 0x10000
            t = pc + 1 + s
            if 0 <= t < len(nodes):
                ctx.targets.add(t)
        elif action in JUMP_OPS_META:
            off = par16 | (par8 << 16)
            if off + 4 <= len(meta):
                offset = struct.unpack_from("<i", meta, off)[0]
                t = pc + 1 + offset
                if 0 <= t < len(nodes):
                    ctx.targets.add(t)
    # 预扫描: 三元表达式候选
    STMT_ACTIONS = {3, 4, 10, 11, 12, 18, 26, 27, 31, 33, 34, 35, 44, 47, 51,
                    72, 73, 76, 77, 78, 80, 84}
    FORBIDDEN_IN_TERNARY = STMT_ACTIONS - {26, 31, 33, 44, 51}
    ternary_candidates = {}
    for pc, (par16, par8, action) in enumerate(nodes):
        if action != 33 or not (par8 & 2):
            continue
        signed = par16 if par16 < 0x8000 else par16 - 0x10000
        else_target = pc + 1 + signed
        if else_target <= pc + 2 or else_target >= len(nodes):
            continue
        goto_pc = else_target - 1
        g_par16, g_par8, g_action = nodes[goto_pc]
        if g_action != 31:
            continue
        g_signed = g_par16 if g_par16 < 0x8000 else g_par16 - 0x10000
        end_target = goto_pc + 1 + g_signed
        if end_target <= else_target or end_target >= len(nodes):
            continue
        ternary_candidates[pc] = (else_target, goto_pc, end_target)
    # pop 紧随 call 族 = 语句体标志(taskd.set_task_round 实证: if/else 两分支
    # [call][pop] 被误判三元 → 整块被吞只剩 else 体)。真三元分支=纯表达式无
    # call+pop; catch 机器码的 pop(end_catch 后)仍豁免。
    def _stmt_pop(k, lo):
        if nodes[k][2] != 51:
            return False
        j = k - 1
        while j > lo and nodes[j][2] in NOP_OPS:
            j -= 1
        return nodes[j][2] in (5, 6, 7, 8, 9)

    ternary_starts = {}
    for pc, (else_target, goto_pc, end_target) in ternary_candidates.items():
        then_ok = True
        for k in range(pc + 1, goto_pc):
            if nodes[k][2] in FORBIDDEN_IN_TERNARY:
                then_ok = False
                break
            if _stmt_pop(k, pc):
                then_ok = False
                break
            if k in ctx.targets and nodes[k][2] not in (14, 50):
                then_ok = False
                break
        if not then_ok:
            continue
        else_ok = True
        nested = []
        for k in range(else_target, end_target):
            if nodes[k][2] in FORBIDDEN_IN_TERNARY:
                else_ok = False
                break
            if _stmt_pop(k, else_target - 1):
                else_ok = False
                break
            if nodes[k][2] == 33:
                if k in ternary_candidates:
                    nested.append(k)
                    continue
                else_ok = False
                break
            if k != else_target and k in ctx.targets:
                if any(k in (ternary_candidates[nt][0], ternary_candidates[nt][2])
                       for nt in nested):
                    continue
                else_ok = False
                break
        if not else_ok:
            continue
        ternary_starts[pc] = (else_target, goto_pc, end_target)
    # 第二遍: 线性反编译
    pending_nops = 0
    for pc, (par16, par8, action) in enumerate(nodes):
        ctx.pc = pc
        ctx.signed = par16 if par16 < 0x8000 else par16 - 0x10000
        ctx.next_action = nodes[pc + 1][2] if pc + 1 < len(nodes) else -1
        # 标签发射必须先于 nop 跳过: pc 可能既是跳转目标又是 nop（体锚,
        # 如 switch 空 case 标签 @锚）, 否则标签丢失 → case 块被结构恢复丢弃
        skip_label = ctx.pending_ternary and ctx.pending_ternary["state"] == "else" \
            and pc == ctx.pending_ternary["else_target"]
        # 律#111: 三元+catch 融合区间的内部标签全部抑制（else 起点/汇合点/
        # catch 内部 @m 均为构造件, 非源码标签）
        if ctx.tc_fuse and pc <= ctx.tc_fuse["end_pc"]:
            skip_label = True
        # 律#112: catch 表达式构造区间标签抑制（@catch/@if 均为构造件）
        if ctx.catch_expr and pc <= ctx.catch_expr["if_pc"]:
            skip_label = True
        if pc in ctx.targets and not skip_label:
            ctx.emit("@%d:" % pc)
            # 行号模型: label 处消费 = 体锚收尾条目(前一指令为 nop ⟺ 带花括号体)
            #           + 落到本 pc 的非链首 case 条目数（探针 g1/g2/g3 实证）
            # 行号消费(探针 f6 实证): case 标签落点=每 case 恰 1 条(链首 0/default 1),
            # 无锚条目; 非 case 的 label(if 假目标/switch 出口)前指令为 nop → 锚 1 条。
            n_take = ctx.case_landings.get(pc, 0)
            if n_take == 0 and pc > 0 and nodes[pc - 1][2] in NOP_OPS:
                n_take += 1
            ctx.take_lines(n_take)
            while ctx.pending_sc:
                left, op, target, popped = ctx.pending_sc[-1]
                if target != pc:
                    break
                if popped and ctx.stack:
                    right = ctx.stack[-1].t
                    if _find_top_level_binop(left) == op:
                        left = left[1:-1]
                    left, right = _wrap_assign(left), _wrap_assign(right)
                    ctx.stack[-1] = V("(%s %s %s)" % (left, op, right))
                ctx.pending_sc.pop()
        if action in NOP_OPS:
            pending_nops += 1
            continue
        ctx.pre_nops = pending_nops
        pending_nops = 0
        if pc in ctx.skip_pcs:
            continue
        # 律#111: 三元+catch 汇合点 —— 弹 B 分支值, 与 A/catch/条件合成三元表达式
        if ctx.tc_fuse and pc == ctx.tc_fuse["end_pc"]:
            tf = ctx.tc_fuse
            ctx.tc_fuse = None
            b_txt = _pop(ctx.stack).t
            a_txt = tf["a"] if tf["a"] is not None else "<ternary?>"
            if tf["catch_body"]:
                a_txt = "(catch(%s), %s)" % (tf["catch_body"], a_txt)
            c_txt = tf["cond"]
            if not (c_txt.startswith("(") and c_txt.endswith(")") and _balanced(c_txt[1:-1])):
                c_txt = "(%s)" % c_txt
            _push(ctx, V("(%s ? %s : %s)" % (c_txt, a_txt, b_txt)))
        # 三元: if 处
        if pc in ternary_starts:
            else_target, goto_pc, end_target = ternary_starts[pc]
            cond = ctx.stack[-1].t if ctx.stack else "0"
            if ctx.stack:
                ctx.stack.pop()
            ctx.take_lines(2)   # 行号模型: 三元条件 ifgoto 同构 [条件行, 假目标行]
            ctx.pending_ternary = {
                "cond": cond, "par8": par8, "then_val": None,
                "goto_pc": goto_pc, "else_target": else_target,
                "end_target": end_target, "state": "then",
                "in_catch": False, "catch_start": -1, "catch_lines": None}
            continue
        # 三元: goto 处（then 结束）
        if ctx.pending_ternary and ctx.pending_ternary["state"] == "then" \
                and pc == ctx.pending_ternary["goto_pc"]:
            ctx.pending_ternary["then_val"] = ctx.stack.pop().t if ctx.stack else "0"
            ctx.pending_ternary["state"] = "else"
            continue
        # （标签已在循环开头 nop 判断之前发射）
        # 三元: @end 处（合并）
        if ctx.pending_ternary and pc == ctx.pending_ternary["end_target"]:
            else_val = ctx.stack.pop().t if ctx.stack else "0"
            cond = ctx.pending_ternary["cond"]
            par8_t = ctx.pending_ternary["par8"]
            then_val = ctx.pending_ternary["then_val"]
            catch_lines = ctx.pending_ternary.get("catch_lines")
            if catch_lines:
                if par8_t & 1:
                    if_cond = cond[2:-1] if cond.startswith("!(") and cond.endswith(")") \
                        else "!(%s)" % cond
                else:
                    if_cond = cond
                body = " ".join(cl.strip() for cl in catch_lines)
                ctx.emit("    if (%s) %s" % (if_cond, body))
            if par8_t & 1:
                ternary = "(%s ? %s : %s)" % (cond, else_val, then_val)
            else:
                if cond.startswith("!(") and cond.endswith(")"):
                    ternary = "(%s ? %s : %s)" % (cond[2:-1], else_val, then_val)
                else:
                    ternary = "(%s ? %s : %s)" % (cond, then_val, else_val)
            ctx.stack.append(V(ternary))
            ctx.pending_ternary = None
        # 三元 then 分支中的 catch 模式
        if ctx.pending_ternary and ctx.pending_ternary["state"] == "then":
            if action == 44:
                ctx.pending_ternary["in_catch"] = True
                ctx.pending_ternary["catch_start"] = len(ctx.lines)
                continue
            if action in (26, 14, 51):
                continue
            if action == 50:
                ctx.pending_ternary["in_catch"] = False
                ctx.pending_ternary["catch_lines"] = ctx.lines[ctx.pending_ternary["catch_start"]:]
                del ctx.lines[ctx.pending_ternary["catch_start"]:]
                for _ in range(par16):
                    if ctx.stack:
                        ctx.stack.pop()
                if ctx.stack:
                    val = ctx.stack.pop().t
                    if val.startswith("error(") or val.startswith("throw("):
                        ctx.pending_ternary["catch_lines"] = ["    catch(%s);" % val]
                    else:
                        ctx.pending_ternary["catch_lines"] = ["    %s;" % val]
                continue
        handler = DT.get(action)
        if handler is not None:
            handler(ctx, par16, par8)
        else:
            ctx.emit("    /* VM: action_%d(par16=0x%04X, par8=0x%02X) */"
                     % (action, par16, par8))
    if ctx.stack:
        ctx.emit("    /* VM stack at end: %s */" % ", ".join(v.t for v in ctx.stack))
    raw = [t for _pc, t in ctx.lines]
    # 体锚判定（律#34/#67/#91 探针实证）: if 跳转目标 T 的前一指令是 nop（体锚）
    # ⟺ 原源码 then 体带花括号; 无锚 ⟺ 无括号单语句体（老编译器不发锚, bygs 同）。
    noanchor = set()
    # if/else 体锚: T=else 起点, nodes[T-1]=goto(跳过 else)。
    #   then_anchor: goto 前一指令是 nop（then 体带括号）
    #   else_anchor: else 结束标号 E=goto 目标, nodes[E-1] 是 nop（else 体带括号）
    else_anchors = {}
    for pc, (par16, par8, action) in enumerate(nodes):
        if action != 33 or not (par8 & 2):
            continue
        signed = par16 if par16 < 0x8000 else par16 - 0x10000
        t = pc + 1 + signed
        if 0 < t <= pc:
            continue
        prev_op = nodes[t - 1][2] if 0 <= t - 1 < len(nodes) else -1
        if prev_op not in NOP_OPS:
            noanchor.add(t)
        else:
            # nop 串歧义消解（探针 g1-g4 实证）: 回溯到首个非 nop 指令 K:
            #   K 为终结指令(ret/goto) → 其后 nop = 死区体锚 → 带花括号
            #   K 为 inc 且 nop 串长 ≤1 → inc 的活尾随 nop（如 return --x）→ 无花括号
            k2 = t - 1
            while k2 > pc + 1 and nodes[k2][2] in NOP_OPS:
                k2 -= 1
            if k2 > pc:
                k_op = nodes[k2][2]
                run = t - 1 - k2
                if k_op == 34 and run <= 1:
                    noanchor.add(t)
        if prev_op == 31 and t - 2 > pc:  # if/else 结构
            g = t - 1
            then_anchor = nodes[g - 1][2] in NOP_OPS
            gs = nodes[g][0] if nodes[g][0] < 0x8000 else nodes[g][0] - 0x10000
            e = g + 1 + gs
            else_anchor = (0 < e <= len(nodes) and nodes[e - 1][2] in NOP_OPS)
            else_anchors[t] = (then_anchor, else_anchor)
    return _drop_dead_gotos(restore_control_flow(raw, noanchor, else_anchors))


_DEAD_TERM_RE = re.compile(r"^(return\b|continue;|break;|goto @\d+;)")
_LABEL_RE2 = re.compile(r"^@\d+:")


def _drop_dead_gotos(lines):
    """死 goto 消除: 紧跟无条件终结语句（return/continue/break/goto）之后、
    且两者之间无 label（跳转目标 ⇒ 可能可达）的 goto/ifgoto 行 = 不可达
    （if/else 机制残留的 then-end goto, admin_block_account.get_account_from_db
    实证: catch{ return X; goto @66; } —— return 后 goto 永不执行）。"""
    out = []
    prev_term = False
    for ln in lines:
        s = ln.strip()
        if not s:
            out.append(ln)
            continue
        if (_LABEL_RE2.match(s) or s.startswith("case ") or s.startswith("default:")
                or s.endswith("{") or s.startswith("}") or s.startswith("else")):
            prev_term = False     # 结构行: label=潜在落点/块边界 ⇒ 重置
            out.append(ln)
            continue
        is_goto = s.startswith("goto @") or (s.startswith("if (") and ") goto @" in s)
        if is_goto and prev_term:
            continue              # 死代码, 丢弃（prev_term 保持 ⇒ 连续死 goto 链全删）
        prev_term = bool(_DEAD_TERM_RE.match(s))
        out.append(ln)
    return out


# ===========================================================================
# §6 控制流恢复（goto/label → if/else/while/switch/try）
# ===========================================================================
IF_GOTO_RE = re.compile(r"^if \((.+)\) goto @(\d+);\s*(?:/\*.*\*/)?$")
GOTO_RE = re.compile(r"^goto @(\d+);\s*(?:/\*.*\*/)?$")
LABEL_RE = re.compile(r"^@(\d+):\s*(?:/\*.*\*/)?$")
CASE_RE = re.compile(r"^case (.+?): goto @(\d+);\s*(?:/\*.*\*/)?$")
DEFAULT_RE = re.compile(r"^default: goto @(\d+);\s*(?:/\*.*\*/)?$")


def _strip_comment(s):
    idx = s.find("/*")
    if idx >= 0:
        s = s[:idx]
    return s.strip()


def _parse_lines(raw_lines):
    items = []
    for raw in raw_lines:
        s = raw.strip()
        if not s:
            continue
        m = LABEL_RE.match(s)
        if m:
            items.append(("label", int(m.group(1)), None))
            continue
        m = IF_GOTO_RE.match(s)
        if m:
            items.append(("ifgoto", m.group(1), int(m.group(2))))
            continue
        m = GOTO_RE.match(s)
        if m:
            items.append(("goto", None, int(m.group(1))))
            continue
        m = CASE_RE.match(s)
        if m:
            items.append(("case", m.group(1), int(m.group(2))))
            continue
        m = DEFAULT_RE.match(s)
        if m:
            items.append(("default", None, int(m.group(1))))
            continue
        m = re.match(r"/\*\s*switch:\s*(.+?)\s*\*/", s)
        if m:
            items.append(("switch_marker", m.group(1), None))
            continue
        m = re.match(r"/\*\s*new_thread_catch\s*->\s*@(\d+)\s*\*/", s)
        if m:
            items.append(("try_catch", int(m.group(1)), None))
            continue
        items.append(("stmt", s, None))
    return items


def _find_back_gotos(items, start, end, label_id):
    res = []
    for k in range(start, end):
        kind, _c, target = items[k]
        if target == label_id and kind in ("goto", "ifgoto"):
            res.append(k)
    return res


_INC_STMT_RE = re.compile(
    r"^\s*[A-Za-z_]\w*(\[[^\]]*\])?\s*(\+\+|--)\s*;"
    r"|^\s*[A-Za-z_]\w*(\[[^\]]*\])?\s*[+-]=\s*-?\d+\s*;")


def _detect_loop_tail_step(items, body_lo, last_back):
    """老编译器 for-continue 形态（zone.load_rooms / start_gs.unload_all /
    scenariod.menu_do_next_scene / chuchongshidao.sf_select_tactics 实证）:
    循环无 for_init/for_step 标记, 尾部 = [...][label L...][nop...][步进语句]
    [nop...][回边], 且体内存在 goto→L（源码 continue 跳步进子句, 律#2）。
    → 渲染 for (; cond; step) 并把 goto→L 映射为 continue（i++ 移入步进子句,
    语义与原 for+continue 精确一致; 留在体内则 continue 会漏掉 i++）。
    返回 (zone_start, tail_labels, step_txt, comments) 或 None。"""
    if last_back - 1 <= body_lo:
        return None
    k = last_back - 1
    tail_labels = set()
    step_stmts = []
    comments = []
    while k >= body_lo:
        kind, txt, _t = items[k]
        if kind == "label":
            tail_labels.add(txt)
            k -= 1
            continue
        if kind == "stmt":
            s = _strip_comment(txt)
            if s in ("nop;", "nop", ""):
                k -= 1
                continue
            if _INC_STMT_RE.match(txt):
                step_stmts.insert(0, s)
                comments[:0] = re.findall(r"/\* L\d+ \*/", txt)
                k -= 1
                continue
            break
        break
    if not tail_labels or not step_stmts:
        return None
    zone_start = k + 1
    # 体内须确有 goto→尾标签（否则只是普通尾步进 while, 勿动）
    if not any(items[m][0] in ("goto", "ifgoto") and items[m][2] in tail_labels
               for m in range(body_lo, last_back)):
        return None
    # for 步进子句: 去尾分号, 多条用逗号连接
    steps = [s.rstrip().rstrip(";").strip() for s in step_stmts]
    return (zone_start, tail_labels, ", ".join(steps), comments)


def _is_only_labels(items, lo, hi):
    return all(items[k][0] == "label" for k in range(lo, hi))


def _is_foreach_continue(i, tgt_idx, loop_ctx, items):
    if loop_ctx is None or len(loop_ctx) < 3 or tgt_idx < 0:
        return False
    loop_start_label, loop_end_idx = loop_ctx[0], loop_ctx[1]
    if loop_start_label is None or loop_end_idx is None or tgt_idx >= loop_end_idx:
        return False
    # ifgoto 与落点之间有实际语句 → if-then 体（非 continue）。
    # taskd.load_task_data_reversed_classes 实证: [if假跳→step前锚][赋值体][@锚]
    # 真身 = `if (cond) 单语句体`——误判 continue 会多编 not+goto 两条。
    # guard 形态([if假跳→体][goto→step])中间只允许 label 或单个 continue-goto。
    saw_goto = False
    for k in range(i + 1, tgt_idx):
        if items[k][0] == "label":
            continue
        if items[k][0] == "goto" and not saw_goto:
            saw_goto = True
            continue
        return False
    for k in range(tgt_idx, loop_end_idx - 1):
        if items[k][0] != "label":
            return False
    if loop_end_idx - 1 < 0:
        return False
    step_item = items[loop_end_idx - 1]
    if step_item[0] == "goto" and step_item[2] == loop_start_label:
        return True
    if step_item[0] == "stmt" and ("goto @%d" % loop_start_label) in step_item[1]:
        return True
    return False


def _structure(items, start, end, labels, indent, loop_ctx=None):
    """loop_ctx = (loop_start_label, loop_end_idx, cond_end_label, switch_break_label,
    case_targets[, step_labels])。新循环 ctx 必须透传外层 [3](switch break) 与
    [5](step labels) —— 否则 switch/外层 for 内恢复的 while 体里, break/continue
    goto 全悬空（treasure_tradingd.get_order_list 实证: foreach+switch 8 处）。"""
    result = []
    i = start
    pad = "    " * indent
    noanchor = _NOANCHOR
    outer_brk = loop_ctx[3] if (loop_ctx and len(loop_ctx) >= 4) else None
    outer_step = loop_ctx[5] if (loop_ctx and len(loop_ctx) >= 6) else None
    outer_ct = loop_ctx[4] if (loop_ctx and len(loop_ctx) >= 5) else None
    while i < end:
        kind, content, target = items[i]

        if kind == "label":
            back_gotos = _find_back_gotos(items, i + 1, end, content)
            if back_gotos:
                last_back = back_gotos[-1]
                last_back_item = items[last_back]
                loop_end_idx = last_back + 1
                # while(头条件)形态: label 后紧跟 if COND'→E（E 越过全部回跳 = 循环出口）
                # 且体内回跳本 label → while (正条件) { ... }。
                # 例: for 体内嵌 while (1) { ...; if (c) continue; break; }
                #    （while 头与 for 体起点重合, for 匹配器把 label 交给本分支处理）
                if i + 1 < end and items[i + 1][0] == "ifgoto" \
                        and "loop_ctrl" not in items[i + 1][1]:
                    e_idx = labels.get(items[i + 1][2], -1)
                    if last_back < e_idx <= end and e_idx > i + 2:
                        pos_cond = _negate_cond(items[i + 1][1])
                        tail = _detect_loop_tail_step(items, i + 2, last_back)
                        if tail:
                            zone_start, tail_labels, step_txt, comments = tail
                            wlc = (content, e_idx, None, outer_brk, outer_ct,
                                   tail_labels, None)
                            cmt = ("  " + " ".join(comments)) if comments else ""
                            result.append(pad + "for (; %s; %s) {%s"
                                          % (pos_cond, step_txt, cmt))
                            result.extend(_structure(items, i + 2, zone_start,
                                                     labels, indent + 1, wlc))
                        else:
                            wlc = (content, e_idx, None, outer_brk, outer_ct,
                                   outer_step, None)
                            result.append(pad + "while (%s) {" % pos_cond)
                            result.extend(_structure(items, i + 2, last_back,
                                                     labels, indent + 1, wlc))
                        result.append(pad + "}")
                        i = e_idx
                        continue
                cond_idx = cond_target = None
                for k in range(i + 1, last_back):
                    if items[k][0] != "ifgoto":
                        continue
                    tgt_k = items[k][2]
                    tgt_k_idx = labels.get(tgt_k, -1)
                    if loop_end_idx <= tgt_k_idx <= end and "loop_ctrl" not in items[k][1]:
                        cond_idx, cond_target = k, tgt_k
                        break
                if cond_idx is not None:
                    cond = items[cond_idx][1]
                    neg_cond = _negate_cond(cond)
                    tail = _detect_loop_tail_step(items, cond_idx + 1, last_back)
                    if tail:
                        zone_start, tail_labels, step_txt, comments = tail
                        nlc = (content, loop_end_idx, cond_target, outer_brk,
                               outer_ct, tail_labels, None)
                        cmt = ("  " + " ".join(comments)) if comments else ""
                        result.append(pad + "for (; %s; %s) {%s"
                                      % (neg_cond, step_txt, cmt))
                        result.extend(_structure(items, cond_idx + 1, zone_start,
                                                 labels, indent + 1, nlc))
                    else:
                        nlc = (content, loop_end_idx, cond_target, outer_brk,
                               outer_ct, outer_step, None)
                        result.append(pad + "while (%s) {" % neg_cond)
                        result.extend(_structure(items, cond_idx + 1, last_back,
                                                 labels, indent + 1, nlc))
                    result.append(pad + "}")
                    i = loop_end_idx
                    continue
                if last_back_item[0] == "ifgoto" and _strip_comment(last_back_item[1]) != "1":
                    # do-while 条件区标签（律#106, monsterd.syntax_value 实证）:
                    # goto→条件区 = do-while 的 continue。收集紧邻回边的 label 集
                    # 传入 ctx[6]。
                    dw_tail = set()
                    k2 = last_back - 1
                    while k2 > i and items[k2][0] == "label":
                        dw_tail.add(items[k2][1])
                        k2 -= 1
                    cond = last_back_item[1]
                    nlc = (content, loop_end_idx, None, outer_brk, outer_ct,
                           outer_step, dw_tail) \
                        if outer_brk is not None or outer_step is not None \
                        else (content, loop_end_idx, None, None, outer_ct, None, dw_tail)
                    result.append(pad + "do {")
                    result.extend(_structure(items, i + 1, last_back, labels,
                                             indent + 1, nlc))
                    result.append(pad + "} while (%s);" % cond)
                    i = loop_end_idx
                    continue
                nlc = (content, loop_end_idx, None, outer_brk, outer_ct,
                       outer_step, None) \
                    if outer_brk is not None or outer_step is not None \
                    else (content, loop_end_idx, None, None, outer_ct, None, None)
                tail = _detect_loop_tail_step(items, i + 1, last_back)
                if tail:
                    zone_start, tail_labels, step_txt, comments = tail
                    nlc = (content, loop_end_idx, None, outer_brk, outer_ct,
                           tail_labels, None)
                    cmt = ("  " + " ".join(comments)) if comments else ""
                    result.append(pad + "for (;; %s) {%s" % (step_txt, cmt))
                    result.extend(_structure(items, i + 1, zone_start, labels,
                                             indent + 1, nlc))
                else:
                    result.append(pad + "while (1) {")
                    result.extend(_structure(items, i + 1, last_back, labels,
                                             indent + 1, nlc))
                result.append(pad + "}")
                i = loop_end_idx
                continue
            i += 1
            continue

        if kind == "ifgoto":
            cond = content
            tgt = target
            tgt_idx = labels.get(tgt, -1)
            if _strip_comment(cond) == "0":
                result.append(pad + "if (0) { }" % ())
                i += 1
                continue
            if tgt_idx == i + 1 or (tgt_idx > i and _is_only_labels(items, i + 1, tgt_idx + 1)):
                result.append(pad + "if (%s) { }" % cond)
                i = tgt_idx + 1
                continue
            if loop_ctx and len(loop_ctx) >= 4 and loop_ctx[0] is not None and tgt == loop_ctx[0]:
                result.append(pad + "if (%s) continue;" % cond)
                i += 1
                continue
            # do-while 条件区标签 = continue（律#106, monsterd.syntax_value 实证）
            if loop_ctx and len(loop_ctx) >= 7 and loop_ctx[6] and tgt in loop_ctx[6]:
                result.append(pad + "if (%s) continue;" % cond)
                i += 1
                continue
            if _is_foreach_continue(i, tgt_idx, loop_ctx, items):
                result.append(pad + "if (%s) continue;" % cond)
                i += 1
                continue
            # 律#96 假路径 continue（taskd.when_enter_world 实证）:
            # [ifgoto→体内 tgt][goto→step] —— 真路径跳体, 假路径落 goto 达 step
            # = `if (!(cond)) { continue; }`（取反条件无需 not 指令, bygs 编译
            # 形态与 REF 精确一致）。then 区间仅一个 goto→step 跳板。
            if tgt_idx > i + 1:
                inner = [k for k in range(i + 1, tgt_idx) if items[k][0] != "label"]
                if len(inner) == 1 and items[inner[0]][0] == "goto":
                    sl96 = loop_ctx[5] if (loop_ctx and len(loop_ctx) >= 6) else None
                    if sl96 and items[inner[0]][2] in sl96:
                        result.append(pad + "if (%s) {" % _negate_cond(cond))
                        result.append(pad + "    continue;")
                        result.append(pad + "}")
                        i = tgt_idx
                        continue
            if loop_ctx and len(loop_ctx) >= 4 and loop_ctx[1] is not None:
                loop_end_idx = loop_ctx[1]
                if tgt_idx >= loop_end_idx and (tgt_idx >= end or tgt_idx < 0 or tgt_idx < i):
                    result.append(pad + "if (%s) break;" % cond)
                    i += 1
                    continue
            if loop_ctx and len(loop_ctx) >= 4 and loop_ctx[3] is not None \
                    and tgt == loop_ctx[3] and (tgt_idx >= end or tgt_idx < i):
                result.append(pad + "if (%s) break;" % cond)   # switch 合并点 = break
                i += 1
                continue
            if tgt_idx > end or tgt_idx < 0 or tgt_idx < i:
                # 律#95b 假路径 continue（teamd.dismiss_team 实证）:
                # [ifgoto→merge(越界)][goto→step] —— 真路径跳汇合点, 假路径落 continue
                # → 真身 `if (!(cond)) continue;`（guard_intimacy 真路径 continue 的对偶）
                if i + 1 < end and items[i + 1][0] == "goto":
                    sl2 = loop_ctx[5] if (loop_ctx and len(loop_ctx) >= 6) else None
                    if sl2 and items[i + 1][2] in sl2:
                        result.append(pad + "if (%s) continue;" % _negate_cond(cond))
                        i += 2
                        continue
                # ifgoto→越区 merge 通用变换（律#104, zhongzhou sf_apply /
                # move_child_item.move_equip 实证）: `if (C) goto M; REST;`
                # （M 越过本区）≡ `if (!(C)) { REST; }` + 外层续渲染——语义恒等
                # （C真→跳过 REST, C假→执行 REST; REST 内部 break/continue/goto
                # 由自身恢复规则处理, 与折叠前一致）。仅前向越区适用; 向后跳转
                # （tgt_idx < i, 循环回边形态）不适用, 落回原路。
                if tgt_idx > end:
                    inner_cond = _negate_cond(cond)
                    if inner_cond.startswith("!(") and inner_cond.endswith(")") \
                            and _balanced(inner_cond[2:-1]):
                        inner_cond = inner_cond[2:-1]
                    then_result = _structure(items, i + 1, end, labels,
                                             indent + 1, loop_ctx)
                    if len(then_result) == 1 and tgt in noanchor:
                        # 无体锚 + 单语句体 → 原源码无花括号（律#34）
                        result.append(pad + "if (%s) %s"
                                      % (inner_cond, then_result[0].strip()))
                    elif then_result:
                        result.append(pad + "if (%s) {" % inner_cond)
                        result.extend(then_result)
                        result.append(pad + "}")
                    else:
                        result.append(pad + "if (%s) { }" % inner_cond)
                    i = end
                    continue
                result.append(pad + "if (%s) goto @%d;" % (cond, tgt))
                i += 1
                continue
            # if-else
            then_end_goto = then_goto_idx = None
            for k in range(tgt_idx - 1, i, -1):
                if items[k][0] == "label":
                    continue
                if items[k][0] == "goto":
                    then_end_goto = items[k][2]
                    then_goto_idx = k
                break
            if then_end_goto is not None and then_goto_idx is not None:
                # for-continue 判定（律#2: continue 跳 step 子句）:
                # then-goto 目标 = 本 for 的 step 标签 → 真身是 if (正条件) continue;
                # 「假分支体」= 循环体后续语句照常（非 if/else）。
                # 实证 guard_intimacy.do_bonus: [if_false→28][goto→step][add_bonus]
                # ⚠ 纯守卫形态限定（teamd.dismiss_team 实证）: then 区间须仅含 label——
                #   否则 goto 属内层守卫(如 if(c){X; if(c2) continue;}), 外层走正常
                #   if-then 结构、内层守卫在体内自会触发本判定。
                step_labels = loop_ctx[5] if (loop_ctx and len(loop_ctx) >= 6) else None
                _pure_guard = all(items[k][0] == "label"
                                  for k in range(i + 1, then_goto_idx))
                if step_labels and then_end_goto in step_labels:
                    if _pure_guard:
                        pos_cond = _negate_cond(cond)
                        then_body = _structure(items, i + 1, then_goto_idx, labels,
                                                indent + 1, loop_ctx)
                        if then_body:
                            result.append(pad + "if (%s) {" % pos_cond)
                            result.extend(then_body)
                            result.append(pad + "    continue;")
                            result.append(pad + "}")
                        else:
                            result.append(pad + "if (%s) continue;" % pos_cond)
                        i = tgt_idx + 1
                        continue
                    # 非纯守卫: goto 属内层 continue(如 if(c){X; if(c2) continue;}) ——
                    # 禁用 if/else 路径(否则真身 if-then 被误渲成 else 包后续语句,
                    # 语义反转), 落到标准 if-then, 内层守卫在体内触发本判定。
                    then_end_goto = then_goto_idx = None
                end_idx = labels.get(then_end_goto, -1)
                effective_end = end
                if end_idx > end and end_idx <= len(items):
                    case_set = loop_ctx[4] if (loop_ctx and len(loop_ctx) >= 5) else None
                    all_merge = True
                    for k in range(end, end_idx):
                        if items[k][0] == "label":
                            if case_set is not None and items[k][1] in case_set:
                                all_merge = False
                                break
                            continue
                        if items[k][0] == "goto" and items[k][2] == then_end_goto:
                            continue
                        all_merge = False
                        break
                    if all_merge:
                        effective_end = end_idx
                    else:
                        # merge 越区且中间交错其他结构（switch default 体等,
                        # feed_pet._main_succ 实证: case1 的 if/else merge=144
                        # 在 case 区外, 中间夹 default 体）→ else 区信任区域
                        # 切片截止到区域尾, 尾部 merge-goto 由 ctx[3] 消化成 break
                        effective_end = end
                        end_idx = end
                if end_idx > tgt_idx and end_idx <= effective_end:
                    neg_cond = _negate_cond(cond)
                    then_result = _structure(items, i + 1, then_goto_idx, labels,
                                             indent + 1, loop_ctx)
                    # else 区从 tgt_idx（含目标 label）起, 不从 tgt_idx+1 ——
                    # 目标 label 常是内层循环头（label 分支靠它触发回边扫描,
                    # filed.dba_comm 实证: else 洞吞 label 44 → 内部 goto 全悬空）。
                    # label 无回边时该分支仅 i+=1 跳过, 与旧行为等价。
                    else_result = _structure(items, tgt_idx, end_idx, labels,
                                             indent + 1, loop_ctx)
                    # 体锚判定: 无锚 + 单语句体 → 原源码无花括号（探针实证 q2/q3 vs q4/q5）
                    ea = _ELSE_ANCHORS.get(tgt, (True, True))
                    # 律#110 空 then 语义恢复（serviced.has_service 实证:
                    #   [if (!(C)) goto @E][goto→merge][@E][goto→ret] —— then 体空,
                    #   收尾 goto 即真实语句）:
                    #   (a) 收尾=break/continue 目标 → then = break;/continue;
                    #   (b) 收尾=汇合跳 + else 非空 → 取反折叠 `if (C) { } else { X }`
                    #       ≡ `if (!(C)) { X }`
                    #   (c) else 体空 → 恒等价于无 else, 纯 if-then
                    if not then_result:
                        teg_idx0 = labels.get(then_end_goto, -1)
                        _ctrl = None
                        if loop_ctx and len(loop_ctx) >= 4 and loop_ctx[3] is not None \
                                and then_end_goto == loop_ctx[3]:
                            _ctrl = "break;"
                        elif loop_ctx and len(loop_ctx) >= 6 and loop_ctx[5] \
                                and then_end_goto in loop_ctx[5]:
                            _ctrl = "continue;"
                        elif loop_ctx and len(loop_ctx) >= 4 and loop_ctx[0] is not None \
                                and then_end_goto == loop_ctx[0]:
                            _ctrl = "continue;"
                        elif loop_ctx and len(loop_ctx) >= 4 and loop_ctx[1] is not None \
                                and teg_idx0 >= loop_ctx[1] and teg_idx0 > i:
                            _ctrl = "break;"
                        if _ctrl:
                            then_result = [pad + "    " + _ctrl]
                    # then 收尾跳转必须精确落在 else 末尾（汇合点）才能取反折叠;
                    # else 区被钳制时收尾跳转越过 else（feed_pet 型）, 保留原形态
                    join_clean = (labels.get(then_end_goto, -1) == end_idx)
                    then_unbrace = (not ea[0]) and len(then_result) == 1
                    else_unbrace = (not ea[1]) and else_result and (
                        len(else_result) == 1 or else_result[0].lstrip().startswith("if ("))
                    if not then_result and not else_result:
                        # 双空且收尾均为汇合跳 → 无操作; 保留条件副作用求值
                        result.append(pad + "if (%s) { }" % neg_cond)
                    elif not then_result and else_result:
                        # 取反折叠: 空 then + 汇合收尾 → `if (!(C)) { X }`
                        if not join_clean:
                            # 收尾跳转越过 else（钳制区）: 保留空 then 形态防丢真跳转
                            result.append(pad + "if (%s) { }" % neg_cond)
                            if else_unbrace:
                                result.append(pad + "else %s" % else_result[0].strip())
                                result.extend(else_result[1:])
                            else:
                                result.append(pad + "else {")
                                result.extend(else_result)
                                result.append(pad + "}")
                        elif else_unbrace:
                            result.append(pad + "if (%s) %s" % (neg_cond,
                                                                else_result[0].strip()))
                            result.extend(else_result[1:])
                        else:
                            result.append(pad + "if (%s) {" % neg_cond)
                            result.extend(else_result)
                            result.append(pad + "}")
                    elif then_result and not else_result:
                        # 空 else 恒等价 → 纯 if-then
                        if then_unbrace:
                            result.append(pad + "if (%s) %s" % (neg_cond,
                                                                then_result[0].strip()))
                        else:
                            result.append(pad + "if (%s) {" % neg_cond)
                            result.extend(then_result)
                            result.append(pad + "}")
                    elif then_unbrace and else_unbrace:
                        result.append(pad + "if (%s) %s" % (neg_cond,
                                                             then_result[0].strip()))
                        result.append(pad + "else %s" % else_result[0].strip())
                        result.extend(else_result[1:])
                    elif then_unbrace:
                        result.append(pad + "if (%s) %s" % (neg_cond,
                                                             then_result[0].strip()))
                        result.append(pad + "else {")
                        result.extend(else_result)
                        result.append(pad + "}")
                    elif else_unbrace:
                        result.append(pad + "if (%s) {" % neg_cond)
                        result.extend(then_result)
                        result.append(pad + "} else %s" % else_result[0].strip())
                        result.extend(else_result[1:])
                    else:
                        result.append(pad + "if (%s) {" % neg_cond)
                        result.extend(then_result)
                        result.append(pad + "} else {")
                        result.extend(else_result)
                        result.append(pad + "}")
                    i = end_idx
                    continue
            # if-then-else（else 跳出）
            if i + 1 < end and items[i + 1][0] == "goto" and tgt_idx > i + 1 and tgt_idx < end:
                out_target = items[i + 1][2]
                out_idx = labels.get(out_target, -1)
                if out_idx > tgt_idx and out_idx >= end and out_idx <= len(items):
                    is_break = is_continue = False
                    if loop_ctx and len(loop_ctx) >= 4 and loop_ctx[1] is not None:
                        if out_idx >= loop_ctx[1]:
                            is_break = True
                        elif loop_ctx[0] is not None and out_target == loop_ctx[0]:
                            is_continue = True
                    if not is_break and not is_continue and loop_ctx \
                            and len(loop_ctx) >= 4 and loop_ctx[3] is not None \
                            and out_target == loop_ctx[3]:
                        is_break = True   # switch 上下文: 跳到合并点 = break
                    if is_break or is_continue:
                        neg_cond = _negate_cond(cond)
                        keyword = "break" if is_break else "continue"
                        result.append(pad + "if (%s) %s;" % (neg_cond, keyword))
                        result.extend(_structure(items, tgt_idx + 1, end, labels,
                                                 indent, loop_ctx))
                    else:
                        then_result = _structure(items, tgt_idx + 1, end, labels,
                                                 indent + 1, loop_ctx)
                        if len(then_result) == 1 and tgt in noanchor:
                            result.append(pad + "if (%s) %s"
                                          % (cond, then_result[0].strip()))
                        elif then_result:
                            result.append(pad + "if (%s) {" % cond)
                            result.extend(then_result)
                            result.append(pad + "}")
                        else:
                            result.append(pad + "if (%s) { }" % cond)
                    i = end
                    continue
            # 标准 if-then
            if tgt_idx > i + 1 and tgt_idx <= end:
                inner_cond = _negate_cond(cond)
                then_result = _structure(items, i + 1, tgt_idx, labels,
                                         indent + 1, loop_ctx)
                if not then_result:
                    result.append(pad + "if (%s) { }" % inner_cond)
                elif len(then_result) == 1 and tgt in noanchor:
                    # 无体锚 + 单语句体 → 原源码无花括号（律#34）
                    result.append(pad + "if (%s) %s" % (inner_cond, then_result[0].strip()))
                else:
                    result.append(pad + "if (%s) {" % inner_cond)
                    result.extend(then_result)
                    result.append(pad + "}")
                i = tgt_idx
                continue
            result.append(pad + "if (%s) goto @%d;" % (cond, tgt))
            i += 1
            continue

        if kind == "goto":
            tgt = target
            tgt_idx = labels.get(tgt, -1)
            if loop_ctx and len(loop_ctx) >= 4 and loop_ctx[0] is not None and tgt == loop_ctx[0]:
                result.append(pad + "continue;")
                i += 1
                continue
            # do-while 条件区标签 = continue（律#106, monsterd.syntax_value 实证）
            if loop_ctx and len(loop_ctx) >= 7 and loop_ctx[6] and tgt in loop_ctx[6]:
                result.append(pad + "continue;")
                i += 1
                continue
            # goto→step 锚链 = continue（if 体尾 continue-goto, taskd.when_enter_world 实证）
            if loop_ctx and len(loop_ctx) >= 6 and loop_ctx[5] and tgt in loop_ctx[5]:
                result.append(pad + "continue;")
                i += 1
                continue
            if _is_foreach_continue(i, tgt_idx, loop_ctx, items):
                result.append(pad + "continue;")
                i += 1
                continue
            if loop_ctx and len(loop_ctx) >= 4 and loop_ctx[3] is not None and tgt == loop_ctx[3]:
                result.append(pad + "break;")
                i += 1
                continue
            if tgt_idx == i + 1 or (tgt_idx > i and _is_only_labels(items, i + 1, tgt_idx + 1)):
                i = tgt_idx + 1
                continue
            if tgt_idx >= end or tgt_idx < 0:
                if loop_ctx and len(loop_ctx) >= 3 and loop_ctx[1] is not None \
                        and tgt_idx >= loop_ctx[1]:
                    result.append(pad + "break;")
                    i += 1
                    continue
                result.append(pad + "goto @%d;" % tgt)
                i += 1
                continue
            result.append(pad + "goto @%d;" % tgt)
            i += 1
            continue

        if kind in ("case", "default"):
            case_list = []
            j = i
            while j < end and items[j][0] in ("case", "default"):
                case_list.append(items[j])
                j += 1
            case_targets = [c[2] for c in case_list]
            max_target = max(case_targets) if case_targets else j
            merge_target = None
            try:
                kmax = max(labels[t] for t in case_targets if t in labels)
            except ValueError:
                kmax = j - 1
            # switch 终点 = merge: 逐 case 投票 —— 只统计每个 case 体**末尾**
            # 的 goto（体末跳转 = break-to-merge）。全局 most_common 会被体内
            # 循环 continue 骗偏, min-idx 会被无关 goto 骗偏
            # （treasure_tradingd.get_order_list 实证: 8×体末 goto@573）。
            # 循环 step 标签排除（userd.internal_restore_item 实证:
            # 体末 goto@244=foreach continue 仅 1 票, kmax 过滤又把
            # 2 票的真 merge@220 排掉 → 244 误判成 merge）。
            extra5 = loop_ctx[5] if (loop_ctx and len(loop_ctx) >= 6) else None
            sorted_ct_idx = sorted(set(labels[t] for t in case_targets if t in labels))
            votes = {}
            for ci, ct_idx0 in enumerate(sorted_ct_idx):
                nxt0 = sorted_ct_idx[ci + 1] if ci + 1 < len(sorted_ct_idx) else end
                k = nxt0 - 1
                while k > ct_idx0 and items[k][0] == "label":
                    k -= 1
                if k > ct_idx0 and items[k][0] == "goto":
                    gt = items[k][2]
                    if gt in case_targets or (extra5 and gt in extra5):
                        continue
                    if labels.get(gt, -1) > kmax:
                        votes[gt] = votes.get(gt, 0) + 1
            if not votes:
                # 回退: 扫描 case 区内越过 kmax 的无条件 goto
                # （instruction_88.tactic_shihungui / serviced.has_service 实证:
                #   merge 证据 = if-链 else 跳出 / default 直跳, 体末扫描不可见）
                lo0 = sorted_ct_idx[0] if sorted_ct_idx else j
                for k in range(lo0, min(kmax + 1, end)):
                    it = items[k]
                    if it[0] != "goto":
                        continue
                    gt = it[2]
                    if gt in case_targets or (extra5 and gt in extra5):
                        continue
                    if labels.get(gt, -1) > kmax:
                        votes[gt] = votes.get(gt, 0) + 1
            if votes:
                # 平票取更早的 label: 最后 case 的体区可能过扩扫到 merge 之后的
                # goto（feed_pet._main_succ 实证 {144:1, 151:1} → 真merge=144）
                merge_target = max(votes.items(),
                                   key=lambda kv: (kv[1], -labels.get(kv[0], 0)))[0]
                switch_end = labels[merge_target]
            elif merge_target is not None and merge_target in labels:
                switch_end = labels[merge_target]
            else:
                switch_end = kmax + 1
            if switch_end > end:
                switch_end = end
            switch_expr = "/* switch_expr */"
            if i > 0 and items[i - 1][0] == "switch_marker":
                switch_expr = items[i - 1][1]
            result.append(pad + "switch (%s) {" % switch_expr)
            sorted_targets = sorted(set(case_targets))
            target_to_labels = {}
            for c in case_list:
                target_to_labels.setdefault(c[2], []).append(c)
            for ct in sorted_targets:
                ct_idx = labels.get(ct, -1)
                if ct_idx < 0:
                    continue
                next_idx = switch_end
                for other_ct in sorted_targets:
                    other_idx = labels.get(other_ct, -1)
                    if other_idx > ct_idx and other_idx < next_idx:
                        next_idx = other_idx
                block_end = next_idx
                had_merge_goto = (block_end > ct_idx and items[block_end - 1][0] == "goto"
                                  and items[block_end - 1][2] == (merge_target or -1))
                if had_merge_goto:
                    block_end -= 1
                for c in target_to_labels.get(ct, []):
                    if c[0] == "case":
                        result.append("    " * (indent + 1) + "case %s:" % c[1])
                    else:
                        result.append("    " * (indent + 1) + "default:")
                is_fall_through = (not had_merge_goto and block_end <= ct_idx + 1)
                if is_fall_through:
                    continue
                case_set = frozenset(case_targets)
                outer_loop = loop_ctx[:3] if loop_ctx else None
                # step_labels 必须透传（default 体 goto→step = continue,
                # sbonusd.get_bonus_item_info 实证, 截断会裸渲 goto）
                extra5 = loop_ctx[5] if (loop_ctx and len(loop_ctx) >= 6) else None
                case_ctx = (tuple(list(outer_loop) + [merge_target, case_set, extra5])
                            if outer_loop
                            else (None, None, None, merge_target, case_set, extra5, None))
                # 目标 label 一并交给 _structure（律#107, ganenta.calculate_bonus_amount
                # / task_monster_refreshd.place_monster 实证: case 目标即循环头,
                # 体首回边 goto→case 目标 —— 跳过 label 则循环不可见, 裸渲 goto）。
                # 无回边时 label 分支仅 i+=1 跳过, 与旧行为等价。
                body_result = _structure(items, ct_idx, block_end, labels,
                                         indent + 2, case_ctx)
                result.extend(body_result)
                # 字节码忠实: 体末的 goto@merge 是真实 break 语句（哪怕前面是 return
                # 的死代码位置）→ 无条件渲染; 无 goto = 真实 fallthrough, 不发明 break
                if had_merge_goto:
                    result.append("    " * (indent + 2) + "break;")
            result.append(pad + "}")
            i = switch_end
            continue

        if kind == "stmt" and "for_loop_init" in content:
            m = re.search(r"for_loop_init\((\w+)\): var=(\S+) iter=(\S+) end=(.+?) step=(\S+)",
                          content)
            if m:
                loop_type, var, iter_expr = m.group(1), m.group(2), m.group(3)
                end_expr, step_expr = m.group(4), m.group(5)
                if i + 1 < end and items[i + 1][0] == "ifgoto" and \
                        "loop_ctrl" in items[i + 1][1]:
                    end_target = items[i + 1][2]
                    end_idx = labels.get(end_target, -1)
                    m_lc = re.search(r"loop_ctrl_(\d+)", items[i + 1][1])
                    lc_num = m_lc.group(1) if m_lc else None
                    if end_idx > 0 and i + 2 < end and items[i + 2][0] == "label":
                        body_start_label = items[i + 2][1]
                        body_start_idx = i + 2
                        step_idx = -1
                        step_pat = ("step loop_ctrl_%s" % lc_num) if lc_num else "step loop_ctrl"
                        for k in range(body_start_idx + 1, end_idx):
                            if items[k][0] == "stmt" and step_pat in items[k][1]:
                                step_idx = k
                                break
                        if step_idx > 0:
                            ct = loop_ctx[4] if (loop_ctx and len(loop_ctx) >= 5) else None
                            # step 前的连续 label 集 = continue 的合法落点（律#2:
                            # for 的 continue 跳 step 子句; guard_intimacy 实证
                            # [if_false][goto→step][体] 被误判成 if/else 空 then）
                            step_labels = set()
                            k = step_idx - 1
                            while k > body_start_idx and items[k][0] == "label":
                                step_labels.add(items[k][1])
                                k -= 1
                            nlc = (body_start_label, end_idx, end_target, outer_brk,
                                   ct, step_labels, None)
                            if loop_type == "foreach":
                                iter_clean = iter_expr.lstrip("&")
                                result.append(pad + "loop (%s in %s) {" % (iter_clean, end_expr))
                            elif loop_type == "for_range":
                                iter_var = iter_expr.lstrip("&")
                                try:
                                    step_val = int(step_expr)
                                except (ValueError, TypeError):
                                    step_val = 0
                                init_expr = ""
                                if result:
                                    last_line = _strip_comment(result[-1])
                                    m_init = re.match(r"^(%s)\s*=\s*(.+?);\s*$"
                                                      % re.escape(iter_var), last_line)
                                    if m_init:
                                        init_expr = m_init.group(2)
                                        result.pop()
                                if step_val == 1:
                                    if init_expr:
                                        result.append(pad + "loop (%s = %s upto %s) {"
                                                      % (iter_var, init_expr, end_expr))
                                    else:
                                        result.append(pad + "loop (%s upto %s) {"
                                                      % (iter_var, end_expr))
                                elif step_val == -1:
                                    if init_expr:
                                        result.append(pad + "loop (%s = %s downto %s) {"
                                                      % (iter_var, init_expr, end_expr))
                                    else:
                                        result.append(pad + "loop (%s downto %s) {"
                                                      % (iter_var, end_expr))
                                else:
                                    cond = "%s <= %s" % (iter_var, end_expr) if step_val > 0 \
                                        else "%s >= %s" % (iter_var, end_expr)
                                    result.append(pad + "for (%s = %s; %s; %s += %s) {"
                                                  % (iter_var, init_expr or "/* init */",
                                                     cond, iter_var, step_expr))
                            else:
                                result.append(pad + "for (;;) {")
                            # 体首 label 一并交给 _structure: for 体内嵌 while 时
                            # （while 头与 for 体起点重合）, label 分支负责 while 检测
                            result.extend(_structure(items, body_start_idx, step_idx,
                                                     labels, indent + 1, nlc))
                            result.append(pad + "}")
                            i = end_idx
                            continue
            result.append(pad + content)
            i += 1
            continue

        if kind == "switch_marker":
            i += 1
            continue

        if kind == "try_catch":
            # REF 机器码形态（探针 b1/b2/a1/a2 + 已验证树实证, 律#69）:
            #   形 B: [try][A][push0][end_catch][goto][caught][pop]   → catch(A); / catch { A };
            #   形 A: [try][A][push0][end_catch][goto][caught][if!(1)→X][B][X:]
            #         → if (catch(A)) B;   （catch 表达式 = 判定值 0/1）
            catch_target = content
            catch_idx = labels.get(catch_target, -1)
            if catch_idx < 0:
                i += 1
                continue
            try_end = catch_idx
            if try_end > i + 1 and items[try_end - 1][0] == "goto":
                try_end -= 1
            body_stmts = [items[k] for k in range(i + 1, try_end)
                          if items[k][0] == "stmt" and _strip_comment(items[k][1]) not in
                          ("0", "0;", "1", "1;")]
            # merge 标签后定位机器码判定（if!(1)）或 pop 残留
            # label 跳过以 merge 标签为界（律#108, activity.reply_login_from_ccs
            # 实证: merge 后紧邻 do{}while(0) 头 label —— 无界跳过会吞掉循环头,
            # 回边 if(0)→头 落裸 if(0){} + 体内 break-goto 全悬空）
            merge_lbl = items[try_end][2] \
                if (try_end > i + 1 and items[try_end][0] == "goto") else None
            k = catch_idx + 1
            while k < end and items[k][0] == "label":
                if merge_lbl is not None and items[k][1] == merge_lbl:
                    k += 1
                    break
                k += 1
            # 形A 判定容许多余 label（形B 兜底前不吞 label, 仅前探）
            k_a = k
            while k_a < end and items[k_a][0] == "label":
                k_a += 1
            if k_a < end and items[k_a][0] == "ifgoto" \
                    and _strip_comment(items[k_a][1]) in ("!(1)", "1"):
                k = k_a
                x_idx = labels.get(items[k][2], -1)
                if x_idx > k + 1:
                    if len(body_stmts) == 1:
                        raw_a = body_stmts[0][1].rstrip()
                        expr = _strip_comment(raw_a).rstrip()
                        if expr.endswith(";"):
                            expr = expr[:-1].rstrip()
                        b_body = _structure(items, k + 1, x_idx, labels,
                                            indent + 1, loop_ctx)
                        b_body = [l for l in b_body
                                  if _strip_comment(l) not in ("0", "0;", "1", "1;")]
                        # 行号注释由 b_body 语句自带(避免双注释, login.save 实证)
                        if len(b_body) == 1 and items[k][2] in noanchor:
                            result.append(pad + "if (catch(%s)) %s"
                                          % (expr, b_body[0].strip()))
                        elif b_body:
                            result.append(pad + "if (catch(%s)) {" % expr)
                            result.extend(b_body)
                            result.append(pad + "}")
                        else:
                            result.append(pad + "if (catch(%s)) { }" % expr)
                        i = x_idx
                        continue
                    # 多语句 try 体: 退回 try/catch 渲染
                    result.append(pad + "try {")
                    result.extend(_structure(items, i + 1, try_end, labels,
                                             indent + 1, loop_ctx))
                    result.append(pad + "} catch {")
                    catch_body = _structure(items, k + 1, x_idx, labels,
                                            indent + 1, loop_ctx)
                    catch_body = [l for l in catch_body
                                  if _strip_comment(l) not in ("0", "0;", "1", "1;")]
                    result.extend(catch_body or
                                  ["    " * (indent + 1) + "/* error handled */"])
                    result.append(pad + "}")
                    i = x_idx
                    continue
            # 形 B: catch 表达式语句 / 语句级 catch
            if len(body_stmts) == 1:
                raw_b = body_stmts[0][1].rstrip()
                expr = _strip_comment(raw_b).rstrip()
                if expr.endswith(";"):
                    expr = expr[:-1].rstrip()
                m_lb = re.search(r"/\* L\d+ \*/\s*$", raw_b)
                tag_b = ("  " + m_lb.group(0)) if m_lb else ""
                result.append(pad + "catch(%s);%s" % (expr, tag_b))
            elif body_stmts:
                # 多语句 catch 体走完整结构恢复（combat.end_combat 实证:
                # 逐条 raw 输出会把体内 foreach marker/step/ifgoto 原样吐出
                # → loop 语句丢失 + step goto 悬空）
                result.append(pad + "catch {")
                catch_body = _structure(items, i + 1, try_end, labels,
                                        indent + 1, loop_ctx)
                result.extend(l for l in catch_body
                              if _strip_comment(l) not in ("0", "0;", "1", "1;"))
                result.append(pad + "};")
            else:
                result.append(pad + "catch { };")
            # 跳过 merge 标签与 pop 残留。k 已越过 merge 标签（律#108 有界扫描）,
            # 其后若紧跟真实 label（如 do{}while(0) 头）必须留给 label 分支处理,
            # 不得吞掉 —— 仅跳过 pop 残留语句。
            after_catch = k
            if after_catch < end and items[after_catch][0] == "stmt" and \
                    _strip_comment(items[after_catch][1]) in ("0", "0;", "1", "1;"):
                after_catch += 1
            i = after_catch
            continue

        if kind == "stmt":
            result.append(pad + content)
            i += 1
            continue

        result.append(pad + str((kind, content, target)))
        i += 1
    return result


def restore_control_flow(raw_lines, noanchor=None, else_anchors=None):
    """noanchor: 无体锚的 if 跳转目标集合（单语句体渲染为无花括号形态）。
    else_anchors: {else目标: (then_anchor, else_anchor)} — if/else 体的括号判定。"""
    global _NOANCHOR, _ELSE_ANCHORS
    items = _parse_lines(raw_lines)
    labels = {}
    for i, (kind, content, _t) in enumerate(items):
        if kind == "label":
            labels[content] = i
    _NOANCHOR = noanchor or set()
    _ELSE_ANCHORS = else_anchors or {}
    return _structure(items, 0, len(items), labels, 1)


_NOANCHOR = set()        # 当前函数的无体锚 if 目标集（体锚判定见 decompile_function）
_ELSE_ANCHORS = {}       # 当前函数的 if/else 体锚表


# ===========================================================================
# §7 文件渲染（GBK + CRLF 输出）
# ===========================================================================
def extract_arg_defaults(fun, info):
    """函数入口 [check_args][默认值...][resort_stack][enter_function] → 参数默认值文本。"""
    n_args = len(fun["args"])
    empty = [None] * n_args
    nodes = fun["nodes"]
    if not nodes or n_args == 0:
        return empty
    default_idxs = [i for i, a in enumerate(fun["args"]) if a[3] != 0]
    if not default_idxs:
        return empty
    ci = ri = -1
    for i in range(min(40, len(nodes))):
        opc = nodes[i][2]
        if opc == 15 and ci < 0:
            ci = i
        elif opc == 71 and ci >= 0:
            ri = i
            break
    if ci < 0 or ri < 0 or ri <= ci + 1:
        return empty
    ctx = FnCtx(fun, info)
    for pc in range(ci + 1, ri):
        p16, p8, action = nodes[pc]
        ctx.pc = pc
        ctx.signed = p16 if p16 < 0x8000 else p16 - 0x10000
        ctx.next_action = nodes[pc + 1][2] if pc + 1 < len(nodes) else -1
        handler = DT.get(action)
        if handler is None:
            return empty
        try:
            handler(ctx, p16, p8)
        except Exception:
            return empty
    if len(ctx.stack) < len(default_idxs):
        return empty
    defaults = list(empty)
    for k, arg_idx in enumerate(default_idxs):
        defaults[arg_idx] = ctx.stack[k].t
    return defaults


def format_fn_signature(fun, info, include_defaults=True):
    ret_type = TYPE_MAP.get(fun.get("ret_type", 255), "mixed")
    attrib = fun.get("attrib", 0)
    ret_attrib = fun.get("ret_attrib", 0)
    modifiers = []
    if attrib & 1:
        modifiers.append("varargs")
    if attrib & 4:
        modifiers.append("private")
    if attrib & 8:
        modifiers.append("override")
    if attrib & 0x10:
        modifiers.append("static")
    if ret_attrib & 8:
        modifiers.append("const")
    mod_str = " ".join(modifiers) + " " if modifiers else ""
    arg_defaults = extract_arg_defaults(fun, info) if include_defaults \
        else [None] * len(fun["args"])
    parts = []
    for idx, (n, t, r, _d) in enumerate(fun["args"]):
        part = "%s %s%s" % (TYPE_MAP.get(t, "mixed"), "&" if r else "", n)
        if arg_defaults[idx] is not None:
            part += " = %s" % arg_defaults[idx]
        parts.append(part)
    return "%s%s %s(%s)" % (mod_str, ret_type, fun["name"], ", ".join(parts))


def decompile_fun_safe(fun, info, with_lines):
    try:
        return decompile_function(fun, info, with_lines=with_lines)
    except Exception as exc:
        lines = ["    /* DECOMPILE FAILED: %s */" % exc,
                 "    /* original nodes (%d) raw bytecode: */" % len(fun["nodes"])]
        for _pc, (_p16, _p8, action) in enumerate(fun["nodes"]):
            lines.append("    /*   pc=%d: %s */" % (
                _pc, fmt_insn(_pc, _p16, _p8, action, fun, info)))
        return lines


def _fn_sort_key(fun):
    """函数排序（律#99）: (文件序, 行号, 偏移)。

    文件序 = __global_code__ symbols 区 flags=3 序列（include 展开序的
    .o 内嵌直接证据, start_gs.o 实证主文件天然殿后; 串池序无信息量——
    32 文件区间 27/31 相邻重叠, 且方向不可探针）。不在序列中的文件
    （无全局语句的 include）按字典序殿后。无行号者最后（按偏移）。
    单文件 .o 退化为纯行号序（原用户规则）。
    """
    sl = fun.get("src_line")
    if not sl:
        return (1, 0, "", 0, fun.get("offset", 0))
    fo = fun.get("src_file_order")
    if fo is not None:
        return (0, fo, "", sl, fun.get("offset", 0))
    return (0, 1 << 20, fun.get("src_file", "") or "", sl, fun.get("offset", 0))


def _gbk_len(s):
    try:
        return len(s.encode("gbk"))
    except UnicodeEncodeError:
        return len(s.encode("gbk", "replace"))


_NAME_MATCH_CACHE = {}


def _code_only_for_name_match(line):
    """去掉 /*..*/ 注释与 ".." 字符串内容，用于变量名匹配（防字符串内同名词误判）。"""
    s = re.sub(r"/\*.*?\*/", "", line)
    s = re.sub(r'"(?:[^"\\]|\\.)*"', '""', s)
    return s


def _place_local_declarations(body, fun):
    """局部声明按 slot 序号还原（探针 p1-p4/r1-r5 实证, bygs 与 REF 规则一致）。

    - slot 字段 = 声明序号: -1 = 最先声明; 物理表序 = 声明序倒序（头插法）。
      声明源码序 = fun['locals'] 列表倒序。
    - <loop_N> 在第 N 个 loop/for 语句处入序; foreach 迭代变量先于隐藏槽入
      (r4/r5 实证)。声明序早于 <loop_N> 的变量声明在 loop 语句行之前,
      晚于的在之后。
    - 隐式声明兜底: 钳制后声明位置越过首用 → 不渲染声明（隐式变量在首用
      语句处入序, 序号天然一致, r4 的 u 实证）。
    - 单行合并与每变量一行字节码等价（p1≡p2）; 渲染用每变量一行。
    - 全部 mixed（用户规则: 类型默认 mixed, 不猜类型）。未使用 → 顶部。
    """
    locals_list = fun["locals"]
    decl_seq = list(reversed(locals_list))      # 声明序: slot -1(最先声明)在前
    is_loop = lambda nm: re.match(r"^<loop_\d+>$", nm)
    loop_rows = [idx for idx, line in enumerate(body)
                 if re.match(r"^\s*(loop|for)\s*\(", line)]
    # 期望插入点 = 首用行; 未使用 → 0(顶部)。
    # 首用点天然满足锚约束: 声明序晚于 <loop_N> 的变量(声明在 loop 语句后),
    # 其首用必在 loop 语句后 → desired ≥ loop行+1 自动成立。
    desired = {}
    for name, _t in decl_seq:
        if is_loop(name):
            continue
        pos = 0
        for idx, line in enumerate(body):
            if re.search(r"\b%s\b" % re.escape(name), _code_only_for_name_match(line)):
                pos = idx
                break
        desired[name] = pos
    # 倒序链钳制: pos[v] = min(首用, 后邻位置); <loop_N> 锚 → bound = loop 行
    # (声明序早于 <loop_N> 的变量声明在 loop 语句行之前——含提前声明)。
    pos = {}
    bound = len(body)
    for name, _t in reversed(decl_seq):
        if is_loop(name):
            k = int(name[6:-1])
            if 1 <= k <= len(loop_rows):
                bound = loop_rows[k - 1]
            continue
        pos[name] = min(desired[name], bound)
        bound = pos[name]
    # 同点合并插入(每变量一行, 保持声明序)
    inserts = {}
    for name, _t in decl_seq:
        if is_loop(name):
            continue
        inserts.setdefault(pos[name], []).append(name)
    out = []
    for idx, line in enumerate(body):
        if idx in inserts:
            indent = line[:len(line) - len(line.lstrip())] or "    "
            for nm in inserts.pop(idx):
                out.append("%smixed %s;" % (indent, nm))
        out.append(line)
    if inserts:                                   # 钳到尾部的残余
        for p in sorted(inserts):
            for nm in inserts[p]:
                out.append("    mixed %s;" % nm)
    return out


def _find_wrap_cut(line, limit, base=0):
    """找折行切点: 深度≥1 且不在字符串内的逗号后; 取 GBK 字节长 ≤limit 的最后一个。

    base = 续行进入时的括号深度(上一段的切点深度)——续行丢失了开括号上下文,
    不带 base 会算出负深度导致找不到切点(实证 lingshou 13732 字节行)。
    返回 (切点位置, 切点处深度)。
    """
    best = None
    best_depth = base
    depth = base
    i = 0
    n = len(line)
    in_str = False
    while i < n:
        ch = line[i]
        if in_str:
            if ch == "\\":
                i += 2
                continue
            if ch == '"':
                in_str = False
        elif ch == '"':
            in_str = True
        elif ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == "," and depth >= 1:
            if _gbk_len(line[:i + 1]) <= limit:
                best = i + 1
                best_depth = depth
        i += 1
    return best, best_depth


def _wrap_long_lines(lines, limit=1900):
    """超长行折行（编译器行上限 2000 字节，探针二分实证; 留 100 余量）。

    在括号内顶层逗号处切行，续行缩进 = 语句缩进 + 4，并携带括号深度上下文。
    字符串内不切; #define / 注释行不折。真实换行已由 quote() 保留。
    """
    out = []
    for line in lines:
        cur = line
        base = 0
        while _gbk_len(cur) > limit and not cur.lstrip().startswith(("#", "//", "*")):
            cut, cut_depth = _find_wrap_cut(cur, limit, base)
            if cut is None or cut >= len(cur.rstrip()):
                break
            indent = " " * (len(cur) - len(cur.lstrip()) + 4)
            out.append(cur[:cut])
            rest = cur[cut:].lstrip()
            if not rest:
                break
            cur = indent + rest
            base = cut_depth
        out.append(cur)
    return out


def render_file(info, with_lines=True):
    """渲染整份 C 源码（文本行列表，编码/行尾由写盘处理）。"""
    out = ["// Decompiled by lpc_decompiler_v38 (LPC .o -> C)",
           "// source .o: %s" % info["path"].name,
           "// version: 0x%08X, functionBC: %d, functions: %d, DLT: %d" % (
               info["version"], info["function_bc"], len(info["functions"]),
               info["dlt_count"]),
           ""]
    if info["inherits"]:
        for inh_name, _inh_data in info["inherits"]:
            out.append('inherit "%s";' % inh_name)
        out.append("")
    if info.get("defines"):
        for _flag, name, value in info["defines"]:
            value_clean = " ".join(value.split()) if value else ""
            out.append(("#define %s %s" % (name, value_clean)).rstrip())
        out.append("")
    # 原型（带参数类型、不带默认值——默认值只写本体，用户规则；
    #   探针实证: 原型写默认值 bygs 也认，但 override 函数的默认值写在声明不认）
    funs = sorted([f for f in info["functions"]
                   if f.get("origin") not in ("global",) and f["name"] != "__global_code__"],
                  key=_fn_sort_key)
    prototypes = []
    for fun in funs:
        if len(fun["nodes"]) == 0:
            continue
        prototypes.append("%s;" % format_fn_signature(fun, info, False))
    if prototypes:
        out.extend(prototypes)
        out.append("")
    # 对象变量
    if info["object_vars"]:
        for idx, vname in enumerate(info["object_vars"]):
            vtype = info["object_var_types"][idx] if idx < len(info["object_var_types"]) else 255
            out.append("%s %s;" % (TYPE_MAP.get(vtype, "mixed"), vname))
        out.append("")
    # 全局代码段（init segment）
    for fun in info["functions"]:
        if fun.get("origin") != "global" and fun["name"] != "__global_code__":
            continue
        body = decompile_fun_safe(fun, info, with_lines)
        top = [l[4:] if l.startswith("    ") else l for l in body]
        if top and _strip_comment(top[-1]) == "return;":
            top.pop()
        out.append("// === global code segment (init) ===")
        out.extend(top)
        out.append("")
        break
    # 函数定义（按 (文件序, 行号) 排序——律#99; 多文件混合时按文件分组）
    mixed = len({f.get("src_file") for f in funs if f.get("src_file")}) > 1
    last_sf = None
    for fun in funs:
        src = ""
        if fun.get("src_line"):
            src = "  // source: %s:%d" % (fun.get("src_file", "?"), fun["src_line"])
        if mixed:
            sf = fun.get("src_file")
            if sf and sf != last_sf:
                out.append("// ==== from %s ====" % sf)
                out.append("")
                last_sf = sf
        if len(fun["nodes"]) == 0:
            out.append("%s;%s" % (format_fn_signature(fun, info, True), src))
            out.append("")
            continue
        # bygs 规则: 默认参数值只写定义行（探针实证: 原型带默认值 override 函数不认）
        sig = format_fn_signature(fun, info, True)
        out.append("%s {%s" % (sig, src))
        visible = [n for n, _t in fun["locals"] if not re.match(r"^<loop_\d+>$", n)]
        body = decompile_fun_safe(fun, info, with_lines)
        # 尾部 return; = 编译器安全网（显式/隐式编译结果相同）→ 删除（律#67）
        while body and _strip_comment(body[-1]) == "return;":
            body.pop()
        # 局部声明: 全 mixed + 按首次使用位置还原（中途声明 + loop 表序锚, 用户规则）
        body = _place_local_declarations(body, fun)
        out.extend(body)
        out.append("}")
        out.append("")
    out = _wrap_long_lines(out)
    if info.get("failures"):
        out.append("// === parse failures retained (%d) ===" % len(info["failures"]))
        for f in info["failures"]:
            out.append("// FAILURE kind=%s error=%s" % (f.get("kind"), f.get("error", "")))
        out.append("")
    return out


def write_c_file(path, lines):
    """GBK + CRLF 写盘（对齐 lib_gs32_c 形态）。"""
    data = ("\r\n".join(lines) + "\r\n").encode("gbk", errors="surrogateescape")
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_bytes(data)
    return len(data)


# ===========================================================================
# §8 归一化指令流比对（--cmp, 纯本地）
# ===========================================================================
def _norm_tokens(info, fun, strip_nops=False):
    """函数 → 归一化 token 流。

    归一化: push_s→串内容; 跳转/case→目标; call/new_func→函数名; gvar/ovar→变量名;
    push_num/push_real/case→常量值; call_ptr 的 par16 归一（探针实证 7 种写法 bygs 恒 0,
    老编译器恒 4 —— 编译器差异, 4≡0）。

    strip_nops=True: 剔除 nop token, 且跳转/case 目标重编址到去 nop 后的指令序
    （体锚数量差异 → 目标平移, 归一后可判 MOD-NOP 而非假 DIFF）。
    """
    nodes = fun["nodes"]
    meta = fun.get("meta", b"")
    # 去 nop 重编址表
    sidx = {}
    n_stripped = 0
    for pc, (_p16, _p8, op) in enumerate(nodes):
        if strip_nops and op in NOP_OPS:
            continue
        sidx[pc] = n_stripped
        n_stripped += 1

    def norm_target(t):
        if t < 0:
            return t
        while t < len(nodes) and strip_nops and nodes[t][2] in NOP_OPS:
            t += 1
        if t >= len(nodes):
            return n_stripped
        return sidx.get(t, n_stripped)

    toks = []
    for pc, (par16, par8, op) in enumerate(nodes):
        signed = par16 if par16 < 0x8000 else par16 - 0x10000
        tgt = norm_target(pc + 1 + signed)
        if op in NOP_OPS:
            if strip_nops:
                continue
            toks.append(("nop",))
        elif op in BINOPS:
            toks.append((OP_NAMES[op],))
        elif op == 64:
            strs = info["program_strings"]
            toks.append(("push_s", strs[par16] if 0 <= par16 < len(strs) else "<s%d>" % par16))
        elif op == 63:
            off = par16 | (par8 << 16)
            v = struct.unpack("<f", struct.pack("<i", struct.unpack_from("<i", meta, off)[0]))[0] \
                if off + 4 <= len(meta) else None
            toks.append(("push_real", v))
        elif op == 60:
            off = par16 | (par8 << 16)
            v = struct.unpack_from("<i", meta, off)[0] if off + 4 <= len(meta) else None
            toks.append(("push_num", v))
        elif op == 59:
            toks.append(("push_i", signed))
        elif op == 6:
            toks.append(("call_efun", efun_name(par16), par8 & 0x3F))
        elif op == 7:
            nm = resolve_global_func_name(par16) if (par8 & 0x80) else \
                resolve_inherited_func_name(info, par16)
            toks.append(("call_fn", nm or ("#%d" % par16), par8 & 0x3F))
        elif op == 23:
            nm = resolve_global_func_name(par16) if (par8 & 0x80) else \
                resolve_inherited_func_name(info, par16)
            toks.append(("new_func", nm or ("#%d" % par16), par8 & 0x3F))
        elif op == 5:
            toks.append(("call_ptr", par8))       # par16 编译器差异归一 (4≡0)
        elif op == 53 or op == 54:
            toks.append((OP_NAMES[op], GLOBAL_VAR_MAP.get(signed, "g%d" % signed)))
        elif op == 76:
            toks.append(("set_global", GLOBAL_VAR_MAP.get(signed, "g%d" % signed)))
        elif op in (55, 56, 57, 58, 77):
            toks.append((OP_NAMES[op], local_name(signed, fun)))
        elif op in (61, 62, 78):
            toks.append((OP_NAMES[op], object_var_name(signed, info)))
        elif op == 10:
            off = par16 | (par8 << 16)
            offset = struct.unpack_from("<i", meta, off)[0] if off + 4 <= len(meta) else 0
            number = struct.unpack_from("<i", meta, off + 4)[0] if off + 8 <= len(meta) else 0
            toks.append(("case_num", number, norm_target(pc + 1 + offset)))
        elif op == 11:
            off = par16 | (par8 << 16)
            offset = struct.unpack_from("<i", meta, off)[0] if off + 4 <= len(meta) else 0
            idx = struct.unpack_from("<I", meta, off + 4)[0] if off + 8 <= len(meta) else -1
            strs = info["program_strings"]
            toks.append(("case_str", strs[idx] if 0 <= idx < len(strs) else "?",
                         norm_target(pc + 1 + offset)))
        elif op in JUMP_OPS_PAR16:
            toks.append((OP_NAMES[op], tgt))
        elif op == 44:
            toks.append(("try", tgt))
        else:
            toks.append((OP_NAMES.get(op, "op%d" % op), par16, par8))
    return toks


def compare_o(ref_path, cmp_path, only_fn=None, verbose=False):
    """逐函数归一化比对。返回 (总函数数, identical, mod_nop, diff, 报告行列表)。"""
    init_global_maps()
    ref_info, cmp_info = parse_file(ref_path), parse_file(cmp_path)
    rfuncs = {f["name"]: f for f in ref_info["functions"]
              if f.get("origin") != "global" and f["name"] != "__global_code__"}
    cfuncs = {f["name"]: f for f in cmp_info["functions"]
              if f.get("origin") != "global" and f["name"] != "__global_code__"}
    report = []
    n_ident = n_mod = n_diff = 0
    for name in rfuncs:
        if only_fn and name != only_fn:
            continue
        rf, cf = rfuncs[name], cfuncs.get(name)
        if cf is None:
            report.append("MISSING  %-30s REFnc=%d  (CMP 无此函数)" % (name, len(rf["nodes"])))
            n_diff += 1
            continue
        rt = _norm_tokens(ref_info, rf)
        ct = _norm_tokens(cmp_info, cf)
        if rt == ct:
            report.append("OK       %-30s nc=%d" % (name, len(rf["nodes"])))
            n_ident += 1
            continue
        # 宽松口径: 去 nop + 跳转目标重编址（体锚数量差异 → 归一为 MOD-NOP）
        rt_l = _norm_tokens(ref_info, rf, strip_nops=True)
        ct_l = _norm_tokens(cmp_info, cf, strip_nops=True)
        if rt_l == ct_l:
            nops_r = sum(1 for t in rt if t[0] == "nop")
            nops_c = sum(1 for t in ct if t[0] == "nop")
            report.append("MOD-NOP  %-30s nc=%d/%d nop=%d/%d" % (
                name, len(rf["nodes"]), len(cf["nodes"]), nops_r, nops_c))
            n_mod += 1
            continue
        # 找首个发散点
        div = ""
        for k in range(max(len(rt_l), len(ct_l))):
            a = rt_l[k] if k < len(rt_l) else None
            b = ct_l[k] if k < len(ct_l) else None
            if a != b:
                div = " @%d ref=%s cmp=%s" % (k, a, b)
                break
        report.append("DIFF     %-30s nc=%d/%d%s" % (
            name, len(rf["nodes"]), len(cf["nodes"]), div))
        n_diff += 1
        if verbose:
            report.append("  REF: " + " | ".join(str(t) for t in rt_l[max(0, k - 3):k + 4]))
            report.append("  CMP: " + " | ".join(str(t) for t in ct_l[max(0, k - 3):k + 4]))
    for name in cfuncs:
        if name not in rfuncs and (not only_fn or name == only_fn):
            report.append("EXTRA    %-30s CMPnc=%d  (REF 无此函数)" % (name,
                          len(cfuncs[name]["nodes"])))
            n_diff += 1
    total = n_ident + n_mod + n_diff
    return total, n_ident, n_mod, n_diff, report


# ===========================================================================
# (原 §9 服务器编译探针已整体移除 —— 验证与反编译分离:
#   反编译脚本纯本地; 编译验证用独立命令/工具另行执行, 不内嵌服务器访问)
# ===========================================================================
# §10 CLI
# ===========================================================================
def _collect_o_files(paths):
    """输入路径列表 → (.o 文件列表, 目录根列表)。目录输入递归扫描 *.o（批量模式）。"""
    files, roots = [], []
    for p in paths:
        path = Path(p)
        if path.is_dir():
            roots.append(path)
            files.extend(sorted(path.rglob("*.o")))
        else:
            files.append(path)
    return files, roots


def _out_target(path, base, roots):
    """输出 .c 路径: REF_ROOT 相对优先（镜像目录结构）; 其次输入目录相对; 否则平铺。"""
    rp = path.resolve()
    try:
        rel = rp.relative_to(REF_ROOT)
        return base / rel.parent / (path.stem + ".c")
    except ValueError:
        pass
    for r in roots:
        try:
            rel = rp.relative_to(r.resolve())
            return base / rel.parent / (path.stem + ".c")
        except ValueError:
            continue
    return base / (path.stem + ".c")


def cmd_decompile(paths, out=None, with_lines=True, out_dir=None, global_file=None):
    """反编译入口。支持:
      - 单/多个 .o 文件
      - 目录输入（递归扫描 *.o, 批量镜像目录结构到 out_dir）
      - 逐文件容错（单文件失败不中断批量, 末尾汇总）
      - global_file 指定启动文件（对齐 v37 -q / v2 -g; 不指定则不加载全局表）
    """
    init_global_maps(global_file)
    files, roots = _collect_o_files(paths)
    if not files:
        print("no .o input under: %s" % ", ".join(str(p) for p in paths))
        return []
    single_out = out is not None and len(files) == 1
    base = Path(out_dir) if out_dir else DEFAULT_OUT_DIR
    results, failures = [], []
    t0 = time.time()
    for i, path in enumerate(files, 1):
        try:
            info = parse_file(path)
            blks = attach_line_info(info)
            lines = render_file(info, with_lines=with_lines)
        except Exception as exc:
            failures.append((str(path), "%s: %s" % (type(exc).__name__, exc)))
            print("FAIL %s | %s: %s" % (path, type(exc).__name__, exc), file=sys.stderr)
            continue
        target = Path(out) if single_out else _out_target(path, base, roots)
        size = write_c_file(target, lines)
        nfn = sum(1 for f in info["functions"] if len(f["nodes"]) > 0)
        # 逐文件实时输出（对齐 v37/v2 批跑行为）
        print("[%d/%d] OK %s -> %s | functions=%d(+defs) lines=%d bytes=%d lineblocks=%d" % (
            i, len(files), path.name, target, nfn, len(lines), size, blks), flush=True)
        results.append(str(target))
    dt = time.time() - t0
    print("== batch: %d/%d .o -> %s | failed=%d | %.1fs" % (
        len(results), len(files), base, len(failures), dt), flush=True)
    for fp, err in failures[:50]:
        print("  FAILED %s | %s" % (fp, err))
    if len(failures) > 50:
        print("  ... and %d more failures" % (len(failures) - 50))
    return results


def cmd_random(n=1, seed=None, out_dir=None, with_lines=True, global_file=None):
    files = sorted(str(p.relative_to(REF_ROOT)).replace("\\", "/")
                   for p in REF_ROOT.rglob("*.o"))
    if not files:
        print("no .o under %s" % REF_ROOT)
        return []
    rng = random.Random(seed)
    picks = rng.sample(files, min(n, len(files)))
    print("random pick%s (seed=%s):" % ("s" if len(picks) > 1 else "", seed))
    for p in picks:
        print("    " + p)
    return cmd_decompile([str(REF_ROOT / p) for p in picks],
                         out_dir=Path(out_dir) if out_dir else DEFAULT_OUT_DIR,
                         with_lines=with_lines, global_file=global_file)


def cmd_disasm(path, fn=None):
    init_global_maps()
    info = parse_file(path)
    attach_line_info(info)
    for fun in info["functions"]:
        if fn and fun["name"] != fn:
            continue
        src = ""
        if fun.get("src_line"):
            src = "  src=%s:%d" % (fun.get("src_file"), fun["src_line"])
        print("---- %s  nc=%d%s" % (fun["name"], len(fun["nodes"]), src))
        for pc, (par16, par8, op) in enumerate(fun["nodes"]):
            print("  [%4d] %s" % (pc, fmt_insn(pc, par16, par8, op, fun, info)))


def main():
    ap = argparse.ArgumentParser(description="LPC .o decompiler v38")
    ap.add_argument("inputs", nargs="*", help=".o 文件路径或目录（目录=递归批量）")
    ap.add_argument("-o", "--output", help="单文件输出 .c 路径")
    ap.add_argument("-k", "--out-dir", default=str(DEFAULT_OUT_DIR),
                    help="批量输出目录（默认 lizi/, 镜像输入目录结构）")
    ap.add_argument("-g", "--global-file", default=None, metavar="STARTUP.o",
                    help="指定启动文件（全局程序 .o）构建全局函数/变量名表"
                         "（对齐 v37 -q/--sefun、v2 -g/--global-file）; "
                         "不指定则不加载全局表")
    ap.add_argument("--no-lines", action="store_true", help="关闭行号估计注释")
    ap.add_argument("--disasm", metavar="FUNC", nargs="?", const="",
                    help="反汇编模式（可选函数名过滤）")
    ap.add_argument("--random", type=int, nargs="?", const=1, default=None,
                    help="从 lib_gs32 随机取 N 个 .o 反编译到输出目录")
    ap.add_argument("--seed", type=int, default=None)
    ap.add_argument("--cmp", nargs=2, metavar=("REF.o", "CMP.o"),
                    help="归一化指令流比对")
    ap.add_argument("-v", "--verbose", action="store_true")
    ns = ap.parse_args()

    with_lines = not ns.no_lines
    if ns.global_file:
        global GLOBAL_FILE_ARG
        GLOBAL_FILE_ARG = ns.global_file

    if ns.cmp:
        ref, cmp_ = ns.cmp
        init_global_maps(ns.global_file)
        total, ident, mod, diff, report = compare_o(ref, cmp_, only_fn=None,
                                                     verbose=ns.verbose)
        for l in report:
            print(l)
        print("== %d funcs: %d identical, %d mod-nop, %d diff" % (total, ident, mod, diff))
        sys.exit(0 if diff == 0 else 1)

    if ns.random is not None:
        outs = cmd_random(ns.random, ns.seed, ns.out_dir, with_lines,
                          global_file=ns.global_file)
        sys.exit(0 if outs else 1)

    if ns.disasm is not None:
        if not ns.inputs:
            print("need .o path for --disasm")
            sys.exit(2)
        init_global_maps(ns.global_file)
        cmd_disasm(ns.inputs[0], ns.disasm or None)
        sys.exit(0)

    if ns.inputs:
        cmd_decompile(ns.inputs, ns.output, with_lines, ns.out_dir,
                      global_file=ns.global_file)
        sys.exit(0)

    ap.print_help()


if __name__ == "__main__":
    main()
