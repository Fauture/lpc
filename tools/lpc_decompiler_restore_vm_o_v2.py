#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations
import argparse
import json
import os
import re
import struct
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Dict


# ---------------------------------------------------------------------------
#  Efun 名称表加载(从magic_Linux32x二进制中提取的完整547项映射)
# ---------------------------------------------------------------------------
_EFUN_NAMES: Dict[int, str] = {}

# 全局程序映射 (vm_pGlobalProgram):
#   _GLOBAL_VAR_MAP: vm_pGlobalVar[idx] -> 变量名 (action 53/54/76 使用)
#   _GLOBAL_FUNC_MAP: vm_pGlobalProgram->pFunctions[idx] -> 函数名 (action 7 bit7=1 使用)
#   _GLOBAL_EFUN_COUNT: 全局程序的 efun_count (simul_efun 区域起始)
#   _GLOBAL_FUNCTION_BC: 全局程序的 functionBC (simul_efun 区域结束)
#   simul_efun 区域 = [_GLOBAL_EFUN_COUNT, _GLOBAL_FUNCTION_BC), 这些函数从 simul_efun.c
#   源文件编译,不在 .o 文件中,无法静态解析名称。
# 通过预扫描所有 .o 文件,找到全局程序 (is_global_program=True 且有 GVAR 条目)，
# 从其 DLT GVAR 条目提取变量名,从 DLT FUNC 条目提取函数名。
_GLOBAL_VAR_MAP: Dict[int, str] = {}
_GLOBAL_FUNC_MAP: Dict[int, str] = {}
_GLOBAL_EFUN_COUNT: int = 0
_GLOBAL_FUNCTION_BC: int = 0


def _load_efun_names():
    """从efun_names_full.json加载efun索引->名称映射。"""
    global _EFUN_NAMES
    if _EFUN_NAMES:
        return _EFUN_NAMES
    candidates = [
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "efun_names_full.json"),
        "efun_names_full.json",
    ]
    for path in candidates:
        if os.path.exists(path):
            try:
                # 检测编码
                with open(path, "rb") as f:
                    head = f.read(3)
                enc = "utf-8-sig" if head == b"\xef\xbb\xbf" else "gb2312"
                with open(path, "r", encoding=enc) as f:
                    data = json.load(f)
                names = data.get("names", data) if isinstance(data, dict) else data
                for k, v in names.items():
                    _EFUN_NAMES[int(k)] = v
                print("加载efun名称表: %d 项 (来自 %s)" % (len(_EFUN_NAMES), path), file=sys.stderr)
                break
            except (OSError, json.JSONDecodeError, UnicodeError) as exc:
                print("警告: 加载efun名称表失败: %s" % exc, file=sys.stderr)
    return _EFUN_NAMES


def efun_name(index: int) -> str:
    """根据efun索引返回名称,未知则返回 efun_N。"""
    table = _load_efun_names()
    if index in table:
        return table[index]
    return "efun_%d" % index


class ParseError(Exception):
    pass


class Reader:
    def __init__(self, data, pos=0):
        self.data, self.pos = data, pos

    def need(self, n):
        if n < 0 or self.pos + n > len(self.data):
            raise ParseError("truncated at 0x%X (need %d bytes)" % (self.pos, n))

    def take(self, n):
        self.need(n)
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
            raise ParseError("invalid string length %d at 0x%X" % (size, self.pos - 4))
        raw = self.take(size)
        if raw.endswith(b"\0"):
            raw = raw[:-1]
        return raw.decode("gb2312", "replace")


def safe_name(value, fallback):
    value = value.strip().replace("\0", "")
    if not value:
        return fallback
    return value


def parse_symbol(r):
    flags = r.u8()
    source = r.string() if flags & 1 else None
    line = r.u32() if flags & 2 else None
    return {"flags": flags, "source": source, "line": line}


def parse_function(r, origin):
    start = r.pos
    raw = r.take(64)
    token = struct.unpack_from("<I", raw, 4)[0]
    attrib, locals_no = struct.unpack_from("<hh", raw, 8)
    ret_attrib, ret_type, max_arg, min_arg = raw[12:16]
    name = r.string()
    if len(name) > 512 or any(ord(c) < 32 for c in name):
        raise ParseError("implausible function name at 0x%X" % (start + 64))
    arg_count = r.i16()
    if not 0 <= arg_count <= 4096:
        raise ParseError("implausible argument count %d" % arg_count)
    args = []
    for i in range(arg_count):
        rec = r.take(20)
        arg_type = rec[5]  # byte[5] = 类型, 与 ret_type 同编码
        arg_is_ref = rec[4] != 0  # byte[4] = 引用标志 (0=值传递, 非0=引用传递 &)
        # byte[8:10] = 参数序号 (u16, 与声明顺序一致)
        # byte[10] = 默认值标志/默认值表达式占用 node 数 (0=无默认值, >0=有默认值)
        #   验证: read_etc_file(string file_name, string mode="a", string file_path="/gs/etc/")
        #         arg0 byte10=0, arg1 byte10=1, arg2 byte10=1
        #         is_limited_channel arg1 type=6(array) byte10=9 (8个push_int + create_array = 9 nodes)
        #   默认值表达式位于函数 nodes 开头 [check_args] [默认值...] [resort_stack] [enter_function]
        arg_default_nnodes = rec[10]
        args.append((safe_name(r.string(), "arg%d" % i), arg_type, arg_is_ref, arg_default_nnodes))
    local_count = r.i16()
    if not 0 <= local_count <= 4096:
        raise ParseError("implausible local count %d" % local_count)
    locals_ = []
    for i in range(local_count):
        rec = r.take(20)
        local_type = rec[5]
        locals_.append((safe_name(r.string(), "local%d" % i), local_type))
    symbol_count = r.u32()
    if symbol_count > 100000:
        raise ParseError("implausible symbol count %d" % symbol_count)
    symbols = [parse_symbol(r) for _ in range(symbol_count)]
    node_count = r.u32()
    if node_count > 1000000:
        raise ParseError("implausible node count %d" % node_count)
    # 空函数检测: name 为空且 nodes=0 且 args=0 且 locals=0
    # 这种情况通常出现在 DLT 表末尾,解析器把非函数字节误识别为空函数。
    # 抛出 ParseError 让 try_function 回退位置,避免生成虚假的 func_XXXX 函数。
    if not name and node_count == 0 and arg_count == 0 and local_count == 0:
        raise ParseError("empty function at 0x%X (name_len=0, nodes=0)" % start)
    nodes = [(r.u16(), r.u8(), r.u8()) for _ in range(node_count)]
    meta_len = r.u32()
    if meta_len > len(r.data) - r.pos:
        raise ParseError("invalid metadata length %d" % meta_len)
    meta = r.take(meta_len)
    runtime_count = r.u32()
    if runtime_count > 1000000:
        raise ParseError("implausible runtime count %d" % runtime_count)
    r.take(runtime_count * 4)
    return {"offset": start, "origin": origin, "token": token, "attrib": attrib,
            "local_no": locals_no, "ret_attrib": ret_attrib, "ret_type": ret_type,
            "max_arg": max_arg, "min_arg": min_arg, "name": safe_name(name, "func_%X" % start),
            "args": args, "locals": locals_, "symbols": symbols, "nodes": nodes, "meta": meta}


def try_function(r, origin):
    saved = r.pos
    try:
        result = parse_function(r, origin)
        return result
    except (ParseError, struct.error, UnicodeError) as exc:
        # 失败保留: 记录从 saved 到失败位置的原始字节,供后续修复使用
        # 用户要求: 失败也要保留,防止使用时出现错误导致无法修复
        failed_at = r.pos
        raw = r.data[saved:failed_at] if failed_at > saved else b""
        r.pos = saved  # 仍然回退,让后续解析继续
        return {"_failed": True, "offset": saved, "end_offset": failed_at,
                "origin": origin, "raw_bytes": raw, "error": str(exc)}


def parse_global_code(r):
    """解析全局代码段 (vm_loadCodeSegment)。

    结构:
      1. symbolsCount (u32)
      2. symbols (每个: flag(u8), [file_name(str) if flag&1], [line_no(u32) if flag&2])
      3. nodes_count (u32)
      4. nodes (nodes_count * 4 字节)
      5. meta_data_length (u32)
      6. meta_data (meta_data_length 字节)
      7. runtime_misc_count (u32)
      8. runtime_info (runtime_misc_count * 4 字节)

    若 nodes_count == 0, 仍读取 meta_data_length(=0) 和 runtime_misc_count(=0)。
    """
    start = r.pos
    symbol_count = r.u32()
    if symbol_count > 100000:
        raise ParseError("implausible global symbol count %d" % symbol_count)
    symbols = [parse_symbol(r) for _ in range(symbol_count)]
    node_count = r.u32()
    if node_count > 1000000:
        raise ParseError("implausible global node count %d" % node_count)
    nodes = [(r.u16(), r.u8(), r.u8()) for _ in range(node_count)] if node_count else []
    meta_len = r.u32()
    if meta_len > len(r.data) - r.pos:
        raise ParseError("invalid global metadata length %d" % meta_len)
    meta = r.take(meta_len) if meta_len else b""
    runtime_count = r.u32()
    if runtime_count > 1000000:
        raise ParseError("implausible global runtime count %d" % runtime_count)
    r.take(runtime_count * 4)
    return {"offset": start, "origin": "global", "token": 0, "attrib": 0,
            "local_no": 0, "ret_attrib": 0, "ret_type": 0, "max_arg": 0, "min_arg": 0,
            "name": "__global_code__", "args": [], "locals": [], "symbols": symbols,
            "nodes": nodes, "meta": meta}


# DLT entry flags (from vm_loadIdentifiers decompilation)
DLT_FLAG_GLOBAL_VAR = 0x80000    # bit 19: global variable reference
DLT_FLAG_OBJECT_VAR = 0x40000    # bit 18: object variable reference
DLT_FLAG_FUNCTION   = 0x8000000  # bit 27: function reference (followed by function body)


def parse_file(path):
    r = Reader(path.read_bytes())
    version, reserved, function_bc, define_count = r.u32(), r.u32(), r.u32(), r.u32()
    defines = []
    for _ in range(define_count):
        defines.append((r.u32(), r.string(), r.string()))
    program_strings = [r.string() for _ in range(r.u32())]
    name_count, names_blob_len = r.u32(), r.u32()
    names_blob_start = r.pos
    names_blob = r.take(names_blob_len)
    names = [x.decode("gb2312", "replace") for x in names_blob.split(b"\0") if x]
    max_variable_no, prog_var_count = r.i16(), r.i16()
    object_vars = []           # list of names (runtime index order after reorder)
    object_var_types = []      # parallel list of type bytes (vm_valueType encoding)
    prog_var_decls = []        # [(name, type, runtime_index)] in declaration order
    prog_vars_start = r.pos
    is_global_program = False
    # 全局程序 (vm_pCurrentProgram 为空时加载的第一个文件) 没有 PROG_VARS 部分,
    # 读取 max_variable_no/prog_var_count 后直接进入 FUNCTIONS 部分。
    # 检测方法: 尝试读取 PROG_VARS, 若失败则回退当作全局程序处理。
    # PROG_VARS 16 字节记录结构 (通过分析 scand.o 确认):
    #   bytes 0-3:   运行时地址/指针
    #   bytes 4-7:   常量指针 (所有变量相同)
    #   byte 8:      类型 (vm_valueType 编码: 2=int, 6=array, 7=mapping 等)
    #   byte 9:      变量标志 (0 或 1, bit0 可能表示"已初始化")
    #   bytes 10-11: runtime index (u16, 声明顺序递减: count-1, count-2, ..., 0)
    #   bytes 12-15: 运行时地址/指针
    try:
        for index in range(prog_var_count):
            rec = r.take(16)
            vtype = rec[8]
            vidx = struct.unpack_from("<H", rec, 10)[0]
            vname = safe_name(r.string(), "object_var%d" % index)
            prog_var_decls.append((vname, vtype, vidx))
    except ParseError:
        # 全局程序: 回退到 PROG_VARS 起始位置之前 (跳过整个 PROG_VARS)
        r.pos = prog_vars_start
        object_vars = []
        object_var_types = []
        is_global_program = True
    efun_count, max_function_no = r.i16(), r.i16()
    hidden_count = r.u32()
    functions = []
    for index in range(hidden_count):
        functions.append(parse_function(r, "hidden"))
    inherits = []
    max_constitution_no = 0
    if not is_global_program:
        # 普通程序: 读取 INHERITS 部分 (max_constitution_no, inherit_count, inherits)
        # 全局程序 (vm_loadInheritPrograms 中 v1 == vm_pGlobalProgram) 跳过 INHERITS
        # 注意: prog_var_count=0 的全局程序(如手动编译的test.o)无法通过PROG_VARS
        # 解析失败检测,这里添加INHERITS解析回退逻辑: 如果INHERITS解析失败,
        # 回退到当前位置当作全局程序处理(直接读DLT)
        inherits_start = r.pos
        try:
            max_constitution_no, inherit_count = r.i16(), r.u32()
            # inherit_count过大说明这不是INHERITS部分(可能是DLT)
            if inherit_count > 10000:
                raise ParseError("implausible inherit_count %d" % inherit_count)
            for _ in range(inherit_count):
                inherits.append((r.string(), tuple(r.i16() for _ in range(6))))
        except ParseError:
            # 回退: 可能是全局程序,跳过INHERITS直接到DLT
            r.pos = inherits_start
            inherits = []
            max_constitution_no = 0
            is_global_program = True
    dlt_count = r.u32()
    dlt_offsets = []
    unparsed_dlt = 0
    # failures: 所有解析失败点的原始数据,用户要求失败也要保留防止无法修复
    failures = []
    # DLT 表包含三类条目 (通过反编译 vm_loadIdentifiers 确认):
    #   1. FUNC (flag 0x8000000): 函数声明,后跟函数体
    #   2. OVAR (flag 0x40000): 对象变量声明,无函数体
    #   3. GVAR (flag 0x80000): 全局变量声明,无函数体
    # DLT 条目结构 (20 字节):
    #   bytes 0-3:  name_offset (在 names blob 中的偏移)
    #   bytes 4-7:  flags
    #   bytes 8-11: (gap, 未使用)
    #   bytes 12-15: variable/function offset
    #   bytes 16-19: extra (function index 等)
    dlt_ovar_entries = []  # (name, var_offset) - 用于校正 object_vars 顺序
    dlt_gvar_entries = []  # (name, var_offset) - 全局变量声明
    # func_index_map: pFunctions索引 -> 函数名
    # DLT 条目的 extra 字段(bytes 16-19)低15位是函数在 pFunctions 数组中的索引。
    # action 7 (call_in_user_fun) 的 par16 就是这个索引(经过 consititution->baseFunOffset 偏移后)。
    # 对于当前类定义的函数, consititution->baseFunOffset = 0 (pConsititutions[0]),
    # 所以 par16 直接等于 pFunctions 索引。
    func_index_map = {}
    for _ in range(dlt_count):
        dlt_offsets.append(r.pos)
        entry = r.take(20)
        flags = struct.unpack_from("<I", entry, 4)[0]
        if flags & DLT_FLAG_FUNCTION:
            # 函数条目: 后跟函数体
            # extra 字段(bytes 16-19)低15位 = pFunctions 索引
            func_idx = struct.unpack_from("<I", entry, 16)[0] & 0x7FFF
            fun = try_function(r, "dlt")
            if fun is None:
                unparsed_dlt += 1
            elif isinstance(fun, dict) and fun.get("_failed"):
                # 解析失败: 保留原始字节到 failures 列表
                unparsed_dlt += 1
                failures.append({"kind": "dlt_function",
                                 "dlt_offset": dlt_offsets[-1],
                                 "func_offset": fun["offset"],
                                 "end_offset": fun["end_offset"],
                                 "raw_bytes": fun["raw_bytes"],
                                 "error": fun["error"]})
            else:
                functions.append(fun)
                func_index_map[func_idx] = fun["name"]
        elif flags & (DLT_FLAG_OBJECT_VAR | DLT_FLAG_GLOBAL_VAR):
            # 变量声明条目: 无函数体,记录名称和偏移
            name_offset = struct.unpack_from("<I", entry, 0)[0]
            var_offset = struct.unpack_from("<I", entry, 12)[0]
            # 从 names blob 提取名称
            if 0 <= name_offset < len(names_blob):
                end = names_blob.find(b"\0", name_offset)
                if end < 0:
                    end = len(names_blob)
                var_name = names_blob[name_offset:end].decode("gb2312", "replace")
            else:
                var_name = "<bad offset %d>" % name_offset
            if flags & DLT_FLAG_OBJECT_VAR:
                dlt_ovar_entries.append((var_name, var_offset))
            if flags & DLT_FLAG_GLOBAL_VAR:
                dlt_gvar_entries.append((var_name, var_offset))
        else:
            # 未知类型: 保留 20 字节原始数据
            unparsed_dlt += 1
            failures.append({"kind": "unknown_dlt",
                             "dlt_offset": dlt_offsets[-1],
                             "raw_bytes": entry,
                             "flags": flags})
    # 构建 object_vars (runtime index 顺序) 和并行的 object_var_types
    # PROG_VARS 是按声明顺序排列的,runtime_index = len - 1 - decl_index
    # 所以 runtime_index 0 对应最后一个声明的变量,反转即得 runtime 顺序。
    # 类型 (byte[8]) 和 runtime_index (bytes 10-11) 来自 PROG_VARS 记录。
    if prog_var_decls:
        n = len(prog_var_decls)
        # runtime_index -> type 映射 (来自 PROG_VARS bytes 10-11)
        rt_type_map = {vidx: vtype for _n, vtype, vidx in prog_var_decls}
        if dlt_ovar_entries and len(dlt_ovar_entries) == n:
            # DLT OVAR 提供 runtime_index -> name 的权威映射
            runtime_names = [None] * n
            for name, offset in dlt_ovar_entries:
                base_var_offset = max_variable_no - n
                local_idx = offset - base_var_offset
                if 0 <= local_idx < n:
                    runtime_names[local_idx] = name
            if all(x is not None for x in runtime_names):
                object_vars = runtime_names
                object_var_types = [rt_type_map.get(i, 255) for i in range(n)]
            else:
                # DLT OVAR 不完整,回退到反转 PROG_VARS 顺序
                decl = list(reversed(prog_var_decls))
                object_vars = [d[0] for d in decl]
                object_var_types = [d[1] for d in decl]
        else:
            # 没有 DLT OVAR 条目,反转 PROG_VARS 顺序
            # (VM 的 runtime 索引总是与声明顺序相反)
            decl = list(reversed(prog_var_decls))
            object_vars = [d[0] for d in decl]
            object_var_types = [d[1] for d in decl]
    # 解析全局代码段 (vm_loadCodeSegment): 包含顶层执行的字节码
    # 结构: symbolsCount(u32) + symbols + nodes_count(u32) + nodes + meta_len(u32) + meta + runtime_count(u32) + runtime_info
    global_code = None
    if r.pos < len(r.data):
        gc_start = r.pos
        try:
            global_code = parse_global_code(r)
        except (ParseError, struct.error, UnicodeError) as exc:
            # 失败保留: 记录全局代码段的原始字节
            gc_end = r.pos
            failures.append({"kind": "global_code",
                             "offset": gc_start,
                             "end_offset": gc_end,
                             "raw_bytes": r.data[gc_start:],
                             "error": str(exc)})
            global_code = None
    if global_code is not None and global_code["nodes"]:
        functions.append(global_code)
    return {"path": path, "version": version, "reserved": reserved, "function_bc": function_bc,
            "defines": defines, "program_strings": program_strings, "name_count": name_count,
            "names": names, "max_variable_no": max_variable_no, "object_vars": object_vars,
            "object_var_types": object_var_types,
            "efun_count": efun_count, "max_function_no": max_function_no, "inherits": inherits,
            "dlt_count": dlt_count, "unparsed_dlt": unparsed_dlt, "functions": functions,
            "dlt_gvar_entries": dlt_gvar_entries, "failures": failures,
            "func_index_map": func_index_map, "is_global_program": is_global_program,
            "raw_data": r.data, "end": r.pos, "size": len(r.data)}


# ---------------------------------------------------------------------------
#  父类文件解析 (用于解析 action 7 中继承自父类的函数名)
#  当 _h_funcall 的 par16 不在当前类的 func_index_map 中时,
#  尝试加载父类 .o 文件,递归解析函数名。
# ---------------------------------------------------------------------------

# 缓存: 已加载的父类 prog dict (key=resolved_path str, val=prog dict or None)
_PARENT_PROG_CACHE: Dict[str, dict] = {}


def _resolve_inherit_path(inherit_name: str, source_path) -> "Path | None":
    """将继承路径 (如 '/inherit/misc/agent.c') 转换为本地 .o 文件路径。

    搜索策略:
      1. 去掉前导 '/', 将 '.c' 替换为 '.o'
      2. 从 source_path 所在目录开始,逐级向上搜索
      3. 也在当前工作目录下搜索
    """
    rel = inherit_name.lstrip("/")
    if rel.endswith(".c"):
        rel = rel[:-2] + ".o"
    rel_path = Path(rel)
    # 从 source_path 所在目录逐级向上搜索
    source_dir = Path(source_path).parent
    for ancestor in [source_dir] + list(source_dir.parents):
        candidate = ancestor / rel_path
        if candidate.exists():
            return candidate
    # 也在当前工作目录下搜索
    if rel_path.exists():
        return rel_path
    return None


def _load_parent_prog(inherit_name: str, source_path):
    """加载并解析父类 .o 文件 (带缓存)。

    返回 prog dict, 或 None (文件不存在/解析失败)。
    循环继承检测由 _resolve_inherited_func_name 的 visited 集合处理。
    """
    resolved = _resolve_inherit_path(inherit_name, source_path)
    if resolved is None:
        return None
    resolved_key = str(resolved)
    if resolved_key in _PARENT_PROG_CACHE:
        return _PARENT_PROG_CACHE[resolved_key]
    try:
        prog = parse_file(resolved)
        _PARENT_PROG_CACHE[resolved_key] = prog
        return prog
    except (ParseError, OSError, struct.error, UnicodeError):
        _PARENT_PROG_CACHE[resolved_key] = None
        return None


def _resolve_inherited_func_name(prog: dict, par16: int, visited: set = None) -> "str | None":
    """递归解析 par16 对应的函数名 (通过加载父类 .o 文件)。

    算法:
      1. 检查当前类的 func_index_map (DLT extra 字段,本地函数)
      2. 遍历 inherits,找到覆盖 par16 的继承:
         baseFunOffset <= par16 < baseFunOffset + expectedFunctionNo
      3. 加载父类 .o 文件,递归解析 parent_par16 = par16 - baseFunOffset
    """
    if visited is None:
        visited = set()
    source_path = prog.get("path")
    if source_path is None:
        return None
    source_key = str(source_path)
    if source_key in visited:
        return None
    visited.add(source_key)

    # 1. 检查本地 func_index_map
    func_index_map = prog.get("func_index_map", {})
    if par16 in func_index_map:
        return func_index_map[par16]

    # 2. 遍历 inherits,找到覆盖 par16 的继承
    for inh_name, inh_data in prog.get("inherits", []):
        if len(inh_data) < 6:
            continue
        base_fun = inh_data[1]       # baseFunOffset
        exp_fun = inh_data[4]        # expectedFunctionNo
        if base_fun <= par16 < base_fun + exp_fun:
            parent_par16 = par16 - base_fun
            parent_prog = _load_parent_prog(inh_name, source_path)
            if parent_prog is not None:
                name = _resolve_inherited_func_name(parent_prog, parent_par16, visited)
                if name is not None:
                    return name
            return None  # 找到了覆盖的继承但无法解析 (父类文件不存在或解析失败)
    return None


# ---------------------------------------------------------------------------
#  全局程序映射初始化 (vm_pGlobalProgram)
#  扫描所有 .o 文件,找到全局程序,构建全局变量名和全局函数名映射。
# ---------------------------------------------------------------------------

def _init_global_maps(files, global_file=None):
    """构建 _GLOBAL_VAR_MAP 和 _GLOBAL_FUNC_MAP。

    优化: 大量文件时预扫描所有 .o 文件很慢。改为指定启动文件 (global_file)。
    - 若指定 global_file: 只解析该文件作为全局程序。
    - 若未指定 global_file: 不搜索启动文件,直接返回 (全局函数/变量名无法解析)。
    """
    global _GLOBAL_VAR_MAP, _GLOBAL_FUNC_MAP, _GLOBAL_EFUN_COUNT, _GLOBAL_FUNCTION_BC
    if _GLOBAL_VAR_MAP or _GLOBAL_FUNC_MAP:
        return  # 已初始化
    best_prog = None
    if global_file is not None:
        # 显式指定启动文件: 只解析该文件
        gp = Path(global_file)
        if not gp.exists():
            print("警告: 指定的启动文件不存在: %s" % gp, file=sys.stderr)
            return
        try:
            info = parse_file(gp)
        except (ParseError, OSError, struct.error, UnicodeError) as exc:
            print("警告: 启动文件解析失败 %s: %s" % (gp, exc), file=sys.stderr)
            return
        if not info.get("is_global_program"):
            print("警告: 指定的文件不是全局程序: %s" % gp, file=sys.stderr)
            return
        best_prog = info
    else:
        # 未指定启动文件: 不搜索,直接返回
        return
    # 构建 var_offset -> name 映射
    for var_name, var_offset in best_prog.get("dlt_gvar_entries", []):
        _GLOBAL_VAR_MAP[var_offset] = var_name
    # 构建 func_idx -> name 映射 (仅 >= functionBC 的全局 LPC 函数)
    _GLOBAL_EFUN_COUNT = best_prog["efun_count"]
    _GLOBAL_FUNCTION_BC = best_prog["function_bc"]
    for func_idx, func_name in best_prog.get("func_index_map", {}).items():
        if func_idx >= _GLOBAL_FUNCTION_BC:
            _GLOBAL_FUNC_MAP[func_idx] = func_name
    print("全局程序: %s (GVAR=%d, global_funcs=%d, efun=%d, funcBC=%d, simul_efun=[%d,%d))" % (
        best_prog["path"].name, len(_GLOBAL_VAR_MAP), len(_GLOBAL_FUNC_MAP),
        _GLOBAL_EFUN_COUNT, _GLOBAL_FUNCTION_BC,
        _GLOBAL_EFUN_COUNT, _GLOBAL_FUNCTION_BC), file=sys.stderr)


def quote(value):
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def pop(stack):
    return stack.pop() if stack else "0"


def pop_args(stack, count):
    # 编译器按逆序压入参数(右到左),所以栈顶是第一个参数。
    # 弹出顺序就是源码顺序(第一个参数在前),不需要反转。
    return [pop(stack) for _ in range(count)]


def function_name(index, functions):
    if 0 <= index < len(functions):
        return functions[index]["name"]
    return "function_%d" % index


def local_name(index, fun):
    """根据本地变量索引返回名称。

    LPC VM 的本地变量布局(根据字节码分析):
      index >= 0: 从 args 和 locals 的合并列表头部开始计数
                  - index 0, 1, ... 先遍历 args, 再遍历 locals
      index <  0: 从 locals 的末尾反向计数
                  - index=-1 对应 locals 的最后一个
                  - index=-2 对应 locals 的倒数第二个
                  - 以此类推
    """
    args = fun["args"]
    locals_ = fun["locals"]
    n_args = len(args)
    n_locals = len(locals_)
    if index >= 0:
        # 正索引: 先 args 后 locals (args/locals 现在是 (name, type) 元组)
        if index < n_args:
            return args[index][0]
        local_idx = index - n_args
        if local_idx < n_locals:
            return locals_[local_idx][0]
        return "local_%d" % index
    # 负索引: 从 locals 末尾反向
    # index=-1 -> locals[n_locals-1], index=-2 -> locals[n_locals-2], ...
    reverse_idx = -index - 1  # -1 -> 0, -2 -> 1, ...
    local_idx = n_locals - 1 - reverse_idx
    if 0 <= local_idx < n_locals:
        return locals_[local_idx][0]
    # 如果超出 locals 范围,尝试 args(从末尾)
    arg_idx = n_args - 1 - reverse_idx
    if 0 <= arg_idx < n_args:
        return args[arg_idx][0]
    return "var_%d" % index


def object_var_name(index, info):
    """根据全局变量索引返回对象变量名。

    LPC VM 的对象变量布局(通过反编译 vm_loadBinaryInherit 确认):
      - .o 文件中的 object_vars 只包含本文件定义的变量
      - 全局索引 [0, baseVarOffset) 属于继承的父类
      - 全局索引 [baseVarOffset, max_variable_no) 属于本文件
      - baseVarOffset = max_variable_no - len(object_vars)
    """
    base_var_offset = info["max_variable_no"] - len(info["object_vars"])
    local_idx = index - base_var_offset
    if 0 <= local_idx < len(info["object_vars"]):
        return info["object_vars"][local_idx]
    if 0 <= index < base_var_offset:
        # 继承自父类的 object_vars(父类 .o 文件不可获取)
        return "inherited_var_%d" % index
    return "object_var_%d" % index


# ---------------------------------------------------------------------------
#  VM float decoding (par16 = byte offset into function metadata)
# ---------------------------------------------------------------------------
def _format_float(v: float) -> str:
    """格式化浮点数: 整数显示为 x.0, 其他用 %g 风格但保证有小数点。"""
    if v != v:  # NaN
        return "nan"
    if v == float("inf"):
        return "inf"
    if v == float("-inf"):
        return "-inf"
    # 整数: 显示为 x.0 以保持 float 语义
    if v == int(v) and abs(v) < 1e15:
        return "%d.0" % int(v)
    s = "%.6g" % v
    # %g 可能输出科学计数法或无小数点的形式,这里保持原样
    return s


def _meta_offset(par16: int, par8: int) -> int:
    """计算 metadata 模式下的字节偏移。

    vm_regularProcessor 中: v12 = par16 | (par8 << 16)
    这是一个 24 位偏移量,允许 metadata 段最大 16MB。
    """
    return par16 | (par8 << 16)


def _meta_dword(meta: bytes, offset: int, signed: bool = False) -> int:
    """从 metadata 读取 4 字节 DWORD。"""
    if offset < 0 or offset + 4 > len(meta):
        return 0
    fmt = "<i" if signed else "<I"
    return struct.unpack_from(fmt, meta, offset)[0]


def _meta_float(meta: bytes, offset: int) -> float:
    """从 metadata 读取 4 字节 IEEE 754 单精度浮点数。"""
    if offset < 0 or offset + 4 > len(meta):
        return 0.0
    return struct.unpack_from("<f", meta, offset)[0]


def _real_from_meta(meta: bytes, offset: int) -> str:
    """从函数 metadata 段读取 4 字节 IEEE 754 单精度浮点数。

    vm_pushRealFromWord 的 actionInfoTable attrib=0x80000008,
    bit31=1 表示 metadata 模式: par16 不是直接值,而是 metadata 段的字节偏移。
    """
    if offset < 0 or offset + 4 > len(meta):
        return "<real:meta+0x%X>" % offset
    v = struct.unpack_from("<f", meta, offset)[0]
    return _format_float(v)


# ---------------------------------------------------------------------------
#  Context dataclass & dispatch helpers
# ---------------------------------------------------------------------------
@dataclass(slots=True)
class Ctx:
    stack: list
    lines: list
    fun: dict
    prog: dict
    targets: set
    pc: int
    signed: int
    # next_action: 下一条节点的 action 号 (供赋值处理器前瞻判断: 赋值是语句还是表达式)
    # 若 next_action 为 pop_stack(51)/pop_n_stack(50), 赋值是语句 (输出行);
    # 否则赋值值会被后续表达式消费 (如函数实参), 应内联为赋值表达式不单独输出。
    next_action: int = -1
    last_assigned_value: str = ""
    # override_names: 本 .o 文件中带有 override 修饰符的函数名集合
    # 调用这些函数时需要加 :: 前缀,否则会调用自身(无限递归)
    override_names: set = field(default_factory=set)
    # pending_inc: 延迟输出的 ++/-- 语句
    # 当 inc_value 时,如果被递增变量的旧值已在栈上 (push &i, convert_to_value, push &i, inc_value),
    # 则旧值会被后续赋值语句使用 (如 matrix_list[i] = ...)。此时 i++; 应在赋值之后输出,而非之前。
    pending_inc: str = ""
    pending_inc_var: str = ""
    # pending_sc: 短路求值跟踪 (列表,支持嵌套短路)
    # 当 _h_if 的 par8 bit1=0 (不弹出条件值) 时,这是一个短路求值操作:
    #   - par8 bit0=1 (TRUE jump): left || right
    #   - par8 bit0=0 (FALSE jump): left && right
    # 模式: push left; if (left) goto @L (don't pop); pop; push right; @L:
    # 嵌套模式 A && (B || C): 外层 && 和内层 || 都以 @L 为目标,
    #   需要用列表存储多条 pending_sc,在 label 处从后往前依次合并。
    # pending_sc 存储 [(left, op, target_pc, popped), ...]
    pending_sc: list = field(default_factory=list)
    # pending_ternary: 三元运算符跟踪
    # 当 _h_if 的 par8 bit1=1 (弹出条件值) 且后续模式为三元运算符时:
    #   模式: push cond; if(pop) goto @else; push then_val; goto @end; @else: push else_val; @end:
    #   bit0=0 (FALSE jump): 结果 (cond ? then_val : else_val),cond 为 !(X) 时简化为 (X ? else_val : then_val)
    #   bit0=1 (TRUE jump):  结果 (cond ? else_val : then_val)
    # pending_ternary 存储 dict 或 None
    # 当 then 分支包含 catch 表达式副作用 (new_thread_catch...end_thread_catch...caught_error...pop_n_stack) 时:
    #   catch_lines 记录 catch 块的行 (error 调用等),catch_cond 记录条件
    #   三元完成后输出 if (cond) { catch_lines } 作为副作用语句
    pending_ternary: object = None


def _find_top_level_binop(expr: str):
    """若 expr 是 (X op Y) 形式(由 _make_binop 生成),返回最外层运算符字符串;否则返回 None。

    跳过字符串字面量内的内容,避免误匹配字符串中的运算符字符。
    """
    if not (expr.startswith("(") and expr.endswith(")")):
        return None
    inner = expr[1:-1]
    depth = 0
    in_string = False
    escape = False
    i = 0
    ops_long = ["==", "!=", "<<", ">>", "<=", ">=", "&&", "||"]
    ops_short = ["+", "-", "*", "/", "%", "&", "|", "^", "<", ">"]
    while i < len(inner):
        c = inner[i]
        if escape:
            escape = False
            i += 1
            continue
        if c == "\\":
            escape = True
            i += 1
            continue
        if c == '"':
            in_string = not in_string
            i += 1
            continue
        if in_string:
            i += 1
            continue
        if c == "(":
            depth += 1
            i += 1
        elif c == ")":
            depth -= 1
            i += 1
        elif depth == 0 and c == " ":
            # 检查后面是否是运算符(前后有空格)
            matched = None
            for op_str in ops_long + ops_short:
                end = i + 1 + len(op_str)
                if inner[i+1:end] == op_str and end < len(inner) and inner[end] == " ":
                    matched = op_str
                    break
            if matched is not None:
                return matched
            i += 1
        else:
            i += 1
    return None


def _make_binop(op: str):
    def handler(ctx: Ctx, par16: int, par8: int):
        right = pop(ctx.stack)
        left = pop(ctx.stack)
        # 扁平化链式同运算符: C 的二元运算符都是左结合,
        # 所以 (X op Y) op Z 等价于 X op Y op Z,去掉 left 外层括号可避免深嵌套。
        # 例: a + b + c + d 不会变成 (((a + b) + c) + d)
        if _find_top_level_binop(left) == op:
            left = left[1:-1]
        # 赋值表达式作为操作数时必须加括号 (LPC 中 = 优先级低于比较/算术运算符)
        if _has_top_level_assign(left):
            left = "(%s)" % left
        if _has_top_level_assign(right):
            right = "(%s)" % right
        ctx.stack.append("(%s %s %s)" % (left, op, right))
    return handler


# ---------------------------------------------------------------------------
#  Action handlers (基于 magic_Linux32x 二进制 actionInfoTable 的真实映射)
#  所有 action 号均来自二进制 ground truth,不再依赖猜测。
# ---------------------------------------------------------------------------
def _h_push_assign(ctx: Ctx, par16: int, par8: int):
    """action 3: assign_value; stack: ... value target  =>  ...value (target = value)

    vm_assignValue 弹出 target,将 value 复制到 target,value 留在栈顶。
    """
    target = pop(ctx.stack)
    if target.startswith("&"):
        target = target[1:]
    value = ctx.stack[-1] if ctx.stack else "0"

    # 检测三元表达式模式:
    # 编译器把 `target = (C) ? val1 : val2;` 编译成:
    #   if (C') goto @L1;  -- C' 成立时跳到 else 分支
    #   push val1          -- then 分支(不产生 line)
    #   goto @L2;
    #   @L1: push val2     -- else 分支(不产生 line)
    #   @L2: push &target; assign
    # 反编译器线性执行,then 和 else 的 push 都在栈上: [..., val1, val2, &target]
    # 弹出 &target 后,栈顶是 val2(else 分支),val1 还在栈上。
    # 模式(ctx.lines 最后4行): "if (C) goto @L1;", "goto @L2;", "@L1:", "@L2:"
    if len(ctx.stack) >= 1 and len(ctx.lines) >= 4:
        lines_stripped = [l.strip() for l in ctx.lines[-4:]]
        m_if = _IF_GOTO.match(lines_stripped[0])
        m_goto = _GOTO.match(lines_stripped[1])
        m_label1 = _LABEL.match(lines_stripped[2])
        m_label2 = _LABEL.match(lines_stripped[3])
        if m_if and m_goto and m_label1 and m_label2:
            cond = m_if.group(1)
            if_target = m_if.group(2)
            goto_target = m_goto.group(1)
            label1 = m_label1.group(1)
            label2 = m_label2.group(1)
            # 验证: if (C) goto @L1; goto @L2; @L1: @L2:
            if if_target == label1 and goto_target == label2:
                # 栈状态(此时): [..., val1(then), val2(else)]
                # value 指向 val2 (栈顶, else 分支)
                val2 = ctx.stack.pop()  # 弹出 else 分支的值
                val1 = ctx.stack.pop()  # 弹出 then 分支的值
                # cond 来自 "if (C) goto @L1 (else 分支)":
                #   C 成立 -> val2 (else)
                #   !C 成立 -> val1 (then)
                # target = (C) ? val2 : val1
                # 如果 C 是 !(X),简化为 target = (X) ? val1 : val2
                if cond.startswith("!(") and cond.endswith(")"):
                    inner = cond[2:-1]
                    # 若 inner 已经是 (X) 形式且平衡,去掉外层括号避免 ((X))
                    if inner.startswith("(") and inner.endswith(")"):
                        depth = 0
                        balanced = True
                        for i, ch in enumerate(inner):
                            if ch == "(":
                                depth += 1
                            elif ch == ")":
                                depth -= 1
                                if depth == 0 and i < len(inner) - 1:
                                    balanced = False
                                    break
                        if balanced:
                            inner = inner[1:-1]
                    assign_expr = "%s = (%s) ? %s : %s" % (target, inner, val1, val2)
                else:
                    assign_expr = "%s = (%s) ? %s : %s" % (target, cond, val2, val1)
                # 替换最后4行(if-goto + goto + 2个label)
                del ctx.lines[-4:]
                # 前瞻: 判断赋值是语句还是表达式
                if ctx.next_action in (50, 51):
                    # 语句: 赋值后紧跟 pop,输出为独立语句
                    ctx.stack.append(val2)
                    ctx.lines.append("    %s;" % assign_expr)
                    ctx.last_assigned_value = val2
                else:
                    # 表达式: 赋值值被后续操作消费,内联为赋值表达式
                    ctx.stack.append(assign_expr)
                return

    # 前瞻: 判断赋值是语句还是表达式
    if ctx.next_action in (50, 51):
        # 语句: 赋值后紧跟 pop_stack/pop_n_stack,输出为独立语句
        ctx.lines.append("    %s = %s;" % (target, value))
        ctx.last_assigned_value = value
    else:
        # 表达式: 赋值值被后续操作消费 (如 strlen(btime = ctime(btime, 3))),
        # 将栈顶替换为赋值表达式,不单独输出语句
        ctx.stack[-1] = "%s = %s" % (target, value)


def _h_push_assign_rev(ctx: Ctx, par16: int, par8: int):
    """action 4: assign_value_reverse; stack: ... target value  =>  ...value (target = value)

    vm_assignValueReverse 弹出 value,复制到 target,value 副本留在栈顶。
    """
    value = pop(ctx.stack)
    target = pop(ctx.stack)
    if target.startswith("&"):
        target = target[1:]
    # 前瞻: 判断赋值是语句还是表达式
    if ctx.next_action in (50, 51):
        ctx.lines.append("    %s = %s;" % (target, value))
        ctx.stack.append(value)
        ctx.last_assigned_value = value
    else:
        ctx.stack.append("%s = %s" % (target, value))


def _has_top_level_assign(s):
    """判断字符串是否为未加括号包裹的赋值表达式 (顶层含 '=' 且非比较/复合运算符)。

    用于二元运算/短路求值操作数: LPC 中 '=' 优先级低于 ==/< 等比较运算符和 &&/||,
    若操作数是内联的赋值表达式 (如 cno = assure_connection("adb")), 必须加括号:
      (cno = assure_connection("adb")) == -1   而非   cno = assure_connection("adb") == -1
    """
    depth = 0
    i = 0
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
            # 排除 ==, !=, <=, >=, +=, -=, *=, /=, %=, &=, |=, ^= 等复合/比较运算符
            prev = s[i - 1] if i > 0 else ""
            nxt = s[i + 1] if i + 1 < n else ""
            if prev in "=!<>+-*/%&|^" or nxt == "=":
                pass
            else:
                return True
        i += 1
    return False


def _h_neg(ctx: Ctx, par16: int, par8: int):
    """action 43: neg_value"""
    ctx.stack.append("-(%s)" % pop(ctx.stack))


def _h_not(ctx: Ctx, par16: int, par8: int):
    """action 45: not_value (逻辑非)"""
    ctx.stack.append("!(%s)" % pop(ctx.stack))


def _h_rev(ctx: Ctx, par16: int, par8: int):
    """action 74: rev_value (按位取反 ~)"""
    ctx.stack.append("~(%s)" % pop(ctx.stack))


def _h_arg_count(ctx: Ctx, par16: int, par8: int):
    """action 52: push_arg_num"""
    ctx.stack.append("arg_count")


def _h_global(ctx: Ctx, par16: int, par8: int):
    """action 53: push_global_var; 访问 vm_pGlobalVar[idx] 运行时全局变量表。

    通过反编译 vm_pushGlobalVar(0x809113c) 确认:
      - pValue = vm_pGlobalVar[a2].pValue  -- a2 = par16
      - vm_pGlobalVar 是运行时全局变量表,在加载 "global" 程序时填充
      - 与 .o 文件中的 object_vars 完全不同(后者通过 action 61 push_obj_var 访问)
    全局变量名通过预扫描全局程序的 DLT GVAR 条目解析 (var_offset -> name)。
    """
    idx = ctx.signed
    name = _GLOBAL_VAR_MAP.get(idx, "global_%d" % idx)
    ctx.stack.append(name)


def _h_addrof_global(ctx: Ctx, par16: int, par8: int):
    """action 54: push_global_vptr; 取 vm_pGlobalVar[idx] 的地址。

    同 action 53,访问运行时全局变量表 vm_pGlobalVar,而非 object_vars。
    """
    idx = ctx.signed
    name = _GLOBAL_VAR_MAP.get(idx, "global_%d" % idx)
    ctx.stack.append("&%s" % name)


def _h_local(ctx: Ctx, par16: int, par8: int):
    """action 55: push_local_value; action 57: push_local_vptr(本函数仅处理 55)"""
    ctx.stack.append(local_name(ctx.signed, ctx.fun))


def _h_local_vptr(ctx: Ctx, par16: int, par8: int):
    """action 57: push_local_vptr (取局部变量地址)"""
    ctx.stack.append("&%s" % local_name(ctx.signed, ctx.fun))


def _h_local_var_i(ctx: Ctx, par16: int, par8: int):
    """action 56: push_local_var_i (索引化局部变量压栈,索引来自栈顶)"""
    idx = pop(ctx.stack)
    ctx.stack.append("local_i[%s]" % idx)


def _h_local_vptr_i(ctx: Ctx, par16: int, par8: int):
    """action 58: push_local_vptr_i (索引化局部变量地址,索引来自栈顶)"""
    idx = pop(ctx.stack)
    ctx.stack.append("&local_i[%s]" % idx)


def _h_small_int(ctx: Ctx, par16: int, par8: int):
    """action 59: push_number(S)"""
    ctx.stack.append(str(ctx.signed))


def _h_long_int(ctx: Ctx, par16: int, par8: int):
    """action 60: push_number(L)

    metadata 模式(attrib=0x80000008, params=1):
      v12 = par16 | (par8 << 16)
      arg1 (value) = meta[v12] as int32
    vm_pushNumber 将其压栈,type=2 (int)。
    """
    off = _meta_offset(par16, par8)
    val = _meta_dword(ctx.fun.get("meta", b""), off, signed=True)
    ctx.stack.append("%d" % val)


def _h_obj_var(ctx: Ctx, par16: int, par8: int):
    """action 61: push_obj_var"""
    ctx.stack.append(object_var_name(ctx.signed, ctx.prog))


def _h_addrof_obj_var(ctx: Ctx, par16: int, par8: int):
    """action 62: push_obj_vptr"""
    ctx.stack.append("&%s" % object_var_name(ctx.signed, ctx.prog))


def _h_real(ctx: Ctx, par16: int, par8: int):
    """action 63: push_real_from_word

    metadata 模式(attrib=0x80000008, params=2):
      v12 = par16 | (par8 << 16)
      arg1 (firstWord) = meta[v12]   -- 4 字节,作为 float 的位模式
      arg2 (secondWord) = meta[v12+4] -- 未使用(vm_pushRealFromWord 忽略)
    """
    off = _meta_offset(par16, par8)
    ctx.stack.append(_real_from_meta(ctx.fun.get("meta", b""), off))


def _h_string(ctx: Ctx, par16: int, par8: int):
    """action 64: push_shared_string"""
    text = ctx.prog["program_strings"][par16] if par16 < len(ctx.prog["program_strings"]) else "<string:%d>" % par16
    ctx.stack.append(quote(text))


def _h_this_object(ctx: Ctx, par16: int, par8: int):
    """action 65: push_this_object"""
    ctx.stack.append("this_object()")


def _h_undefined(ctx: Ctx, par16: int, par8: int):
    """action 66: push_undefined
    vm_pushUndefined 设置 type=0, v.number=0 (IDA @0x8090f6c)
    LPC 中 undefined 是合法关键字,但在变量默认值/赋值上下文中 0 等价且更清晰
    """
    ctx.stack.append("0")


def _h_member(ctx: Ctx, par16: int, par8: int):
    """action 67: query_member (vm_queryMember); 索引操作 obj[index]

    通过反编译 vm_queryMember(0x0808fa60) 确认: 这是索引操作,不是成员访问。
    根据栈顶 obj 的类型分支处理:
      type=4 (string):  string[index]  - 字符索引
      type=5 (buffer):  buffer[index]  - 缓冲区索引
      type=6 (array):   array[index]   - 数组索引
      type=7 (mapping): mapping[key]   - 映射查找
    因此渲染为 obj[index], 不是 obj->member。
    LPC 中的 obj->method() 是 call_other, 由其他 action 实现。
    """
    member = pop(ctx.stack)
    obj = pop(ctx.stack)
    if obj.startswith("&"):
        obj = obj[1:]
    ctx.stack.append("%s[%s]" % (obj, member))


def _h_addrof_member(ctx: Ctx, par16: int, par8: int):
    """action 68: query_member_ptr (vm_queryMemberPtr); 取索引地址用于赋值

    通过反编译 vm_queryMemberPtr(0x0808f8e4) 确认: 这是索引取地址操作,用于赋值左侧。
    处理 array (type=6) 和 mapping (type=7) 的元素地址。
    渲染为 &obj[index] (或 obj[index] 若 obj 已是地址形式)。
    """
    member = pop(ctx.stack)
    obj = pop(ctx.stack)
    if obj.startswith("&"):
        # obj 已经是地址,直接拼接(&X[index] 表示该元素的地址)
        ctx.stack.append("%s[%s]" % (obj, member))
    else:
        ctx.stack.append("&%s[%s]" % (obj, member))


def _h_range(ctx: Ctx, par16: int, par8: int):
    """action 69: query_range; stack: ... val start end => ...val[start..end]

    通过反编译 vm_queryRange(0x808fd04) 确认:
      par16 = flag, bit0(flag&1): end 索引为"从末尾倒数"(<N),
              bit1(flag&2): start 索引为"从末尾倒数"(<N)。
      栈布局: [value, start, end] (end 在栈顶)
      v4 = stack[vm_sp] = end, number = stack[vm_sp+1] = start
      flag&1 → v4(end) 从末尾倒数: v4 = length - v4
      flag&2 → number(start) 从末尾倒数: number = length - number
    LPC 语法: <N 表示从末尾倒数第 N 个 (如 <1 = 最后一个元素)
    """
    end = pop(ctx.stack)
    start = pop(ctx.stack)
    value = pop(ctx.stack)
    if value.startswith("&"):
        value = value[1:]
    if par16 & 2:  # start 从末尾倒数
        start = "<%s" % start
    if par16 & 1:  # end 从末尾倒数
        end = "<%s" % end
    ctx.stack.append("%s[%s..%s]" % (value, start, end))


def _h_addrof_range(ctx: Ctx, par16: int, par8: int):
    """action 70: query_range_ptr; 同 action 69 但取地址

    par16 flag 语义同 action 69 (vm_queryRange):
      bit0: end 从末尾倒数, bit1: start 从末尾倒数
    """
    end = pop(ctx.stack)
    start = pop(ctx.stack)
    value = pop(ctx.stack)
    if par16 & 2:  # start 从末尾倒数
        start = "<%s" % start
    if par16 & 1:  # end 从末尾倒数
        end = "<%s" % end
    if value.startswith("&"):
        ctx.stack.append("%s[%s..%s]" % (value, start, end))
    else:
        ctx.stack.append("&%s[%s..%s]" % (value, start, end))


def _h_return_from_fun(ctx: Ctx, par16: int, par8: int):
    """action 73: return_from_fun (带返回值)

    若栈为空,通常是函数末尾的安全网 return(编译器生成的死代码),
    输出 "return;" 而非 "return vm_stack_underflow();"。
    注: 曾经输出 "return;  /* trailing safety return */" 注释,
    但该注释会误导(中间的 early return 也会被标记),现在统一输出 "return;",
    由 render() 的后处理负责清理函数末尾冗余的 safety return。

    return 之后栈上残留的值属于死代码(线性反编译器固有限制:
    源码中 return 之后的代码不会执行,但反编译器仍线性解析后续字节码,
    把未消费的 push 操作残留到栈上)。清空栈以避免污染 stack_at_end。
    """
    if not ctx.stack:
        ctx.lines.append("    return;")
    else:
        ctx.lines.append("    return %s;" % pop(ctx.stack))
    ctx.stack.clear()


def _h_return_from_fake_fun(ctx: Ctx, par16: int, par8: int):
    """action 72: return_from_fake_fun (无返回值,如 create() 等假函数)

    同 action 73,return 之后栈上的残留属于死代码,清空栈避免污染 stack_at_end。
    """
    ctx.lines.append("    return;")
    ctx.stack.clear()


def _h_global_assign(ctx: Ctx, par16: int, par8: int):
    """action 76: set_global_var; 写入 vm_pGlobalVar[idx],value 留在栈顶。

    同 action 53,访问运行时全局变量表 vm_pGlobalVar,而非 object_vars。
    """
    idx = ctx.signed
    name = _GLOBAL_VAR_MAP.get(idx, "global_%d" % idx)
    value = ctx.stack[-1] if ctx.stack else "0"
    if ctx.next_action in (50, 51):
        ctx.lines.append("    %s = %s;" % (name, value))
        ctx.last_assigned_value = value
    else:
        ctx.stack[-1] = "%s = %s" % (name, value)


def _h_local_assign(ctx: Ctx, par16: int, par8: int):
    """action 77: set_local_var; vm_setLocalVar 不弹栈,value 留在栈顶。"""
    value = ctx.stack[-1] if ctx.stack else "0"
    if ctx.next_action in (50, 51):
        ctx.lines.append("    %s = %s;" % (local_name(ctx.signed, ctx.fun), value))
        ctx.last_assigned_value = value
    else:
        ctx.stack[-1] = "%s = %s" % (local_name(ctx.signed, ctx.fun), value)


def _h_obj_var_assign(ctx: Ctx, par16: int, par8: int):
    """action 78: set_obj_var; 同样不弹栈,value 留在栈顶。"""
    value = ctx.stack[-1] if ctx.stack else "0"
    if ctx.next_action in (50, 51):
        ctx.lines.append("    %s = %s;" % (object_var_name(ctx.signed, ctx.prog), value))
        ctx.last_assigned_value = value
    else:
        ctx.stack[-1] = "%s = %s" % (object_var_name(ctx.signed, ctx.prog), value)


def _h_sizeof(ctx: Ctx, par16: int, par8: int):
    """action 79: sizeof"""
    ctx.stack.append("sizeof(%s)" % pop(ctx.stack))


def _h_call_fp(ctx: Ctx, par16: int, par8: int):
    """action 5: call_fun_ptr

    通过反编译 vm_callFunPtr(0x809441c) 确认:
      - par8  = 栈上总项数 = 1(函数指针) + N(参数个数), 无 flag 位, 不需掩码
      - par16 = 错误处理标志 (0=报错 "No such function.", !=0=静默失败压 undefined),
        正常执行路径不使用 par16。
      - 栈布局(顶->底): func_ptr, arg1, arg2, ..., argN
      - 调用方式: LPC 函数指针调用语法 (*func_ptr)(args)
      - 编译器按逆序压栈(右到左), pop_args 弹出顺序即源码顺序。
    """
    n_total = par8                       # 总栈项数(含函数指针), 无需 & 0x3F
    items = pop_args(ctx.stack, n_total)
    func_ptr = items[0]
    args = items[1:]
    ctx.stack.append("(*%s)(%s)" % (func_ptr, ", ".join(args)))


# 已知零参数 efun 集合:这些 efun 不接受参数,但编译器在某些上下文
# (如条件分支的死代码路径)中可能生成 par8 > 0 的字节码。
# 对于这些 efun,我们仍然弹出 par8 个栈项以保持栈平衡,但渲染时不显示参数。
# 通过反编译 vm_f_this_object(0x80f8c6c) 确认:argNum > 0 时调用 vm_error 终止,
# 但 .o 文件中的字节码仍带 par8=2(该路径为条件分支中永不执行的死代码)。
_ZERO_ARG_EFUNS = {
    380,  # this_object() - vm_f_this_object 检查 argNum > 0 则 vm_error
}


def _h_efun(ctx: Ctx, par16: int, par8: int):
    """action 6: call_in_efun (调用 efun 内置函数)

    par8 通常表示参数个数。但对于已知零参数 efun(如 this_object),
    编译器在条件分支的死代码路径中可能生成 par8 > 0 的字节码。
    此时我们仍弹出 par8 个栈项以保持栈平衡,但渲染时不显示参数。
    """
    name = efun_name(par16)
    n_pop = par8 & 0x3F
    if par16 in _ZERO_ARG_EFUNS:
        # 零参数 efun:弹出栈项以保持平衡,但不显示为参数
        pop_args(ctx.stack, n_pop)
        ctx.stack.append("%s()" % name)
    else:
        args = pop_args(ctx.stack, n_pop)
        ctx.stack.append("%s(%s)" % (name, ", ".join(args)))


def _h_funcall(ctx: Ctx, par16: int, par8: int):
    """action 7: call_in_user_fun

    通过反编译 vm_callInUserFun(0x8094080) 确认:
    par8 bit7=0: 调用 thisObject->pProgram->pFunctions[consititution->baseFunOffset + par16]
    par8 bit7=1: 调用 vm_pGlobalProgram->pFunctions[par16] (全局 LPC 函数)

    对于当前类定义的函数, consititution->baseFunOffset=0, 所以 pFunctions 索引 = par16。
    DLT 条目的 extra 字段记录了每个函数的 pFunctions 索引, 据此建立映射。
    继承的父类函数通过递归加载父类 .o 文件解析函数名。
    """
    # par8 bit7=0: 本地/继承函数调用
    if not (par8 & 0x80):
        # 先查本地 func_index_map, 再递归查父类 .o 文件
        resolved = _resolve_inherited_func_name(ctx.prog, par16)
        if resolved is not None:
            name = resolved
        else:
            # 无法解析 (父类 .o 文件不存在或函数不在继承范围内)
            name = "local_fn_%d" % par16
    else:
        # par8 bit7=1: 全局 LPC 函数 (vm_pGlobalProgram->pFunctions[par16])
        # 通过预扫描全局程序 .o 文件的 DLT FUNC 条目解析函数名。
        # simul_efun 区域 [efun_count, functionBC) 的名称从 efun_names_full.json 查询
        # (这些函数从 simul_efun.c 编译,运行时通过 vm_addEfuns 注册到 pFunctions)
        efun_table = _load_efun_names()
        if par16 < _GLOBAL_FUNCTION_BC and par16 in efun_table:
            # efun 区域 [0, functionBC): 包括常规 efun 和 simul_efun
            name = efun_table[par16]
        elif par16 in _GLOBAL_FUNC_MAP:
            name = _GLOBAL_FUNC_MAP[par16]
        else:
            name = "global_func_%d" % par16
    # override 函数自调用需加 :: 前缀,调用父类版本避免无限递归
    if name in ctx.override_names and name == ctx.fun.get("name"):
        name = "::%s" % name
    ctx.stack.append("%s(%s)" % (name, ", ".join(pop_args(ctx.stack, par8 & 0x3F))))


def _h_call_other(ctx: Ctx, par16: int, par8: int):
    """action 8: call_in_other_fun_ex -> obj->func(args...)

    通过反编译 vm_callInOtherFunEx(0x080BD0FC) 和 vm_callInOtherFun(0x08094CD8) 确认:
      - par16 = callFlags (bit0: 返回值处理, bit1: ?, bit2: 静默失败)
      - par8  = argn (额外参数个数,不包括强制的 obj + func_name)
      - 栈布局(顶->底): obj, func_name, arg1, arg2, ..., argN
      - 编译器按逆序压栈,所以弹出顺序就是源码顺序
    """
    n_extra = par8 & 0xFF  # 额外参数个数
    n_args = 2 + n_extra   # 总共弹出: obj + func_name + 额外参数
    args = pop_args(ctx.stack, n_args)
    obj = args[0]
    func = args[1]
    if func.startswith('"') and func.endswith('"'):
        func = func[1:-1]
    call_args = args[2:]
    if call_args:
        ctx.stack.append("%s->%s(%s)" % (obj, func, ", ".join(call_args)))
    else:
        ctx.stack.append("%s->%s()" % (obj, func))


def _h_os_fun(ctx: Ctx, par16: int, par8: int):
    """action 9: call_os_fun (操作符函数)"""
    os_fun_map = {
        0:  ("+", True),   1:  ("-", True),   2:  ("*", True),   3:  ("/", True),
        4:  ("%", True),   5:  ("==", True),  6:  ("!=", True),  7:  ("<", True),
        8:  (">", True),   9:  ("<=", True),  10: (">=", True),  11: ("&&", True),
        12: ("||", True),  13: ("&", True),   14: ("|", True),   15: ("^", True),
        16: ("<<", True),  17: (">>", True),  18: ("!", False),  19: ("~", False),
        20: ("++", False), 21: ("--", False),
    }
    n_args = par8 & 0x3F
    args = pop_args(ctx.stack, n_args)
    if par16 in os_fun_map:
        op, is_binop = os_fun_map[par16]
        if is_binop and n_args >= 2:
            left, right = args[0], args[1]
            # 赋值表达式作为操作数时必须加括号 (LPC 中 = 优先级低于比较/算术运算符)
            if _has_top_level_assign(left):
                left = "(%s)" % left
            if _has_top_level_assign(right):
                right = "(%s)" % right
            ctx.stack.append("(%s %s %s)" % (left, op, right))
        elif not is_binop and n_args >= 1:
            ctx.stack.append("%s(%s)" % (op, args[0]))
        else:
            ctx.stack.append("os_fun_%d(%s)" % (par16, ", ".join(args)))
    else:
        ctx.stack.append("os_fun_%d(%s)" % (par16, ", ".join(args)))


# ---------------------------------------------------------------------------
#  以下为新增的 action 处理器(对应之前缺失的 action 号)
# ---------------------------------------------------------------------------
def _emit_switch_marker_if_first(ctx: Ctx):
    """如果是 case 序列的第一个,在前面插入 switch 表达式标记。

    case_check_num/str/default 都会在序列开头调用此函数。
    判断方法: 检查 ctx.lines 最后一行是否是 case/default 语句。
    如果不是(包括 ctx.lines 为空的情况,如函数开头的 switch),
    说明这是序列的第一个,插入 /* switch: <expr> */ 标记。
    """
    if ctx.lines:
        last = ctx.lines[-1].strip()
        # case/default 语句: "case X: goto @N;" 或 "default: goto @N;"
        if last.startswith("case ") or last.startswith("default:"):
            return
    # 这是 case 序列的第一个,插入 switch 表达式标记
    # switch 表达式是当前栈顶
    if ctx.stack:
        expr = ctx.stack[-1]
        ctx.lines.append("    /* switch: %s */" % expr)


def _h_case_check_num(ctx: Ctx, par16: int, par8: int):
    """action 10: case_check_number; 匹配则出栈并跳转

    metadata 模式(attrib=0x80000001, params=2):
      v12 = par16 | (par8 << 16)
      arg1 (offset) = meta[v12]   as int32 -- 跳转偏移(有符号)
      arg2 (number) = meta[v12+4] as int32 -- case 比较值
    vm_caseCheckNumber: 若栈顶 int 等于 number,则弹出并 ip += offset。
    """
    off = _meta_offset(par16, par8)
    meta = ctx.fun.get("meta", b"")
    offset = _meta_dword(meta, off, signed=True)
    number = _meta_dword(meta, off + 4, signed=True)
    target = ctx.pc + 1 + offset
    ctx.targets.add(target)
    _emit_switch_marker_if_first(ctx)
    ctx.lines.append("    case %d: goto @%d;" % (number, target))


def _h_case_check_str(ctx: Ctx, par16: int, par8: int):
    """action 11: case_check_string; 字符串 case 匹配

    metadata 模式(attrib=0x80000001, params=2):
      v12 = par16 | (par8 << 16)
      arg1 (offset) = meta[v12]   as int32 -- 跳转偏移
      arg2 (stridx) = meta[v12+4] as int32 -- program_strings 索引
    vm_caseCheckString: 若栈顶 string 等于 program_strings[stridx],则弹出并跳转。
    """
    off = _meta_offset(par16, par8)
    meta = ctx.fun.get("meta", b"")
    offset = _meta_dword(meta, off, signed=True)
    stridx = _meta_dword(meta, off + 4, signed=False)
    target = ctx.pc + 1 + offset
    ctx.targets.add(target)
    _emit_switch_marker_if_first(ctx)
    strs = ctx.prog.get("program_strings", [])
    text = strs[stridx] if 0 <= stridx < len(strs) else "<string:%d>" % stridx
    ctx.lines.append('    case %s: goto @%d;' % (quote(text), target))


def _h_case_default(ctx: Ctx, par16: int, par8: int):
    """action 12: case_default; 出栈并跳转

    非 metadata 模式(attrib=0x00000001, params=2):
      arg1 (offset) = par16 as int16 (signed)
      arg2 = par8 (未使用)
    """
    target = ctx.pc + 1 + ctx.signed
    ctx.targets.add(target)
    _emit_switch_marker_if_first(ctx)
    # case_default 会弹出 switch 表达式
    pop(ctx.stack)
    ctx.lines.append("    default: goto @%d;" % target)


def _h_cast_type(ctx: Ctx, par16: int, par8: int):
    """action 13: cast_type; par16=目标类型,栈顶值就地转换"""
    type_map = {2: "int", 3: "float", 4: "string", 5: "buffer", 6: "array", 7: "mapping"}
    target_type = type_map.get(par16, "type_%d" % par16)
    val = pop(ctx.stack)
    ctx.stack.append("(%s)%s" % (target_type, val))


def _h_caught_error(ctx: Ctx, par16: int, par8: int):
    """action 14: caught_error

    通过反编译 vm_caughtError(0x80ab020) 确认: 只是 push 1。
    这是 try-catch 的 catch 块开头,编译器生成的内部 action,不是 LPC 源码函数。
    catch 块中 push 1 用于标记已捕获错误,在 _structure 中被 try-catch 模式消费。
    静默处理,不输出 (由 try-catch 模式处理)。
    """
    ctx.stack.append("1")


def _h_check_args(ctx: Ctx, par16: int, par8: int):
    """action 15: check_args_num

    编译器生成的运行时参数数量检查,在源码中无对应语句。
    par16=最小参数数量,不足时运行时报错。
    静默处理,不输出任何内容。
    """
    pass


def _h_check_is_ref_type(ctx: Ctx, par16: int, par8: int):
    """action 16: check_is_ref_type; par16 = 变量索引

    通过反编译 vm_checkIsRefType(0x8091750) 确认:
      - par16(a1) 是变量索引,不是从求值栈弹出
      - 函数读取 vm_stack[bp + a1] 的 type 字段判断是否为引用类型(type==8)
      --vm_sp 是帧级 sp 调整(包含 args/locals),不影响求值栈
    check_is_ref_type 是 VM 内部操作码,不是 LPC 可调用函数。
    编译器为 & 参数自动生成此检查,源码中不可见。
    反编译输出 1 (true),因为参数已声明为 &,运行时通常为引用。
    """
    ctx.stack.append("1")


def _h_check_is_ref_type_i(ctx: Ctx, par16: int, par8: int):
    """action 17: check_is_ref_type_i; 变量索引来自栈顶

    通过反编译 vm_checkIsRefTypeI(0x8091850) 确认:
      - 从栈顶弹出索引值(vm_getValueFromStack),减 1 后作为变量索引
      - 然后调用 vm_checkIsRefType(index) 检查该变量
    同 action 16,输出 1 (VM 内部操作码,非 LPC 函数)。
    """
    pop(ctx.stack)
    ctx.stack.append("1")


def _h_cond_jmp_for_loop(ctx: Ctx, par16: int, par8: int):
    """action 18: cond_jmp_for_loop; 循环条件检查

    通过反编译 vm_condJmpForLoop(0x080BCDC8) 和 vm_cmpAndJmpInLoop(0x80bcae0) 确认:
      - par16 = continueOffset (条件满足时跳回循环体)
      - par8  = varNo (循环控制变量的本地索引)
    vm_cmpAndJmpInLoop 从 vm_stack[varNo+bp] 读取 loop_ctrl(type=20),
    检查迭代器是否还有元素:
      - 有: ip += continueOffset (继续循环)
      - 无: ip += 0 (跳出循环,fall through)
    注意:不弹出 eval stack,循环控制存储在本地变量中。
    """
    target = ctx.pc + 1 + ctx.signed
    ctx.lines.append("    if (loop_ctrl_%d) goto @%d;  /* loop continue */" % (par8, target))


def _h_convert_to_ptr(ctx: Ctx, par16: int, par8: int):
    """action 19: convert_to_ptr; 值转引用"""
    val = pop(ctx.stack)
    ctx.stack.append("(&%s)" % val)


def _h_convert_to_value(ctx: Ctx, par16: int, par8: int):
    """action 20: convert_to_value; 引用转值"""
    val = pop(ctx.stack)
    if val.startswith("&"):
        ctx.stack.append(val[1:])
    else:
        ctx.stack.append("(*%s)" % val)


def _h_create_argument_array(ctx: Ctx, par16: int, par8: int):
    """action 21: create_argument_array; 将当前函数参数打包成数组

    LPC 语法: $< 是 varargs 函数中引用可变参数的伪变量。
    """
    ctx.stack.append("$<")


def _h_create_array(ctx: Ctx, par16: int, par8: int):
    """action 22: create_array; par16=元素个数,弹出 N 个元素构造数组

    编译器按逆序压栈(最后一个元素先压),弹出顺序即源码顺序。
    LPC 数组字面量语法: ({ elem1, elem2, ... })
    """
    count = par16
    elements = pop_args(ctx.stack, count)
    ctx.stack.append("({%s})" % ", ".join(elements))


def _h_create_function_value(ctx: Ctx, par16: int, par8: int):
    """action 23: create_function_value; 函数值(闭包)

    par8 bit7=0: 本地/继承函数 (通过 _resolve_inherited_func_name 解析,使用 func_index_map)
    par8 bit7=1: 全局函数 (simul_efun 区域 [efun_count, functionBC) 或全局 LPC 函数)
    par8 & 0x3F = 闭包绑定的参数个数

    LPC 函数指针语法:
      无参数: (: func_name :)
      有参数: (: func_name, arg1, arg2 :)
    """
    n_args = par8 & 0x3F
    args = pop_args(ctx.stack, n_args)
    efun_table = _load_efun_names()
    if not (par8 & 0x80):
        # bit7=0: 本地/继承函数,使用 func_index_map 解析 (与 action 7 相同)
        resolved = _resolve_inherited_func_name(ctx.prog, par16)
        if resolved is not None:
            name = resolved
        else:
            name = "local_fn_%d" % par16
    else:
        # bit7=1: 全局函数 (efun + simul_efun + 全局 LPC 函数)
        if par16 < _GLOBAL_FUNCTION_BC and par16 in efun_table:
            # efun 区域 [0, functionBC): 包括常规 efun 和 simul_efun
            name = efun_table[par16]
        elif par16 in _GLOBAL_FUNC_MAP:
            name = _GLOBAL_FUNC_MAP[par16]
        else:
            name = "global_func_%d" % par16
    # override 函数自调用(函数指针形式)需加 :: 前缀,调用父类版本避免无限递归
    if name in ctx.override_names and name == ctx.fun.get("name"):
        name = "::%s" % name
    # LPC 函数指针语法: (: name :) 或 (: name, arg1, arg2 :)
    if not args:
        ctx.stack.append("(: %s :)" % name)
    else:
        ctx.stack.append("(: %s, %s :)" % (name, ", ".join(args)))


def _h_create_mapping(ctx: Ctx, par16: int, par8: int):
    """action 24: create_mapping; par16=栈项总数(键+值合计),弹出 par16 个值构造映射

    通过反编译 vm_createMapping(0x8091af0) 确认:
      - pValue = par16 = 栈项总数(不是对数)
      - m = vm_allocateMapping(pValue >> 1)  -- 分配 pValue/2 个槽位
      - 循环 pValue/2 次,每次消费 2 个栈项(key, value)
      - vm_stack[vm_sp] 为 key(栈顶),vm_stack[vm_sp+1] 为 value;v6 从栈顶递增,
        即从最后一对向第一对处理。
    编译器对每对按 value-first 压栈(value_k, key_k),按源码顺序逐对压入,
    故栈顶为最后一对的 key。pop_args 弹出顺序为 [keyN, valN, ..., key0, val0]
    (逆序),需反转 pairs 恢复源码顺序 (key0:val0, ..., keyN:valN)。
    """
    total_count = par16
    items = pop_args(ctx.stack, total_count)
    pairs = []
    for i in range(0, len(items), 2):
        if i + 1 < len(items):
            pairs.append("%s: %s" % (items[i], items[i + 1]))
        else:
            pairs.append("%s: <missing_value>" % items[i])
    pairs.reverse()  # pop 顺序为逆序(最后一对在前),反转恢复源码顺序
    ctx.stack.append("([%s])" % ", ".join(pairs))


def _h_end_thread_catch(ctx: Ctx, par16: int, par8: int):
    """action 26: end_thread_catch

    通过反编译 vm_endThreadCatch(0x80aae08) 确认: 注销 catch handler。
    LPC 语义: try 块正常结束,由 _structure 的 try-catch 模式消费。
    静默处理,不输出。
    """
    pass


def _h_enter_function(ctx: Ctx, par16: int, par8: int):
    """action 27: enter_function; par16=minArgNum,为缺失参数压入 undefined

    通过反编译 vm_enterFunction(0x80955E8) 和 vm_resortStack(0x80954A8) 确认:
      - 在 enter_function 之前,编译器会压入默认参数值和初始局部变量
      - resort_stack 会将这些值与实际传入的参数合并重排
      - enter_function 之后,这些值通过 local_name(index) 访问,不需要留在虚拟栈上
    因此在 enter_function 时清空虚拟栈,消除编译器初始化代码的栈残留。
    """
    ctx.stack.clear()


def _h_finish_action(ctx: Ctx, par16: int, par8: int):
    """action 29: finish_action (无操作)"""
    pass


def _h_if(ctx: Ctx, par16: int, par8: int):
    """action 33: if; 条件跳转

    通过反编译 vm_if (0x0809746C) 确认 par8 flags 语义:
      跳转条件 = (vm_isZero(cond) && bit0==0) || (!vm_isZero(cond) && bit0==1)
      即:
        bit0=0: 条件为 FALSE (zero) 时跳转
        bit0=1: 条件为 TRUE (non-zero) 时跳转
      bit1=0: 不弹出条件值 (条件留在栈上供后续使用) -> 短路求值模式
      bit1=1: 弹出条件值 -> 普通 if 语句
    """
    cond = ctx.stack[-1] if ctx.stack else "0"

    # bit1=0: 短路求值模式 (|| 或 &&)
    # 模式: push left; if (left) goto @L (don't pop); pop; push right; @L:
    # 结果: left || right (bit0=1, TRUE jump) 或 left && right (bit0=0, FALSE jump)
    # 嵌套: A && (B || C) 时,外层 && 和内层 || 都追加到 pending_sc 列表,
    #   在 label 处从后往前依次合并。
    if not (par8 & 2):
        target = ctx.pc + 1 + ctx.signed
        op = "||" if (par8 & 1) else "&&"
        # 不输出 if-goto,不弹出 cond,追加到 pending_sc 列表供后续 pop_stack 和 label 处理
        ctx.pending_sc.append((cond, op, target, False))
        ctx.last_assigned_value = cond  # 抑制后续 pop_stack 的输出
        return

    # bit1=1: 普通 if 语句,弹出条件值
    pop(ctx.stack)
    target = ctx.pc + 1 + ctx.signed
    if par8 & 1:
        # bit0=1: 条件为 TRUE 时跳转
        ctx.lines.append("    if (%s) goto @%d;" % (cond, target))
    else:
        # bit0=0: 条件为 FALSE 时跳转 -> if (!(cond))
        # 简化: 若 cond 已经是 !(x) 形式,则 !(!(x)) 等价于 x
        if cond.startswith("!(") and cond.endswith(")"):
            ctx.lines.append("    if (%s) goto @%d;" % (cond[2:-1], target))
        else:
            ctx.lines.append("    if (!(%s)) goto @%d;" % (cond, target))


def _h_inc_value(ctx: Ctx, par16: int, par8: int):
    """action 34: inc_value; par16=增量(通常 1 或 -1)

    修改目标后弹出目标指针,常用于 ++/-- 语句。
    后自增模式: push &i, convert_to_value(旧值), push &i, inc_value
    此时栈顶是旧值,应替换为 i++ 表达式,保留在索引/表达式中。
    独立递增: 栈顶不是旧值,直接输出 i++; 语句。
    """
    target = pop(ctx.stack)
    if target.startswith("&"):
        target = target[1:]
    inc = ctx.signed
    # 检查后自增模式: 栈顶是否是被递增变量的旧值
    if ctx.stack and ctx.stack[-1] == target:
        if inc == 1:
            ctx.stack[-1] = "%s++" % target
        elif inc == -1:
            ctx.stack[-1] = "%s--" % target
        ctx.last_assigned_value = target
        return
    if inc == 1:
        ctx.lines.append("    %s++;" % target)
        ctx.last_assigned_value = target
    elif inc == -1:
        ctx.lines.append("    %s--;" % target)
        ctx.last_assigned_value = target
    else:
        ctx.lines.append("    %s += %d;" % (target, inc))
        ctx.last_assigned_value = target


def _h_init_for_loop(ctx: Ctx, par16: int, par8: int):
    """action 35: init_for_loop; par16=varNo(循环变量索引)

    通过反编译 vm_initForLoop(0x080BC8F0) 确认:
      - par16 = varNo (循环控制变量的本地索引,存储 loop_ctrl type=20)
      - 弹出 3 个值(栈顶到栈底): step, end, iterator
      - 创建 loop_ctrl 结构存入 vm_stack[varNo+bp]
    支持:
      - 整数范围循环: step=1/-1, end=整数, iterator=&i
        条件: step=1 时 i<=end 继续; step=-1 时 i>=end 继续
      - 数组遍历: step=997, end=数组, iterator=&var
      - 映射遍历: step=997, end=映射, iterator=&var
    """
    step_str = pop(ctx.stack)
    end_str = pop(ctx.stack)
    iterator = pop(ctx.stack)
    var = local_name(ctx.signed, ctx.fun)
    # 判断循环类型
    try:
        step_val = int(step_str)
        if step_val == 997:
            loop_type = "foreach"
        else:
            loop_type = "for_range"
    except (ValueError, TypeError):
        loop_type = "for_loop"
    ctx.lines.append("    /* for_loop_init(%s): var=%s iter=%s end=%s step=%s */" % (
        loop_type, var, iterator, end_str, step_str))


def _h_is_type(ctx: Ctx, par16: int, par8: int):
    """action 36: is_type; par16=类型号,栈顶替换为布尔值

    类型常量(来自 vm_valueType):
      0=undefined, 1=void, 2=integer, 3=float, 4=string, 5=buffer,
      6=array, 7=mapping, 8=pointer, 9=function, 10=object,
      20=loop_ctrl, 255=mixed

    LPC 中通常使用对应的 efun (is_array, is_mapping 等) 而非 is_type(X, T)。
    当类型有对应 efun 时,渲染为 is_T(X) 形式。
    efun 对照 (来自 efun_names_full.json):
      is_array(65), is_buffer(66), is_int(68), is_function(69),
      is_mapping(70), is_object(71), is_real(72), is_string(73),
      is_undefined(74), is_void(75)
    """
    # 类型号 -> 对应的 LPC efun 名称 (仅当有对应 efun 时)
    type_to_efun = {
        0: "is_undefined",   # undefined
        1: "is_void",        # void
        2: "is_int",         # integer
        3: "is_real",        # float (LPC 中 is_real 检查浮点)
        4: "is_string",      # string
        5: "is_buffer",      # buffer
        6: "is_array",       # array
        7: "is_mapping",     # mapping
        9: "is_function",    # function
        10: "is_object",     # object
    }
    # 无对应 efun 的类型用 type_N 形式
    type_names = {
        8: "pointer", 20: "loop_ctrl", 255: "mixed",
    }
    val = pop(ctx.stack)
    if par16 in type_to_efun:
        ctx.stack.append("%s(%s)" % (type_to_efun[par16], val))
    else:
        target_type = type_names.get(par16, "type_%d" % par16)
        ctx.stack.append("is_type(%s, %s)" % (val, target_type))


def _h_new_thread_catch(ctx: Ctx, par16: int, par8: int):
    """action 44: new_thread_catch

    通过反编译 vm_newThreadCatch(0x80aaf84) 确认:
      v6->catchIp = vm_currentContext->ip + par16
    其中 ip 是执行完 new_thread_catch 后的 pc (即 pc+1)。
    catch_target = pc + 1 + signed(par16)
    catch_target 处是 catch 块的开始 (通常是 caught_error() action 14)。

    LPC 语义: try { ... } catch { ... }
      - new_thread_catch 注册 catch handler
      - try 块中的语句
      - end_thread_catch 注销 catch handler (正常结束)
      - catch_target 处的 caught_error() (push 1) 标记 catch 块开始
    """
    catch_target = ctx.pc + 1 + ctx.signed
    ctx.targets.add(catch_target)
    ctx.lines.append("    /* new_thread_catch -> @%d */" % catch_target)


def _h_null_action(ctx: Ctx, par16: int, par8: int):
    """action 46: null_action (完全无操作)"""
    pass


def _h_op_eq(ctx: Ctx, par16: int, par8: int):
    """action 47: op_eq_value; 复合赋值 (op=)

    par16=操作类型: 2=+, 3=-, 4=*, 5=/, 6=&, 7=|, 8=^, 9=>>, 10=<<, 11=%
    vm_opEqValue 弹出 value 和 target,执行 op=,结果(target 的新值)留在栈顶。
    """
    op_map = {2: "+", 3: "-", 4: "*", 5: "/", 6: "&", 7: "|", 8: "^", 9: ">>", 10: "<<", 11: "%"}
    op = op_map.get(par16, "?")
    value = pop(ctx.stack)
    target = pop(ctx.stack)
    if target.startswith("&"):
        target = target[1:]
    ctx.lines.append("    %s %s= %s;" % (target, op, value))
    # 复合赋值的结果是 target 的新值,压回栈顶
    ctx.stack.append(target)
    ctx.last_assigned_value = target  # 抑制后续 pop_stack 的重复输出


def _h_pickup_os_value(ctx: Ctx, par16: int, par8: int):
    """action 49: pickup_os_value

    metadata 模式(attrib=0x80000000, params=2):
      v12 = par16 | (par8 << 16)
      arg1 (ptr)  = meta[v12]   -- 数据指针(运行时地址)
      arg2 (len)  = meta[v12+4] -- 读取长度(1/2/4 字节)
    vm_pickupOSValue 按 len 读取 ptr 处的值并压栈(type=int)。
    由于 ptr 是运行时地址,反编译时无法解析具体值,保留为占位。
    """
    off = _meta_offset(par16, par8)
    meta = ctx.fun.get("meta", b"")
    length = _meta_dword(meta, off + 4, signed=False)
    ctx.stack.append("pickup_os_value(meta+0x%X, %d)" % (off, length))


def _h_pop_n_stack(ctx: Ctx, par16: int, par8: int):
    """action 50: pop_n_stack; par16=弹出数量"""
    for _ in range(par16):
        if ctx.stack:
            val = pop(ctx.stack)
            # 跳过已被赋值语句渲染的重复值
            if val == ctx.last_assigned_value:
                ctx.last_assigned_value = ""
                continue
            ctx.lines.append("    %s;" % val)


def _h_pop_stack(ctx: Ctx, par16: int, par8: int):
    """action 51: pop_stack; 弹出一个值作为表达式语句

    若栈顶值刚刚被赋值语句渲染(如 assign_value 后的 pop_stack),
    则跳过以避免重复输出。
    若栈顶值是短路求值的左操作数(pending_sc),标记 popped=True 并抑制输出。
    """
    val = pop(ctx.stack)
    # 短路求值: pop 的是左操作数,标记最后一条 pending_sc 为 popped=True,不输出
    if ctx.pending_sc:
        left, op, target, popped = ctx.pending_sc[-1]
        if val == left and not popped:
            ctx.pending_sc[-1] = (left, op, target, True)
            ctx.last_assigned_value = ""
            return
    if val == ctx.last_assigned_value:
        ctx.last_assigned_value = ""
        return
    ctx.lines.append("    %s;" % val)


def _h_resort_stack(ctx: Ctx, par16: int, par8: int):
    """action 71: resort_stack (栈重排,VM 内部操作)"""
    pass


def _h_step_jmp_for_loop(ctx: Ctx, par16: int, par8: int):
    """action 80: step_jmp_for_loop; for 循环步进+条件检查

    通过反编译 vm_stepJmpForLoop(0x080BCDB0) 确认:
      - par16 = continueOffset (步进后条件满足时跳回循环体)
      - par8  = varNo (循环控制变量索引)
    vm_cmpAndJmpInLoop 先步进迭代器,再检查是否继续:
      - 继续: ip += continueOffset
      - 结束: ip += 0 (fall through)
    """
    target = ctx.pc + 1 + ctx.signed
    ctx.lines.append("    /* step loop_ctrl_%d */ goto @%d;  /* loop step */" % (par8, target))


def _h_to_reserve(ctx: Ctx, par16: int, par8: int):
    """action 82: to_reserve_stack_value"""
    pass


def _h_to_unreserve(ctx: Ctx, par16: int, par8: int):
    """action 83: to_unreserve_stack_value"""
    pass


def _h_trace(ctx: Ctx, par16: int, par8: int):
    """action 84: trace"""
    ctx.lines.append("    /* trace(%d) */" % par16)


def _h_goto(ctx: Ctx, par16: int, par8: int):
    """action 31: goto; par16=offset(有符号)"""
    ctx.lines.append("    goto @%d;" % (ctx.pc + 1 + ctx.signed))


def _h_unknown(ctx: Ctx, par16: int, par8: int):
    """fallback for unrecognized actions"""
    pass  # 由调用方通过 ctx.lines.append(...) 处理


# ---------------------------------------------------------------------------
#  Dispatch table (基于二进制 actionInfoTable 的真实 action 号映射)
# ---------------------------------------------------------------------------
DT: Dict[int, Callable] = {
    # 二元运算(无参数,pops 2 pushes 1)
    1:  _make_binop("+"),       # add_value
    2:  _make_binop("&"),       # and_value (按位与/集合交集)
    25: _make_binop("/"),       # div_value
    28: _make_binop("=="),      # eq_value
    30: _make_binop(">="),      # ge_value
    32: _make_binop(">"),       # gt_value
    37: _make_binop("<="),      # le_value
    38: _make_binop("<<"),      # lsh_value
    39: _make_binop("<"),       # lt_value
    40: _make_binop("%"),       # mod_value
    41: _make_binop("*"),       # mul_value
    42: _make_binop("!="),      # ne_value
    48: _make_binop("|"),       # or_value (按位或)
    75: _make_binop(">>"),      # rsh_value
    81: _make_binop("-"),       # sub_value
    85: _make_binop("^"),       # xor_value

    # 一元运算
    43: _h_neg,                 # neg_value
    45: _h_not,                 # not_value (逻辑非)
    74: _h_rev,                 # rev_value (按位取反)

    # 赋值
    3:  _h_push_assign,         # assign_value (... value target => target = value)
    4:  _h_push_assign_rev,     # assign_value_reverse (... target value => target = value)
    47: _h_op_eq,               # op_eq_value (复合赋值 op=)
    34: _h_inc_value,           # inc_value (++/--)

    # 函数调用
    5:  _h_call_fp,             # call_fun_ptr
    6:  _h_efun,                # call_in_efun
    7:  _h_funcall,             # call_in_user_fun
    8:  _h_call_other,          # call_in_other_fun_ex
    9:  _h_os_fun,              # call_os_fun

    # switch/case
    10: _h_case_check_num,      # case_check_number
    11: _h_case_check_str,      # case_check_string
    12: _h_case_default,        # case_default

    # 类型相关
    13: _h_cast_type,           # cast_type
    16: _h_check_is_ref_type,   # check_is_ref_type
    17: _h_check_is_ref_type_i, # check_is_ref_type_i
    36: _h_is_type,             # is_type

    # 跳转
    18: _h_cond_jmp_for_loop,   # cond_jmp_for_loop (循环 break/continue)
    31: _h_goto,                # goto
    33: _h_if,                  # if (条件跳转)
    80: _h_step_jmp_for_loop,   # step_jmp_for_loop (循环步进)

    # 容器构造
    21: _h_create_argument_array,  # create_argument_array
    22: _h_create_array,           # create_array
    23: _h_create_function_value,  # create_function_value
    24: _h_create_mapping,         # create_mapping

    # 循环
    35: _h_init_for_loop,       # init_for_loop

    # 函数控制
    27: _h_enter_function,      # enter_function
    72: _h_return_from_fake_fun,  # return_from_fake_fun (无返回值)
    73: _h_return_from_fun,     # return_from_fun (带返回值)

    # 栈操作
    19: _h_convert_to_ptr,      # convert_to_ptr
    20: _h_convert_to_value,    # convert_to_value
    49: _h_pickup_os_value,     # pickup_os_value
    50: _h_pop_n_stack,         # pop_n_stack
    51: _h_pop_stack,           # pop_stack
    71: _h_resort_stack,        # resort_stack
    82: _h_to_reserve,          # to_reserve_stack_value
    83: _h_to_unreserve,        # to_unreserved_stack_value

    # 压栈:常量
    52: _h_arg_count,           # push_arg_num
    59: _h_small_int,           # push_number(S)
    60: _h_long_int,            # push_number(L)
    63: _h_real,                # push_real_from_word
    64: _h_string,              # push_shared_string
    65: _h_this_object,         # push_this_object
    66: _h_undefined,           # push_undefined

    # 压栈:变量
    53: _h_global,              # push_global_var
    54: _h_addrof_global,       # push_global_vptr
    55: _h_local,               # push_local_value
    56: _h_local_var_i,         # push_local_var_i
    57: _h_local_vptr,          # push_local_vptr
    58: _h_local_vptr_i,        # push_local_vptr_i
    61: _h_obj_var,             # push_obj_var
    62: _h_addrof_obj_var,      # push_obj_vptr

    # 成员/范围查询
    67: _h_member,              # query_member
    68: _h_addrof_member,       # query_member_ptr
    69: _h_range,               # query_range
    70: _h_addrof_range,        # query_range_ptr

    # 赋值(变量)
    76: _h_global_assign,       # set_global_var
    77: _h_local_assign,        # set_local_var
    78: _h_obj_var_assign,      # set_obj_var

    # 其他
    14: _h_caught_error,        # caught_error
    15: _h_check_args,          # check_args_num
    26: _h_end_thread_catch,    # end_thread_catch
    29: _h_finish_action,       # finish_action
    44: _h_new_thread_catch,    # new_thread_catch
    46: _h_null_action,         # null_action
    79: _h_sizeof,              # sizeof
    84: _h_trace,               # trace
}


# ---------------------------------------------------------------------------
#  Control flow recovery (把 goto/label 恢复成 if/else/while/switch)
# ---------------------------------------------------------------------------
# 正则:匹配 if (...) goto @N;
_IF_GOTO = re.compile(r'^if \((.+)\) goto @(\d+);\s*(?:/\*.*\*/)?$')
# 正则:匹配 goto @N;
_GOTO = re.compile(r'^goto @(\d+);\s*(?:/\*.*\*/)?$')
# 正则:匹配 @N: 标签
_LABEL = re.compile(r'^@(\d+):\s*$')
# 正则:匹配 case N: goto @L;
_CASE = re.compile(r'^case (.+?): goto @(\d+);\s*$')
# 正则:匹配 default: goto @L;
_DEFAULT = re.compile(r'^default: goto @(\d+);\s*$')


def _strip_block_comment(s):
    """去掉行尾的 /* ... */ 注释。"""
    idx = s.find("/*")
    if idx >= 0:
        return s[:idx].rstrip()
    return s


def _negate_cond(cond):
    """逻辑取反条件。"""
    cond = cond.strip()
    # !(X) -> X
    if cond.startswith("!(") and cond.endswith(")"):
        return cond[2:-1]
    # a == b -> a != b, a != b -> a == b
    for op, neg in [("==", "!="), ("!=", "=="), ("<=", ">"), (">=", "<"),
                    ("<", ">="), (">", "<=")]:
        # 找到最外层的 op(跳过 -> 和字符串)
        depth = 0
        in_str = False
        str_ch = None
        i = 0
        while i < len(cond):
            c = cond[i]
            if in_str:
                if c == str_ch and cond[i-1] != '\\':
                    in_str = False
                i += 1
                continue
            if c in '"\'':
                in_str = True
                str_ch = c
                i += 1
                continue
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            elif depth == 0 and cond[i:i+len(op)] == op:
                # 排除 -> 中的 > (当 op == ">" 时)
                if op == ">" and i > 0 and cond[i-1] == '-':
                    i += 1
                    continue
                # 排除 -> 中的 > (当 op == "<=" 不会匹配,但保险)
                left = cond[:i].rstrip()
                right = cond[i+len(op):].lstrip()
                return "%s %s %s" % (left, neg, right)
            i += 1
        break
    return "!(%s)" % cond


def _parse_lines(raw_lines):
    """把原始行解析成 (kind, content, target) 列表。

    kind:
      'label'         - @N: 标签,content=label_id, target=None
      'ifgoto'        - if (C) goto @N;, content=cond, target=label_id
      'goto'          - goto @N;, content=None, target=label_id
      'case'          - case X: goto @N;, content=X, target=label_id
      'default'       - default: goto @N;, content=None, target=label_id
      'switch_marker' - /* switch: <expr> */, content=expr, target=None
      'stmt'          - 普通语句,content=语句, target=None
    """
    items = []
    for raw in raw_lines:
        s = raw.strip()
        if not s:
            continue
        # @N: 标签
        m = _LABEL.match(s)
        if m:
            items.append(("label", int(m.group(1)), None))
            continue
        # if (C) goto @N;
        m = _IF_GOTO.match(s)
        if m:
            items.append(("ifgoto", m.group(1), int(m.group(2))))
            continue
        # goto @N;
        m = _GOTO.match(s)
        if m:
            items.append(("goto", None, int(m.group(1))))
            continue
        # case X: goto @N;
        m = _CASE.match(s)
        if m:
            items.append(("case", m.group(1), int(m.group(2))))
            continue
        # default: goto @N;
        m = _DEFAULT.match(s)
        if m:
            items.append(("default", None, int(m.group(1))))
            continue
        # /* switch: <expr> */ 标记
        m = re.match(r'/\*\s*switch:\s*(.+?)\s*\*/', s)
        if m:
            items.append(("switch_marker", m.group(1), None))
            continue
        # /* new_thread_catch -> @N */ 标记 (try-catch 的 try 块开始)
        m = re.match(r'/\*\s*new_thread_catch\s*->\s*@(\d+)\s*\*/', s)
        if m:
            items.append(("try_catch", int(m.group(1)), None))
            continue
        # 普通语句
        items.append(("stmt", s, None))
    return items


def _build_label_map(items):
    """构建 label_id -> index 映射。"""
    labels = {}
    for i, (kind, content, target) in enumerate(items):
        if kind == "label":
            labels[content] = i
    return labels


def _find_back_gotos(items, start, end, label_id):
    """查找 [start, end) 范围内所有回跳到 label_id 的位置。

    回跳包括:
      - goto @label_id           (while 循环的无条件回跳)
      - if (0) goto @label_id    (do-while(0) 的条件回跳, 条件 0 不跳=退出)
      - if (1) goto @label_id    (do-while(1) 的条件回跳, 等价于 while(1))
      - if (C) goto @label_id    (do-while(C) 的条件回跳, C 为真实条件)
    普通 if (C) goto @OTHER 不算回跳到 label_id(target 不同)。
    """
    result = []
    for k in range(start, end):
        kind, content, target = items[k]
        if target != label_id:
            continue
        if kind == "goto":
            result.append(k)
        elif kind == "ifgoto":
            result.append(k)
    return result


def _is_only_labels(items, lo, hi):
    """检查 [lo, hi) 范围内是否全是 label。"""
    return all(items[k][0] == "label" for k in range(lo, hi))


def _is_foreach_continue(tgt_idx, loop_ctx, items):
    """检查 goto/ifgoto 是否是 foreach 循环的 continue。

    foreach 的 continue 跳到 step goto 前的 label(循环体末尾),
    然后执行 step goto 回到循环开始。
    模式: @continue_label: /* step loop_ctrl_N */ goto @loop_start;
    """
    if loop_ctx is None or tgt_idx < 0 or len(loop_ctx) < 3:
        return False
    loop_start_label = loop_ctx[0]
    loop_end_idx = loop_ctx[1]
    if loop_start_label is None or loop_end_idx is None:
        return False
    if tgt_idx >= loop_end_idx or tgt_idx < 0:
        return False
    # [tgt_idx, loop_end_idx - 1) 应全是 label
    for k in range(tgt_idx, loop_end_idx - 1):
        if items[k][0] != "label":
            return False
    # items[loop_end_idx - 1] 应是 step goto @loop_start_label
    # step goto 可能是 "goto" item 或 "stmt" item (含 "/* step loop_ctrl_N */ goto @L;")
    # 因为 _GOTO 正则要求行首是 "goto",带注释前缀的 step goto 会被解析为 stmt
    if loop_end_idx - 1 < 0:
        return False
    step_item = items[loop_end_idx - 1]
    if step_item[0] == "goto" and step_item[2] == loop_start_label:
        return True
    if step_item[0] == "stmt":
        step_content = step_item[1]
        if "goto @%d" % loop_start_label in step_content:
            return True
    return False


def _structure(items, start, end, labels, indent, loop_ctx=None):
    """递归处理 [start, end) 范围内的 items,生成结构化代码行。

    loop_ctx: 形如 (loop_start_label, loop_end_idx, cond_end_label, switch_break_label)
               用于识别 break(跳到 loop_end_idx 或之后) 和 continue(跳回 loop_start_label)。
               switch_break_label: 当前 switch 的合并 label_id,跳到此处的 goto 应渲染为 break;
                                   None 表示不在 switch 内。
               None 表示不在循环体内。
    """
    result = []
    i = start
    while i < end:
        kind, content, target = items[i]

        # 裸标签: 检查是否是循环开始(后面有回跳到此 label)
        if kind == "label":
            back_gotos = _find_back_gotos(items, i + 1, end, content)
            if back_gotos:
                # 找到所有回跳,最后一个回跳之后是循环结束
                last_back = back_gotos[-1]
                last_back_item = items[last_back]
                loop_end_idx = last_back + 1
                # 查找循环条件:扫描循环体开头找第一个 ifgoto 跳到 loop_end_idx 或之后
                # 这是 while 循环的特征: if (C) goto @END; S...; goto @L;
                cond_idx = None
                cond_target = None
                for k in range(i + 1, last_back):
                    if items[k][0] != "ifgoto":
                        continue
                    tgt_k = items[k][2]
                    tgt_k_idx = labels.get(tgt_k, -1)
                    if tgt_k_idx >= loop_end_idx and tgt_k_idx <= end:
                        cond_idx = k
                        cond_target = tgt_k
                        break
                if cond_idx is not None:
                    # while 循环: if (C) goto @END; S...; goto @L;
                    # 等价于 while (!C) { S... }
                    cond = items[cond_idx][1]
                    neg_cond = _negate_cond(cond)
                    _case_tgts = loop_ctx[4] if (loop_ctx and len(loop_ctx) >= 5) else None
                    new_loop_ctx = (content, loop_end_idx, cond_target, None, _case_tgts)
                    result.append("    " * indent + "while (%s) {" % neg_cond)
                    result.extend(_structure(items, cond_idx + 1, last_back, labels,
                                              indent + 1, loop_ctx=new_loop_ctx))
                    result.append("    " * indent + "}")
                    i = loop_end_idx
                    continue
                # 没有 while 循环的向前跳转条件,检查是否是 do-while 模式
                # do-while: @L: S...; if (C) goto @L;
                # 等价于: do { S... } while (C);
                # 注意: 条件"1"(do-while(1))等价于 while(1),排除以保持 while(1) 输出;
                #       条件"0"(do-while(0))执行一次后退出,必须识别为 do-while(0)
                if (last_back_item[0] == "ifgoto" and
                        last_back_item[1].strip() != "1"):
                    cond = last_back_item[1]
                    _case_tgts = loop_ctx[4] if (loop_ctx and len(loop_ctx) >= 5) else None
                    new_loop_ctx = (content, loop_end_idx, None, None, _case_tgts)
                    result.append("    " * indent + "do {")
                    result.extend(_structure(items, i + 1, last_back, labels,
                                              indent + 1, loop_ctx=new_loop_ctx))
                    result.append("    " * indent + "} while (%s);" % cond)
                    i = loop_end_idx
                    continue
                # 没有循环条件 -> while (1)
                _case_tgts = loop_ctx[4] if (loop_ctx and len(loop_ctx) >= 5) else None
                new_loop_ctx = (content, loop_end_idx, None, None, _case_tgts)
                result.append("    " * indent + "while (1) {  /* loop @%d */" % content)
                result.extend(_structure(items, i + 1, last_back, labels,
                                          indent + 1, loop_ctx=new_loop_ctx))
                result.append("    " * indent + "}")
                i = loop_end_idx
                continue
            # 不是循环开始,跳过裸标签
            i += 1
            continue

        # if (C) goto @L;
        if kind == "ifgoto":
            cond = content
            tgt = target
            tgt_idx = labels.get(tgt, -1)

            # dead code: if (0) goto @L; -> 保留为空 if (用户要求保留所有代码)
            if cond.strip() == "0":
                result.append("    " * indent + "if (0) { /* empty jump to @%d */ }" % tgt)
                i += 1
                continue

            # 情况1: 空跳转 - if 后紧跟目标标签(中间只有 label)
            # 保留为空 if (用户要求保留所有代码,条件可能含副作用)
            if tgt_idx == i + 1 or (tgt_idx > i and _is_only_labels(items, i + 1, tgt_idx + 1)):
                result.append("    " * indent + "if (%s) { /* empty jump to @%d */ }" % (cond, tgt))
                i = tgt_idx + 1
                continue

            # 情况2: continue (跳到外层循环开始 label 或 foreach step label)
            if loop_ctx is not None and len(loop_ctx) >= 4 and loop_ctx[0] is not None and tgt == loop_ctx[0]:
                result.append("    " * indent + "if (%s) continue;" % cond)
                i += 1
                continue
            if _is_foreach_continue(tgt_idx, loop_ctx, items):
                result.append("    " * indent + "if (%s) continue;" % cond)
                i += 1
                continue

            # 情况3: break (跳到循环结束或之后)
            if loop_ctx is not None and len(loop_ctx) >= 4 and loop_ctx[1] is not None:
                loop_end_idx = loop_ctx[1]
                if tgt_idx >= loop_end_idx and (tgt_idx >= end or tgt_idx < 0 or tgt_idx < i):
                    result.append("    " * indent + "if (%s) break;" % cond)
                    i += 1
                    continue

            # 情况4: 目标在当前范围之外 (严格大于 end,不包括 == end)
            # 当 tgt_idx == end 时,标签正好在 scope 末尾,if-then 模式可以处理
            if tgt_idx > end or tgt_idx < 0 or tgt_idx < i:
                # 尝试 guard clause 模式:
                #   if (!(X)) goto @OUT; S...  →  if (X) { S }
                # 适用条件:
                #   1. cond 是 !(X) 形式(取反条件)
                #   2. [i+1, end) 范围内没有其他 goto/ifgoto 跳出当前块
                #     (如果有,guard clause 会错误地包含它们)
                if cond.startswith("!(") and cond.endswith(")"):
                    # 检查 [i+1, end) 是否有其他跳转跳出当前块
                    has_outer_jump = False
                    for k in range(i + 1, end):
                        if items[k][0] in ("goto", "ifgoto"):
                            kt = items[k][2] if items[k][0] == "ifgoto" else items[k][2]
                            kt_idx = labels.get(kt, -1)
                            if kt_idx >= end or kt_idx < 0:
                                has_outer_jump = True
                                break
                    if not has_outer_jump:
                        inner_cond = cond[2:-1]
                        # 去掉多余外层括号
                        if inner_cond.startswith("(") and inner_cond.endswith(")"):
                            depth = 0
                            balanced = True
                            for ci, ch in enumerate(inner_cond):
                                if ch == "(":
                                    depth += 1
                                elif ch == ")":
                                    depth -= 1
                                    if depth == 0 and ci < len(inner_cond) - 1:
                                        balanced = False
                                        break
                            if balanced:
                                inner_cond = inner_cond[1:-1]
                        then_result = _structure(items, i + 1, end, labels,
                                                  indent + 1, loop_ctx)
                        if then_result:
                            result.append("    " * indent + "if (%s) {" % inner_cond)
                            result.extend(then_result)
                            result.append("    " * indent + "}")
                        else:
                            # 空 then 块,保留为空 if (用户要求保留所有代码,条件可能含副作用)
                            result.append("    " * indent + "if (%s) { /* empty guard, jump to @%d */ }" % (inner_cond, tgt))
                        i = end
                        continue
                # 普通 goto 保留
                result.append("    " * indent + "if (%s) goto @%d;" % (cond, tgt))
                i += 1
                continue

            # 情况3: if-else 模式
            # 标准: if (C) goto @else; S_then...; goto @end; @else: S_else...; @end:
            # 扫描 [i+1, tgt_idx) 找最后一个非 label 项,如果是 goto 则是 if-else
            then_end_goto = None
            then_goto_idx = None
            for k in range(tgt_idx - 1, i, -1):
                if items[k][0] == "label":
                    continue
                if items[k][0] == "goto":
                    then_end_goto = items[k][2]
                    then_goto_idx = k
                break

            if then_end_goto is not None and then_goto_idx is not None:
                end_idx = labels.get(then_end_goto, -1)
                # 当 end_idx > end 时,检查 [end, end_idx) 是否全是 label
                # 或 goto then_end_goto (合并 goto),允许跨 scope 的 if-else 检测
                # 这种情况出现在嵌套 if-else 中:外层 then 块内的 if-else
                # 其 else 分支也以 goto @merge 结尾,跨越了外层 then 的 scope
                effective_end = end
                if end_idx > end and end_idx <= len(items):
                    # 获取 case_targets,防止 effective_end 跨越 case/default 标签
                    case_targets_set = loop_ctx[4] if (loop_ctx and len(loop_ctx) >= 5) else None
                    all_merge = True
                    for k in range(end, end_idx):
                        if items[k][0] == "label":
                            # 不跨越 case/default 标签(它们是 case target)
                            if case_targets_set is not None and items[k][1] in case_targets_set:
                                all_merge = False
                                break
                            continue
                        elif items[k][0] == "goto" and items[k][2] == then_end_goto:
                            # 合并 goto: 跳到同一个合并点的 goto
                            continue
                        else:
                            all_merge = False
                            break
                    if all_merge:
                        effective_end = end_idx
                if end_idx > tgt_idx and end_idx <= effective_end:
                    # if-else 模式: if (C) goto @else; S_then; goto @end; @else: S_else; @end:
                    # 等价于 if (!C) { S_then } else { S_else }
                    neg_cond = _negate_cond(cond)
                    then_result = _structure(items, i + 1, then_goto_idx, labels,
                                              indent + 1, loop_ctx)
                    # 在 switch 上下文中,then 块末尾的 goto @merge 是 break,需要显式输出
                    # (then_end_goto 被 if-else 模式消费,不会在 then_result 中出现)
                    if loop_ctx is not None and len(loop_ctx) >= 4 and loop_ctx[3] is not None and then_end_goto == loop_ctx[3]:
                        then_result.append("    " * (indent + 1) + "break;")
                    else_result = _structure(items, tgt_idx + 1, end_idx, labels,
                                              indent + 1, loop_ctx)
                    if not then_result and not else_result:
                        # 都空,保留为空 if-else (用户要求保留所有代码,条件可能含副作用)
                        # 注意: if (C) goto @else 意味着 C 成立->else, 等价于 if (!C) { then } else { else }
                        result.append("    " * indent + "if (%s) { /* empty then */ }" % neg_cond)
                        result.append("    " * indent + "else { /* empty else */ }")
                        i = end_idx
                        continue
                    if then_result and not else_result:
                        # 只有 then 块,else 为空 -> 保留空 else (用户要求保留所有代码)
                        result.append("    " * indent + "if (%s) {" % neg_cond)
                        result.extend(then_result)
                        result.append("    " * indent + "} else { /* empty else */ }")
                        i = end_idx
                        continue
                    if not then_result and else_result:
                        # 只有 else 块,then 为空 -> 保留空 then (用户要求保留所有代码)
                        result.append("    " * indent + "if (%s) { /* empty then */ }" % neg_cond)
                        result.append("    " * indent + "else {")
                        result.extend(else_result)
                        result.append("    " * indent + "}")
                        i = end_idx
                        continue
                    # 都非空
                    result.append("    " * indent + "if (%s) {" % neg_cond)
                    result.extend(then_result)
                    result.append("    " * indent + "} else {")
                    result.extend(else_result)
                    result.append("    " * indent + "}")
                    i = end_idx
                    continue

            # 情况3.5: if-then-else (else 跳出当前范围) 模式
            # 模式: if (C) goto @L; goto @OUT; @L: S
            #   - items[i+1] 是 goto @OUT
            #   - @OUT (out_idx) 在当前 end 之外(或等于 end)
            #   - @L (tgt_idx) 在当前 end 内
            # 语义: C 成立 -> 跳到 @L 执行 then 块 S; C 不成立 -> goto @OUT (else 分支)
            # 分两种情况:
            #   a) @OUT 是循环退出 (break) 或循环开始 (continue):
            #      输出 guard clause: if (!C) break/continue; S
            #      (then 块 S 不嵌套,与 if 同级)
            #   b) @OUT 是前向合并点 (forward merge):
            #      then 块和 else 都最终到达 @OUT,else 为空
            #      输出: if (C) { S }  (then 块嵌套)
            if (i + 1 < end and items[i + 1][0] == "goto" and
                    tgt_idx > i + 1 and tgt_idx < end):
                out_target = items[i + 1][2]
                out_idx = labels.get(out_target, -1)
                if (out_idx > tgt_idx and out_idx >= end and
                        out_idx <= len(items)):
                    # 检查 @OUT 是否是循环退出 (break) 或循环开始 (continue)
                    is_break = False
                    is_continue = False
                    if loop_ctx is not None and len(loop_ctx) >= 4 and loop_ctx[1] is not None:
                        loop_end_idx = loop_ctx[1]
                        loop_start_label = loop_ctx[0]
                        if out_idx >= loop_end_idx:
                            is_break = True
                        elif loop_start_label is not None and out_target == loop_start_label:
                            is_continue = True

                    if is_break or is_continue:
                        # guard clause 模式: if (!C) break/continue; S
                        # then 块 S 范围 [tgt_idx + 1, end),不嵌套
                        neg_cond = _negate_cond(cond)
                        keyword = "break" if is_break else "continue"
                        result.append("    " * indent + "if (%s) %s;" % (neg_cond, keyword))
                        then_result = _structure(items, tgt_idx + 1, end, labels,
                                                  indent, loop_ctx)
                        result.extend(then_result)
                        i = end
                        continue
                    else:
                        # @OUT 是前向合并点,else 为空
                        # then 块范围 [tgt_idx + 1, end),嵌套在 if 内
                        then_result = _structure(items, tgt_idx + 1, end, labels,
                                                  indent + 1, loop_ctx)
                        if then_result:
                            result.append("    " * indent + "if (%s) {" % cond)
                            result.extend(then_result)
                            result.append("    " * indent + "}")
                        else:
                            # 空 then 块,保留为空 if (用户要求保留所有代码,条件可能含副作用)
                            result.append("    " * indent + "if (%s) { /* empty then, merge to @%d */ }" % (cond, out_target))
                        i = end
                        continue

            # 情况4: 标准 if-then 模式
            # if (C) goto @L; S... @L: ...
            # C 成立时跳过 S,等价于 if (!C) { S }
            if tgt_idx > i + 1 and tgt_idx <= end:
                inner_cond = _negate_cond(cond)
                then_result = _structure(items, i + 1, tgt_idx, labels,
                                          indent + 1, loop_ctx)
                if not then_result:
                    # 空 then 块,保留为空 if (用户要求保留所有代码,条件可能含副作用)
                    result.append("    " * indent + "if (%s) { /* empty then, jump to @%d */ }" % (inner_cond, tgt))
                    i = tgt_idx
                    continue
                result.append("    " * indent + "if (%s) {" % inner_cond)
                result.extend(then_result)
                result.append("    " * indent + "}")
                i = tgt_idx
                continue

            # 兜底:保留原始 if-goto
            result.append("    " * indent + "if (%s) goto @%d;" % (cond, tgt))
            i += 1
            continue

        # goto @N;
        if kind == "goto":
            tgt = target
            tgt_idx = labels.get(tgt, -1)
            # 先检查 continue (跳到外层循环开始 label,无论 range)
            if loop_ctx is not None and len(loop_ctx) >= 4 and loop_ctx[0] is not None and tgt == loop_ctx[0]:
                result.append("    " * indent + "continue;")
                i += 1
                continue
            # 检查 foreach continue (跳到 step goto 前的 label)
            if _is_foreach_continue(tgt_idx, loop_ctx, items):
                result.append("    " * indent + "continue;")
                i += 1
                continue
            # switch break: 跳到当前 switch 的合并 label,渲染为 break
            if loop_ctx is not None and len(loop_ctx) >= 4 and loop_ctx[3] is not None and tgt == loop_ctx[3]:
                result.append("    " * indent + "break;")
                i += 1
                continue
            # 顺序 goto: goto @L; @L: ... -> 消除
            if tgt_idx == i + 1 or (tgt_idx > i and _is_only_labels(items, i + 1, tgt_idx + 1)):
                i = tgt_idx + 1
                continue
            # 跳到当前块外: break
            if tgt_idx >= end or tgt_idx < 0:
                if loop_ctx is not None and len(loop_ctx) >= 3 and loop_ctx[1] is not None:
                    loop_end_idx = loop_ctx[1]
                    if tgt_idx >= loop_end_idx:
                        result.append("    " * indent + "break;")
                        i += 1
                        continue
                result.append("    " * indent + "goto @%d;" % tgt)
                i += 1
                continue
            # 其他 goto 保留
            result.append("    " * indent + "goto @%d;" % tgt)
            i += 1
            continue

        # case / default: switch 模式
        if kind in ("case", "default"):
            # 收集连续的 case/default 序列
            case_list = []
            j = i
            while j < end and items[j][0] in ("case", "default"):
                case_list.append(items[j])
                j += 1
            # case_list 是 case 序列,j 是第一个 case 块的开始(应该是一个 label)
            # 找到所有 case 的目标,确定 switch 的结束(合并点)
            case_targets = [c[2] for c in case_list]
            # 合并点是所有 case 块末尾 goto 的目标
            # 简化:找到所有 case 目标的最大值作为 switch 范围
            max_target = max(case_targets) if case_targets else j
            # 查找 switch 的合并点:扫描 case 块,找 goto 的目标
            merge_target = None
            for k in range(j, max(labels[t] for t in case_targets if t in labels) + 1):
                if k >= end:
                    break
                if items[k][0] == "goto":
                    gt = items[k][2]
                    if gt not in case_targets and gt > max_target:
                        if merge_target is None or gt > merge_target:
                            merge_target = gt

            # switch_end 是 items index(不是 label_id)
            # merge_target 是 label_id,需要转换为 items index
            if merge_target is not None and merge_target in labels:
                switch_end = labels[merge_target]
            else:
                # fallback:使用最大 case target 的 items index + 1
                max_tgt_idx = max(labels[t] for t in case_targets if t in labels)
                switch_end = max_tgt_idx + 1
            if switch_end > end:
                switch_end = end

            # 从 case 序列前的 switch_marker 中提取 switch 表达式
            # decompiler 会在第一个 case 前插入 /* switch: <expr> */ 标记
            switch_expr = "/* switch_expr */"
            if i > 0 and items[i - 1][0] == "switch_marker":
                switch_expr = items[i - 1][1]
            result.append("    " * indent + "switch (%s) {" % switch_expr)

            # 处理每个 case 块
            # case_targets 是各个 case 跳转到的 label
            # 按 label 顺序处理 (多个 case 跳到同一 label 则共享代码体 = fall-through)
            sorted_targets = sorted(set(case_targets))
            # 统计每个 target 对应的所有 case 标签 (fall-through: 多个 case 共享同一 target)
            target_to_labels = {}
            for c in case_list:
                target_to_labels.setdefault(c[2], []).append(c)
            for ct in sorted_targets:
                ct_idx = labels.get(ct, -1)
                if ct_idx < 0:
                    continue
                # 找到该 case 块的范围:从 ct_idx 到下一个 case 目标的 label
                next_idx = switch_end
                for other_ct in sorted_targets:
                    other_idx = labels.get(other_ct, -1)
                    if other_idx > ct_idx and other_idx < next_idx:
                        next_idx = other_idx
                # case 块到 next_idx,但可能末尾有 goto @merge
                block_end = next_idx
                # 去掉末尾的 goto @merge
                if block_end > ct_idx and items[block_end - 1][0] == "goto" and \
                        items[block_end - 1][2] == (merge_target or -1):
                    block_end -= 1
                # 输出对应的 case 标签 (fall-through: 多个 case 共享同一 target 时全部输出)
                labels_for_ct = target_to_labels.get(ct, [])
                for c in labels_for_ct:
                    if c[0] == "case":
                        result.append("    " * (indent + 1) + "case %s:" % c[1])
                    else:
                        result.append("    " * (indent + 1) + "default:")
                # fall-through 检测: 代码体为空 (case label 紧接下一个 case label,无任何语句)
                # 这是真正的 fall-through, 不添加 break
                # 注意: 区分 "goto @merge 被去掉后变空" (应有 break) 和 "原本就无代码" (fall-through)
                # 判断: 去掉 goto @merge 前是否有 goto @merge
                #   - 有 goto @merge: 是 break, 去掉后虽空但应添加 break
                #   - 无 goto @merge 且代码体为空: 是 fall-through (case label 紧接下一个 case label)
                case_targets_set_check = frozenset(case_targets)
                orig_block_end = next_idx
                had_merge_goto = (orig_block_end > ct_idx and items[orig_block_end - 1][0] == "goto" and \
                                  items[orig_block_end - 1][2] == (merge_target or -1))
                is_fall_through = (not had_merge_goto and block_end <= ct_idx + 1)
                if is_fall_through:
                    # fall-through: 无函数体,不添加 break,直接进入下一个 case
                    continue
                # 将 merge_target 作为 switch_break_label 传入,case 体内的 goto @merge 应渲染为 break
                # 保留外层 loop_ctx 的前3项,替换第4项为当前 switch 的 merge_target
                # 第5项传递 case_targets,防止 if-else 的 effective_end 跨越 case 标签
                case_targets_set = frozenset(case_targets)
                outer_loop = loop_ctx[:3] if loop_ctx else None
                if outer_loop:
                    case_ctx = tuple(list(outer_loop) + [merge_target, case_targets_set])
                else:
                    case_ctx = (None, None, None, merge_target, case_targets_set)
                body_result = _structure(items, ct_idx + 1, block_end, labels, indent + 2, case_ctx)
                result.extend(body_result)
                # 若 case 体末尾已是终止语句 (break/return/continue/goto),不再添加冗余 break
                last_line = body_result[-1].strip() if body_result else ""
                _terminated = (last_line.endswith("break;") or last_line.endswith("return;") or
                               last_line.endswith("return ") or last_line.endswith("continue;") or
                               last_line.startswith("goto ") or last_line.startswith("return "))
                if not _terminated:
                    result.append("    " * (indent + 2) + "break;")

            result.append("    " * indent + "}")
            i = switch_end
            continue

        # for_loop_init 注释 + loop_ctrl 结构
        if kind == "stmt" and "for_loop_init" in content:
            # 提取循环信息
            # 格式: /* for_loop_init(foreach): var=<loop_1> iter=&ob end=(...) step=997 */
            # end 字段可能含空格(如 (count - 1)),用非贪婪 .+? 匹配到 step= 之前
            m = re.search(r'for_loop_init\((\w+)\): var=(\S+) iter=(\S+) end=(.+?) step=(\S+)', content)
            if m:
                loop_type = m.group(1)
                var = m.group(2)
                iter_expr = m.group(3)
                end_expr = m.group(4)
                step_expr = m.group(5)
                # 模式:
                #   i:   for_loop_init 注释
                #   i+1: if (loop_ctrl_N) goto @END;  -- 条件检查,target=END
                #   i+2: @BODY: (循环体开始 label)
                #   ...循环体...
                #   step_idx: /* step loop_ctrl_N */ goto @BODY;  -- 步进并回跳
                #   end_idx: @END: (循环结束 label)
                if (i + 1 < end and items[i + 1][0] == "ifgoto" and
                        "loop_ctrl" in items[i + 1][1]):
                    end_target = items[i + 1][2]
                    end_idx = labels.get(end_target, -1)
                    # 提取 loop_ctrl 编号(如 loop_ctrl_246 -> 246),
                    # 避免误匹配嵌套循环的 step 注释
                    m_lc = re.search(r'loop_ctrl_(\d+)', items[i + 1][1])
                    lc_num = m_lc.group(1) if m_lc else None
                    if (end_idx > 0 and i + 2 < end and
                            items[i + 2][0] == "label"):
                        body_start_label = items[i + 2][1]
                        body_start_idx = i + 2
                        # 在循环体内 [body_start_idx+1, end_idx) 找 step 注释
                        # 必须匹配相同的 loop_ctrl 编号,避免误匹配嵌套循环
                        step_idx = -1
                        step_pat = ("step loop_ctrl_%s" % lc_num) if lc_num else "step loop_ctrl"
                        for k in range(body_start_idx + 1, end_idx):
                            if items[k][0] == "stmt" and step_pat in items[k][1]:
                                step_idx = k
                                break
                        if step_idx > 0:
                            # 循环体是 [body_start_idx+1, step_idx)
                            _case_tgts = loop_ctx[4] if (loop_ctx and len(loop_ctx) >= 5) else None
                            new_loop_ctx = (body_start_label, end_idx, end_target, None, _case_tgts)
                            if loop_type == "foreach":
                                # foreach (elem in collection)
                                # iter_expr 是 &elem 形式,去掉 & 得到元素变量名
                                # end_expr 是被遍历的集合(数组或映射)
                                # var 是内部 loop_ctrl 变量,不是 foreach 的元素变量
                                iter_clean = iter_expr.lstrip("&")
                                result.append("    " * indent + "loop (%s in %s) {" % (iter_clean, end_expr))
                            elif loop_type == "for_range":
                                # for_range: iterator 是 &i,循环变量是 i
                                # step=1: i <= end 继续; step=-1: i >= end 继续
                                # i 的初始值在 for_loop_init 之前赋值,已作为独立语句输出
                                iter_var = iter_expr.lstrip("&")
                                try:
                                    step_val = int(step_expr)
                                    if step_val > 0:
                                        cond = "%s <= %s" % (iter_var, end_expr)
                                    elif step_val < 0:
                                        cond = "%s >= %s" % (iter_var, end_expr)
                                    else:
                                        cond = "/* %s */" % end_expr
                                except (ValueError, TypeError):
                                    cond = "/* end=%s */" % end_expr
                                # 尝试从 for_loop_init 之前的语句提取初始化
                                init_expr = ""
                                if result:
                                    last_line = result[-1].strip()
                                    # 形如 "i = (val + 1);" 或 "i = 0;"
                                    m_init = re.match(r'^(%s)\s*=\s*(.+?);$' % re.escape(iter_var), last_line)
                                    if m_init:
                                        init_expr = m_init.group(2)
                                        result.pop()  # 移除初始化语句,合并到 for 中
                                result.append("    " * indent + "for (%s = %s; %s; %s += %s) {" % (
                                    iter_var, init_expr if init_expr else "/* init */",
                                    cond, iter_var, step_expr))
                            else:
                                result.append("    " * indent + "for (/* %s */; ; ) {" % loop_type)
                            result.extend(_structure(items, body_start_idx + 1, step_idx, labels,
                                                      indent + 1, loop_ctx=new_loop_ctx))
                            result.append("    " * indent + "}")
                            i = end_idx
                            continue
            # 无法识别的 for_loop_init,保留原样
            result.append("    " * indent + content)
            i += 1
            continue

        # switch_marker: 已被 case 处理消费,跳过
        if kind == "switch_marker":
            i += 1
            continue

        # try-catch 模式: /* new_thread_catch -> @N */
        # 字节码模式: new_thread_catch(catch_target); try_body; end_thread_catch;
        #             @catch_target: caught_error() [push 1]; catch_body
        # LPC 语义: try { try_body } catch { catch_body }
        if kind == "try_catch":
            catch_target = content
            catch_idx = labels.get(catch_target, -1)
            if catch_idx < 0:
                # catch target 不在范围内,跳过
                i += 1
                continue
            # try 块范围: [i+1, catch_idx)
            # end_thread_catch 已被 _h_end_thread_catch 静默 (不产生 item)
            try_end = catch_idx
            # try 块末尾可能有 goto @merge (跳过 catch 块),需要去掉
            if (try_end > i + 1 and items[try_end - 1][0] == "goto"):
                try_end -= 1
            # catch 块: @catch_target label 之后
            # catch_target 处的 caught_error() (action 14) push 1 标记 catch 块开始
            # catch 块到合并点 label 结束
            # 字节码模式: ... goto @merge; @catch_target: [catch_body] goto @merge; @merge: stmt 1;
            #   或空 catch: ... goto @merge; @catch_target: @merge: stmt 1;
            catch_start = catch_idx + 1
            # 跳过 caught_error 产生的 stmt "1" (在 catch 块开头或合并点)
            # 查找 catch 块结束: 下一个 label (合并点) 或 goto
            catch_end = end
            merge_idx = end
            for k in range(catch_start, end):
                if items[k][0] == "label":
                    # 合并点 label,catch 块到此结束
                    catch_end = k
                    merge_idx = k
                    break
                elif items[k][0] == "goto":
                    catch_end = k + 1
                    break
            # 如果 catch 块为空 (catch_start == catch_end,即 catch_target 紧跟合并点 label)
            # 输出 try 块
            result.append("    " * indent + "try {")
            result.extend(_structure(items, i + 1, try_end, labels, indent + 1, loop_ctx))
            result.append("    " * indent + "} catch {")
            # catch 块体
            if catch_start < catch_end:
                catch_body = _structure(items, catch_start, catch_end, labels, indent + 1, loop_ctx)
                # 过滤掉 catch 块中残留的 stmt "0"/"0;"/"1"/"1;" (catch 返回值)
                # 0=正常路径(push 0 by small_int), 1=异常路径(caught_error push 1)
                catch_body = [l for l in catch_body if l.strip() not in ("0", "0;", "1", "1;")]
                if catch_body:
                    result.extend(catch_body)
                else:
                    result.append("    " * (indent + 1) + "/* error handled */")
            else:
                result.append("    " * (indent + 1) + "/* error handled */")
            result.append("    " * indent + "}")
            # 跳过合并点 label 后的 stmt "1" 或 "1;" (caught_error push 1 残留)
            after_catch = catch_end
            # 跳过合并点 label
            if after_catch < end and items[after_catch][0] == "label":
                after_catch += 1
            # 跳过 caught_error/catch返回值 的 stmt "0"/"0;"/"1"/"1;" (pop_n_stack 输出)
            if after_catch < end and items[after_catch][0] == "stmt" and \
                    items[after_catch][1].strip() in ("0", "0;", "1", "1;"):
                after_catch += 1
            i = after_catch
            continue

        # 普通语句
        if kind == "stmt":
            result.append("    " * indent + content)
            i += 1
            continue

        # 兜底
        result.append("    " * indent + str((kind, content, target)))
        i += 1

    return result


def restore_control_flow(raw_lines):
    """把基于 goto/label 的代码恢复成结构化控制流。"""
    items = _parse_lines(raw_lines)
    labels = _build_label_map(items)

    # 预处理:识别 while 循环模式
    # @L: ... if (C) goto @L;  -> while (C) { ... }
    # 这需要在递归处理中识别,这里先做简单标记

    result = _structure(items, 0, len(items), labels, 1)
    return result


# ---------------------------------------------------------------------------
#  Argument default values extraction
# ---------------------------------------------------------------------------
def extract_arg_defaults(fun, info):
    """从函数 nodes 开头提取参数默认值文本。

    编译器在函数入口处生成固定模式:
      [check_args(opc=15)] [默认值表达式...] [resort_stack(opc=71)] [enter_function(opc=27)]
    默认值按参数声明顺序压栈,resort_stack 之前栈(底->顶)对应有默认值的参数。

    返回与 fun["args"] 等长的列表,无默认值的位置为 None。
    提取失败时返回全 None,不影响正常反编译输出。
    """
    n_args = len(fun["args"])
    empty = [None] * n_args
    nodes = fun["nodes"]
    if not nodes or n_args == 0:
        return empty
    # 有默认值的参数索引列表 (byte[10] != 0)
    default_idxs = [i for i, a in enumerate(fun["args"]) if a[3] != 0]
    if not default_idxs:
        return empty
    # 在前 40 条内找 check_args 和随后的 resort_stack
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
    # 执行默认值表达式序列
    ctx = Ctx(stack=[], lines=[], fun=fun, prog=info, targets=set(), pc=0, signed=0)
    for pc in range(ci + 1, ri):
        p16, p8, action = nodes[pc]
        ctx.pc = pc
        ctx.signed = p16 if p16 < 0x8000 else p16 - 0x10000
        ctx.next_action = nodes[pc + 1][2] if pc + 1 < len(nodes) else -1
        handler = DT.get(action)
        if handler is not None:
            try:
                handler(ctx, p16, p8)
            except Exception:
                return empty
    # 栈(底->顶) = 有默认值参数(声明顺序)
    if len(ctx.stack) < len(default_idxs):
        return empty
    defaults = list(empty)
    for k, arg_idx in enumerate(default_idxs):
        defaults[arg_idx] = ctx.stack[k]
    return defaults


# ---------------------------------------------------------------------------
#  Decompile (optimized v2.0)
# ---------------------------------------------------------------------------
def decompile(fun, info):
    nodes = fun["nodes"]
    ctx = Ctx(stack=[], lines=[], fun=fun, prog=info, targets=set(), pc=0, signed=0)
    # 收集本文件所有 override 函数名; override 函数自调用需加 :: 前缀,
    # 否则会调用自身(override 版)导致无限递归,源码用 :: 调用父类版本
    ctx.override_names = {f["name"] for f in info.get("functions", []) if f.get("attrib", 0) & 8}

    # First pass: collect jump targets (所有跳转类 action 的目标)
    # 跳转 action: 10/11=case_check(metadata 模式,offset 在 meta 中),
    #              12=case_default, 18=cond_jmp_for_loop, 31=goto, 33=if, 80=step_jmp_for_loop
    # 注意: vm_regularProcessor 在调用 entry(par16, par8) 之前会先执行 ip++
    # (反汇编确认: mov [ecx+0Ch], eax 在 call dword ptr [pActionInfo+8] 之前)
    # 因此跳转处理器中 vm_currentContext->ip 已经是 pc+1,所以新目标 = pc + 1 + signed
    # 对于 metadata 模式的 case_check(10/11),offset 存储在 meta[v12] 中,
    # 由对应的 handler 在第二遍处理时添加 target,这里跳过。
    PAR16_OFFSET_ACTIONS = (12, 18, 31, 33, 80)  # offset 直接在 par16 中
    META_OFFSET_ACTIONS = (10, 11)                # offset 在 metadata 中
    meta = fun.get("meta", b"")
    for pc, (_par16, _par8, action) in enumerate(nodes):
        if action in PAR16_OFFSET_ACTIONS:
            s = _par16 if _par16 < 0x8000 else _par16 - 0x10000
            target = pc + 1 + s
            if 0 <= target < len(nodes):
                ctx.targets.add(target)
        elif action in META_OFFSET_ACTIONS:
            # 从 metadata 读取真实的跳转偏移
            off = _meta_offset(_par16, _par8)
            offset = _meta_dword(meta, off, signed=True)
            target = pc + 1 + offset
            if 0 <= target < len(nodes):
                ctx.targets.add(target)

    # Pre-scan: detect ternary expression patterns
    # 模式: push cond; if(pop) goto @else; push then_val; goto @end; @else: push else_val; @end:
    # 这些 if-else 在 _structure 中会被误判为"都空"而丢弃,需要在此预先检测并合并为三元表达式
    # 语句类 action (会产生 ctx.lines 输出,不应出现在三元运算的 then/else 分支中)
    STMT_ACTIONS = {3, 4, 10, 11, 12, 18, 26, 27, 31, 33, 34, 35, 44, 47, 51,
                    72, 73, 76, 77, 78, 80, 84}
    # 三元表达式 then/else 分支中禁止出现的 action (控制流、赋值等)
    # 但允许 catch 表达式模式: new_thread_catch(44), end_thread_catch(26),
    #   pop_stack(51, pop error返回值), goto(31, 跳过catch块),
    #   caught_error(14, catch入口), pop_n_stack(50, pop catch返回值)
    # 验证: itemd.o improve_object 中 catch(error("...")) 作为 then 分支副作用,
    #   返回值被 pop,不影响栈上 then_val
    # 三元表达式 then/else 分支中禁止出现的 action (控制流、赋值等)
    # 但允许 catch 表达式模式: new_thread_catch(44), end_thread_catch(26),
    #   pop_stack(51, pop error返回值), goto(31, 跳过catch块),
    #   caught_error(14, catch入口), pop_n_stack(50, pop catch返回值)
    # 允许嵌套三元: if(33) 在 else 分支中作为内层三元 (由 ternary_candidates 检查)
    # 验证: itemd.o improve_object 中 catch(error("...")) 作为 then 分支副作用,
    #   返回值被 pop,不影响栈上 then_val
    # 验证: cmd_l_block_account.o block_account 中 (interval == 0) ? ... : ((interval < 0) ? ... : ...)
    #   外层 else 分支含内层三元 (if action=33)
    FORBIDDEN_IN_TERNARY = STMT_ACTIONS - {26, 31, 33, 44, 51}
    ternary_starts = {}  # if_pc -> (else_target, goto_pc, end_target)
    # 预扫描所有 if (action 33, par8 bit1=1) 的基本三元候选,用于嵌套三元检测
    # 嵌套三元: else 分支可能包含另一个 if (action 33) 作为内层三元
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

    for pc, (else_target, goto_pc, end_target) in ternary_candidates.items():
        # 检查 then 分支 [pc+1, goto_pc): 无控制流/赋值 action
        # 允许 catch 模式的 action (26,31,44,51) 和 catch 模式的 label (caught_error=14, pop_n_stack=50)
        then_ok = True
        for k in range(pc + 1, goto_pc):
            if nodes[k][2] in FORBIDDEN_IN_TERNARY:
                then_ok = False
                break
            if k in ctx.targets:
                # 允许 catch 模式产生的 label (caught_error 和 pop_n_stack 位置)
                if nodes[k][2] not in (14, 50):
                    then_ok = False
                    break
        if not then_ok:
            continue
        # 检查 else 分支 [else_target, end_target):
        # 允许嵌套三元 (if action=33 且该 if 也是 ternary_candidates)
        # 允许 goto (31) 用于内层三元跳转
        else_ok = True
        nested_ternaries = []
        for k in range(else_target, end_target):
            if nodes[k][2] in FORBIDDEN_IN_TERNARY:
                else_ok = False
                break
            # 允许嵌套三元 (if action=33 且该 if 是 ternary_candidates)
            if nodes[k][2] == 33:
                if k in ternary_candidates:
                    nested_ternaries.append(k)
                    continue
                else_ok = False
                break
            if k != else_target and k in ctx.targets:
                # 允许嵌套三元的 label (else_target/end_target 位置)
                if any(k == ternary_candidates[nt][0] or k == ternary_candidates[nt][2]
                       for nt in nested_ternaries):
                    continue
                else_ok = False
                break
        if not else_ok:
            continue
        ternary_starts[pc] = (else_target, goto_pc, end_target)

    # Second pass: decompile
    for pc, (par16, par8, action) in enumerate(nodes):
        ctx.pc = pc
        ctx.signed = par16 if par16 < 0x8000 else par16 - 0x10000
        # 前瞻: 下一条节点的 action 号, 供赋值处理器判断语句/表达式
        ctx.next_action = nodes[pc + 1][2] if pc + 1 < len(nodes) else -1

        # 三元运算符: if 指令处 (开始) — 弹出条件,设置 pending_ternary,跳过 if-goto 输出
        if pc in ternary_starts:
            else_target, goto_pc, end_target = ternary_starts[pc]
            cond = ctx.stack[-1] if ctx.stack else "0"
            ctx.stack.pop()
            ctx.pending_ternary = {
                'cond': cond, 'par8': par8, 'then_val': None,
                'goto_pc': goto_pc, 'else_target': else_target, 'end_target': end_target,
                'state': 'then',
                # catch 副作用跟踪: then 分支可能有 catch(error(...)) 副作用
                # new_thread_catch(44) 开始, pop_n_stack(50) 结束
                # catch_lines 记录 catch 块内的输出行, catch_cond 是 if 条件
                'in_catch': False, 'catch_lines': None, 'catch_start': -1,
            }
            continue

        # 三元运算符: goto 处 (then 分支结束) — 保存 then_val,跳过 goto 输出
        if (ctx.pending_ternary is not None and
                ctx.pending_ternary['state'] == 'then' and
                pc == ctx.pending_ternary['goto_pc']):
            ctx.pending_ternary['then_val'] = ctx.stack.pop() if ctx.stack else "0"
            ctx.pending_ternary['state'] = 'else'
            continue

        # 标签输出 (跳过三元运算符的 @else label)
        skip_label = (ctx.pending_ternary is not None and
                      ctx.pending_ternary['state'] == 'else' and
                      pc == ctx.pending_ternary['else_target'])
        if pc in ctx.targets and not skip_label:
            ctx.lines.append("@%d:" % pc)
            # 解析短路求值: 从后往前处理所有 target==pc 的 pending_sc 条目
            # 嵌套短路 A && (B || C): 内层 || 先合并成 (B || C),
            #   外层 && 再合并成 (A && (B || C))
            while ctx.pending_sc:
                left, op, target, popped = ctx.pending_sc[-1]
                if target != pc:
                    break
                if popped and ctx.stack:
                    # 栈顶是右操作数(fell-through 路径),替换为 left op right
                    right = ctx.stack[-1]
                    # 扁平化链式 &&/||: 若 left 是 (X op Y) 形式,去括号
                    if _find_top_level_binop(left) == op:
                        left = left[1:-1]
                    # 赋值表达式作为操作数时必须加括号 (LPC 中 = 优先级低于 &&/||)
                    if _has_top_level_assign(left):
                        left = "(%s)" % left
                    if _has_top_level_assign(right):
                        right = "(%s)" % right
                    ctx.stack[-1] = "(%s %s %s)" % (left, op, right)
                # 若 not popped,说明走了 jump 路径,栈顶已是 left,值正确,保持不变
                ctx.pending_sc.pop()

        # 三元运算符: @end label 处 (合并) — 弹出 else_val,构造三元表达式压栈
        if (ctx.pending_ternary is not None and
                pc == ctx.pending_ternary['end_target']):
            if ctx.stack:
                else_val = ctx.stack.pop()
            else:
                else_val = "0"
            cond = ctx.pending_ternary['cond']
            par8_t = ctx.pending_ternary['par8']
            then_val = ctx.pending_ternary['then_val']
            # 若 then 分支有 catch 副作用,先输出 if (cond) { catch(error(...)); }
            # 验证: itemd.o improve_object 中 (field[<2..<1]==".c") ? catch(error("...")), "/gs/skills/"+field : ...
            # 字节码: then 分支有 new_thread_catch...error...end_thread_catch...caught_error...pop_n_stack
            catch_lines = ctx.pending_ternary.get('catch_lines')
            if catch_lines:
                # bit0=0 (FALSE jump): cond 为 TRUE 时执行 then 分支 (含 catch),输出 if (cond)
                # bit0=1 (TRUE jump): cond 为 FALSE 时执行 then 分支,输出 if (!(cond))
                # 简化: 若 cond 已是 !(X) 形式,取反得到 X
                if par8_t & 1:
                    if cond.startswith("!(") and cond.endswith(")"):
                        if_cond = cond[2:-1]
                    else:
                        if_cond = "!(%s)" % cond
                else:
                    if_cond = cond
                # 输出 if (cond) catch(expr); (单行形式,_structure 不支持 { } 块缩进)
                # catch_lines 通常只有一个元素 (catch(error(...));)
                catch_body = " ".join(cl.strip() for cl in catch_lines)
                ctx.lines.append("    if (%s) %s" % (if_cond, catch_body))
            if par8_t & 1:
                # bit0=1: TRUE jump -> cond ? else_val : then_val
                ternary = "(%s ? %s : %s)" % (cond, else_val, then_val)
            else:
                # bit0=0: FALSE jump -> cond ? then_val : else_val
                # 若 cond 是 !(X) 形式,简化: !(X) ? then_val : else_val == X ? else_val : then_val
                if cond.startswith("!(") and cond.endswith(")"):
                    inner = cond[2:-1]
                    ternary = "(%s ? %s : %s)" % (inner, else_val, then_val)
                else:
                    ternary = "(%s ? %s : %s)" % (cond, then_val, else_val)
            ctx.stack.append(ternary)
            ctx.pending_ternary = None

        # 在三元 then 分支中,跳过 catch 模式的异常路径 action
        # new_thread_catch(44): 不输出标记,记录 catch 块开始
        # end_thread_catch(26): 静默跳过
        # caught_error(14): 异常路径 (push 1),正常路径不执行
        # pop_stack(51): pop error 返回值,跳过
        # pop_n_stack(50): pop catch 返回值,跳过,标记 catch 块结束
        if (ctx.pending_ternary is not None and
                ctx.pending_ternary['state'] == 'then'):
            if action == 44:  # new_thread_catch: catch 块开始
                ctx.pending_ternary['in_catch'] = True
                ctx.pending_ternary['catch_start'] = len(ctx.lines)
                continue
            if action in (26, 14, 51):  # end_thread_catch, caught_error, pop_stack
                continue
            if action == 50:  # pop_n_stack: catch 块结束
                ctx.pending_ternary['in_catch'] = False
                ctx.pending_ternary['catch_lines'] = ctx.lines[ctx.pending_ternary['catch_start']:]
                del ctx.lines[ctx.pending_ternary['catch_start']:]
                # 清理 catch 期间残留的栈值,并提取 catch 表达式内容
                # catch 块栈操作: efun error -> push error_ret; small_int 0 -> push 0
                # pop_stack 被跳过 (未弹 error_ret), pop_n_stack par16 个值需弹出
                for _ in range(par16):
                    if ctx.stack:
                        ctx.stack.pop()
                # pop_stack (action 51) 被跳过,需补弹 error 返回值
                # 检查栈顶是否是 catch 内的函数调用 (如 error("...")),提取为 catch 内容
                if ctx.stack:
                    val = ctx.stack.pop()
                    # 若值是 error(...) 等 efun 调用,转换为 catch(expr) 语句
                    if val.startswith("error(") or val.startswith("throw("):
                        ctx.pending_ternary['catch_lines'] = \
                            ["    catch(%s);" % val]
                    else:
                        # 其他情况: 保留为表达式语句
                        ctx.pending_ternary['catch_lines'] = \
                            ["    %s;" % val]
                continue

        handler = DT.get(action)
        if handler is not None:
            handler(ctx, par16, par8)
        else:
            ctx.lines.append("    /* VM: action_%d(par16=0x%04X, par8=0x%02X) */" % (action, par16, par8))

    if ctx.stack:
        ctx.lines.append("    /* VM evaluation stack at end: %s */" % ", ".join(ctx.stack))
    # 控制流恢复:把 goto/label 转换成 if/else/while/switch
    return restore_control_flow(ctx.lines)



# ---------------------------------------------------------------------------
#  Render
# ---------------------------------------------------------------------------
def _format_fn_signature(fun, info, type_map, include_defaults=True):
    """生成函数签名串 (不含 { 或 ;)。复用参数默认值提取逻辑。

    include_defaults=True 时输出参数默认值 (用于声明/原型),
    include_defaults=False 时不输出默认值 (用于函数定义体)。
    """
    ret_type = type_map.get(fun.get("ret_type", 255), "mixed")
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
    arg_defaults = extract_arg_defaults(fun, info) if include_defaults else [None] * len(fun["args"])
    parts = []
    for idx, (n, t, r, _d) in enumerate(fun["args"]):
        part = "%s %s%s" % (type_map.get(t, "mixed"), "&" if r else "", n)
        if arg_defaults[idx] is not None:
            part += " = %s" % arg_defaults[idx]
        parts.append(part)
    args_sig = ", ".join(parts)
    return "%s%s %s(%s)" % (mod_str, ret_type, fun["name"], args_sig)


# ---------------------------------------------------------------------------
def render(info):
    # 类型映射: vm_valueType 编码 -> LPC 类型名
    # 适用于函数返回类型、参数类型、局部变量类型和 object_var 类型
    # 验证: vm_returnFromFun 中 v7==1 表示 void, v7==255 表示 mixed
    type_map = {
        0: "void", 1: "void", 2: "int", 3: "float", 4: "string",
        5: "buffer", 6: "array", 7: "mapping", 8: "mixed",
        9: "function", 10: "object", 255: "mixed",
    }
    # 收集所有被引用的 efun 索引,用于在头部注释中列出
    efun_refs = set()
    for fun in info["functions"]:
        for _pc, (_p16, _p8, action) in enumerate(fun["nodes"]):
            if action == 6:  # _h_efun
                efun_refs.add(_p16)
    efun_list = sorted(efun_refs)
    efun_summary = ", ".join("%d=%s" % (i, efun_name(i)) for i in efun_list[:20])
    if len(efun_list) > 20:
        efun_summary += ", ... (+%d more)" % (len(efun_list) - 20)

    out = ["/* Restored VM bytecode pseudo-LPC (v2.1). */",
           "/* source: %s */" % info["path"].name,
           "/* version: 0x%08X, functionBC: %d */" % (info["version"], info["function_bc"]),
           "/* functions: %d, DLT: %d (non-function DLT: %d) */" % (len(info["functions"]), info["dlt_count"], info["unparsed_dlt"]),
           "/* object variables (%d): %s */" % (len(info["object_vars"]), ", ".join(info["object_vars"]) or "none"),
           "/* inherits (%d): %s */" % (len(info["inherits"]), ", ".join(x[0] for x in info["inherits"]) or "none"),
           "/* program-strings: %d, efuns declared: %d */" % (len(info["program_strings"]), info["efun_count"]),
           "/* efun references (%d): %s */" % (len(efun_list), efun_summary or "none"),
           ""]
    # 渲染 inherits 表
    if info["inherits"]:
        # 输出 LPC inherit 声明语句 (源码中的 inherit "/path/to/file.c";)
        for inh_name, inh_data in info["inherits"]:
            out.append('inherit "%s";' % inh_name)
        out.append("")
        # 注释: 记录 inherit 的偏移信息 (baseConst, baseFun, baseVar, expSubConst, expFun, expVar)
        for inh_name, inh_data in info["inherits"]:
            out.append("/* inherit: %s (%s) */" % (inh_name, ", ".join(str(x) for x in inh_data)))
        out.append("")
    # 输出 defines (宏定义) - 完整展现所有预处理器定义
    # .o 文件中的 defines 包含源码中的 #define 宏,是反编译数据的重要组成部分
    if info.get("defines"):
        out.append("/* defines (%d) */" % len(info["defines"]))
        for flag, name, value in info["defines"]:
            # 清理 value 中的多余空白
            value_clean = ' '.join(value.split()) if value else ""
            if value_clean:
                out.append("#define %s %s" % (name, value_clean))
            else:
                out.append("#define %s" % name)
        out.append("")
    # 输出函数声明 (prototypes) - 放在 #define 之后、变量声明和全局代码段之前
    # 作用: LPC 源文件通常在文件开头声明所有函数,使后续函数定义顺序无关
    # 仅输出有函数体的函数 (nodes>0),nodes=0 的本身就是 prototype 会在函数列表中输出
    prototypes = []
    for fun in info["functions"]:
        if fun.get("_failed"):
            continue
        if fun.get("origin") == "global" or fun.get("name") == "__global_code__":
            continue
        if len(fun["nodes"]) == 0:
            continue  # 跳过原本就是 prototype 的函数,避免与函数列表重复
        prototypes.append("    %s;" % _format_fn_signature(fun, info, type_map))
    if prototypes:
        out.append("/* function prototypes (%d) */" % len(prototypes))
        out.extend(prototypes)
        out.append("")
    # 输出 object_vars 作为变量声明(完整展现所有对象变量)
    # .o 文件中声明的所有 object_vars 都应输出,即使未被代码引用
    # 类型来自 PROG_VARS 记录的 byte[8] (vm_valueType 编码)
    if info["object_vars"]:
        out.append("/* object variable declarations */")
        ovar_types = info.get("object_var_types", [])
        for idx, vname in enumerate(info["object_vars"]):
            vtype = type_map.get(ovar_types[idx], "mixed") if idx < len(ovar_types) else "mixed"
            out.append("%s %s;" % (vtype, vname))
        out.append("")
    # 输出 DLT GVAR (全局变量声明) - 完整展现所有全局变量
    # DLT 表中的 GVAR 条目是源码中声明的全局变量,即使未被字节码引用也应输出
    if info.get("dlt_gvar_entries"):
        out.append("/* global variable declarations (%d) */" % len(info["dlt_gvar_entries"]))
        for var_name, var_offset in info["dlt_gvar_entries"]:
            out.append("mixed %s;  /* offset: %d */" % (var_name, var_offset))
        out.append("")
    # 输出全局代码段 (顶层执行字节码) - 放在文件头,变量声明之后、函数定义之前
    # 这符合 LPC 源文件结构: 顶层代码在变量声明之后,函数定义之前/之间
    # 全局代码段 (vm_loadBinary 中的 vm_loadCodeSegment(&buf, 0, ...)):
    #   不是真实的函数 — 没有函数名/参数/返回类型,是.o 文件顶层的执行字节码,
    #   被 vm_spawnProcess 直接创建进程执行 (不是函数调用)。
    for fun in info["functions"]:
        if fun.get("origin") != "global" and fun.get("name") != "__global_code__":
            continue
        out.append("/* === global code segment at 0x%X; nodes=%d (顶层执行字节码,非函数) === */" % (
            fun["offset"], len(fun["nodes"])))
        # 失败保留: decompile 失败时输出原始 nodes 字节码,防止数据丢失
        try:
            body_lines = decompile(fun, info)
        except Exception as exc:
            body_lines = ["/* DECOMPILE FAILED: %s */" % str(exc),
                          "/* original nodes (%d) raw bytecode: */" % len(fun["nodes"])]
            for _pc, (_p16, _p8, _action) in enumerate(fun["nodes"]):
                body_lines.append("/*   pc=%d: action=%d par16=0x%04X par8=0x%02X */" % (
                    _pc, _action, _p16, _p8))
        # 顶层语句: 去掉函数体内的 4 空格缩进 (变为顶层 0 缩进)
        # 但保留嵌套语句的相对缩进
        top_lines = []
        for line in body_lines:
            if line.startswith("    "):
                top_lines.append(line[4:])
            else:
                top_lines.append(line)
        # 清理末尾冗余的 safety return (全局代码段末尾的 return 表示文件加载结束)
        if top_lines:
            last = top_lines[-1].strip()
            if last == "return;":
                top_lines.pop()
        out.extend(top_lines)
        out.append("")
        break  # 只有一个全局代码段
    # 输出函数定义列表 (跳过全局代码段,已在文件头输出)
    for fun in info["functions"]:
        # 跳过全局代码段 — 已在文件头输出
        if fun.get("origin") == "global" or fun.get("name") == "__global_code__":
            continue
        # 根据返回类型决定函数签名
        # retType 字段编码与 vm_valueType 一致 (vm_fun.c 的 vm_returnFromFun 用此字段检查返回值类型):
        #   0=undefined, 1=void, 2=integer, 3=float, 4=string, 5=buffer,
        #   6=array, 7=mapping, 8=pointer, 9=function, 10=object, 20=loop_ctrl, 255=mixed
        # 验证: vm_returnFromFun 中 v7==1 表示 void (不应返回值), v7==255 表示 mixed (不检查类型)
        ret_type = type_map.get(fun.get("ret_type", 255), "mixed")
        # 函数修饰符 (attrib 字段位定义, 通过反编译 vm_disassemble 0x8111d40 确认):
        #   bit0(1)=varargs, bit1(2)=primitive (内部标志,非 LPC 源码修饰符,不输出),
        #   bit2(4)=private, bit3(8)=override, bit4(16)=static
        # bit3=override 经验证: xuanming-baogualing.o 继承 teleport.c 并用 bit3 标记
        #   add_teleport/del_teleport/modify_teleport 等覆写方法
        # ret_attrib: bit3(8)=const (从 "public const mapping" 推断)
        out.append("/* %s function at 0x%X; nodes=%d, args=%d, locals=%d */" % (
            fun["origin"], fun["offset"], len(fun["nodes"]),
            len(fun["args"]), len(fun["locals"])))
        # 向前声明 (forward declaration / prototype): node_count=0, 无函数体
        # 64 字节函数头 bytes 16-19 = node_count, 0 表示这是原型声明而非定义
        # 这类函数有完整的签名 (返回类型、参数类型) 但没有实现体
        if len(fun["nodes"]) == 0:
            # 原型声明: 保留参数默认值
            sig = _format_fn_signature(fun, info, type_map, include_defaults=True)
            out.append("%s;" % sig)
            out.append("")
            continue
        # 函数定义: 不保留参数默认值 (默认值已在原型声明中)
        sig = _format_fn_signature(fun, info, type_map, include_defaults=False)
        out.append("%s {" % sig)
        # 过滤掉编译器生成的内部 loop_ctrl 变量 (<loop_N>)
        visible_locals = [(n, t) for n, t in fun["locals"] if not re.match(r'^<loop_\d+>$', n)]
        if visible_locals:
            # 按类型分组,同类型变量合并声明: "mixed a, b, c; int d, e;"
            from collections import OrderedDict
            groups = OrderedDict()
            for n, t in visible_locals:
                tn = type_map.get(t, "mixed")
                groups.setdefault(tn, []).append(n)
            parts = ["%s %s" % (tn, ", ".join(names)) for tn, names in groups.items()]
            locals_sig = ", ".join(parts)
            out.append("    %s;" % locals_sig)
        # 失败保留: decompile 失败时输出原始 nodes 字节码,防止数据丢失
        try:
            body_lines = decompile(fun, info)
        except Exception as exc:
            body_lines = ["    /* DECOMPILE FAILED: %s */" % str(exc),
                          "    /* original nodes (%d) raw bytecode: */" % len(fun["nodes"])]
            for _pc, (_p16, _p8, _action) in enumerate(fun["nodes"]):
                body_lines.append("    /*   pc=%d: action=%d par16=0x%04X par8=0x%02X */" % (
                    _pc, _action, _p16, _p8))
        # 后处理: 清理函数末尾冗余的 trailing safety return
        # 编译器在函数末尾自动添加 "return;" (无返回值的安全网)
        # 如果前一行已经是 return 语句,则移除 safety return (死代码)
        if body_lines:
            last = body_lines[-1].strip()
            if last == "return;":
                if len(body_lines) >= 2:
                    prev = body_lines[-2].strip()
                    if prev.startswith("return"):
                        # 前一行已是 return,移除冗余的 safety return
                        body_lines.pop()
        out.extend(body_lines)
        out.append("}")
        out.append("")
    if info["end"] != info["size"]:
        # 失败保留: parser_stopped 时输出原始字节 hex dump,防止数据丢失
        trailing = info["raw_data"][info["end"]:]
        out.append("/* parser stopped at 0x%X; %d trailing bytes retained uninterpreted. */" % (info["end"], len(trailing)))
        out.append("/* trailing bytes hex dump: */")
        hex_str = trailing.hex()
        for j in range(0, len(hex_str), 64):
            out.append("/*   %s */" % hex_str[j:j+64])
    # 失败保留: 输出所有解析失败点的原始数据
    # 用户要求: 失败也要保留,防止使用时出现错误导致无法修复
    failures = info.get("failures", [])
    if failures:
        out.append("")
        out.append("/* === FAILURES (%d) - 解析失败原始数据保留 === */" % len(failures))
        for i, fail in enumerate(failures):
            kind = fail.get("kind", "unknown")
            if kind == "dlt_function":
                out.append("/* FAILURE %d: kind=dlt_function dlt_offset=0x%X func_offset=0x%X end_offset=0x%X */" % (
                    i+1, fail["dlt_offset"], fail["func_offset"], fail["end_offset"]))
                out.append("/*   error: %s */" % fail["error"])
            elif kind == "unknown_dlt":
                out.append("/* FAILURE %d: kind=unknown_dlt dlt_offset=0x%X flags=0x%X */" % (
                    i+1, fail["dlt_offset"], fail["flags"]))
            elif kind == "global_code":
                out.append("/* FAILURE %d: kind=global_code offset=0x%X end_offset=0x%X */" % (
                    i+1, fail["offset"], fail["end_offset"]))
                out.append("/*   error: %s */" % fail["error"])
            else:
                out.append("/* FAILURE %d: kind=%s */" % (i+1, kind))
            raw = fail.get("raw_bytes", b"")
            if raw:
                out.append("/*   raw bytes (%d): */" % len(raw))
                hex_str = raw.hex()
                for j in range(0, len(hex_str), 64):
                    out.append("/*   %s */" % hex_str[j:j+64])
    # 输出所有未被字节码引用的 program_strings (孤儿字符串)
    # 这些字符串在 .o 文件中定义但未被 action 64/11 引用,可能源自编译器死代码消除或条件编译
    # 用户要求保留所有文本数据,不丢弃任何内容
    referenced_strs = set()
    import struct as _struct
    for fun in info["functions"]:
        for _pc, (_p16, _p8, action) in enumerate(fun["nodes"]):
            if action == 64:  # push_shared_string
                referenced_strs.add(_p16)
            elif action == 11:  # case_check_str
                off = _meta_offset(_p16, _p8)
                meta = fun.get("meta", b"")
                if off + 4 <= len(meta):
                    stridx = _struct.unpack_from("<I", meta, off + 4)[0]
                    referenced_strs.add(stridx)
    unreferenced_strs = [(i, s) for i, s in enumerate(info["program_strings"]) if i not in referenced_strs]
    if unreferenced_strs:
        out.append("")
        out.append("/* unreferenced program-strings (%d) - 字节码未引用但 .o 文件中定义的字符串 */" % len(unreferenced_strs))
        for idx, s in unreferenced_strs:
            # 转义特殊字符用于注释
            s_escaped = s.replace('*/', '*\\/').replace('\n', '\\n').replace('\r', '\\r')
            if len(s_escaped) > 120:
                s_escaped = s_escaped[:120] + "..."
            out.append('/* [%d] %r */' % (idx, s_escaped))
    return "\n".join(out) + "\n"


def output_path(directory, source, base=None):
    """计算输出文件路径, 在输出目录下保持源文件相对输入目录的多层结构。

    参数:
      directory: 输出根目录 (Path, 相对于当前工作目录, 不嵌入输入目录下)
      source: 源 .o 文件路径 (Path)
      base: 输入根目录 (Path), 用于计算相对路径以保持多层目录结构。
            若为 None 或 source 不在 base 下, 则平铺到 directory 下。

    返回: 目标 .c 文件路径 (Path), 若已存在则直接覆盖。
    """
    if base is not None:
        try:
            rel = source.relative_to(base)
            target_dir = directory / rel.parent
        except ValueError:
            target_dir = directory
    else:
        target_dir = directory
    target_dir.mkdir(parents=True, exist_ok=True)
    return target_dir / (source.stem + ".c")


def main():
    parser = argparse.ArgumentParser(description="Optimized VM .o to pseudo-LPC restorer v2.0")
    parser.add_argument("input", nargs="?", default=".",
                        help="输入文件或目录 (支持多层目录递归)")
    parser.add_argument("-o", "--output", default="restored_lpc",
                        help="输出目录 (相对于当前工作目录, 与输入目录同级)")
    parser.add_argument("-g", "--global-file", default=None,
                        help="启动文件 (全局程序 .o 文件), 用于解析全局函数/变量名。"
                             "未指定则不搜索启动文件 (大量文件时显著加速)")
    parser.add_argument("-v", action="store_true", help="verbose")
    ns = parser.parse_args()
    base = Path(ns.input)
    # 多层目录递归查找所有 .o 文件 (rglob 递归遍历子目录)
    if base.is_dir():
        files = sorted(base.rglob("*.o"))
    else:
        files = [base]
    # 输出目录始终相对于当前工作目录, 不嵌入输入目录下
    output = Path(ns.output)
    if not output.is_absolute():
        output = Path.cwd() / output
    output.mkdir(parents=True, exist_ok=True)
    failures = 0
    t0 = time.perf_counter()
    # 预扫描全局程序: 构建全局变量名和全局函数名映射 (action 53/54/76/7-bit7 使用)
    # 优化: 仅当显式指定 -g 时才解析启动文件, 否则跳过 (大量文件时避免预扫描开销)
    _init_global_maps(files, global_file=ns.global_file)
    for source in files:
        try:
            info = parse_file(source)
            # 在输出目录下保持源文件的多层目录结构, 同名直接覆盖
            target = output_path(output, source, base if base.is_dir() else None)
            target.write_text(render(info), encoding="gb2312", errors="replace", newline="\n")
            fail_count = len(info.get("failures", []))
            print("OK %s -> %s | functions=%d inherits=%d object_vars=%d failures_retained=%d" % (
                source.name, target.relative_to(output),
                len(info["functions"]), len(info["inherits"]),
                len(info["object_vars"]), fail_count))
        except (ParseError, OSError, struct.error, UnicodeError) as exc:
            failures += 1
            # 失败保留: parse_file 完全失败时,保存原始字节到 .failed.bin
            # 用户要求: 失败也要保留,防止使用时出现错误导致无法修复
            # 保持与源文件相同的相对目录结构, 同名直接覆盖
            try:
                failed_dir = output
                if base.is_dir():
                    try:
                        rel = source.relative_to(base)
                        failed_dir = output / rel.parent
                    except ValueError:
                        failed_dir = output
                failed_dir.mkdir(parents=True, exist_ok=True)
                failed_bin = failed_dir / (source.stem + ".failed.bin")
                failed_bin.write_bytes(source.read_bytes())
                print("FAIL %s: %s (raw bytes saved to %s)" % (
                    source.name, exc, failed_bin.relative_to(output)), file=sys.stderr)
            except OSError:
                print("FAIL %s: %s (could not save raw bytes)" % (
                    source.name, exc), file=sys.stderr)
    elapsed = time.perf_counter() - t0
    print("Done in %.3fs, %d files, %d failures." % (elapsed, len(files), failures))
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
