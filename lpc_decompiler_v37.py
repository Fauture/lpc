# -*- coding: gbk -*-
"""
LPC .o Decompiler v37 - Readability Enhanced Version

Based on v36 with verified 86-entry opcode table from IDA actionInfoTable @ 0x83CC680
Major improvements:
  - Intelligent variable name mapping from .o file metadata
  - EFUN documentation with inline functional descriptions
  - Enhanced pseudocode with control flow structure visualization
  - Switch/case detection and beautification
  - Function-level annotations and purpose inference
  - Cleaner expression formatting with reduced noise
  - Structured output with separated code/decorator sections
"""
import os, struct, sys, re, bisect, gc
from collections import Counter

# SEFUN_TABLE: maps global function indices (>=572) to names.
# Auto-generated from start_aaa.o (primary) and start_gs.o (secondary).
# Used to resolve global_fn_XXX references in call_user/new_func.
try:
    from sefun_table import SEFUN_TABLE
except ImportError:
    SEFUN_TABLE = {}

# GLOBAL_VAR_TABLE: maps global variable indices to names, sourced from the
# startup .o (e.g. start_aaa.o). Used to resolve global_XX references in files
# that have no own variable table (Variables: 0) or whose index is out of range.
# Populated dynamically by -q/--sefun; stays empty when -q is absent.
GLOBAL_VAR_TABLE = {}

# Parent .o parse cache: in batch mode many files inherit from the same parent
# .o files, and _load_inherited_funcs re-parses them every time. Cache the
# extracted func_table per (abspath, encoding) so shared parents are parsed
# only once per process. Only the small func_table dict is retained.
_PARENT_TABLE_CACHE = {}


def _parse_parent_func_table(filepath, encoding):
    """Parse a parent .o file and return its func_table (cached per process).

    Used by _load_inherited_funcs to fill inherited function names. The cache
    is keyed by absolute path + encoding; each worker process keeps its own
    cache, so batch decompilation only parses each distinct parent once.
    """
    key = (os.path.abspath(filepath), encoding)
    hit = _PARENT_TABLE_CACHE.get(key)
    if hit is not None:
        return hit
    o = LPCOFile(filepath, encoding=encoding)
    o.parse()
    _PARENT_TABLE_CACHE[key] = o.func_table
    return o.func_table

# ============================================================================
# Type System (from IDA vm_efunInit @ 0x80D4245)
# ============================================================================
TYPE_CODES = {
    0:  "undefined", 1: "void",      2: "int",       3: "float",
    4:  "string",    5: "buffer",    6: "array",     7: "mapping",
    8:  "vptr",      9: "function",  10: "object",    255: "mixed"
}

# typeof反向映射：类型码 -> is_X()函数名
# 来自IDA vm_yyparse分析：is_X(x)在编译期被转为typeof操作码+类型码
# 反编译时反向还原为is_X(x)形式，使输出更接近原始源码
IS_TYPE_MAP = {
    0: "is_undefined", 1: "is_void", 2: "is_int", 3: "is_real",
    4: "is_string", 5: "is_buffer", 6: "is_array", 7: "is_mapping",
    9: "is_function", 10: "is_object"
}

# ============================================================================
# Verified Opcode Table (from IDA actionInfoTable @ 0x83CC680, 86 entries)
# Format: actionNo -> (handler_name, param_count, attrib, lpc_semantic)
# ============================================================================
OPCODES = {
    0:  ("nullAction",         0, 0x00,        "nop"),
    1:  ("addValue",           0, 0x00,        "+"),
    2:  ("andValue",           0, 0x00,        "&"),
    3:  ("assignValue",        0, 0xd0,        "="),
    4:  ("assignValueReverse", 0, 0xd0,        "=rev"),
    5:  ("callFunPtr",         2, 0x60,        "call_ptr"),
    6:  ("callInEfun",         2, 0x60,        "call_efun"),
    7:  ("callInUserFun",      2, 0x60,        "call_user"),
    8:  ("callInOtherFunEx",   2, 0x60,        "call_other"),
    9:  ("callOSFun",          2, 0x80000060,  "call_os"),
    10: ("caseCheckNumber",    2, 0x80000001,  "case_num"),
    11: ("caseCheckString",    2, 0x80000001,  "case_str"),
    12: ("caseDefault",        1, 0x01,        "default"),
    13: ("castType",           2, 0x00,        "cast"),
    14: ("caughtError",        0, 0x00,        "catch"),
    15: ("checkArgNum",        1, 0x01,        "checkargs"),
    16: ("checkIsRefType",     1, 0x00,        "checkreftype"),
    17: ("checkIsRefTypeI",    0, 0x00,        "checkreftype_i"),
    18: ("condJmpForLoop",     2, 0x01,        "for_loop_jmp"),
    19: ("convertToPtr",       0, 0x40,        "to_ptr"),
    20: ("convertToValue",     0, 0x40,        "to_value"),
    21: ("createArgumentArray",0, 0x00,        "arg_array"),
    22: ("createArray",        1, 0x00,        "new_array"),
    23: ("createFunctionValue",2, 0x00,        "new_func"),
    24: ("createMapping",      1, 0x00,        "new_mapping"),
    25: ("divValue",           0, 0x00,        "/"),
    26: ("endThreadCatch",     0, 0x00,        "end_catch"),
    27: ("enterFunction",      0, 0x00,        "enter"),
    28: ("eqValue",            0, 0x00,        "=="),
    29: ("finishAction",       0, 0x00,        "finish"),
    30: ("geValue",            0, 0x00,        ">="),
    31: ("goto",               1, 0x01,        "goto"),
    32: ("gtValue",            0, 0x00,        ">"),
    33: ("if",                 2, 0x01,        "if_jmp"),
    34: ("incValue",           1, 0xc0,        "++"),
    35: ("initForLoop",        1, 0x00,        "for_init"),
    36: ("isType",             1, 0x00,        "typeof"),
    37: ("leValue",            0, 0x00,        "<="),
    38: ("lshValue",           0, 0x00,        "<<"),
    39: ("ltValue",            0, 0x00,        "<"),
    40: ("modValue",           0, 0x00,        "%"),
    41: ("mulValue",           0, 0x00,        "*"),
    42: ("neValue",            0, 0x00,        "!="),
    43: ("negValue",           0, 0x00,        "neg"),
    44: ("newThreadCatch",     1, 0x01,        "try"),
    45: ("notValue",           0, 0x00,        "!"),
    46: ("nullAction",         0, 0x00,        "nop"),
    47: ("opEqValue",          1, 0xc0,        "op_eq"),
    48: ("orValue",            0, 0x00,        "|"),
    49: ("pickupOSValue",      2, 0x80000000,  "pickup_os"),
    50: ("popNStack",          1, 0x00,        "pop_n"),
    51: ("popStack",           0, 0x00,        "pop"),
    52: ("pushArgNum",         0, 0x00,        "push_argnum"),
    53: ("pushGlobalVar",      1, 0x04,        "push_global"),
    54: ("pushGlobalVptr",     1, 0x42,        "push_global_ptr"),
    55: ("pushLocalVar",       1, 0x04,        "push_local"),
    56: ("pushLocalVarI",      0, 0x04,        "push_local_i"),
    57: ("pushLocalVptr",      1, 0x42,        "push_local_ptr"),
    58: ("pushLocalVptrI",     0, 0x42,        "push_local_ptr_i"),
    59: ("pushShortNumber",    1, 0x08,        "push_i"),
    60: ("pushNumber",         1, 0x80000008,  "push_num"),
    61: ("pushObjVar",         1, 0x04,        "push_ovar"),
    62: ("pushObjVptr",        1, 0x42,        "push_ovar_ptr"),
    63: ("pushRealFromWord",   2, 0x80000008,  "push_real"),
    64: ("pushSharedString",   1, 0x08,        "push_s"),
    65: ("pushThisObject",     0, 0x08,        "push_this"),
    66: ("pushUndefined",      0, 0x08,        "push_0"),
    67: ("queryMember",        1, 0x40,        "member"),
    68: ("queryMemberPtr",     1, 0x40,        "member_ptr"),
    69: ("queryRange",         1, 0x40,        "range"),
    70: ("queryRangePtr",      1, 0x40,        "range_ptr"),
    71: ("resortStack",        0, 0x00,        "resort"),
    72: ("returnFromFakeFun",  0, 0x00,        "ret_fake"),
    73: ("returnFromFun",      0, 0x00,        "ret"),
    74: ("revValue",           0, 0x00,        "~"),
    75: ("rshValue",           0, 0x00,        ">>"),
    76: ("setGlobalVar",       1, 0x50,        "set_global"),
    77: ("setLocalVar",        1, 0x50,        "set_local"),
    78: ("setObjVar",          1, 0x50,        "set_ovar"),
    79: ("sizeof",             0, 0x00,        "sizeof"),
    80: ("stepJmpForLoop",     2, 0x01,        "for_step"),
    81: ("subValue",           0, 0x00,        "-"),
    82: ("toReserveStackValue",1, 0x00,        "reserve"),
    83: ("toUnreserveStackValue",1, 0x00,       "unreserve"),
    84: ("trace",              1, 0x00,        "trace"),
    85: ("xorValue",           0, 0x00,        "^"),
}

KNOWN_OPS = set(OPCODES.keys())

# ============================================================================
# EFUN Documentation - Short functional descriptions
# ============================================================================
EFUN_DOCS = {
    "abs": "|x|",
    "atoi": "str->int",
    "atof": "str->float",
    "bind_parameters": "?=val",
    "broadcast": "msg->all",
    "call_other": "obj->func",
    "catch": "err?",
    "clean_array": "dedup[]",
    "clone_object": "clone()",
    "create_thread": "new_thread",
    "ctime": "time->str",
    "delete_thread": "kill_thread",
    "destruct_object": "destroy()",
    "error": "throw",
    "explode": "split()",
    "find_object": "lookup_obj",
    "find_function": "lookup_fn",
    "get_thread_info": "thread_info",
    "implode": "join()",
    "is_array": "is[]",
    "is_mapping": "is([",
    "is_object": "is_obj",
    "is_string": "is_str",
    "is_int": "is_int",
    "is_function": "is_fn",
    "keys": "keys()",
    "load_object": "load_obj",
    "log_file": "log()",
    "lower_case": "lc()",
    "md5": "MD5",
    "member_array": "index",
    "move_object": "move()",
    "notify_fail": "err_msg",
    "objectp": "is_obj",
    "present": "find_inv",
    "previous_object": "caller",
    "printf": "print_f",
    "random": "rand()",
    "regexp": "regex",
    "replace_string": "replace",
    "restore_object": "load_obj",
    "save_object": "save_obj",
    "save_string": "intern_str",
    "say": "broadcast",
    "set_as_daemon": "daemon",
    "shutdown": "exit",
    "sizeof": "len()",
    "sleep": "sleep()",
    "sort": "sort[]",
    "sprintf": "fmt()",
    "sscanf": "parse()",
    "strlen": "len",
    "strcmp": "str==",
    "tell_object": "msg->obj",
    "tell_room": "msg->room",
    "this_object": "self",
    "this_player": "player",
    "time": "now()",
    "to_int": "int()",
    "to_string": "str()",
    "trim": "trim()",
    "typeof": "type()",
    "values": "vals()",
    "write": "print",
    "sin": "sin", "cos": "cos", "tan": "tan",
    "pow": "x^y", "exp": "e^x", "log": "ln",
    "ceil": "ceil", "floor": "floor",
    "sqrt": "sqrt",
    "mysql_bind_parameters": "sql_bind",
    "mysql_connect": "sql_connect",
    "mysql_execute": "sql_exec",
    "mysql_query": "sql_query",
    "mysql_fetch_record": "sql_fetch",
    "mysql_num_records": "sql_rows",
    "mysql_field_value": "sql_field",
    "sql_open_db": "sql_open",
    "sql_execute": "sql_exec",
    "sql_query": "sql_query",
    "sql_step": "sql_next",
}

def efun_doc(name):
    """Get EFUN short description"""
    if name in EFUN_DOCS:
        return EFUN_DOCS[name]
    # Auto-generate for common patterns
    if name.startswith("mysql_"):
        return "sql:" + name[6:]
    if name.startswith("sql_"):
        return "sql:" + name[4:]
    if name.startswith("is_"):
        return "is?" + name[3:]
    if name.startswith("get_"):
        return "get"
    if name.startswith("set_"):
        return "set"
    return ""

# ============================================================================
# Function Purpose Inference
# ============================================================================
FUNC_PURPOSE_PATTERNS = [
    (r'^(create|do_init|setup)$',    'constructor: initialize object state'),
    (r'^destruct$',                   'destructor: cleanup before object removal'),
    (r'^query_',                      'accessor: read property'),
    (r'^get_',                        'accessor: retrieve value'),
    (r'^set_',                        'mutator: set property'),
    (r'^do_',                         'action handler: process request'),
    (r'^handle_',                     'event handler: respond to event'),
    (r'^write_',                      'data writer: persist information'),
    (r'^fill_',                       'data mapper: populate structure'),
    (r'^select_',                     'query: database retrieval'),
    (r'^is_|^has_|^can_',            'predicate: boolean check'),
    (r'^_do_|^_handle_',             'internal handler: sub-action processor'),
    (r'^to_',                         'converter: transform data'),
    (r'^parse_|^decode_',            'parser: extract/transform data'),
    (r'^validate_|^check_',          'validator: verify data'),
    (r'^notify_|^notice_',           'notification: send message'),
    (r'^verify_|^assure_',           'verification: confirm/guarantee state'),
]

def infer_purpose(name, insts):
    """Infer function purpose from name pattern and bytecode patterns"""
    for pattern, desc in FUNC_PURPOSE_PATTERNS:
        if re.match(pattern, name):
            return desc
    # Additional bytecode-level inference
    has_return = any(a == 73 for a, _, _ in insts)
    has_call_other = any(a == 8 for a, _, _ in insts)
    has_efun = any(a == 6 for a, _, _ in insts)
    if not has_return:
        return "void: side-effect only, no return value"
    if has_call_other:
        return "interaction: calls other objects"
    return ""


# Argument name hints based on function name patterns (LPC conventions)
_ARG_NAME_HINTS = {
    'create': [''],
    'do_init': [''],
    'query_': ['key', 'name'],
    'get_': ['key', 'name', 'id'],
    'set_': ['key', 'value'],
    'is_': ['target'],
    'has_': ['target'],
    'can_': ['target'],
    'check_': ['target', 'arg'],
    'verify_': ['target', 'arg'],
    'count_': ['target'],
    'find_': ['target', 'key'],
    'search_': ['target', 'key'],
    'load_': ['filename', 'key'],
    'save_': ['filename', 'data'],
    'read_': ['filename', 'key'],
    'write_': ['filename', 'data'],
    'handle_': ['arg', 'data'],
    'do_': ['arg', 'data'],
    'parse_': ['input', 'format'],
    'format_': ['data', 'format'],
    'build_': ['data', 'type'],
    'append_': ['target', 'data'],
    'fill_': ['target', 'data'],
    'remove_': ['target', 'key'],
    'delete_': ['target', 'key'],
    'update_': ['target', 'data'],
    'notify_': ['target', 'msg'],
    'send_': ['target', 'msg'],
    'to_': ['value'],
}


def infer_arg_names(fname, nargs, insts, str_table):
    """Infer argument names from function name and usage patterns."""
    if nargs == 0:
        return []
    names = []
    # Get hint from function name prefix
    hint = None
    for prefix, args in _ARG_NAME_HINTS.items():
        if fname == prefix or fname.startswith(prefix):
            hint = args
            break

    if hint:
        for i in range(nargs):
            if i < len(hint) and hint[i]:
                names.append(hint[i])
            else:
                names.append("arg_%d" % i)
    else:
        # Default: arg_0, arg_1, ...
        for i in range(nargs):
            names.append("arg_%d" % i)

    # Enhance: scan bytecode for string constants used near arg_0/arg_1
    # to infer better names (e.g., if arg_0 is used as a key for a mapping
    # with string "name", rename arg_0 to "name_key")
    return names


# Argument type hints based on function name patterns
_ARG_TYPE_HINTS = {
    'is_': ['string'],
    'has_': ['string'],
    'can_': ['string'],
    'count_': ['mixed'],
    'find_': ['string', 'string'],
    'search_': ['string', 'string'],
    'load_': ['string', 'string'],
    'save_': ['string', 'mixed'],
    'read_': ['string', 'string'],
    'write_': ['string', 'mixed'],
    'set_': ['string', 'mixed'],
    'query_': ['string', 'mixed'],
    'get_': ['string', 'mixed'],
    'check_': ['string'],
    'verify_': ['string'],
    'parse_': ['string', 'string'],
    'format_': ['mixed', 'string'],
}


def _needs_paren_for_member(obj):
    """Return True if `obj` (the LHS of a `->` member call) is a complex
    expression that must be wrapped in parentheses so the '->' operator binds
    to the whole expression.

    Simple cases that need NO parens:
      - bare identifiers: `ob`, `agent`
      - string literals: `"/path/obj.c"`
      - parameterless calls: `this_object()`, `previous_object()`

    Everything else (array subscript `arr[i]`, chained `a->b`, call results
    `f(x)`, binary/ternary ops with spaces, etc.) needs parentheses.
    """
    if not isinstance(obj, str) or not obj:
        return True
    s = obj.strip()
    # string literal
    if s.startswith('"'):
        return False
    # bare identifier
    if re.match(r'^[A-Za-z_]\w*$', s):
        return False
    # parameterless function call: this_object(), previous_object(), etc.
    if re.match(r'^[A-Za-z_]\w*\(\)$', s):
        return False
    return True


def extract_default_args(insts, min_arg, nargs, str_table):
    """Extract optional-argument default values from the function header
    bytecode. Between checkargs(an=15) and enter(an=27), the compiler emits
    push instructions for each optional argument's default value (in order,
    starting at arg index = min_arg). resort(an=71) rearranges and is skipped.

    Returns a dict {arg_idx: default_value_string}.
    """
    defaults = {}
    if not insts or min_arg is None or min_arg < 0 or min_arg >= nargs:
        return defaults
    i = 0
    n = len(insts)
    while i < n and insts[i][0] != 15:  # locate checkargs
        i += 1
    if i >= n:
        return defaults
    i += 1  # skip checkargs
    vals = []
    while i < n:
        an, p16, p8 = insts[i]
        if an == 27:  # enter - end of header prologue
            break
        if an == 64:  # push_s
            if 0 <= p16 < len(str_table):
                s = str_table[p16]
                esc = s.replace('\\', '\\\\').replace('"', '\\"')
                vals.append('"%s"' % esc)
            else:
                vals.append('""')
        elif an == 59:  # push_i (16-bit signed)
            v = p16 - 0x10000 if p16 >= 0x8000 else p16
            vals.append(str(v))
        elif an == 60:  # push_num (32-bit)
            v = p16 | (p8 << 16)
            vals.append(str(v))
        elif an == 66:  # push_0 / pushUndefined
            vals.append("0")
        elif an == 63:  # push_real
            raw = p16 | (p8 << 16)
            fv = struct.unpack('<f', struct.pack('<i', raw))[0]
            s = ("%.6f" % fv).rstrip('0').rstrip('.')
            vals.append(s if s else "0")
        # an == 71 (resort) and other non-push opcodes: skip
        i += 1
    for j, v in enumerate(vals):
        arg_idx = min_arg + j
        if 0 <= arg_idx < nargs:
            defaults[arg_idx] = v
    return defaults


def infer_arg_types(fname, nargs, insts, str_table):
    """Default all argument types to 'mixed'; real types come from DLT
    (Vm_Fun_Arg_T type byte) which overrides these defaults in the caller.
    Name-pattern hints are no longer used because they produced wrong types
    (e.g. 'check_' -> 'string') when DLT declared 'mixed'."""
    return ["mixed"] * nargs if nargs > 0 else []

# ============================================================================
# Efun Table - loaded from efun_table.json
#   v37 variant: 572 entries (magic_Linux32x, IDA vm_addEfuns registration order)
#   old variant: 561 entries (legacy magic_Linux32), enabled via -old
# SEFUN threshold follows len(EFUN_NAMES) automatically.
# ============================================================================
def _load_efun_names(variant='v37'):
    """Load EFUN_NAMES from efun_table.json (same dir as this script)."""
    import json as _json
    _json_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'efun_table.json')
    with open(_json_path, 'r', encoding='gb2312') as _f:
        _tables = _json.load(_f)
    names = _tables.get(variant) or _tables.get('v37')
    if not names:
        raise RuntimeError("efun_table.json: variant '%s' not found" % variant)
    return list(names)

EFUN_NAMES = _load_efun_names('v37')  # default; -old in main() reloads 'old'

EFUN_TABLE = {i: name for i, name in enumerate(EFUN_NAMES)}

def efun_name(idx):
    return EFUN_TABLE.get(idx, "efun_%d" % idx)

# ============================================================================
# .o File Parser (same core logic as v36)
# ============================================================================
class LPCOFile:
    def __init__(self, filepath, encoding='gb2312'):
        self.filepath = filepath
        self.encoding = encoding
        with open(filepath, 'rb') as f:
            self.data = f.read()
        self.filename = os.path.basename(filepath)

    def parse(self):
        data = self.data
        try:
            return self._parse(data)
        except Exception as e:
            self._set_defaults()
            return self

    def _set_defaults(self):
        self.header = {}
        self.inherited_func = ''
        self.inherit_names = []  # Names of inherited programs (from INHERITS section)
        self.hidden_funcs = []  # Parsed HIDDEN_FUNS entries (vm_loadFun format)
        self.str_table = []
        self.user_funcs = []
        self.all_symbol_names = []
        self.all_name_bytes = []
        self.local_vars = []
        self.func_local_vars = {}  # {vm_idx: name} for function local vars (legacy global)
        self.func_local_vars_by_seg = {}  # {nodes_start: {vm_idx: name}} per-function
        self.func_args_by_seg = {}  # {nodes_start: [(vm_idx, name), ...]} per-function
        self.func_arg_types_by_seg = {}  # {nodes_start: {vm_idx: type_byte}} per-function
        self.func_ret_types_by_seg = {}  # {nodes_start: ret_type_byte} per-function
        self.func_attribs_by_seg = {}  # {nodes_start: attrib_word} per-function
        self.func_min_arg_nos_by_seg = {}  # {nodes_start: minArgNo} from Vm_Fun_T
        self.func_max_arg_nos_by_seg = {}  # {nodes_start: maxArgNo} from Vm_Fun_T
        self.func_decl_info_by_name = {}  # {func_name: {ret_type, args, arg_types, attrib, min_arg, max_arg}}
        self.dlt_func_segs = {}  # {nodes_start: nodes_count} from DLT parsing (authoritative)
        self.dlt_func_names_by_seg = {}  # Patch 44: {nodes_start: func_name} authoritative
        self.init_code_start = 0  # Start of init code segment (post-DLT)
        self.source_file = ''
        self.user_strings = []
        self.prog_start = 0
        self.prog = b''
        self.func_table = {}  # {table_idx: func_name} from DLT segment
        self.var_types = {}   # {valueNo: type_byte} from PROG_VARS

    def _parse(self, data):
        ver = struct.unpack_from('<HH', data, 0)
        fBC, definesCount = struct.unpack_from('<II', data, 8)
        self.header = dict(version=ver, functionBC=fBC, definesCount=definesCount)
        # Initialize per-function local var/arg containers (populated by
        # _parse_dlt_func_locals later in _parse_var_names).
        self.func_local_vars_by_seg = {}
        self.func_args_by_seg = {}
        self.func_arg_types_by_seg = {}
        self.func_ret_types_by_seg = {}
        self.func_attribs_by_seg = {}
        self.func_min_arg_nos_by_seg = {}
        self.func_max_arg_nos_by_seg = {}
        self.func_decl_info_by_name = {}
        self.dlt_func_segs = {}
        self.dlt_func_names_by_seg = {}  # Patch 44: {nodes_start: func_name}

        # File layout per vm_loadBinary (vm_binary.c):
        #   [0]  version (4B) = 0x00010001
        #   [4]  reserved (4B)
        #   [8]  functionBC (4B)
        #   [12] definesCount (4B) — read by vm_loadDefines
        #   [16] defines[definesCount]: each = nargs(4B) + name_len(4B) + name + exps_len(4B) + exps
        #   [N]  ptr = string count (4B)
        #   [N+4] strings[ptr]: each = slen(4B) + content(slen bytes, null-terminated)
        #   [...] identifiers (vm_loadIdentifiers)
        #   [...] code segment (vm_loadCodeSegment)
        self.inherited_func = ''
        self.inherit_names = []
        self.hidden_funcs = []

        # 1. Skip defines (vm_loadDefines format)
        pos = 16
        self.defines = []
        if 0 < definesCount < 50000:
            for _ in range(definesCount):
                if pos + 8 > len(data): break
                nargs = struct.unpack_from('<I', data, pos)[0]; pos += 4
                nlen = struct.unpack_from('<I', data, pos)[0]; pos += 4
                if nlen <= 0 or nlen > 5000: break
                if pos + nlen > len(data): break
                name = data[pos:pos+nlen].decode(self.encoding, 'surrogateescape').rstrip('\x00'); pos += nlen
                if pos + 4 > len(data): break
                elen = struct.unpack_from('<I', data, pos)[0]; pos += 4
                if elen <= 0 or elen > 5000: break
                if pos + elen > len(data): break
                exps = data[pos:pos+elen].decode(self.encoding, 'surrogateescape').rstrip('\x00'); pos += elen
                self.defines.append((name, exps, nargs))

        # 2. Read string pool
        str_entries = []
        if pos + 4 <= len(data):
            ptr = struct.unpack_from('<I', data, pos)[0]; pos += 4
            if 0 < ptr < 50000:
                for _ in range(ptr):
                    if pos + 4 > len(data): break
                    slen = struct.unpack_from('<I', data, pos)[0]
                    if slen == 0 or slen > 5000: break
                    pos += 4
                    if pos + slen > len(data): break
                    raw = data[pos:pos+slen]
                    s = raw.decode(self.encoding, errors='surrogateescape').rstrip('\x00')
                    str_entries.append(s)
                    pos += slen
        self.str_table = str_entries
        self.prog_start = pos
        self.prog = data[pos:]

        self._parse_func_names()
        self._parse_var_names()
        self._parse_func_table()
        # Patch 18: 修正 source_file 误用 inherit 路径
        # _parse_var_names 扫描的 .c 路径可能包含 inherit 路径
        # 移除 inherit_names 中的路径, 取剩余作为 source_file
        if hasattr(self, 'source_files') and self.inherit_names:
            inherit_set = set(self.inherit_names)
            non_inherit = [s for s in self.source_files if s not in inherit_set]
            if non_inherit:
                self.source_file = non_inherit[0]
        # Load inherited function names from parent .o files so that
        # call_user references to inherited functions (table_idx < N)
        # are resolved to their real names instead of `local_fn_N`.
        self._load_inherited_funcs()
        known_inherited = {'Input password: ', '.', 'Logout\n', 'Bad password!\n\n',
                           'New telnet cilent connected! i = %d\n', '%s', '\n%s', ''}
        self.user_strings = [s for s in str_entries
                            if s not in known_inherited and s and '\x00' not in s
                            and any(c.isprintable() for c in s)]
        return self

    def _load_inherited_funcs(self):
        """Load parent .o files and merge their func_table to fill
        inherited function names (table_idx < child's own start).
        Also track idx->parent_path mapping for fallback reference."""
        # idx -> parent inherit name (for precise `"/path"::local_fn_N`)
        self._inherit_idx_to_path = {}
        # Parent inherit names that failed to load (.o missing/invalid)
        self._inherit_failed = []
        # (start_idx, end_idx, iname) for each successfully loaded parent.
        # Used to find the correct parent path for call_user indices that
        # fall in a parent's range but aren't in func_table (e.g. the
        # parent's own inherited functions).
        self._inherit_ranges = []
        if not self.inherit_names:
            return
        # Resolve game root: walk up from self.filepath until a parent
        # .o file (computed from the first inherit name) exists.
        o_dir = os.path.dirname(self.filepath)
        def _resolve(root, iname):
            base = iname[:-2] if iname.endswith('.c') else iname
            rel = base.lstrip('/').replace('/', os.sep) + '.o'
            return os.path.join(root, rel)
        game_root = None
        cur = o_dir
        for _ in range(10):
            cand = _resolve(cur, self.inherit_names[0])
            if os.path.exists(cand):
                game_root = cur
                break
            parent = os.path.dirname(cur)
            if parent == cur:
                break
            cur = parent
        if game_root is None:
            # All parents failed to load; record them for fallback path
            self._inherit_failed = list(self.inherit_names)
            return
        # Patch 23: use baseFunOffset from INHERITS segment instead of
        # cumulative idx_cursor. baseFunOffset is the starting table_idx
        # of each parent's functions in the child's func_table.
        # Verified via vm_loadBinaryInherit @ 0x80a2b58 and arenad.o:
        # eternal_data.c baseFunOffset=13 (3 funcs -> idx 13-15),
        # task_timer.c baseFunOffset=16 (8 funcs -> idx 16-23),
        # register.c baseFunOffset=24 (6 funcs -> idx 24-29),
        # form2.c baseFunOffset=30 (5 funcs -> idx 30-34).
        # Child's own functions start at idx 35.
        base_offsets = getattr(self, '_inherit_base_fun_offsets', [])
        idx_cursor = 0  # fallback when baseFunOffset is unavailable
        for i, iname in enumerate(self.inherit_names):
            parent_path = _resolve(game_root, iname)
            if not os.path.exists(parent_path):
                self._inherit_failed.append(iname)
                continue
            try:
                parent_ft = _parse_parent_func_table(parent_path, self.encoding)
            except Exception:
                self._inherit_failed.append(iname)
                continue
            if not parent_ft:
                if iname not in self._inherit_failed:
                    self._inherit_failed.append(iname)
                # Still advance idx_cursor for subsequent parents
                continue
            # Use baseFunOffset if available, else fall back to idx_cursor
            if i < len(base_offsets):
                idx_base = base_offsets[i]
            else:
                idx_base = idx_cursor
            for local_idx, name in parent_ft.items():
                child_idx = idx_base + local_idx
                if child_idx not in self.func_table:
                    self.func_table[child_idx] = name
                self._inherit_idx_to_path[child_idx] = iname
            max_idx = max(parent_ft.keys())
            # Patch 24: record this parent's idx range for call_user fallback
            self._inherit_ranges.append((idx_base, idx_base + max_idx, iname))
            idx_cursor = idx_base + max_idx + 1
    def _parse_func_names(self):
        prog = self.prog
        if len(prog) >= 4 and struct.unpack_from('<I', prog, 0)[0] == 0xFFFFFFFF:
            self._parse_func_names_marker()
            return
        # Format B: NAMES section = count(4) + nameSize(4) + names_buffer(nameSize)
        # names_buffer is null-separated identifier names.
        # Verified via vm_loadIdentifiers @ 0x08087FB8 (see LPC反编译技术文档 25.3).
        func_block = []
        func_block_bytes = []
        self._names_end = 0
        if len(prog) >= 8:
            names_count = struct.unpack_from('<I', prog, 0)[0]
            names_size = struct.unpack_from('<I', prog, 4)[0]
            if (0 <= names_count < 5000 and 0 < names_size < 100000
                    and 8 + names_size <= len(prog)):
                names_buf = prog[8:8+names_size]
                names_parsed = 0
                for raw in names_buf.split(b'\x00'):
                    if raw:
                        try:
                            s = raw.decode(self.encoding, 'surrogateescape')
                            # Validate: must be a valid identifier (filters out
                            # binary padding/garbage in NAMES buffer that happens
                            # to survive null-split, e.g. in tiny .o files where
                            # nameSize > actual names and padding bytes decode as
                            # surrogate chars). Matches func local var parsing.
                            # Patch 43: len >= 1 to accept single-char names (e.g. a, b, c)
                            # Format A path (L848) already uses len >= 1; this was missed.
                            if len(s) >= 1 and s.isidentifier():
                                func_block.append(s)
                                func_block_bytes.append(raw)
                                names_parsed += 1
                        except Exception:
                            pass
                        # Respect NAMES count field: stop after parsing count names
                        if names_count > 0 and names_parsed >= names_count:
                            break
                self._names_end = 8 + names_size
            elif names_size == 0 and len(prog) >= 8:
                # Patch 16: names_count == 0 且 names_size == 0: 没有 NAMES 段,
                # 但仍有后续 VARIABLES/FUNCTIONS/INHERITS 段需要解析。
                self._names_end = 8
        # user_funcs will be filtered in _parse_var_names (remove variable names).
        self.user_funcs = func_block
        self.all_symbol_names = list(func_block)
        self.all_name_bytes = list(func_block_bytes)

    def _parse_func_names_marker(self):
        """Parse FFFFFFFF entries - separate constants from function names.
        Only entries WITHOUT a recognizable constant value are treated as function names.
        Entries with HEX/INT/STR/CODE values are constants and handled by parse_constants().
        """
        prog = self.prog
        func_block = []
        func_offsets = {}
        all_names = []  # ALL entry names (for code segment matching)
        all_name_bytes = []  # raw bytes parallel to all_names
        i = 0
        while i < len(prog) - 8:
            marker = struct.unpack_from('<I', prog, i)[0]
            if marker == 0xFFFFFFFF:
                nl = struct.unpack_from('<I', prog, i + 4)[0]
                if 1 <= nl <= 500 and i + 8 + nl <= len(prog):
                    raw = prog[i+8:i+8+nl]
                    if raw[nl-1] == 0:
                        fname = raw[:nl-1].decode(self.encoding, 'surrogateescape')
                        name_end = i + 8 + nl
                        # Read optional value
                        value = ''
                        is_const = False
                        if name_end + 4 <= len(prog):
                            vl = struct.unpack_from('<I', prog, name_end)[0]
                            if 1 <= vl <= 2000 and name_end + 4 + vl <= len(prog):
                                vraw = prog[name_end+4:name_end+4+vl]
                                if vraw[vl-1] == 0:
                                    value = vraw[:vl-1].decode(self.encoding, 'surrogateescape')
                                    ctype = self._classify_value(value)
                                    if ctype:
                                        is_const = True  # has constant value, skip
                                    name_end = name_end + 4 + vl
                        # Always add to all_names (for code segment matching)
                        # Keep raw bytes (without trailing \0) for exact byte-level matching
                        name_raw = raw[:nl-1]  # raw bytes of the name
                        # Validate: must be a valid identifier (filters binary garbage)
                        if len(fname) >= 1 and fname.isidentifier():
                            all_names.append(fname)
                            all_name_bytes.append(name_raw)
                        # Only add to func_block if NOT a constant
                        if not is_const and len(fname) >= 1 and fname.isidentifier():
                            if not (fname.startswith('_') and fname.endswith('_H_')):
                                func_block.append(fname)
                                func_offsets.setdefault(fname, []).append(i)
                        i = name_end
                        continue
            i += 1
        self.user_funcs = func_block
        self.func_marker_offsets = func_offsets
        self.all_symbol_names = all_names  # ALL names for code matching
        self.all_name_bytes = all_name_bytes  # raw bytes parallel to all_symbol_names

    def parse_constants(self):
        """Parse constant definitions from defines (vm_loadDefines format) and
        FFFFFFFF entries.
        Defines format (vm_loadDefines @ 0x08087080):
          nargs(4) + name[len][str\0] + exps[len][str\0]
          nargs=0xFFFFFFFF means simple constant, other values = macro with params.
        FFFFFFFF format (legacy):
          FFFFFFFF [namelen(4)] [name\0] [valuelen(4)] [value\0]
        Only entries with recognizable value formats are constants.
        """
        prog = self.prog
        constants = []

        # Primary source: self.defines (parsed in _parse via vm_loadDefines)
        for name, exps, nargs in self.defines:
            sv = exps.strip()
            if not sv:
                # Constant with empty value (e.g. #define FOO)
                continue
            ctype = self._classify_value(sv)
            if ctype:
                constants.append((name, sv, ctype))

        # Fallback: scan for FFFFFFFF markers (legacy format)
        # Use bytes.find to jump directly between markers instead of walking
        # the whole prog byte-by-byte (significant speedup on large programs).
        i = 0
        marker_bytes = b'\xff\xff\xff\xff'
        while True:
            m = prog.find(marker_bytes, i)
            if m < 0 or m > len(prog) - 8:
                break
            nl = struct.unpack_from('<I', prog, m + 4)[0]
            if 1 <= nl <= 500 and m + 8 + nl <= len(prog):
                raw = prog[m+8:m+8+nl]
                if raw[nl-1] == 0:
                    name = raw[:nl-1].decode(self.encoding, 'surrogateescape')
                    name_end = m + 8 + nl
                    value = ''
                    ctype = None
                    if name_end + 4 <= len(prog):
                        vl = struct.unpack_from('<I', prog, name_end)[0]
                        if 1 <= vl <= 2000 and name_end + 4 + vl <= len(prog):
                            vraw = prog[name_end+4:name_end+4+vl]
                            if vraw[vl-1] == 0:
                                value = vraw[:vl-1].decode(self.encoding, 'surrogateescape')
                                ctype = self._classify_value(value)
                                if ctype:
                                    constants.append((name, value.strip(), ctype))
                                name_end = name_end + 4 + vl
                    i = name_end
                    continue
            i = m + 1
        return constants

    @staticmethod
    def _classify_value(value):
        """Classify constant value. Returns type string or None (not a constant)."""
        sv = value.strip()
        if not sv:
            return None
        # Hex constants
        if sv.startswith('0x') or sv.startswith('0X'):
            return 'HEX'
        # Quoted string constants
        if sv.startswith('L"') or (sv.startswith('"') and sv.endswith('"')):
            return 'STRING'
        # File path constants
        if sv.startswith('/') and ('.c' in sv or '.h' in sv):
            return 'STRING'
        # Pure integer constants
        if sv.replace('-', '').isdigit() and len(sv) <= 10:
            return 'INT'
        # Code macros
        if '{' in sv or ';' in sv or '(' in sv or '->' in sv:
            return 'CODE'
        # Bare hex (no 0x prefix, all hex digits)
        try:
            int(sv, 16)
            return 'HEX'
        except ValueError:
            pass
        # String with embedded quotes
        if '"' in sv and len(sv) < 200 and not sv.startswith('$$'):
            return 'STRING'
        # Code-like (LPC expressions)
        if sv.startswith('(') or sv.startswith('['):
            return 'CODE'
        # Reference-style constant (all caps with _ or $$ prefix)
        if sv.startswith('$$') and sv[2:].isdigit():
            return 'HEX'  # $$XXXXXX is an encoded value
        if sv.replace('_', '').replace(' ', '').isalnum() and sv.isupper():
            return 'REF'  # LIKE_THIS -> reference to another constant
        if ' ' in sv:
            # Multi-word: could be "NAME CONST", skip spaces
            parts = sv.split()
            # Check if it's "IDENTIFIER = VALUE" pattern
            if any(p.isupper() for p in parts if len(p) >= 3):
                return 'REF'
        # Contains known constant references
        if sv.replace(' ','').replace('_','').isalnum() and len(sv) >= 4:
            return 'REF'  # generic identifier reference
        return None

    def _parse_var_names(self):
        # Parse VARIABLES + PROG_VARS sections to build variable name mapping.
        #
        # .o file layout (after string pool):
        #   NAMES:      count(4) + nameSize(4) + names_buffer(nameSize)
        #   VARIABLES:  maxVarNo(2) + progVarCount(2)
        #   PROG_VARS:  progVarCount * [16-byte Vm_Prog_Var_T meta][uint32 len][name\0]
        #   FUNCTIONS:  ...
        #
        # Vm_Prog_Var_T structure (16 bytes):
        #   offset 0x8: type (1 byte)
        #   offset 0x9: attrib (1 byte)
        #   offset 0xA: valueNo (2 bytes, Vm_Index_T) - the global variable index
        #
        # metaData blocks' map_idx field directly equals valueNo.
        # local_vars is indexed by valueNo so that local_vars[map_idx] returns
        # the correct variable name.
        # Verified via vm_loadIdentifiers @ 0x08087FB8 (see LPC反编译技术文档 25.3).
        prog = self.prog
        local_vars = []        # indexed by valueNo (== map_idx)
        func_local_vars = {}   # {vm_idx: name} for function local variables
        source_files = []
        var_types = {}         # valueNo -> type byte (from PROG_VARS)
        names_end = getattr(self, '_names_end', 0)

        # VARIABLES section: maxVarNo(2) + progVarCount(2)
        vpos = names_end
        prog_vars_end = vpos
        var_name_set = set()
        if vpos + 4 <= len(prog):
            progVarCount = struct.unpack_from('<h', prog, vpos + 2)[0]
            # PROG_VARS section: progVarCount entries
            pvpos = vpos + 4
            var_names = {}  # valueNo -> name
            for i in range(progVarCount):
                if pvpos + 20 > len(prog):
                    break
                meta = prog[pvpos:pvpos+16]
                slen = struct.unpack_from('<I', prog, pvpos+16)[0]
                if slen == 0 or slen > 200:
                    break
                if pvpos + 20 + slen > len(prog):
                    break
                s = prog[pvpos+20:pvpos+20+slen-1].decode(self.encoding, 'surrogateescape')
                valueNo = struct.unpack_from('<h', meta, 10)[0]
                var_names[valueNo] = s
                var_name_set.add(s)
                # Save type byte (meta[8]) - see vm_valueType @ 0x0808A13C:
                # 0=undef,1=void,2=int,3=float,4=string,5=buffer,6=array,
                # 7=mapping,8=pointer,9=function,10=object,20=loop_ctrl,255=mixed
                var_types[valueNo] = meta[8]
                pvpos += 16 + 4 + slen
            prog_vars_end = pvpos

            # Build local_vars indexed by valueNo
            if var_names:
                max_vn = max(var_names.keys())
                if max_vn >= 0:
                    local_vars = ["global_%d" % i for i in range(max_vn + 1)]
                    for vn, name in var_names.items():
                        if 0 <= vn < len(local_vars):
                            local_vars[vn] = name

        self.local_vars = local_vars
        self.var_types = var_types  # valueNo -> type byte (PROG_VARS only)

        # Remove variable names from user_funcs (NAMES contains both funcs and vars)
        if var_name_set:
            filtered_names = []
            filtered_bytes = []
            for n, b in zip(self.all_symbol_names, self.all_name_bytes):
                if n not in var_name_set:
                    filtered_names.append(n)
                    filtered_bytes.append(b)
            self.user_funcs = [n for n in self.user_funcs if n not in var_name_set]
            self.all_symbol_names = filtered_names
            self.all_name_bytes = filtered_bytes

        # Parse function local vars: scan after PROG_VARS for [len][name] patterns
        # with VM index (signed int16) located 12 bytes before the [len] field.
        # Matches vm_pushLocalVar @ 0x080914DC (see LPC反编译技术文档 25.5).
        # A 4-byte LE length in [1,200] has byte layout [L,0,0,0], so jump to
        # the next candidate with bytes.find instead of walking byte-by-byte.
        i = prog_vars_end
        limit = len(prog) - 4
        while i < limit:
            z = prog.find(b'\x00\x00\x00', i + 3)
            if z < 0:
                break
            li = z - 1  # position of the length byte (z >= i+3 => li >= i)
            L = prog[li]
            if 1 <= L <= 200:
                str_start = li + 4
                str_end = str_start + L
                if str_end < len(prog) and prog[str_end - 1] == 0:
                    raw = prog[str_start:str_end]
                    s = raw[:L-1].decode(self.encoding, 'surrogateescape')
                    if s.isidentifier() and len(s) >= 1:
                        # Try to read VM index from 12 bytes before [len]
                        if li >= 12:
                            vm_idx_raw = struct.unpack_from('<h', prog, li - 12)[0]
                            if -200 < vm_idx_raw < 0:
                                func_local_vars[vm_idx_raw] = s
                        i = str_end
                        continue
                    elif s.startswith('/') and s.endswith('.c'):
                        source_files.append(s)
                        i = str_end
                        continue
            i = li + 1

        self.func_local_vars = func_local_vars
        self.source_files = source_files
        if source_files:
            self.source_file = source_files[0]

        # Parse DLT section to extract per-function local variables and args.
        # This is more accurate than the scan above (which mixes vars from
        # different functions). See vm_loadFun @ 0x080959B8 and
        # vm_loadCodeSegment @ 0x08096128 in magic_Linux32x.
        self._parse_dlt_func_locals()

    def _parse_dlt_func_locals(self):
        """Parse DLT section to extract per-function local variables and args.

        DLT structure (verified via vm_loadFun/vm_loadCodeSegment):
          DLT_count(4)
          For each entry (20 bytes):
            name_off(4) + attrib(4) + var_idx(4) + func_idx(4) + extra(4)
            If attrib & 0x8000000 (function):
              Vm_Fun_T(64) + name[4B len][str\0]
              + arg_count(2) + args[arg_count x (Vm_Fun_Arg_T(20) + name[4B len][str\0])]
              + var_count(2) + vars[var_count x (Vm_Fun_Arg_T(20) + name[4B len][str\0])]
              + symbolsCount(4) + variable_symbols
              + nodesCount(4) + nodes[nodesCount*4]
              + metaDataLen(4) + meta[metaDataLen]
              + runtimeMiscCount(4) + runtimeMisc[runtimeMiscCount*4]
        """
        prog = self.prog
        if not prog or len(prog) < 20:
            return
        names_end = getattr(self, '_names_end', 0)
        if not names_end:
            return

        pos = names_end
        # VARIABLES: maxVarNo(2) + progVarCount(2)
        if pos + 4 > len(prog):
            return
        try:
            progVarCount = struct.unpack_from('<h', prog, pos + 2)[0]
        except struct.error:
            return
        pos += 4
        # PROG_VARS: only present for non-global programs.
        # Global programs (vm_pGlobalProgram) skip PROG_VARS entirely.
        # Detect by checking if first entry has valid slen.
        is_global = False
        if progVarCount > 0 and pos + 20 <= len(prog):
            first_slen = struct.unpack_from('<I', prog, pos + 16)[0]
            if first_slen == 0 or first_slen > 200:
                is_global = True
        if not is_global:
            for i in range(progVarCount):
                if pos + 20 > len(prog):
                    return
                try:
                    slen = struct.unpack_from('<I', prog, pos + 16)[0]
                except struct.error:
                    return
                if slen == 0 or slen > 200:
                    return
                pos += 16 + 4 + slen
        # FUNCTIONS: efunsCount(2) + maxFunctionNo(2)
        if pos + 4 > len(prog):
            return
        maxFunctionNo = struct.unpack_from('<h', prog, pos + 2)[0]
        pos += 4
        # HIDDEN_FUNS: count(4) + count x vm_loadFun entries
        # (IDA: hidden functions loaded via vm_loadFun directly, NO 20-byte DLT header)
        # Each entry = Vm_Fun_T(64) + name[4B len][str\0]
        #            + arg_count(2) + args[arg_count x (Vm_Fun_Arg_T(20) + name[4B len][str\0])]
        #            + var_count(2) + vars[var_count x (Vm_Fun_Arg_T(20) + name[4B len][str\0])]
        #            + symbolsCount(4) + variable_symbols
        #            + nodesCount(4) + nodes[nodesCount*4]
        #            + metaDataLen(4) + meta[metaDataLen]
        #            + runtimeMiscCount(4) + runtimeMisc[runtimeMiscCount*4]
        if pos + 4 > len(prog):
            return
        hidden_count = struct.unpack_from('<I', prog, pos)[0]
        pos += 4
        hidden_funcs = []  # store (name, nodes_start, nodes_count, args, vars, ret_type, attrib)
        for hi in range(hidden_count):
            try:
                if pos + 64 > len(prog):
                    break
                # Vm_Fun_T header (64 bytes)
                hfun_ret_type_byte = prog[pos + 13]
                hfun_attrib = struct.unpack_from('<H', prog, pos + 8)[0]
                hfun_min_arg_no = prog[pos + 0xF]  # Vm_Fun_T.offset 0x0F: minArgNo (signed byte)
                if hfun_min_arg_no & 0x80: hfun_min_arg_no = hfun_min_arg_no - 256
                hfun_max_arg_no = prog[pos + 0xE]  # Vm_Fun_T.offset 0x0E: maxArgNo (signed byte)
                if hfun_max_arg_no & 0x80: hfun_max_arg_no = hfun_max_arg_no - 256
                pos += 64
                # Function name
                if pos + 4 > len(prog):
                    break
                hname_len = struct.unpack_from('<I', prog, pos)[0]
                if hname_len == 0 or hname_len > 200:
                    break
                if pos + 4 + hname_len > len(prog):
                    break
                hname = prog[pos + 4:pos + 4 + hname_len - 1].decode(self.encoding, 'surrogateescape')
                pos += 4 + hname_len
                # Args
                if pos + 2 > len(prog):
                    break
                harg_count = struct.unpack_from('<h', prog, pos)[0]
                pos += 2
                hargs = []
                harg_types = []
                harg_by_refs = {}  # Patch 31
                for ai in range(harg_count):
                    if pos + 20 > len(prog):
                        pos = len(prog); break
                    harg_vm_idx = struct.unpack_from('<h', prog, pos + 8)[0]
                    harg_type_byte = prog[pos + 5]
                    harg_by_refs[harg_vm_idx] = prog[pos + 4]  # Patch 31
                    pos += 20
                    if pos + 4 > len(prog):
                        pos = len(prog); break
                    haname_len = struct.unpack_from('<I', prog, pos)[0]
                    if haname_len == 0 or haname_len > 200:
                        pos = len(prog); break
                    if pos + 4 + haname_len > len(prog):
                        pos = len(prog); break
                    haname = prog[pos + 4:pos + 4 + haname_len - 1].decode(self.encoding, 'surrogateescape')
                    pos += 4 + haname_len
                    hargs.append((harg_vm_idx, haname))
                    harg_types.append((harg_vm_idx, harg_type_byte))
                # Local vars
                if pos + 2 > len(prog):
                    break
                hvar_count = struct.unpack_from('<h', prog, pos)[0]
                pos += 2
                hvargs = []
                for vi in range(hvar_count):
                    if pos + 20 > len(prog):
                        pos = len(prog); break
                    hvar_vm_idx = struct.unpack_from('<h', prog, pos + 8)[0]
                    pos += 20
                    if pos + 4 > len(prog):
                        pos = len(prog); break
                    hvname_len = struct.unpack_from('<I', prog, pos)[0]
                    if hvname_len == 0 or hvname_len > 200:
                        pos = len(prog); break
                    if pos + 4 + hvname_len > len(prog):
                        pos = len(prog); break
                    hvname = prog[pos + 4:pos + 4 + hvname_len - 1].decode(self.encoding, 'surrogateescape')
                    pos += 4 + hvname_len
                    hvargs.append((hvar_vm_idx, hvname))
                # symbolsCount + variable symbols
                if pos + 4 > len(prog):
                    break
                hsymbolsCount = struct.unpack_from('<I', prog, pos)[0]
                pos += 4
                for si in range(hsymbolsCount):
                    if pos >= len(prog):
                        pos = len(prog); break
                    hsFlag = prog[pos]
                    pos += 1
                    if (hsFlag & 1):
                        if pos + 4 > len(prog):
                            pos = len(prog); break
                        hslen = struct.unpack_from('<I', prog, pos)[0]
                        pos += 4 + hslen
                    if (hsFlag & 2):
                        pos += 4
                # nodesCount + nodes (bytecode!)
                if pos + 4 > len(prog):
                    break
                hnodesCount = struct.unpack_from('<I', prog, pos)[0]
                pos += 4
                hnodes_start = pos
                pos += hnodesCount * 4
                self.dlt_func_segs[hnodes_start] = hnodesCount
                self.dlt_func_names_by_seg[hnodes_start] = hname  # Patch 44
                # metaDataLen + metaData
                if pos + 4 > len(prog):
                    break
                hmetaDataLen = struct.unpack_from('<I', prog, pos)[0]
                pos += 4 + hmetaDataLen
                # runtimeMiscCount + runtimeMisc
                if pos + 4 > len(prog):
                    break
                hruntimeMiscCount = struct.unpack_from('<I', prog, pos)[0]
                pos += 4
                if hruntimeMiscCount > 0:
                    pos += hruntimeMiscCount * 4
                # Store the hidden function info
                hidden_funcs.append({
                    'name': hname,
                    'nodes_start': hnodes_start,
                    'nodes_count': hnodesCount,
                    'args': hargs,
                    'vars': hvargs,
                    'arg_types': harg_types,
                    'ret_type': hfun_ret_type_byte,
                    'attrib': hfun_attrib,
                })
                # Also register in per-seg maps so decompile_file_v37 can pick them up
                self.func_local_vars_by_seg[hnodes_start] = {vi: vn for vi, vn in hvargs}
                self.func_args_by_seg[hnodes_start] = hargs
                self.func_arg_types_by_seg[hnodes_start] = {vi: tb for vi, tb in harg_types}
                self.func_arg_by_refs_by_seg = getattr(self, 'func_arg_by_refs_by_seg', {})
                self.func_arg_by_refs_by_seg[hnodes_start] = dict(harg_by_refs)  # Patch 31
                self.func_ret_types_by_seg[hnodes_start] = hfun_ret_type_byte
                self.func_attribs_by_seg[hnodes_start] = hfun_attrib
                self.func_min_arg_nos_by_seg[hnodes_start] = hfun_min_arg_no
                self.func_max_arg_nos_by_seg[hnodes_start] = hfun_max_arg_no
                self.func_decl_info_by_name[hname] = {
                    'ret_type': hfun_ret_type_byte,
                    'args': hargs,
                    'arg_types': {vi: tb for vi, tb in harg_types},
                    'arg_by_refs': dict(harg_by_refs),  # Patch 31
                    'attrib': hfun_attrib,
                    'min_arg': hfun_min_arg_no,
                    'max_arg': hfun_max_arg_no,
                }
            except (struct.error, IndexError):
                # On any parse error, stop parsing further hidden functions
                break
        self.hidden_funcs = hidden_funcs
        self._inherit_base_fun_offsets = []  # Patch 23: baseFunOffset per inherit
        self._inherit_base_var_offsets = []  # baseVarOffset per inherit (for var name resolution)
        # INHERITS: only for non-global programs
        # Format (IDA: vm_loadInheritPrograms @ 0x808729c):
        #   maxConsititutionNo(Vm_Index_T, 2B) + count(uint32)
        #   per entry: name[slen(4B)+content(slen-1 bytes, null-terminated)] + 6 x Vm_Index_T (12B)
        if not is_global:
            if pos + 6 > len(prog):
                return
            # Patch 43: sanity check for INHERITS section.
            # vm_loadInheritPrograms @ 0x808729c: for global programs
            # (vm_pCurrentProgram == vm_pGlobalProgram), INHERITS is skipped
            # entirely (returns 1 without reading any data). For non-global
            # programs, format is maxConsititutionNo(2B) + count(4B), where
            # count <= maxConsititutionNo (buffer capacity).
            # If inherit_count > maxConsititutionNo, this is NOT a valid
            # INHERITS section -- likely a global program where the bytes at
            # this position are actually the DLT section. Skip INHERITS.
            inherit_maxCon = struct.unpack_from('<h', prog, pos)[0]
            inherit_count = struct.unpack_from('<I', prog, pos + 2)[0]
            if (inherit_maxCon > 0 and inherit_maxCon <= 1000
                    and inherit_count <= inherit_maxCon):
                pos += 6
                for i in range(inherit_count):
                    if pos + 4 > len(prog):
                        return
                    slen = struct.unpack_from('<I', prog, pos)[0]
                    if slen == 0 or slen > 200:
                        return
                    # Read inherit name (skip trailing null byte at slen-1)
                    if pos + 4 + slen <= len(prog) and slen >= 1:
                        raw_name = prog[pos + 4:pos + 4 + slen - 1]
                        try:
                            iname = raw_name.decode(self.encoding, 'surrogateescape')
                            self.inherit_names.append(iname)
                        except Exception:
                            self.inherit_names.append("<inherit_%d>" % i)
                    else:
                        self.inherit_names.append("<inherit_%d>" % i)
                    pos += 4 + slen
                    # Patch 23: read baseFunOffset from 6 x Vm_Index_T (2 bytes each)
                    # Format: baseConsititutionOffset(2) + baseFunOffset(2) + baseVarOffset(2)
                    #         + expectedSubConsititutionNo(2) + expectedFunctionNo(2) + expectedVariableNo(2)
                    # baseFunOffset is the starting table_idx of this parent's functions
                    # in the child's func_table (verified via vm_loadBinaryInherit @ 0x80a2b58)
                    if pos + 12 <= len(prog):
                        base_fun_offset = struct.unpack_from('<H', prog, pos + 2)[0]
                        base_var_offset = struct.unpack_from('<H', prog, pos + 4)[0]
                    else:
                        base_fun_offset = 0
                        base_var_offset = 0
                    self._inherit_base_fun_offsets.append(base_fun_offset)
                    self._inherit_base_var_offsets.append(base_var_offset)
                    pos += 12  # 6 x Vm_Index_T (2 bytes each)
        # DLT
        if pos + 4 > len(prog):
            return
        dlt_count = struct.unpack_from('<I', prog, pos)[0]
        pos += 4

        # Build name offset -> name mapping for DLT header name resolution
        names_size = struct.unpack_from('<I', prog, 4)[0] if len(prog) >= 8 else 0
        if names_size > 0 and names_size < len(prog):
            names_buf = prog[8:8+names_size]
            all_names = {}
            ni = 0
            while ni < len(names_buf):
                nend = names_buf.find(b'\x00', ni)
                if nend < 0:
                    break
                nm = names_buf[ni:nend].decode(self.encoding, 'surrogateescape')
                if nm:
                    all_names[ni] = nm
                ni = nend + 1
        else:
            all_names = {}
        func_table = {}
        dlt_var_names = {}  # var_index -> name (from DLT variable entries)

        for entry_idx in range(dlt_count):
            if pos + 20 > len(prog):
                break
            entry_name_off = struct.unpack_from('<I', prog, pos)[0]
            attrib = struct.unpack_from('<I', prog, pos + 4)[0]
            entry_var_idx = struct.unpack_from('<I', prog, pos + 12)[0]
            entry_table_idx = struct.unpack_from('<I', prog, pos + 16)[0]
            pos += 20
            is_function = (attrib & 0x8000000) != 0
            if is_function:
                # Resolve function name from DLT header name_off
                if entry_name_off in all_names and entry_table_idx <= 0xFFFF:
                    if entry_table_idx not in func_table:
                        func_table[entry_table_idx] = all_names[entry_name_off]
            else:
                # Variable entry: v70 (offset 12) is the variable index
                # attrib 0x80000 = global var, 0x40000 = object var
                if entry_name_off in all_names:
                    dlt_var_names[entry_var_idx] = all_names[entry_name_off]
                continue
            try:
                # Vm_Fun_T (64 bytes) - extract retType (offset 13), attrib (offset 8-9),
                # minArgNo (offset 0x0F), maxArgNo (offset 0x0E)
                # retType: see vm_returnFromFun: v7 = *((unsigned __int8 *)p_word + 13)
                # attrib: see vm_disassemble: bit2=private, bit0=varargs, bit8=hidden
                #         see vm_gatherProgramFunctionTable: pFun->attrib & 4
                # minArgNo/maxArgNo: see vm_checkArgNum @ 0x809521C
                fun_ret_type_byte = prog[pos + 13]
                fun_attrib = struct.unpack_from('<H', prog, pos + 8)[0]
                fun_min_arg_no = prog[pos + 0xF]  # signed byte
                if fun_min_arg_no & 0x80: fun_min_arg_no = fun_min_arg_no - 256
                fun_max_arg_no = prog[pos + 0xE]  # signed byte
                if fun_max_arg_no & 0x80: fun_max_arg_no = fun_max_arg_no - 256
                pos += 64
                # Function name
                name_len = struct.unpack_from('<I', prog, pos)[0]
                if name_len == 0 or name_len > 200:
                    break
                fun_name = prog[pos + 4:pos + 4 + name_len - 1].decode(self.encoding, 'surrogateescape')
                pos += 4 + name_len
                # Args
                arg_count = struct.unpack_from('<h', prog, pos)[0]
                pos += 2
                args = []
                arg_types = []  # [(vm_idx, type_byte), ...]
                arg_by_refs = {}  # {vm_idx: by_ref_flag} Patch 31
                for ai in range(arg_count):
                    if pos + 20 > len(prog):
                        return
                    arg_vm_idx = struct.unpack_from('<h', prog, pos + 8)[0]
                    # type byte at offset 5 (see vm_declareVariable: a1[5] for
                    # args stores type; locals always set a1[5] = -1 = mixed).
                    # Type values: see vm_valueType @ 0x0808A13C
                    arg_type_byte = prog[pos + 5]
                    # Patch 31: by_ref flag at offset 4 (0=normal, 1=reference).
                    # Reference params are declared as `type &name` in LPC.
                    arg_by_ref = prog[pos + 4]
                    arg_by_refs[arg_vm_idx] = arg_by_ref  # Patch 31 fix: store by_ref flag
                    pos += 20
                    aname_len = struct.unpack_from('<I', prog, pos)[0]
                    if aname_len == 0 or aname_len > 200:
                        return
                    aname = prog[pos + 4:pos + 4 + aname_len - 1].decode(self.encoding, 'surrogateescape')
                    pos += 4 + aname_len
                    args.append((arg_vm_idx, aname))
                    arg_types.append((arg_vm_idx, arg_type_byte))
                # Local vars
                var_count = struct.unpack_from('<h', prog, pos)[0]
                pos += 2
                vargs = []
                for vi in range(var_count):
                    if pos + 20 > len(prog):
                        return
                    var_vm_idx = struct.unpack_from('<h', prog, pos + 8)[0]
                    pos += 20
                    vname_len = struct.unpack_from('<I', prog, pos)[0]
                    if vname_len == 0 or vname_len > 200:
                        return
                    vname = prog[pos + 4:pos + 4 + vname_len - 1].decode(self.encoding, 'surrogateescape')
                    pos += 4 + vname_len
                    vargs.append((var_vm_idx, vname))
                # symbolsCount + variable symbols
                symbolsCount = struct.unpack_from('<I', prog, pos)[0]
                pos += 4
                for si in range(symbolsCount):
                    if pos >= len(prog):
                        return
                    symbolFlag = prog[pos]
                    pos += 1
                    if (symbolFlag & 1):
                        if pos + 4 > len(prog):
                            return
                        slen = struct.unpack_from('<I', prog, pos)[0]
                        pos += 4 + slen
                    if (symbolFlag & 2):
                        pos += 4
                # nodesCount
                nodesCount = struct.unpack_from('<I', prog, pos)[0]
                pos += 4
                nodes_start = pos
                pos += nodesCount * 4
                self.dlt_func_segs[nodes_start] = nodesCount
                self.dlt_func_names_by_seg[nodes_start] = fun_name  # Patch 44
                # metaDataLen
                metaDataLen = struct.unpack_from('<I', prog, pos)[0]
                pos += 4 + metaDataLen
                # runtimeMiscCount
                runtimeMiscCount = struct.unpack_from('<I', prog, pos)[0]
                pos += 4
                if runtimeMiscCount > 0:
                    pos += runtimeMiscCount * 4

                # Store per-function local vars and args indexed by nodes_start
                self.func_local_vars_by_seg[nodes_start] = {vi: vn for vi, vn in vargs}
                self.func_args_by_seg[nodes_start] = args
                self.func_arg_types_by_seg[nodes_start] = {vi: tb for vi, tb in arg_types}
                self.func_arg_by_refs_by_seg = getattr(self, 'func_arg_by_refs_by_seg', {})
                self.func_arg_by_refs_by_seg[nodes_start] = dict(arg_by_refs)  # Patch 31
                self.func_ret_types_by_seg[nodes_start] = fun_ret_type_byte
                self.func_attribs_by_seg[nodes_start] = fun_attrib
                self.func_min_arg_nos_by_seg[nodes_start] = fun_min_arg_no
                self.func_max_arg_nos_by_seg[nodes_start] = fun_max_arg_no
                self.func_decl_info_by_name[fun_name] = {
                    'ret_type': fun_ret_type_byte,
                    'args': args,
                    'arg_types': {vi: tb for vi, tb in arg_types},
                    'arg_by_refs': dict(arg_by_refs),  # Patch 31
                    'attrib': fun_attrib,
                    'min_arg': fun_min_arg_no,
                    'max_arg': fun_max_arg_no,
                }
            except (struct.error, IndexError):
                break

        # Save func_table built from DLT headers
        self.func_table = func_table
        # Update local_vars with DLT variable names (for global programs
        # where PROG_VARS is absent, DLT variable entries provide the names)
        if dlt_var_names:
            if not self.local_vars:
                max_vn = max(dlt_var_names.keys())
                if max_vn >= 0:
                    self.local_vars = ["global_%d" % i for i in range(max_vn + 1)]
            for vn, name in dlt_var_names.items():
                if 0 <= vn < len(self.local_vars):
                    self.local_vars[vn] = name
        # Save the position after DLT section = init code segment start
        self.init_code_start = pos

    def _parse_func_table(self):
        """Parse function table (DLT segment) to build table_idx -> func_name mapping.

        DLT entries are variable-length records parsed by vm_loadIdentifiers.
        Each entry starts with a 20-byte header:
          name_off(4) + flags(4) + v70(4) + v71(4) + table_idx(4)
        Function entries have (flags & 0x8000000) != 0; after the header,
        vm_loadFun data follows when (v71 & 0x8000) == 0 (function has code).
        Variable entries (flags & 0x8000000 == 0) only occupy the 20-byte header.
        call_user_fn(#NN) bytecode uses table_idx (NN) to reference functions.
        Verified via vm_loadIdentifiers @ 0x08087FB8 (see LPC 25.3).

        Primary parsing is done in _parse_dlt_func_locals (sequential DLT walk).
        This method is a fallback when that fails (e.g. parse error).
        """
        # If _parse_dlt_func_locals already built func_table, use it
        if getattr(self, 'func_table', None):
            return
        self.func_table = {}
        prog = self.prog
        if not prog or len(prog) < 20:
            return

        names_size = struct.unpack_from('<I', prog, 4)[0] if len(prog) >= 8 else 0
        if names_size <= 0 or names_size >= len(prog):
            return
        names_buf = prog[8:8+names_size]

        # Build name offset -> name mapping
        all_names = {}
        idx = 0
        while idx < len(names_buf):
            end = names_buf.find(b'\x00', idx)
            if end < 0:
                break
            name = names_buf[idx:end].decode(self.encoding, 'surrogateescape')
            if name:
                all_names[idx] = name
            idx = end + 1

        # Fallback scan: find positions where name_off is valid AND
        # flags has the function bit (0x8000000) set.
        func_table = {}
        pos = 4
        while pos + 20 <= len(prog):
            name_off = struct.unpack_from('<I', prog, pos)[0]
            flags = struct.unpack_from('<I', prog, pos + 4)[0]
            if name_off < names_size and (flags & 0x8000000) != 0:
                table_idx = struct.unpack_from('<I', prog, pos + 16)[0]
                if table_idx <= 0xFFFF and name_off in all_names:
                    name = all_names[name_off]
                    if table_idx not in func_table:
                        func_table[table_idx] = name
                    pos += 20
                    continue
            pos += 1

        self.func_table = func_table


# ============================================================================
# Init Code Segment Parser (post-DLT data)
# ============================================================================
# Patch 38: side-effect detector for init code segment pop handling.
# Detects function calls (call_ptr, call_efun, call_user, call_other) whose
# results are discarded by pop but whose execution has side effects.
def _has_side_effect_init(val):
    if not val:
        return False
    # Function pointer call: (*expr)(args)
    if val.startswith("(*") and ")(" in val and val.endswith(")"):
        return True
    # efun call: name(args)
    import re
    if re.match(r'^[a-zA-Z_]\w*\s*\(', val):
        return True
    # call_other: obj->func(args)
    if "->" in val and "(" in val:
        return True
    return False

def parse_init_code_segment(o):
    """Parse the init code segment (post-DLT data) and reconstruct global
    variable initializations.

    Returns a list of LPC pseudocode lines, or empty list on failure.
    """
    prog = o.prog
    start = getattr(o, 'init_code_start', 0)
    if not start or start + 8 > len(prog):
        return []

    pos = start
    end = len(prog)
    try:
        # symbolsCount
        symbolsCount = struct.unpack_from('<I', prog, pos)[0]
        pos += 4
        if symbolsCount < 0 or symbolsCount > 1000:
            return []
        # symbols
        symbols = []
        current_file = None
        current_line = 0
        for i in range(symbolsCount):
            if pos >= end: return []
            flag = prog[pos]; pos += 1
            if flag & 1:
                if pos + 4 > end: return []
                slen = struct.unpack_from('<I', prog, pos)[0]; pos += 4
                if 0 < slen < 200:
                    current_file = prog[pos:pos+slen].decode(o.encoding, 'surrogateescape').rstrip('\x00'); pos += slen
                else:
                    return []
            if flag & 2:
                if pos + 4 > end: return []
                current_line = struct.unpack_from('<I', prog, pos)[0]; pos += 4
            symbols.append((current_file, current_line))
        # nodesCount
        nodesCount = struct.unpack_from('<I', prog, pos)[0]; pos += 4
        if nodesCount < 0 or nodesCount > 10000:
            return []
        # nodes
        nodes = []
        for i in range(nodesCount):
            if pos + 4 > end: return []
            nodes.append(struct.unpack_from('<I', prog, pos)[0]); pos += 4
        # Read metaData (after nodes) for complex instructions
        # (push_num/push_real) that use p16|p8<<16 as a byte offset.
        init_meta_data = b''
        if pos + 4 <= end:
            init_meta_len = struct.unpack_from('<I', prog, pos)[0]; pos += 4
            if 0 < init_meta_len < 100000 and pos + init_meta_len <= end:
                init_meta_data = prog[pos:pos+init_meta_len]
                pos += init_meta_len
    except (struct.error, IndexError):
        return []

    # Helper functions
    def get_var_name(idx):
        lvars = o.local_vars
        if lvars and 0 <= idx < len(lvars):
            name = lvars[idx]
            if name and not re.match(r'^global_\d+$', name):
                return name
            # Placeholder slot: try global program table first
            if idx in GLOBAL_VAR_TABLE:
                return GLOBAL_VAR_TABLE[idx]
            return "global_%d" % idx
        # File has 0 variables OR idx beyond file's table: try startup .o
        if idx in GLOBAL_VAR_TABLE:
            return GLOBAL_VAR_TABLE[idx]
        return "global_%d" % idx

    def get_str(idx):
        if 0 <= idx < len(o.str_table):
            return o.str_table[idx]
        return None

    # Reconstruct global variable initializations using stack simulation.
    # Opcode semantics mirror the pseudo-code generator (_gen_inst) so that
    # call_ptr/call_other/call_os/member/inc/op_eq etc. produce the same
    # expressions.  Output is an ordered list of statements to preserve the
    # original init-code order (assignments interleaved with in-place mods).
    stack = []
    statements = []  # ordered list of (text, line_info)
    # Patch 40: if_blocks tracks active if-blocks for proper block close
    # emission. Each entry: {'start': ni, 'target': target, 'cond': cond,
    # 'do_pop': bool, 'else_emitted': bool, 'end_target': target}
    if_blocks = []
    # Patch 40: when do_pop=0 if is encountered, the condition stays on stack
    # and is consumed by the NEXT pop instruction. This flag tells the pop
    # handler to skip that pop (condition already consumed by the if handler).
    pending_if_pop = False
    # Ternary expression detection: when an if/goto pattern produces a value
    # (both branches contain only stack-manipulating "quiet" opcodes), treat
    # it as a ternary expression (cond ? then_val : else_val) instead of a
    # statement-level if-else block. This prevents empty if-else blocks and
    # stack misalignment in mapping/array literals.
    ternary_stack = []
    # Quiet opcodes: only push/pop values on the stack, emit no statements.
    QUIET_INIT_OPS = frozenset({
        1, 2, 5, 6, 7, 8, 9, 13, 19, 20, 22, 23, 24, 25, 28, 30, 32, 36, 37,
        38, 39, 40, 41, 42, 43, 45, 48, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 74, 75, 79, 81, 85,
    })
    func_table = getattr(o, 'func_table', {}) or {}

    # Helpers for binary/unary arithmetic (pop/push on the local stack).
    def _binop(sym):
        if len(stack) >= 2:
            b = stack.pop(); a = stack.pop()
            stack.append("(%s %s %s)" % (a, sym, b))
        elif stack:
            stack.pop(); stack.append("(<?> %s <?>)" % sym)

    def _unop(sym):
        if stack:
            a = stack.pop()
            stack.append("%s(%s)" % (sym, a))

    for i, insn in enumerate(nodes):
        an = (insn >> 24) & 0xFF
        p16 = insn & 0xFFFF
        p8 = (insn >> 16) & 0xFF

        line_info = ""
        if i < len(symbols) and symbols[i][0]:
            line_info = "// %s:%d" % (symbols[i][0], symbols[i][1])

        # Patch 40: close if-blocks when reaching their end target.
        # The end_target is initially the if's jump target (end of
        # then-branch); updated to the else-goto's target when an
        # else is detected (end of else-branch).
        while if_blocks and i >= if_blocks[-1]['end_target']:
            blk = if_blocks.pop()
            statements.append(("}", line_info))
        # Close ternary expressions when reaching their else-branch end.
        # Pop the else-value (pushed by the else-branch), combine with the
        # saved then-value, and push the ternary result onto the stack.
        while ternary_stack and i >= ternary_stack[-1]['else_end']:
            t = ternary_stack.pop()
            else_val = stack.pop() if stack else "<?>"
            stack.append("(%s ? %s : %s)" % (t['cond'], t['then_val'], else_val))

        # ---- Push literals ----
        if an == 64:  # push_s
            s = get_str(p16)
            if s is not None:
                stack.append('"%s"' % s.replace('"', '\\"'))
            else:
                stack.append('<str_%d>' % p16)
        elif an == 59:  # push_i
            stack.append(str(p16))
        # Patch 38: push_global (an=53) was missing in init code segment
        # parsing, causing all subsequent call_ptr invocations to have wrong
        # stack layout. This silently dropped the entire GS startup sequence
        # (load_dir calls for /gs/daemons/, /gs/clone/char/, /gs/cmds/, etc.).
        elif an == 53:  # push_global
            stack.append(get_var_name(p16))
        elif an == 60:  # push_num (complex: p16|p8<<16 = meta byte offset)
            meta_idx = p16 | (p8 << 16)
            if init_meta_data and meta_idx + 4 <= len(init_meta_data):
                val = struct.unpack_from('<i', init_meta_data, meta_idx)[0]
            else:
                val = meta_idx  # fallback: use raw value
            stack.append(str(val))
        elif an == 63:  # push_real (complex: p16|p8<<16 = meta byte offset)
            meta_idx = p16 | (p8 << 16)
            if init_meta_data and meta_idx + 4 <= len(init_meta_data):
                raw = struct.unpack_from('<i', init_meta_data, meta_idx)[0]
            else:
                raw = meta_idx  # fallback
            fv = struct.unpack('<f', struct.pack('<i', raw))[0]
            s = ("%.6f" % fv).rstrip('0').rstrip('.')
            if s == "" or s == "-":
                s = "0"
            stack.append(s)
        elif an == 65:  # push_this (pushThisObject) - no p16 operand
            stack.append("this_object()")
        elif an == 66:  # push_0 (pushUndefined) - no p16 operand
            stack.append("0")
        elif an == 52:  # push_argnum - pushes argc, NOT pop
            stack.append("argc")

        # ---- Push variables ----
        elif an == 61:  # push_ovar (pushObjVar)
            stack.append(get_var_name(p16))
        elif an == 62:  # push_ovar_ptr (pushObjVptr)
            stack.append("&%s" % get_var_name(p16))
        elif an == 53:  # push_global
            stack.append(get_var_name(p16))
        elif an == 54:  # push_global_ptr
            stack.append("&" + get_var_name(p16))
        elif an in (55, 56):  # push_local / push_local_i
            stack.append("local_%d" % p16)
        elif an in (57, 58):  # push_local_ptr / push_local_ptr_i
            stack.append("&local_%d" % p16)

        # ---- Array/Mapping creation ----
        elif an == 22:  # new_array
            if p16 == 0:
                stack.append("({ })")
            else:
                # Patch 42: removed args.reverse().
                # Compiler pushes elements in REVERSE source order (last first),
                # so stack top = first source element. vm_createArray reads
                # vm_stack[v4+vm_sp] with v4 from 0, so array[0]=stack top.
                # Popping yields correct source order; reverse was wrong.
                args = [stack.pop() if stack else "<?>" for _ in range(p16)]
                stack.append("({ %s })" % ", ".join(args))
        elif an == 24:  # new_mapping
            if p16 == 0:
                stack.append("([ ])")
            else:
                args = [stack.pop() if stack else "<?>" for _ in range(p16)]
                args.reverse()
                # VM push order is: value first, key last (per vm_createMapping:
                # vm_stack[v6] is key, vm_stack[v6+1] is value, v6 starts at
                # stack top and grows toward stack bottom). After reverse,
                # args[0] is the first pushed = value, args[1] = key, etc.
                # So pair is (args[j+1]=key : args[j]=value).
                pairs = []
                for j in range(0, len(args), 2):
                    if j + 1 < len(args):
                        pairs.append("%s : %s" % (args[j+1], args[j]))
                    else:
                        pairs.append(args[j])
                stack.append("([ %s ])" % ", ".join(pairs))

        # ---- Function pointer creation (closures) ----
        elif an == 23:  # new_func - create function pointer (closure)
            # vm_createFunctionValue(p16=funcIdx, p8=flags)
            # p8 bit7=isGlobal, bits0-5 (p8&0x3F)=num captured vars
            nargs = p8 & 0x3F
            is_global = (p8 >> 7) & 1
            if is_global:
                # Patch 39 (same as call_user): is_global=1 means the
                # global program function table = [efuns 0..N-1, sefuns N..].
                # Check func_table first (the global program's own table),
                # then fall back to the efun/sefun ranges like call_user.
                if p16 in func_table:
                    fname = func_table[p16]
                else:
                    fname = EFUN_TABLE.get(p16) or SEFUN_TABLE.get(p16, "global_func_%d" % p16)
            elif p16 in func_table:
                fname = func_table[p16]
                # Patch 8: inherited-slot closure -> (:: fname) - see
                # PseudoGenV37 new_func handler for the full rationale.
                _i2p = getattr(o, '_inherit_idx_to_path', {})
                if p16 in _i2p and any(v == fname and k not in _i2p
                                       for k, v in func_table.items()):
                    fname = '::%s' % fname
            else:
                fname = "#%d" % p16
            if nargs > 0:
                # Patch 29: pop order = lambda param order (no reverse).
                # IDA vm_createFunctionValueEx @ 0x8091cd0: VM pops from
                # stack TOP into Memory[0..n-1]; Memory[0] is the first
                # function argument (vm_callFunPtr @ 0x809441c).
                args = [stack.pop() if stack else "<?>" for _ in range(nargs)]
                stack.append("(: %s, %s :)" % (fname, ", ".join(args)))
            else:
                stack.append("(: %s :)" % fname)

        # ---- Member access (IDA: vm_queryMember @ 0x808fa60) ----
        # p16 flag: bit0=idx negative (from end, i.e. length - idx)
        elif an == 67:  # member - obj[idx]
            if len(stack) >= 2:
                idx = stack.pop(); obj = stack.pop()
                if p16 & 1:
                    idx = "<%s" % idx  # from_end
                # Wrap $< (function args) in parentheses for subscript
                if obj == "$<":
                    obj = "($<)"
                stack.append("%s[%s]" % (obj, idx))
            elif stack:
                stack.pop(); stack.append("<?>[<?>]")
        elif an == 68:  # member_ptr - &obj[idx]
            if len(stack) >= 2:
                idx = stack.pop(); obj = stack.pop()
                if p16 & 1:
                    idx = "<%s" % idx  # from_end
                # When obj already starts with & (nested member_ptr), don't add
                # another &. The & applies to the entire subscript expression.
                if obj.startswith('&'):
                    stack.append("%s[%s]" % (obj, idx))
                else:
                    stack.append("&%s[%s]" % (obj, idx))
            elif stack:
                stack.pop(); stack.append("&<?>[<?>]")

        # ---- Assignments ----
        elif an == 3:  # assignValue (ptr, val -> var = val)
            if len(stack) >= 2:
                ptr = stack.pop()
                val = stack.pop()
                var = ptr.lstrip('&') if ptr.startswith('&') else ptr
                statements.append(("%s = %s;" % (var, val), line_info))
                stack.append(var)
        elif an == 76:  # set_global
            if stack:
                val = stack.pop()
                statements.append(("%s = %s;" % (get_var_name(p16), val), line_info))
        elif an == 78:  # set_ovar
            if stack:
                val = stack.pop()
                statements.append(("%s = %s;" % (get_var_name(p16), val), line_info))
        elif an == 77:  # set_local
            if stack:
                val = stack.pop()
                statements.append(("local_%d = %s;" % (p16, val), line_info))

        # ---- In-place modifications ----
        elif an == 34:  # inc - var += amt (vm_incValue reads ptr from stack)
            if stack:
                ptr = stack.pop()
                var = ptr[1:] if ptr.startswith('&') else ptr
                amt = p16 if p16 < 0x8000 else p16 - 0x10000
                if amt == 1:
                    statements.append(("++%s;" % var, line_info))
                elif amt == -1:
                    statements.append(("--%s;" % var, line_info))
                else:
                    statements.append(("%s += %d;" % (var, amt), line_info))
        elif an == 47:  # op_eq - compound assignment (ptr, val -> var op= val)
            ops = {2: "+=", 3: "-=", 4: "*=", 5: "/=", 6: "%=",
                   7: "|=", 8: "^=", 9: ">>=", 10: "<<="}
            op = ops.get(p16, "?=")
            if len(stack) >= 2:
                val = stack.pop()
                ptr = stack.pop()
                var = ptr.lstrip('&') if ptr.startswith('&') else ptr
                statements.append(("%s %s %s;" % (var, op, val), line_info))
                stack.append(var)

        # ---- Function calls ----
        elif an == 6:  # call_efun
            nargs = p8
            args = [stack.pop() if stack else "<?>" for _ in range(nargs)]
            stack.append("%s(%s)" % (efun_name(p16), ", ".join(args)))
        elif an == 7:  # call_user
            nargs = p8 & 0x3F
            is_global = (p8 >> 7) & 1
            args = [stack.pop() if stack else "<?>" for _ in range(nargs)]
            # Patch 39: per IDA vm_callInUserFun @ 0x08094080, is_global=1 means
            # vm_pGlobalProgram->pFunctions[v6] (global program function table),
            # NOT efun/sefun. Check func_table FIRST when is_global=1.
            if is_global:
                if p16 in func_table:
                    fname = func_table[p16]
                else:
                    fname = EFUN_TABLE.get(p16) or SEFUN_TABLE.get(p16, "global_fn_%d" % p16)
            elif p16 in func_table:
                fname = func_table[p16]
            else:
                fname = "local_fn_%d" % p16
            stack.append("%s(%s)" % (fname, ", ".join(args)))
        elif an == 8:  # call_other - obj->func(args)
            nargs = p8
            obj = stack.pop() if stack else "<?>"
            fn_val = stack.pop() if stack else "<?>"
            args = [stack.pop() if stack else "<?>" for _ in range(nargs)]
            if isinstance(fn_val, str) and fn_val.startswith('"'):
                fname = fn_val.strip('"')
            else:
                fname = str(fn_val)
            # Wrap obj in parentheses when it is a complex expression so the
            # '->' operator binds to the whole object expression.
            if _needs_paren_for_member(obj):
                obj = "(%s)" % obj
            stack.append("%s->%s(%s)" % (obj, fname, ", ".join(args)))
        elif an == 5:  # call_ptr - funcptr(args)
            # Stack layout (IDA vm_callFunPtr): [args..., funcptr] (funcptr on top)
            # p8 = total count INCLUDING funcptr. Pop order: funcptr, args.
            nargs = p8
            popped = [stack.pop() if stack else "<?>" for _ in range(nargs)]
            funcptr = popped[0] if popped else "<?>"
            call_args = popped[1:]
            # Patch 27: Use (*funcptr)(args) uniformly. LPC supports (*expr)(args)
            # for arbitrary expressions (verified by user), so no need to distinguish
            # simple vs complex. This supersedes Patch 25 (evaluate) and Patch 26 (temp).
            stack.append("(*%s)(%s)" % (funcptr, ", ".join(call_args)))
        elif an == 9:  # call_os - os_N(args)
            nargs = p8
            args = [stack.pop() if stack else "<?>" for _ in range(nargs)]
            stack.append("os_%d(%s)" % (p16, ", ".join(args)))

        # ---- Cast / typeof / sizeof ----
        elif an == 13:  # cast
            t = {2: "int", 3: "real", 4: "string", 5: "buffer",
                 6: "array", 7: "mapping"}
            if stack:
                a = stack.pop()
                stack.append("(%s)%s" % (t.get(p16, "T%d" % p16), a))
        elif an == 36:  # typeof
            is_fn = IS_TYPE_MAP.get(p16)
            if is_fn and stack:
                stack[-1] = "%s(%s)" % (is_fn, stack[-1])
            elif stack:
                tn = TYPE_CODES.get(p16, "T%d" % p16)
                stack[-1] = "(typeof(%s) == %s)" % (stack[-1], tn)
        elif an == 79:  # sizeof
            if stack:
                a = stack.pop()
                stack.append("sizeof(%s)" % a)

        # ---- Arithmetic (binary) ----
        elif an == 1:  # add - also handles mapping merge in init code
            if len(stack) >= 2:
                right = stack.pop()
                left = stack.pop()
                # Merge: if both are mapping literals, combine entries
                if left.startswith('([') and right.startswith('(['):
                    l_inner = left[2:-2].strip()
                    r_inner = right[2:-2].strip()
                    if not l_inner:
                        merged = right
                    elif not r_inner:
                        merged = left
                    else:
                        merged = "([ %s, %s ])" % (l_inner, r_inner)
                    stack.append(merged)
                else:
                    stack.append("(%s + %s)" % (left, right))
        elif an == 81:  _binop("-")    # sub
        elif an == 41:  _binop("*")    # mul
        elif an == 25:  _binop("/")    # div
        elif an == 40:  _binop("%")    # mod
        elif an == 2:   _binop("&")    # and (bitwise)
        elif an == 48:  _binop("|")    # or  (bitwise)
        elif an == 85:  _binop("^")    # xor
        elif an == 38:  _binop("<<")   # lsh
        elif an == 75:  _binop(">>")   # rsh

        # ---- Comparison ----
        elif an == 28:  _binop("==")   # eq
        elif an == 42:  _binop("!=")   # ne
        elif an == 32:  _binop(">")    # gt
        elif an == 30:  _binop(">=")   # ge
        elif an == 39:  _binop("<")    # lt
        elif an == 37:  _binop("<=")  # le

        # ---- Unary ----
        elif an == 43:  _unop("-")     # neg
        elif an == 45:  _unop("!")     # not (logical)
        elif an == 74:  _unop("~")     # rev (bitwise not)

        # ---- Patch 41: pointer/range operations (were silently skipped) ----
        # IDA vm_convertToValue @ 0x808c028: pop buffer ptr from stack top,
        # copy the value pointed to into stack top (i.e. dereference).
        elif an == 20:  # to_value (convertToValue)
            if stack:
                a = stack.pop()
                # Simplify *&var to var (push_global_ptr + convertToValue
                # is the common pattern for reading a variable's value
                # before using it in an expression / assignment).
                if a.startswith("&"):
                    stack.append(a[1:])
                else:
                    stack.append("(*%s)" % a)
        elif an == 19:  # to_ptr (convertToPtr)
            if stack:
                a = stack.pop()
                if a.startswith("&"):
                    stack.append(a)  # already a pointer
                else:
                    stack.append("&%s" % a)
        # IDA vm_queryRange @ 0x808fd04: pop idx2, pop idx1, pop container,
        # push container[idx1..idx2]. p16 bit0=idx2 from end, bit1=idx1 from end.
        elif an == 69:  # range - container[idx1..idx2]
            if len(stack) >= 3:
                idx2 = stack.pop()
                idx1 = stack.pop()
                obj = stack.pop()
                obj = obj.lstrip("&") if obj.startswith("&") else obj
                if p16 & 1:
                    idx2 = "<%s" % idx2
                if p16 & 2:
                    idx1 = "<%s" % idx1
                if obj == "$<":
                    obj = "($<)"
                stack.append("%s[%s..%s]" % (obj, idx1, idx2))
        elif an == 70:  # range_ptr - &container[idx1..idx2]
            if len(stack) >= 3:
                idx2 = stack.pop()
                idx1 = stack.pop()
                obj = stack.pop()
                obj = obj.lstrip("&") if obj.startswith("&") else obj
                if p16 & 1:
                    idx2 = "<%s" % idx2
                if p16 & 2:
                    idx1 = "<%s" % idx1
                stack.append("&%s[%s..%s]" % (obj, idx1, idx2))

        # ---- Stack ops ----
        # Patch 38: pop (an=51) was silently discarding function call results.
        # When the popped value has side effects (e.g. a call_ptr/call_efun
        # invocation like load_dir("/gs/daemons/")), emit it as a statement
        # before discarding, so the startup sequence is not lost.
        elif an == 51:  # pop
            # Patch 40: skip pop that follows a do_pop=0 if
            # (condition already consumed by the if handler)
            if pending_if_pop:
                pending_if_pop = False
            elif stack:
                val = stack.pop()
                if val and _has_side_effect_init(val):
                    statements.append(("%s;" % val, line_info))
        elif an == 50:  # pop_n
            for _ in range(p16):
                if stack:
                    stack.pop()

        # ---- Control flow ----
        # Patch 40: real if/goto block handling. The if instruction's p16
        # is a signed jump offset: target = i + 1 + p16 (if p16 < 0x8000,
        # else p16 - 0x10000). p8 bit 0 (is_jz): 0=jump-if-false (then-branch
        # between if and target), 1=jump-if-true (condition negated). p8 bit 1
        # (do_pop): 1=if consumes condition, 0=condition stays on stack
        # (consumed by next pop, skipped via pending_if_pop flag).
        elif an == 33:  # if
            is_jz = (p8 & 1) != 0
            do_pop = (p8 & 2) != 0
            cond = stack.pop() if stack else "<?>"
            if not do_pop:
                pending_if_pop = True
            jump = p16 if p16 < 0x8000 else p16 - 0x10000
            target = i + 1 + jump
            # Negate condition for is_jz=1 (jump if true)
            if is_jz:
                if cond.startswith('(!') and cond.endswith(')'):
                    cond = cond[2:-1]
                else:
                    cond = "!(%s)" % cond
            # Detect ternary expression: if the then-branch ends with a goto
            # jumping past the else-branch, and both branches contain only
            # quiet (stack-manipulating) opcodes, treat as a ternary value
            # (cond ? then_val : else_val) rather than a statement-level
            # if-else block. This fixes empty if-else blocks that arise from
            # conditional expressions inside mapping/array literals.
            is_ternary = False
            else_end = None
            if target - 1 > i + 1:  # then-branch has >=1 value op + goto
                g_insn = nodes[target - 1]
                if ((g_insn >> 24) & 0xFF) == 31:  # goto at end of then-branch
                    g_p16 = g_insn & 0xFFFF
                    g_jump = g_p16 if g_p16 < 0x8000 else g_p16 - 0x10000
                    g_target = target + g_jump  # = (target-1)+1+g_jump
                    if g_target > target:
                        else_end = g_target
            if else_end is not None:
                then_quiet = all(
                    ((nodes[j] >> 24) & 0xFF) in QUIET_INIT_OPS
                    for j in range(i + 1, target - 1))
                else_quiet = all(
                    ((nodes[j] >> 24) & 0xFF) in QUIET_INIT_OPS
                    for j in range(target, else_end))
                if then_quiet and else_quiet:
                    is_ternary = True
            if is_ternary:
                ternary_stack.append({
                    'cond': cond, 'then_val': None, 'then_val_set': False,
                    'else_end': else_end,
                })
            else:
                if_blocks.append({'start': i, 'target': target, 'cond': cond,
                                  'else_emitted': False, 'end_target': target})
                statements.append(("if (%s) {" % cond, line_info))
        elif an == 31:  # goto
            jump = p16 if p16 < 0x8000 else p16 - 0x10000
            gtarget = i + 1 + jump
            # Check if this goto is the else-boundary of an active ternary
            # expression. If so, capture the then-value (on stack top) and
            # do NOT emit "} else {" -- the ternary is an expression, not a
            # statement-level block.
            if ternary_stack and not ternary_stack[-1]['then_val_set']:
                t = ternary_stack[-1]
                if gtarget == t['else_end']:
                    t['then_val'] = stack.pop() if stack else "<?>"
                    t['then_val_set'] = True
                else:
                    statements.append(("/* goto insn_%d */" % p16, line_info))
            # Check if this goto is the else-jump of the current if-block
            elif if_blocks and not if_blocks[-1]['else_emitted']:
                cur_if = if_blocks[-1]
                if gtarget > cur_if['target']:
                    cur_if['else_emitted'] = True
                    cur_if['end_target'] = gtarget
                    statements.append(("} else {", line_info))
                else:
                    statements.append(("/* goto insn_%d */" % p16, line_info))
            else:
                statements.append(("/* goto insn_%d */" % p16, line_info))
        elif an == 73:  # ret
            if stack:
                stack.pop()
            statements.append(("return;", line_info))
            stack.clear()
        elif an == 72:  # ret_fake
            statements.append(("return;", line_info))
            stack.clear()
        # Opcodes 0/15/27/29/46 are markers (nop/checkargs/enter/finish/nop) - skip
        # Unhandled opcodes: silently skip to preserve stack coherence

    # Safety cleanup: close any ternary expressions not closed within the
    # main loop (e.g. when the else-branch is the very last instruction).
    while ternary_stack:
        t = ternary_stack.pop()
        if t['then_val_set']:
            else_val = stack.pop() if stack else "<?>"
            stack.append("(%s ? %s : %s)" % (t['cond'], t['then_val'], else_val))

    # Build output lines
    out_lines = []
    # Filter out trailing 'return;' — it's just the ret instruction ending the
    # init function, not a real initialization. Suppress the entire section
    # when only 'return;' remains (no actual global variable initializations).
    # Patch 40: compute indentation level based on if/else/} markers
    # so nested if-blocks are properly indented.
    filtered = [s for s in statements if s[0] != "return;"]
    if filtered:
        out_lines.append("/* ================================================================")
        out_lines.append("   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)")
        out_lines.append("   ================================================================ */")
        out_lines.append("")
        indent_level = 0
        for text, line_info in filtered:
            # Decrease indent before printing closing brace.
            # Also decrease for } else { (the } closes the then-branch).
            if text == "}" or text == "} else {":
                indent_level = max(0, indent_level - 1)
            line = "   " + ("    " * indent_level) + text
            if line_info:
                line += "  " + line_info
            out_lines.append(line)
            # Increase indent after printing if/else opening
            if text.endswith(" {") or text == "} else {":
                indent_level += 1
        out_lines.append("")
    return out_lines


# ============================================================================
# Segment Detection (same as v36)
# ============================================================================
def find_prologue_segments(prog):
    segs = []
    for i in range(len(prog) - 8):
        # First instruction must be checkargs (an=15)
        if prog[i+3] != 15:
            continue
        if i < 4:
            continue
        nc = struct.unpack_from('<I', prog, i - 4)[0]
        if not (3 <= nc <= 2000):
            continue
        # Validate that instructions are mostly known opcodes
        valid = 0
        total = min(nc, 20)
        for ni in range(total):
            off = i + ni * 4
            if off + 4 > len(prog):
                break
            if prog[off + 3] in KNOWN_OPS:
                valid += 1
        if valid < total * 0.7:
            continue
        # Check for enter(27) in the first few instructions (not necessarily
        # the second one - some functions have push/sizesize before enter).
        # This filters out false positives that happen to start with checkargs.
        # Patch 22: extend search range to 30 to cover multi-arg functions
        # (e.g. 20-arg functions have ~20 sizesize/save_register before enter).
        has_checkargs = False
        for ni in range(min(nc, 30)):
            off = i + ni * 4
            if off + 4 > len(prog):
                break
            if prog[off + 3] == 27:  # enter
                has_checkargs = True
                break
        if not has_checkargs:
            # Fallback: accept if the first 3 instructions are all known opcodes
            # (some functions may not have enter at all)
            first_valid = 0
            for ni in range(min(nc, 3)):
                off = i + ni * 4
                if off + 4 > len(prog):
                    break
                if prog[off + 3] in KNOWN_OPS:
                    first_valid += 1
            if first_valid < 3:
                continue
        segs.append({
            'ns': i,
            'nc': nc,
            'ne': i + nc * 4,
        })
    return segs


def extract_func_metadata(prog, ns, nc):
    """Extract per-function metaData for complex instructions (case_num/case_str).

    The .o file layout per function (vm_saveCodeSegment):
      [symbolsCount(4)] [symbols...] [nodesCount(4)] [nodes(nc*4)] [metaDataLen(4)] [metaData...]

    find_prologue_segments locates the nodes region [ns, ns+nc*4).
    metaData starts right after nodes: at offset (ns + nc*4) + 4 (skip metaDataLen).

    Returns: bytes of metaData, or empty bytes if not available.
    """
    meta_len_off = ns + nc * 4
    if meta_len_off + 4 > len(prog):
        return b''
    meta_len = struct.unpack_from('<I', prog, meta_len_off)[0]
    if meta_len <= 0 or meta_len > 100000:
        return b''
    meta_start = meta_len_off + 4
    if meta_start + meta_len > len(prog):
        return prog[meta_start:]
    return prog[meta_start:meta_start + meta_len]


def match_func_names(prog, user_funcs, segs, name_bytes_list=None, encoding='gb2312'):
    """Match code segments to nearest preceding symbol name in prog.

    Searches for [len(4)][name\\0] patterns near each code segment,
    using ALL symbol names extracted from FFFFFFFF entries (user_funcs).
    Falls back to internal_N for unmatched segments.

    Uses confidence-based two-phase matching:
      Phase 1: Greedy closest-first assignment (original behavior)
      Phase 2: Steal resolution - if a name is matched to multiple segments,
               keep the closest, free the others for re-matching

    name_bytes_list: optional list of raw bytes parallel to user_funcs.
      If provided, uses raw bytes directly for binary search (avoids
      re-encoding errors for names with non-gb2312 characters).
    """
    func_name_positions = []
    if name_bytes_list is not None and len(name_bytes_list) == len(user_funcs):
        # Use raw bytes directly (no encoding/decoding)
        for nm, nm_bytes in zip(user_funcs, name_bytes_list):
            target = struct.pack('<I', len(nm_bytes) + 1) + nm_bytes + b'\x00'
            idx = 0
            while True:
                idx = prog.find(target, idx)
                if idx < 0:
                    break
                func_name_positions.append((idx, nm))
                idx += 1
    else:
        # Fallback: encode names to gb2312 (skip unencodable ones)
        for nm in user_funcs:
            try:
                nm_bytes = nm.encode(encoding)
            except (UnicodeEncodeError, UnicodeDecodeError):
                continue
            target = struct.pack('<I', len(nm_bytes) + 1) + nm_bytes + b'\x00'
            idx = 0
            while True:
                idx = prog.find(target, idx)
                if idx < 0:
                    break
                func_name_positions.append((idx, nm))
                idx += 1
    func_name_positions.sort()

    MAX_NAME_DIST = 5000

    # Pre-compute sorted position-only list for bisect: O(P) once here,
    # enabling O(log P) lookup per segment instead of O(P) linear scan.
    _pos_only = [pp[0] for pp in func_name_positions] if func_name_positions else []

    # Phase 1: assign each segment to closest preceding name.
    # func_name_positions is sorted by position, so the closest preceding name
    # within MAX_NAME_DIST is the last entry with pos < ns.
    for p in segs:
        ns = p['ns']
        lo = bisect.bisect_left(_pos_only, ns - MAX_NAME_DIST)
        hi = bisect.bisect_left(_pos_only, ns)
        if hi > lo:
            best_pos, best_nm = func_name_positions[hi - 1]
            p['func_name'] = best_nm
            p['name_pos'] = best_pos
        else:
            p['func_name'] = None
            p['name_pos'] = None

    # Phase 2: resolve duplicates - closest match wins, others freed
    name_to_segs = {}
    for p in segs:
        nm = p.get('func_name')
        if nm:
            name_to_segs.setdefault(nm, []).append(p)
    for nm, plist in name_to_segs.items():
        if len(plist) > 1:
            plist.sort(key=lambda p: p['ns'] - p['name_pos'])
            for p in plist[1:]:
                p['func_name'] = None
                p['name_pos'] = None

    # Phase 3: re-match freed segments with remaining unused names.
    # Uses bisect to find the [ns-MAX_NAME_DIST, ns) slice, then scans backward
    # to pick the closest unused name.
    used_names = set()
    for p in segs:
        if p.get('func_name'):
            used_names.add(p['func_name'])

    for p in segs:
        if p.get('func_name'):
            continue
        ns = p['ns']
        lo = bisect.bisect_left(_pos_only, ns - MAX_NAME_DIST)
        hi = bisect.bisect_left(_pos_only, ns)
        for j in range(hi - 1, lo - 1, -1):
            pos, nm = func_name_positions[j]
            if nm not in used_names:
                p['func_name'] = nm
                p['name_pos'] = pos
                used_names.add(nm)
                break

    # Fallback: generic names for unmatched
    hidden_idx = 0
    for p in sorted(segs, key=lambda x: x['ns']):
        if not p.get('func_name'):
            p['func_name'] = "internal_%d" % hidden_idx
            hidden_idx += 1

    return segs


# ============================================================================
# Variable Name Inference Engine
# ============================================================================
class VarNameInfer:
    """Intelligently map bytecode variable indices to human-readable names.

    Note: Variable names extracted from .o file are not mapped to bytecode
    indices (indices are per-function while names are global). Therefore:
    - Arguments use arg_0, arg_1, ... (LPC convention)
    - Local variables use local_1, local_2, ... (first-declared-first)
    - Object variables try to use extracted names when available
    """

    def __init__(self, local_vars, user_funcs, strings, func_local_vars=None, func_args=None):
        self.local_vars = list(local_vars)  # extracted var names from .o
        self.user_funcs = set(user_funcs) if user_funcs else set()
        self.strings = strings
        self.func_local_vars = func_local_vars or {}  # {vm_idx: name}
        # Per-function argument names from DLT: {vm_idx(0-based): name}
        self.func_args = func_args or {}
        # Build a filtered list: replace function names with generic var names
        # to avoid returning function names as variable names
        self._clean_vars = []
        for v in self.local_vars:
            if v in self.user_funcs:
                # This is likely a function name, not a variable
                self._clean_vars.append(None)
            else:
                self._clean_vars.append(v)

    def arg_name(self, idx):
        """Name for argument local[idx] - arg_0 = first param, arg_1 = second, etc.
        Uses real argument name from DLT when available."""
        if idx in self.func_args:
            return self.func_args[idx]
        return "arg_%d" % idx

    def local_name(self, idx):
        """Name for local variable local[-idx].
        Uses extracted name from .o file when available (via func_local_vars).
        The VM index is negative (e.g., -2 for local_2), stored in .o file
        12 bytes before the variable name's [len] field.
        """
        if idx in self.func_local_vars:
            return self.func_local_vars[idx]
        return "local_%d" % abs(idx)

    def local_name_by_vmidx(self, vm_idx):
        """Name for local variable by VM index (negative).
        Wrapper for local_name that accepts the raw VM index.
        """
        return self.local_name(vm_idx)

    def ovar_name(self, p16):
        """Name for object variable (this_object global). Try to use extracted names.

        Uses local_vars directly (unfiltered) like ovar_name_dasm().
        _clean_vars filters out names that appear in user_funcs, but user_funcs
        includes ALL identifiers from the program data block (both function names
        and global variable names), so filtering would incorrectly drop valid
        variable names like duijiang_start_time, bonus_end_time, etc.
        """
        if 0 <= p16 < len(self.local_vars):
            return self.local_vars[p16]
        return "this_var_%d" % p16


# ============================================================================
# Enhanced Pseudocode Generator (v37)
# ============================================================================
class PseudoGenV37:
    """Generate highly readable pseudocode from LPC bytecode.

    Improvements over v36:
      - Proper variable naming via VarNameInfer
      - EFUN documentation comments
      - Control flow structure annotations
      - Switch/case block detection
      - Expression simplification (remove redundant parentheses)
      - Function result tracking
    """

    def __init__(self, strings, local_vars, user_funcs, metadata=b'', ret_type='mixed',
                 func_local_vars=None, func_table=None, func_args=None, var_types=None,
                 inherit_idx_to_path=None, inherit_failed=None, inherit_ranges=None,
                 suppress_parent_path=False, suppress_trailing_comments=False):
        self.strings = strings
        self.user_funcs = user_funcs
        self.func_table = func_table or {}  # {table_idx: func_name} from DLT segment
        # Inherit state from LPCOFile for parent-path fallback in call_user.
        self._inherit_idx_to_path = inherit_idx_to_path or {}
        self._inherit_failed = inherit_failed or []
        self._inherit_ranges = inherit_ranges or []  # [(start, end, iname), ...]
        self.metadata = metadata  # per-function metaData bytes (for case_num/case_str)
        self.var_names = VarNameInfer(local_vars, user_funcs, strings, func_local_vars, func_args)
        self.global_var_types = var_types or {}  # {valueNo: type_byte} from PROG_VARS
        self.stack = []
        self.lines = []
        self.indent = 1
        self._if_stack = []
        self._loop_stack = []
        self._ret_type = ret_type  # function return type for suppressing void returns
        self.var_types = {}  # inferred local variable types (populated after generate)
        self.suppress_parent_path = suppress_parent_path
        self.suppress_trailing_comments = suppress_trailing_comments

    def _ind(self):
        return "    " * self.indent

    def _push(self, val):
        self.stack.append(val)

    def _pop(self):
        if self.stack:
            return self.stack.pop()
        return "<?>"

    # Variable type mapping (see vm_valueType @ 0x0808A13C)
    _TYPE_NAMES = {
        0: 'undefined', 1: 'void', 2: 'int', 3: 'float', 4: 'string',
        5: 'buffer', 6: 'array', 7: 'mapping', 8: 'pointer', 9: 'function',
        10: 'object', 20: 'loop_ctrl', 255: 'mixed',
    }

    def _var_type_name(self, idx):
        """Return LPC type name for a global variable index, or 'mixed' if unknown."""
        tb = self.global_var_types.get(idx)
        if tb is None:
            return 'mixed'
        return self._TYPE_NAMES.get(tb, 'mixed')

    def _prev_real_line_is_return(self):
        """Scan backwards past comments/labels/empty lines to check if the
        last real statement was a return."""
        i = len(self.lines) - 1
        while i >= 0:
            s = self.lines[i].strip()
            if not s or s.startswith('//'):
                i -= 1
                continue
            return s.startswith('return')
        return False

    def _cleanup_trailing_label(self):
        """Remove trailing label comments left after suppressing dead returns."""
        while self.lines:
            s = self.lines[-1].strip()
            if not s or s.startswith('// --- label'):
                self.lines.pop()
            else:
                break

    def _arg_name(self, idx):
        if idx >= 0x8000: idx = idx - 0x10000
        if idx < 0:
            return self.var_names.local_name(idx)  # pass negative VM index
        return self.var_names.arg_name(idx)

    def _global_var_name(self, idx):
        """Resolve push_global/push_global_ptr/set_global index to a name.

        pushGlobalVar references the GLOBAL PROGRAM's variable table (the
        startup .o, e.g. start_aaa.o) - NOT this object's own table (which
        push_ovar/push_ovar_ptr/set_ovar use). Verified in accountd.o:
        push_global p16 ranges up to 79 while the own table has only 30
        entries; own vars (e.g. g_auth_method = own #27) are always accessed
        via push_ovar, while push_global(#27) is start_aaa.o's INI_FILE.

        Resolution order:
        1. GLOBAL_VAR_TABLE (startup .o's variable table via -q).
        2. When no startup table is available (-q absent), fall back to the
           file's own table (correct when decompiling the startup itself).
        3. Fallback to global_<idx>.
        """
        import re
        if idx in GLOBAL_VAR_TABLE:
            return GLOBAL_VAR_TABLE[idx]
        if not GLOBAL_VAR_TABLE:
            lvars = self.var_names.local_vars
            if lvars and 0 <= idx < len(lvars):
                name = lvars[idx]
                if name and not re.match(r'^global_\d+$', name):
                    return name
        return "global_%d" % idx

    def _has_side_effect(self, val):
        """Check if expression has side effects worth emitting as a statement.

        Returns True for:
        - Direct function calls: func(args)
        - Function pointer calls: (*funcptr)(args)
        - call_other calls: obj->method(args) [including quoted/Chinese names]
        - Function closure calls: (:id, nargs:)->method(args)
        - Expressions containing function calls: (func(...) != 0), etc.
        """
        # Patch 37: removed `len(val) <= 3` early-return that misjudged short
        # function calls like f() / x() (length exactly 3) as side-effect-free,
        # causing pop/pop_n to silently discard their statements. Rely on the
        # regex matchers below for all side-effect detection.
        if not val:
            return False
        # Direct function call: identifier(...)
        if re.match(r'^[a-zA-Z_]\w*\s*\(', val):
            return True
        # Patch 24+28: Function pointer call via (*expr)(args).
        # After Patch 27, all call_ptr output uses (*funcptr)(args) form,
        # covering: (*f)(), (*mks[1])(), (*find_function_arg(...))(), etc.
        # Match (*anything-that-ends-with))() - the leading "(*" and trailing
        # ")(...)" is the signature of a function pointer dereference call.
        if val.startswith("(*") and re.match(r'^\(\*.+\)\s*\(', val):
            return True
        # Legacy subscript form (pre-Patch 27): identifier[...](args)
        if re.match(r'^[a-zA-Z_]\w*(\[[^\]]*\])+\s*\(', val):
            return True
        # Parent-path function call: "path"::func(...) or "path"::local_fn_N(...)
        if re.match(r'^"[^"]*"::[a-zA-Z_]\w*\s*\(', val):
            return True
        # call_other patterns:
        #   obj->method(args), obj->"method"(args), obj->中文(args)
        #   (:N,N:)->method(args),  (:N,N:)->"中文"(args)
        if '->' in val and '(' in val:
            return True
        # Function call at the top level of an expression (e.g. (func(...) != 0)):
        # Strip string literals and bracket contents to avoid matching calls
        # nested inside member-access indices (e.g. thread_dbc[get_thread_id()]
        # is a member access, not a side-effecting call). Then look for an
        # identifier immediately followed by '('.
        stripped = re.sub(r'"[^"]*"', '""', val)
        prev = None
        while prev != stripped:
            prev = stripped
            stripped = re.sub(r'\[[^\[\]]*\]', '[]', stripped)
        if re.search(r'[a-zA-Z_]\w*\s*\(', stripped):
            return True
        # Patch 30: Mapping merge statement (m1 << m2) modifies m1
        # in-place (side effect). LPC << on mapping is in-place merge;
        # on int it is a shift (no side effect). As a popped statement,
        # << almost always means mapping merge. PseudoGenV37 binop emits
        # "(a op b)" form, so check if top-level op (depth 0) is <<.
        if val.startswith('(') and val.endswith(')'):
            inner = val[1:-1]
            depth = 0
            for i in range(len(inner) - 3):
                c = inner[i]
                if c == '(':
                    depth += 1
                elif c == ')':
                    depth -= 1
                if depth == 0 and inner[i:i+4] == ' << ':
                    return True
        return False

    def _simplify_expr(self, expr):
        """Simplify expression by removing unnecessary parentheses."""
        # Remove leading/trailing parens when possible
        if expr.startswith('(') and expr.endswith(')') and '(' not in expr[1:-1]:
            return expr[1:-1]
        return expr

    def _infer_var_types(self):
        """Infer local variable types from generated pseudocode context.

        Analyzes self.lines for type hints and returns a dict {var_name: type}.
        Uses multiple heuristics in priority order:
        1. typeof(var) == TYPE  (strongest - explicit runtime type check)
        2. var = (TYPE)expr     (explicit cast in assignment)
        3. foreach (x in var)   (container -> array/mapping)
        4. var == int / var << N / arr[var]  (integer usage patterns)
        5. Float literal in assignment expr   (float type)
        6. var += float_expr    (compound assignment with float operand)
        7. Known method return types (query_skill_class -> int, etc.)
        8. All-integer arithmetic expression -> int
        """
        types = {}
        all_text = '\n'.join(self.lines)

        # Rule 1: typeof(var) == TYPE  (strongest signal)
        for m in re.finditer(r'typeof\(\s*(\w+)\s*\)\s*==\s*(\w+)', all_text):
            var, typ = m.group(1), m.group(2)
            types[var] = typ  # strongest, overwrites

        # Rule 1b: var = "string literal";  => string
        for m in re.finditer(r'\b(\w+)\s*=\s*"[^"]*"\s*;', all_text):
            var = m.group(1)
            if var not in types:
                types[var] = 'string'

        # Rule 1c: var["key"]  => mapping (subscript with string key)
        for m in re.finditer(r'\b(\w+)\s*\[\s*"[^"]*"\s*\]', all_text):
            var = m.group(1)
            if var not in types:
                types[var] = 'mapping'

        # Rule 1d: var = 0; or var = INTEGER;  => int (but not if it's a bool flag)
        for m in re.finditer(r'\b(\w+)\s*=\s*(\d+)\s*;', all_text):
            var = m.group(1)
            if var not in types:
                types[var] = 'int'

        # Rule 2: var = (TYPE)expr;  (explicit cast assignment)
        for m in re.finditer(r'\b(\w+)\s*=\s*\((int|float|real|string|object|array|mapping|buffer)\)',
                             all_text):
            var, typ = m.group(1), m.group(2)
            if typ == 'real':
                typ = 'float'
            if var not in types:
                types[var] = typ

        # Rule 3: foreach (x in var) -> var is array (or mapping)
        for m in re.finditer(r'foreach\s*\(\s*\w+\s+in\s+(\w+)\s*\)', all_text):
            var = m.group(1)
            if var not in types:
                types[var] = 'array'

        # Rule 4: Integer usage patterns
        # 4a: var compared to integer literal (not float)
        for m in re.finditer(r'\b(\w+)\s*(==|!=|>=|<=|>|<)\s*\d+(?!\.\d)', all_text):
            var = m.group(1)
            if var not in types:
                types[var] = 'int'
        # 4b: var << N or var >> N (bit shift implies int)
        for m in re.finditer(r'\b(\w+)\s*(<<|>>)\s*\d+', all_text):
            var = m.group(1)
            if var not in types:
                types[var] = 'int'
        # 4c: var used as array index [var] (single identifier inside brackets)
        for m in re.finditer(r'\[\s*(\w+)\s*\]', all_text):
            var = m.group(1)
            if var not in types:
                types[var] = 'int'
        # 4d: var in bitwise OR with shift expr, e.g. (x << 8) | var  or  var | (x << 8)
        # (distinguishes from || by requiring << on one side)
        for m in re.finditer(r'<<\s*\d+\s*\)\s*\|\s*(\w+)', all_text):
            var = m.group(1)
            if var not in types:
                types[var] = 'int'
        for m in re.finditer(r'(\w+)\s*\|\s*\w+\s*<<', all_text):
            var = m.group(1)
            if var not in types:
                types[var] = 'int'

        # Rule 5: Float literal in assignment -> float
        for m in re.finditer(r'\b(\w+)\s*=\s*([^;]+);', all_text):
            var, expr = m.group(1), m.group(2)
            if var in types:
                continue
            if expr.strip().startswith('"'):
                continue
            expr_no_str = re.sub(r'"[^"]*"', '', expr)
            if re.search(r'\d+\.\d+', expr_no_str):
                if not re.match(r'\s*\(int\)', expr.strip()):
                    types[var] = 'float'

        # Rule 6: var += float_expr (compound assignment with float operand)
        for m in re.finditer(r'\b(\w+)\s*\+=\s*([^;]+);', all_text):
            var, expr = m.group(1), m.group(2)
            if var in types:
                continue
            expr_no_str = re.sub(r'"[^"]*"', '', expr)
            if re.search(r'\d+\.\d+', expr_no_str):
                types[var] = 'float'
            else:
                for vm in re.finditer(r'\b(\w+)\b', expr_no_str):
                    if vm.group(1) in types and types[vm.group(1)] == 'float':
                        types[var] = 'float'
                        break

        # Rule 7: Method-based type inference (conservative - only well-known methods)
        method_return_types = {
            'query_skill_class': 'int',
            'query_skill_ladder': 'int',
            'query_skill_subclass': 'int',
            'query_skill_by_object': 'int',
            'query_eqp_skill_by_object': 'int',
            'query_raw_skill_by_object': 'int',
            'query_status_para': 'mapping',
        }
        for m in re.finditer(r'\b(\w+)\s*=\s*\w+->(\w+)\s*\(', all_text):
            var, method = m.group(1), m.group(2)
            if var not in types and method in method_return_types:
                types[var] = method_return_types[method]

        # Rule 8: All-integer arithmetic expression -> int (second pass)
        for m in re.finditer(r'\b(\w+)\s*=\s*([^;]+);', all_text):
            var, expr = m.group(1), m.group(2)
            if var in types:
                continue
            # Skip expressions with function/method calls
            if '->' in expr or re.search(r'\b\w+\s*\(', expr):
                continue
            # Skip if has float literal
            expr_no_str = re.sub(r'"[^"]*"', '', expr)
            if re.search(r'\d+\.\d+', expr_no_str):
                continue
            # Check if all variables in expression are known int
            vars_in_expr = set(re.findall(r'\b[a-zA-Z_]\w*\b', expr_no_str))
            vars_in_expr.discard(var)
            if vars_in_expr and all(v in types and types[v] == 'int' for v in vars_in_expr):
                types[var] = 'int'

        return types

    def generate(self, insts, nc):
        """Generate enhanced pseudocode."""
        self.insts = insts
        # Optional-argument default values are emitted between checkargs and
        # enter as pushes (e.g. push_i(0); resort). They are already rendered
        # in the signature by extract_default_args, so they must not be treated
        # as body stack values; otherwise a leftover default remains on the
        # simulated stack and causes a bogus return 0; at the end of void
        # functions (e.g. build_onlinemall_buf).
        self._prologue_pushes = set()
        _in_prologue = False
        for _ni, (_an, _p16, _p8) in enumerate(insts):
            if _an == 15:
                _in_prologue = True
                continue
            if _an == 27:
                break
            if _in_prologue and _an in (59, 60, 63, 64, 66):
                self._prologue_pushes.add(_ni)
        # Collect all jump targets and classify them
        self.structured_targets = set()
        self.goto_targets = set()
        # short-circuit OR/AND: map target -> list of (op, first_ni).
        # Multiple first-ifs may point to the same terminal in multi-segment
        # chains like A||B||(C&&(D||E)); each is merged right-associatively.
        self._short_circuit = {}
        # pending conditions per target: list of (op, cond) from first-ifs
        self._sc_pending = {}
        # Protective null-check guard pending: not_ni -> guard_cond.
        # When a do_pop=0 if targets a `not`(45) instruction (compiler-
        # generated guard against null dereference), the guard condition is
        # saved here so the `not` handler can merge it as
        # !(guard_cond && operand) instead of !operand.
        self._guard_pending = {}
        # Catch-block exception-check if (an=33) to suppress: when end_catch
        # is followed by the pattern push_i(0); goto L_body; L_catch: catch;
        # L_body: if_jnz(pop) -> L_normal; ret; L_normal: ..., the if tests
        # the exception flag and is the compiler's catch dispatch, not user
        # code. Suppress the `if (0) { return; }` emission.
        self._catch_check_skip = set()
        # Short-circuit AND/OR-to-assignment pending: keyed by the
        # push_*_ptr ni (the if target). Value = (is_jz, left_cond).
        # When the push_*_ptr is reached, merge left_cond with the
        # right value on stack: is_jz=0 => (A && B); is_jz=1 => (A || B).
        self._sc_assign_pending = {}
        # Ret instructions to skip: when the preceding catch-check if
        # was suppressed, the following ret is the exception path (only
        # reached when an exception was caught). Skip it to let normal
        # code flow continue to L_normal.
        self._catch_ret_skip = set()
        # Type-guard if (an=33) to suppress: when checkreftype(16) is
        # immediately followed by if_jnz(pop), the if is a compiler-internal
        # type check, not user code. Suppress emission.
        self._type_guard_ifs = set()
        # Track if-blocks whose condition was popped by the if itself (do_pop=0).
        # These blocks have stack operations in their body and must NOT be
        # deleted by the empty-block optimization.
        self._if_with_cond_pop = set()
        # When True, the next pop instruction is skipped (condition value was
        # already consumed by a do_pop=0 if).
        self._pending_if_pop = False
        # When True, the next if() emission should be merged as "} else if()"
        # instead of a separate "} else {" + "if () {". Set by else detection
        # when the else block's first instruction is an if; consumed by the
        # if handler at emission time.
        self._pending_else_if = False
        # loop detection: map from loop-head index to loop-exit index
        self._loop_heads = {}   # head_ni -> exit_ni
        self._loop_exits = {}   # goto_ni -> (head_ni, exit_ni)
        self._loop_conds = {}   # cond_if_ni -> exit_ni (while condition)
        self._while_stack = []  # active while loops: (cond_ni, exit_ni)
        self._do_while_heads = {}  # head_ni -> (if_ni, exit_ni)  do-while loop
        self._do_while_ifs = {}    # if_ni -> (head_ni, exit_ni)  do-while back-edge
        self._while1_heads = set()  # Patch 20: while(1) dead loop heads (no exit cond)
        self._in_catch = False  # track catch block state for try/catch
        self._in_catch_block = False  # track catch block entry (for goto skip)
        self._foreach_info = {} # for_cond_ni -> (loop_var, container/limit, step)
        self._pending_foreach = None  # (loop_var, container/limit, step) from for_init, for next for_cond
        for ni, (an, p16, p8) in enumerate(insts):
            if an in (18, 31, 33, 44, 80):
                t = ni + 1 + (p16 if p16 < 0x8000 else p16 - 0x10000)
                if 0 <= t < nc:
                    if an == 31:
                        self.goto_targets.add(t)
                    else:
                        self.structured_targets.add(t)

        # Detect short-circuit OR/AND patterns:
        #   eval A; if_jz(is_jz=1, jump-if-true) -> L; pop; eval B; L: if_jnz(is_jz=0, jump-if-false) -> M
        #   =>  if (A || B) { ... } M
        #   eval A; if_jnz(is_jz=0, jump-if-false) -> L; pop; eval B; L: if_jz(is_jz=1, jump-if-true) -> M
        #   =>  if (A && B) { ... } M
        for ni, (an, p16, p8) in enumerate(insts):
            if an != 33:
                continue
            is_jz = (p8 & 1) != 0
            do_pop = (p8 & 2) != 0
            if do_pop:
                continue
            target = ni + 1 + (p16 if p16 < 0x8000 else p16 - 0x10000)
            if not (0 <= target < nc):
                continue
            # next instruction must be pop
            if ni + 1 >= nc or insts[ni+1][0] != 51:
                continue
            # target must be an `if` instruction or a `ret` (short-circuit
            # ending in return: A; if !A goto L; pop; B; L: ret => return A && B)
            tan, tp16, tp8 = insts[target]
            if tan == 73:  # ret: short-circuit ending in return
                # Short-circuit pattern: if(no_pop) -> L; pop; eval B; L: ret
                # The region between pop and L may contain only simple expression
                # evaluation. Control-flow instructions indicate a different structure.
                # do_pop=0 ifs in the region are chain members (multi-segment
                # OR/AND like A||B||(C&&(D||E))); only do_pop=1 ifs and other
                # control flow disqualify the short-circuit pattern.
                control_flow_ops = {14, 26, 31, 44, 73}  # catch,end_catch,goto,try,ret
                has_control_flow = False
                for mid_ni in range(ni + 2, target):
                    man, mp16, mp8 = insts[mid_ni]
                    if man in control_flow_ops:
                        has_control_flow = True
                        break
                    if man == 33 and (mp8 & 2) != 0:  # do_pop=1 if = real control flow
                        has_control_flow = True
                        break
                if has_control_flow:
                    continue
                # is_jz=0 (jump if false) => A && B (if A is false, skip B, return A=false)
                # is_jz=1 (jump if true)  => A || B (if A is true, skip B, return A=true)
                if is_jz:
                    self._short_circuit.setdefault(target, []).append(('||', ni))
                else:
                    self._short_circuit.setdefault(target, []).append(('&&', ni))
                continue
            if tan != 33:
                continue
            t_is_jz = (tp8 & 1) != 0
            t_do_pop = (tp8 & 2) != 0
            if not t_do_pop:
                # Allow second-if do_pop=0 when:
                # - both ifs are if_false (AND pattern): is_jz=0, t_is_jz=0
                # - both ifs are if_true (OR chain middle): is_jz=1, t_is_jz=1
                # - OR terminal pattern: is_jz=1, t_is_jz=0 (second if do_pop=0,
                #   pop follows as next instruction)
                # - AND terminal pattern: is_jz=0, t_is_jz=1 (first if is AND
                #   start/jump-if-false, second is OR start/jump-if-true;
                #   e.g. (A && B) || ... where AND's terminal is also OR's start)
                # The OR chain middle handles multi-condition || like a||b||c||d
                if not ((is_jz == 0 and t_is_jz == 0) or (is_jz == 1 and t_is_jz == 1)
                        or (is_jz == 1 and t_is_jz == 0)
                        or (is_jz == 0 and t_is_jz == 1)):
                    continue
            # Short-circuit pattern: if(no_pop) -> L; pop; eval B; L: if(pop) -> M
            # The region between pop and L may contain only simple expression
            # evaluation (push/pop/arith). Control-flow instructions (try,
            # catch, end_catch, goto, ret, another if) indicate a different
            # structure and must NOT be treated as short-circuit.
            # do_pop=0 ifs in the region are chain members (multi-segment
            # OR/AND like A||B||(C&&(D||E))); only do_pop=1 ifs and other
            # control flow disqualify the short-circuit pattern.
            control_flow_ops = {14, 26, 31, 44, 73}  # catch,end_catch,goto,try,ret
            has_control_flow = False
            for mid_ni in range(ni + 2, target):
                man, mp16, mp8 = insts[mid_ni]
                if man in control_flow_ops:
                    has_control_flow = True
                    break
                if man == 33 and (mp8 & 2) != 0:  # do_pop=1 if = real control flow
                    has_control_flow = True
                    break
            if has_control_flow:
                continue
            # OR: first if is jump-if-true (is_jz=1), second is jump-if-false (is_jz=0)
            if is_jz and not t_is_jz:
                self._short_circuit.setdefault(target, []).append(('||', ni))
            # AND: first if is jump-if-false (is_jz=0), second is jump-if-true (is_jz=1)
            elif (not is_jz) and t_is_jz:
                self._short_circuit.setdefault(target, []).append(('&&', ni))
            # AND (both jnz): first if is jump-if-false (is_jz=0), second is also jump-if-false (is_jz=0)
            # Pattern: if_jnz(no_pop) -> L; pop; push B; L: if_jnz(pop) -> M
            # Semantics: if A is false, jump to L (A stays on stack, tested by second if)
            #            if A is true, fall through, pop A, push B, test B at L
            #            => if (A && B) { ... } M
            elif (not is_jz) and (not t_is_jz):
                self._short_circuit.setdefault(target, []).append(('&&', ni))
            # OR chain middle: if_jz(no_pop) -> L; pop; eval B; L: if_jz(no_pop) -> M
            # Both jump-if-true, no_pop. Multi-condition OR chain (a || b || c || ...).
            # Each middle node forwards its merged condition to the next target's
            # pending list until the terminal if_jnz(pop) emits the merged condition.
            elif is_jz and t_is_jz:
                self._short_circuit.setdefault(target, []).append(('||', ni))

        # Detect loop back-edges: goto(31) or for_step(80) with target <= current ni
        for ni, (an, p16, p8) in enumerate(insts):
            if an not in (31, 80, 33):
                continue
            target = ni + 1 + (p16 if p16 < 0x8000 else p16 - 0x10000)
            if 0 <= target <= ni:
                if an == 33:
                    # if back-edge = do-while loop. The instruction after the if
                    # is the loop exit; goto to exit should become "break".
                    exit_ni = ni + 1
                    self._do_while_heads[target] = (ni, exit_ni)
                    self._do_while_ifs[ni] = (target, exit_ni)
                    continue
                # find the exit: the if(33) or for_cond(18) at or before the loop
                # head that jumps past ni. for_cond is typically at target-1.
                exit_ni = None
                cond_ni = None
                search_start = max(0, target - 1)
                for hi in range(search_start, ni):
                    han, hp16, hp8 = insts[hi]
                    if han == 18:  # for_cond = foreach loop exit
                        ht = hi + 1 + (hp16 if hp16 < 0x8000 else hp16 - 0x10000)
                        if ht > ni:
                            exit_ni = ht
                            cond_ni = hi
                            break
                    if han == 33 and (hp8 & 1) == 0:  # if_jnz (jump if false) = loop exit
                        ht = hi + 1 + (hp16 if hp16 < 0x8000 else hp16 - 0x10000)
                        if ht > ni:
                            exit_ni = ht
                            cond_ni = hi
                            break
                # Patch 20: while(1) dead loop - no loop exit condition found
                if exit_ni is None or cond_ni is None:
                    # No exit if found - treat as while(1) dead loop.
                    # exit_ni = first non-nop instruction after back-edge
                    # (used for break detection and loop closing).
                    _exit = ni + 1
                    while _exit < len(insts) and insts[_exit][0] in (0, 29, 46):
                        _exit += 1
                    if _exit >= len(insts):
                        _exit = ni + 1
                    self._loop_heads[target] = _exit
                    self._loop_exits[ni] = (target, _exit)
                    self._while1_heads.add(target)
                    continue
                if exit_ni is not None and cond_ni is not None:
                    # Skip dead-code goto back-edges: if the instruction
                    # immediately before this goto is also a goto (or ret),
                    # the previous instruction already jumped away
                    # unconditionally, so this goto is dead code and must
                    # NOT be treated as a loop back-edge.
                    # NOTE: only goto(31) back-edges are checked. for_step(80)
                    # is a VM-generated loop step that always sits between
                    # for_cond and the loop exit, even when the loop body
                    # ends with break/continue/return (which emit goto);
                    # it must always be recognized as the loop back-edge.
                    if an == 31:
                        is_dead_code = False
                        for prev_ni in range(ni - 1, cond_ni, -1):
                            prev_an = insts[prev_ni][0]
                            if prev_an in (0, 29, 46):  # nop
                                continue
                            if prev_an in (31, 73):  # goto or ret
                                is_dead_code = True
                            break
                        if is_dead_code:
                            # The previous non-nop is an unconditional goto/ret,
                            # so the back-edge is unreachable by fall-through.
                            # But if any instruction between that goto/ret and
                            # the back-edge (inclusive) is a jump target, the
                            # back-edge is still reachable via that target
                            # (the intervening nops fall through to it), so it
                            # must NOT be treated as dead code. This handles
                            # while(1) loops whose body ends with:
                            #   if (cond) { ... goto L_cont; } else { goto L_exit; }
                            #   L_cont: nop; goto L_head;  // back-edge
                            # where the break's goto precedes the back-edge.
                            all_tgts = self.goto_targets | self.structured_targets
                            for tni in range(prev_ni + 1, ni + 1):
                                if tni in all_tgts:
                                    is_dead_code = False
                                    break
                        if is_dead_code:
                            continue
                    self._loop_heads[target] = exit_ni
                    self._loop_exits[ni] = (target, exit_ni)
                    self._loop_conds[cond_ni] = exit_ni

        # Compute the final (highest-ni) back-edge for each loop head.
        # Only the final back-edge closes the while loop; intermediate
        # back-edges are "continue" statements handled by _gen_inst.
        self._loop_close_backedges = {}
        for _goto_ni, (_head_ni, _exit_ni) in self._loop_exits.items():
            if (_head_ni not in self._loop_close_backedges
                    or _goto_ni > self._loop_close_backedges[_head_ni]):
                self._loop_close_backedges[_head_ni] = _goto_ni

        self.targets = self.goto_targets - self.structured_targets
        self.all_targets = self.structured_targets | self.goto_targets
        consumed_targets = set()
        self._consumed_gotos = set()  # gotos already emitted as break/continue
        self._catch_skip_gotos = set()  # compiler-generated gotos that skip the catch handler
        self._skip_until_if_close = False  # skip dead code after continue/break/return in if-block

        # Detect switch/case patterns (populates _switch_starts, _switch_cases,
        # _switch_ends, _switch_default)
        self._detect_switch_pattern(insts, nc)

        # Track which switch blocks we've opened, to close them at _switch_ends
        self._open_switches = []  # list of (switch_start_ni, end_ni)

        # Dead-code tracking: after ret, skip instructions until a jump target
        self._returned = False
        # Build set of all reachable targets (for resetting _returned flag)
        reachable = self.all_targets.copy()
        if hasattr(self, '_switch_cases'):
            reachable |= set(self._switch_cases.keys())

        for ni, (an, p16, p8) in enumerate(insts):
            # Reset dead-code flag at reachable jump targets
            if ni in reachable:
                self._returned = False

            # Skip dead code after continue/break/return inside an if-block.
            # The if-block closes at its target; until then, skip all instructions.
            if self._skip_until_if_close:
                # Check if this ni is the if-block close target
                if self._if_stack and ni == self._if_stack[-1][1]:
                    self._skip_until_if_close = False
                    # Fall through to close the if-block below
                else:
                    # Check if this ni is reachable from elsewhere (jump target)
                    if ni in reachable:
                        self._skip_until_if_close = False
                        # Fall through to process normally
                    else:
                        continue  # skip dead code

            # Close if/else blocks at target FIRST (inner blocks close before outer)
            while self._if_stack and ni == self._if_stack[-1][1]:
                # If this if/else block was opened BEFORE the innermost switch,
                # the switch (being inner) must close first. Break to let the
                # switch-close code below handle it, then we'll resume.
                if self._open_switches:
                    switch_start = self._open_switches[-1][0]
                    if_ni_check = self._if_stack[-1][0]
                    if if_ni_check < switch_start:
                        break
                if_ni, _, block_type = self._if_stack.pop()
                self.indent -= 1
                if block_type == "ternary":
                    # Complete ternary: pop false-branch value, push ternary expr
                    td = getattr(self, '_ternary_data', None)
                    if td is not None:
                        cond, true_val, is_jz = td
                        false_val = self._pop() if self.stack else "<?>"
                        # is_jz=0 (jump if false): true branch is the if-body
                        # is_jz=1 (jump by true): branches are swapped
                        if is_jz:
                            t, f = false_val, true_val
                        else:
                            t, f = true_val, false_val
                        self._push("(%s ? %s : %s)" % (cond, t, f))
                        if hasattr(self, '_ternary_data'):
                            del self._ternary_data
                        if hasattr(self, '_ternary_check'):
                            del self._ternary_check
                        if hasattr(self, '_ternary_else_state'):
                            del self._ternary_else_state
                        consumed_targets.add(ni)
                        continue
                if block_type == "switch" and self._case_in_switch:
                    self.lines.append("%s// --- end switch ---" % self._ind())
                    self._case_in_switch = False
                elif block_type == "if" and self.lines and self.lines[-1].rstrip().endswith(" {"):
                    # Empty if block (body had only discarded expressions)
                    # But if this if had do_pop=0 (condition was popped by the if
                    # itself), the body contains stack operations that affect
                    # later conditions - must NOT delete the block.
                    if if_ni in self._if_with_cond_pop:
                        self._if_with_cond_pop.discard(if_ni)
                        self.lines.append("%s}" % self._ind())
                    else:
                        # Remove the opening line instead of emitting a closing brace
                        self.lines.pop()
                elif block_type == "else" and self.lines and self.lines[-1].rstrip().endswith(" {"):
                    # Empty else block - remove "} else {" and preceding "if (cond) {"
                    self.lines.pop()  # remove "} else {" or "if (...) {"
                    if self.lines and self.lines[-1].rstrip().endswith(" {"):
                        self.lines.pop()  # remove the "if (cond) {" line
                else:
                    self.lines.append("%s}" % self._ind())
                consumed_targets.add(ni)

            # Close switch blocks whose end has been reached (outer blocks).
            # After closing a switch, resume closing if/else blocks whose
            # target equals ni (they may be parent blocks that wrapped the switch).
            while self._open_switches and ni >= self._open_switches[-1][1]:
                _, end_ni = self._open_switches.pop()
                self.indent -= 1
                self.lines.append("%s}  // end switch" % self._ind())
                self._case_in_switch = False
                # Resume closing if/else blocks now that the switch is closed
                while self._if_stack and ni == self._if_stack[-1][1]:
                    # Don't close if/else blocks belonging to an outer switch
                    if self._open_switches:
                        outer_start = self._open_switches[-1][0]
                        if self._if_stack[-1][0] < outer_start:
                            break
                    if_ni, _, block_type = self._if_stack.pop()
                    self.indent -= 1
                    if block_type == "ternary":
                        td = getattr(self, '_ternary_data', None)
                        if td is not None:
                            cond, true_val, is_jz = td
                            false_val = self._pop() if self.stack else "<?>"
                            if is_jz:
                                t, f = false_val, true_val
                            else:
                                t, f = true_val, false_val
                            self._push("(%s ? %s : %s)" % (cond, t, f))
                            if hasattr(self, '_ternary_data'):
                                del self._ternary_data
                            if hasattr(self, '_ternary_check'):
                                del self._ternary_check
                            if hasattr(self, '_ternary_else_state'):
                                del self._ternary_else_state
                            consumed_targets.add(ni)
                            continue
                    if block_type == "switch" and self._case_in_switch:
                        self.lines.append("%s// --- end switch ---" % self._ind())
                        self._case_in_switch = False
                    elif block_type == "if" and self.lines and self.lines[-1].rstrip().endswith(" {"):
                        if if_ni in self._if_with_cond_pop:
                            self._if_with_cond_pop.discard(if_ni)
                            self.lines.append("%s}" % self._ind())
                        else:
                            self.lines.pop()
                    elif block_type == "else" and self.lines and self.lines[-1].rstrip().endswith(" {"):
                        self.lines.pop()
                        if self.lines and self.lines[-1].rstrip().endswith(" {"):
                            self.lines.pop()
                    else:
                        self.lines.append("%s}" % self._ind())
                    consumed_targets.add(ni)

            # Close while loop at back-edge goto (only at the final back-edge
            # for this loop head; intermediate back-edges are continue statements
            # handled by _gen_inst's goto -> continue detection).
            if ni in self._loop_exits:
                head_ni, exit_ni = self._loop_exits[ni]
                if ni != self._loop_close_backedges.get(head_ni, -1):
                    # Not the final back-edge - fall through to _gen_inst
                    # for continue detection.
                    pass
                else:
                    # Check if the innermost if-block's target is a break (goto to
                    # loop exit). If so, emit it as `} else { break; }` before
                    # closing the loop, so the break stays inside the loop body.
                    if (self._if_stack and self._if_stack[-1][1] > ni
                            and self._if_stack[-1][0] >= head_ni
                            and self._if_stack[-1][1] < nc
                            and self.insts[self._if_stack[-1][1]][0] == 31):
                        if_target = self._if_stack[-1][1]
                        break_target_ni = if_target + 1 + (self.insts[if_target][1] if self.insts[if_target][1] < 0x8000 else self.insts[if_target][1] - 0x10000)
                        if break_target_ni == exit_ni:
                            # The if-target is a goto to loop exit = break
                            _, _, block_type = self._if_stack.pop()
                            self.indent -= 1
                            self.lines.append("%s} else {" % self._ind())
                            self.indent += 1
                            self.lines.append("%sbreak;" % self._ind())
                            self.indent -= 1
                            self.lines.append("%s}" % self._ind())
                            consumed_targets.add(if_target)
                            self._consumed_gotos.add(if_target)
                    # close any remaining inner if blocks first (only those opened inside the loop)
                    while self._if_stack and self._if_stack[-1][1] > ni and self._if_stack[-1][0] >= head_ni:
                        _, _, block_type = self._if_stack.pop()
                        self.indent -= 1
                        self.lines.append("%s}" % self._ind())
                    self.indent -= 1
                    self.lines.append("%s}  // end loop (verified)" % self._ind())
                    if self._while_stack:
                        self._while_stack.pop()
                    consumed_targets.add(ni)
                    continue  # skip emitting the goto

            # Close do-while loop at back-edge if. This must happen even in
            # dead code (after ret/break/continue) because the block structure
            # must be closed. Without this, do-while loops whose back-edge
            # follows a ret/break are never closed, producing "do { ... }"
            # without the matching "} while(...);".
            if ni in self._do_while_ifs:
                head_ni, exit_ni = self._do_while_ifs[ni]
                dw_is_jz = (p8 & 1) != 0
                dw_do_pop = (p8 & 2) != 0
                if dw_do_pop and self.stack:
                    cond = self._pop()
                elif self.stack:
                    cond = self.stack[-1]
                else:
                    # Dead code: stack is empty because the preceding push
                    # was also skipped. Extract the literal from the
                    # previous instruction if it's a push.
                    cond = "1"
                    if ni > 0:
                        prev_an, prev_p16, prev_p8 = insts[ni - 1]
                        if prev_an == 59:  # push_i
                            cond = str(prev_p16)
                        elif prev_an == 60:  # push_num
                            cond = str(prev_p16 | (prev_p8 << 16))
                        elif prev_an == 66:  # push_0 (undefined)
                            cond = "0"
                if dw_is_jz:
                    while_cond = cond
                else:
                    while_cond = "!(%s)" % cond
                # Close any inner if blocks first (only those opened inside the do-while)
                while self._if_stack and self._if_stack[-1][1] > ni and self._if_stack[-1][0] >= head_ni:
                    _, _, block_type = self._if_stack.pop()
                    self.indent -= 1
                    self.lines.append("%s}" % self._ind())
                self.indent -= 1
                self.lines.append("%s} while (%s);" % (self._ind(), self._simplify_expr(while_cond)))
                if self._while_stack:
                    self._while_stack.pop()
                self._returned = False  # do-while doesn't end function
                consumed_targets.add(ni)
                continue

            # Skip dead code after ret (but still close blocks above)
            if self._returned:
                continue

            # Open do-while loop at the loop head (backward if target)
            if ni in self._do_while_heads:
                # Patch 21: if this head is also a while(1) head, emit
                # while(1) first (outer loop wraps the do-while). This
                # happens when two back-edges target the same head: an
                # if back-edge (do-while(0) breakable block) and a goto
                # back-edge (while(1) dead loop).
                if ni in self._while1_heads:
                    w1_exit = self._loop_heads[ni]
                    self.lines.append("%swhile (1) {" % self._ind())
                    self.indent += 1
                    self._while_stack.append((ni, w1_exit))
                    consumed_targets.add(ni)
                if_ni, exit_ni = self._do_while_heads[ni]
                self.lines.append("%sdo {" % self._ind())
                self.indent += 1
                self._while_stack.append((if_ni, exit_ni))

            # Patch 20: Open while(1) dead loop at the loop head
            # (back-edge goto with no loop exit condition).
            elif ni in self._while1_heads:
                exit_ni = self._loop_heads[ni]
                self.lines.append("%swhile (1) {" % self._ind())
                self.indent += 1
                self._while_stack.append((ni, exit_ni))
                consumed_targets.add(ni)  # suppress label

            if ni in self.targets and ni not in consumed_targets:
                self.lines.append("\n%s// --- label L%04d ---" % (self._ind(), ni))

            # Emit case labels if this ni is a switch body target
            if hasattr(self, '_switch_cases') and ni in self._switch_cases:
                # Close if/else blocks from previous case body whose target
                # extends beyond this case label. Without this, case labels
                # get incorrectly nested inside the previous case's else branch
                # (the else target typically points to loop exit / switch end,
                #  which is far beyond the next case body).
                # Only close blocks opened INSIDE the switch (if_ni >= switch_start)
                # to avoid closing the switch's own parent if/else block.
                if self._open_switches:
                    switch_start = self._open_switches[-1][0]
                    while self._if_stack and self._if_stack[-1][1] > ni:
                        if_ni, _, _ = self._if_stack[-1]
                        if if_ni < switch_start:
                            break  # outside switch, don't close
                        _, _, block_type = self._if_stack.pop()
                        self.indent -= 1
                        self.lines.append("%s}" % self._ind())
                for kind, val in self._switch_cases[ni]:
                    if kind == 'default':
                        self.lines.append("%sdefault:" % self._ind())
                    elif kind == 'num':
                        self.lines.append("%scase %s:" % (self._ind(), val))
                    elif kind == 'str':
                        self.lines.append("%scase \"%s\":" % (self._ind(), val))
                self._case_in_switch = True

            if ni not in self._prologue_pushes:
                self._gen_inst(ni, an, p16, p8, nc)

        # Close any remaining open switches
        while self._open_switches:
            _, end_ni = self._open_switches.pop()
            self.indent -= 1
            self.lines.append("%s}  // end switch" % self._ind())

        while self._if_stack:
            _, _, block_type = self._if_stack.pop()
            self.indent -= 1
            self.lines.append("%s}" % self._ind())

        # Safety net: close any do-while loops whose back-edge was never
        # reached (e.g. the function ends inside the loop body).
        while self._while_stack:
            self._while_stack.pop()
            self.indent -= 1
            self.lines.append("%s} while (1);" % self._ind())

        # Infer local variable types from generated pseudocode context
        self.var_types = self._infer_var_types()

        return self.lines

    def _read_meta_int(self, byte_offset):
        """Read a 4-byte little-endian int32 from metaData at byte offset."""
        if byte_offset + 4 > len(self.metadata):
            return 0
        return struct.unpack_from('<i', self.metadata, byte_offset)[0]

    def _detect_switch_pattern(self, insts, nc):
        """Detect switch(expr) { case X: ... case Y: ... default: ... } patterns.

        For case_num (an=10) and case_str (an=11): these are complex instructions
        (attrib 0x80000001, parameters=2). The metaIdx = p16 | (p8<<16) is a BYTE
        offset into the per-function metaData. metaData[metaIdx]=jump_offset,
        metaData[metaIdx+4]=case_value (int for case_num, string_idx for case_str).

        For default (an=12): non-complex instruction (parameters=1, attrib 0x01).
        p16 is the direct jump offset.

        IDA references:
          vm_caseCheckNumber @ 0x80974c0: if stack_top==number, pop+jump ip+=offset
          vm_caseCheckString @ 0x80974fc: if stack_top==string, pop+jump ip+=offset
          vm_caseDefault @ 0x8097584: pop+jump ip+=offset
        """
        self._switch_starts = {}      # first_case_ni -> True (emit switch header)
        self._switch_cases = {}       # body_ni -> list of (kind, value_str) labels
        self._switch_ends = {}        # body_ni -> switch_end_ni (for closing brace)
        self._switch_fallthru = {}    # body_ni -> fallthru_ni (break target inside switch)
        self._switch_default = {}     # body_ni -> True (is default label)
        # Find runs of case_num/case_str/default instructions
        case_runs = []
        run_start = None
        run_end = None
        for ni, (an, p16, p8) in enumerate(insts):
            if an in (10, 11, 12):  # case_num, case_str, default
                if run_start is None:
                    run_start = ni
                run_end = ni
            else:
                if run_start is not None and run_end is not None:
                    if run_end - run_start >= 1:  # at least 2 cases
                        case_runs.append((run_start, run_end))
                    run_start = None
                    run_end = None
        if run_start is not None and run_end is not None:
            if run_end - run_start >= 1:
                case_runs.append((run_start, run_end))

        # For each case run, decode all cases and build label->body mapping
        for run_start, run_end in case_runs:
            self._switch_starts[run_start] = True
            # Collect all case labels and their body jump targets
            all_targets = []  # list of (body_ni, kind, value_str)
            for ni in range(run_start, run_end + 1):
                an, p16, p8 = insts[ni]
                if an == 10:  # case_num (complex, params=2)
                    meta_idx = p16 | (p8 << 16)
                    offset = self._read_meta_int(meta_idx)
                    value = self._read_meta_int(meta_idx + 4)
                    body_ni = ni + 1 + offset
                    all_targets.append((body_ni, 'num', str(value)))
                elif an == 11:  # case_str (complex, params=2)
                    meta_idx = p16 | (p8 << 16)
                    offset = self._read_meta_int(meta_idx)
                    str_idx = self._read_meta_int(meta_idx + 4)
                    body_ni = ni + 1 + offset
                    # str_idx is 0-based string table index (same as push_s p16)
                    s = "STR#%d" % str_idx
                    if 0 <= str_idx < len(self.strings):
                        s = self.strings[str_idx]
                        if len(s) > 30:
                            s = s[:30] + "...<trunc>"
                    all_targets.append((body_ni, 'str', s))
                elif an == 12:  # default (non-complex, p16=offset)
                    offset = p16 if p16 < 0x8000 else p16 - 0x10000
                    body_ni = ni + 1 + offset
                    all_targets.append((body_ni, 'default', None))

            # Group cases by body_ni (cases sharing same body)
            for body_ni, kind, val in all_targets:
                if body_ni not in self._switch_cases:
                    self._switch_cases[body_ni] = []
                self._switch_cases[body_ni].append((kind, val))
                if kind == 'default':
                    self._switch_default[body_ni] = True

            # Determine switch end: the last body_ni + its body length.
            # Approximate: switch ends at the next ret/goto after the last body,
            # or at the instruction after the run's default body.
            # For simplicity, mark switch_end as the max body_ni (bodies usually
            # end with ret/break which doesn't fall through).
            if all_targets:
                max_body = max(t[0] for t in all_targets)
                # Find end: scan from max_body for ret/goto/break/for_step
                # for_step (an=80) is a loop back-edge; switch must close
                # BEFORE it so the loop closes outside the switch.
                # Skip over if-blocks (an=33) so that gotos inside an if-body
                # are not mistaken for the switch body's terminating goto.
                end_ni = max_body
                ni = max_body
                while ni < nc:
                    an = insts[ni][0]
                    if an == 80:  # for_step (loop back-edge)
                        end_ni = ni  # switch ends at for_step
                        break
                    if an in (73, 72):  # ret, ret_fake
                        end_ni = ni + 1
                        break
                    if an == 31:  # goto (switch-body level, not inside an if)
                        end_ni = ni + 1
                        break
                    if an == 33:  # if_jnz: skip the entire if-block body
                        p16 = insts[ni][1]
                        if_target = ni + 1 + (p16 if p16 < 0x8000 else p16 - 0x10000)
                        # Guard against backward jumps (do-while back-edge):
                        # if_target must be forward to make progress.
                        if if_target > ni:
                            ni = if_target
                        else:
                            ni += 1
                        continue
                    ni += 1
                self._switch_ends[run_start] = end_ni
                # Track the "fallthrough" target: the common goto target that
                # case bodies jump to in order to exit the switch. This is the
                # most frequent goto target among case bodies that points to
                # code at or after the last case body but before end_ni.
                # Collect goto targets from within the switch body.
                from collections import Counter
                goto_counter = Counter()
                body_targets = set(t[0] for t in all_targets)
                for bni in sorted(body_targets):
                    # Scan this case body for top-level gotos (not inside if-blocks)
                    # until next case body or end
                    next_body = min((t for t in body_targets if t > bni), default=end_ni)
                    gni = bni
                    while gni < min(next_body, end_ni):
                        gan, gp16, gp8 = insts[gni]
                        if gan == 31:  # goto at top level of case body
                            gt = gni + 1 + (gp16 if gp16 < 0x8000 else gp16 - 0x10000)
                            # Allow targets at end_ni or end_ni+1 (switch break),
                            # or before end_ni (fallthrough to switch-end code).
                            # Only count gotos that jump to or past the NEXT case
                            # body start (gt >= next_body). Gotos within the same
                            # case body (internal jumps, nested switch breaks)
                            # must NOT be counted as outer switch breaks.
                            if gt != end_ni and gt not in body_targets and gt <= end_ni + 1 and gt >= next_body:
                                goto_counter[gt] += 1
                            gni += 1
                        elif gan == 33:  # if - skip the if-block body
                            if_target = gni + 1 + (gp16 if gp16 < 0x8000 else gp16 - 0x10000)
                            # Guard against backward jumps (do-while back-edge)
                            if if_target > gni:
                                gni = if_target
                            else:
                                gni += 1
                        else:
                            gni += 1
                if goto_counter:
                    # Most common fallthrough target = switch break point
                    self._switch_fallthru[run_start] = goto_counter.most_common(1)[0][0]
                    # Cap switch end at the fallthru (break target). The scan
                    # from max_body may extend past the switch into post-switch
                    # code when the default body is empty (just nops with no
                    # goto/ret to stop the scan). The fallthru - where all break
                    # gotos jump to - IS the switch end.
                    _fallthru_ni = self._switch_fallthru[run_start]
                    if _fallthru_ni < self._switch_ends[run_start]:
                        self._switch_ends[run_start] = _fallthru_ni

    def _gen_inst(self, ni, an, p16, p8, nc):
        def jt():
            return ni + 1 + (p16 if p16 < 0x8000 else p16 - 0x10000)

        # Skip nop, enter, checkargs
        if an in (0, 15, 27, 46, 29):
            return

        # ---- Push operations ----
        if an == 59:     # push_i (16-bit signed immediate, non-complex)
            v = p16 - 0x10000 if p16 >= 0x8000 else p16
            self._push(str(v));
            return
        if an == 60:     # push_num (complex: p16|p8<<16 = meta byte offset)
            meta_idx = p16 | (p8 << 16)
            v = self._read_meta_int(meta_idx)
            self._push(str(v)); return
        if an == 63:     # push_real (complex: p16|p8<<16 = meta byte offset)
            meta_idx = p16 | (p8 << 16)
            raw = self._read_meta_int(meta_idx)
            fv = struct.unpack('<f', struct.pack('<i', raw))[0]
            # Strip trailing zeros for readability
            s = ("%.6f" % fv).rstrip('0').rstrip('.')
            if s == "" or s == "-" or s == "-0":
                s = "0"
            self._push(s); return
        if an == 66:     # push_0
            self._push("0"); return
        if an == 65:     # push_this
            self._push("this_object()"); return
        if an == 52:     # push_argnum
            self._push("argc"); return
        if an == 55:     # push_local
            self._push(self._arg_name(p16)); return
        if an == 56:     # push_local_i
            self._push(self._arg_name(0)); return
        if an == 57:     # push_local_ptr
            if ni in self._sc_assign_pending:
                is_jz, left_cond = self._sc_assign_pending.pop(ni)
                right_val = self._pop() if self.stack else "<?>"
                op = "||" if is_jz else "&&"
                merged = self._simplify_expr("(%s %s %s)" % (left_cond, op, right_val))
                self._push(merged)
            self._push("&" + self._arg_name(p16)); return
        if an == 58:     # push_local_ptr_i
            self._push("&" + self._arg_name(0)); return
        if an == 53:     # push_global
            self._push(self._global_var_name(p16)); return
        if an == 54:     # push_global_ptr
            if ni in self._sc_assign_pending:
                is_jz, left_cond = self._sc_assign_pending.pop(ni)
                right_val = self._pop() if self.stack else "<?>"
                op = "||" if is_jz else "&&"
                merged = self._simplify_expr("(%s %s %s)" % (left_cond, op, right_val))
                self._push(merged)
            self._push("&" + self._global_var_name(p16)); return
        if an == 61:     # push_ovar
            self._push(self.var_names.ovar_name(p16)); return
        if an == 62:     # push_ovar_ptr
            if ni in self._sc_assign_pending:
                is_jz, left_cond = self._sc_assign_pending.pop(ni)
                right_val = self._pop() if self.stack else "<?>"
                op = "||" if is_jz else "&&"
                merged = self._simplify_expr("(%s %s %s)" % (left_cond, op, right_val))
                self._push(merged)
            self._push("&" + self.var_names.ovar_name(p16)); return
        if an == 64:     # push_s
            # bytecode p16 is 0-based index into the .o string table.
            # (vm_mapStrNoTo performs identity mapping when pStrOffsetMapping
            # is NULL, i.e. for a freshly loaded program; the .o str_table
            # preserves file order, so p16 indexes it directly.)
            idx = p16
            if 0 <= idx < len(self.strings):
                s = self.strings[idx].replace('\n','\\n').replace('\r','\\r').replace('\x00','')
                s = s.replace('"', '\\"')
                self._push('"%s"' % s)
            else:
                self._push('STR#%d' % p16)
            return
        if an == 63:     # push_real
            v = struct.unpack('<f', struct.pack('<HH', p16, p8))[0]
            self._push("%.4f" % v); return

        # ---- Arithmetic (2 args -> 1 result) ----
        bin_ops = {1: '+', 81: '-', 41: '*', 25: '/', 40: '%',
                   2: '&', 48: '|', 85: '^', 38: '<<', 75: '>>'}
        if an in bin_ops:
            b = self._pop(); a = self._pop()
            self._push("(%s %s %s)" % (a, bin_ops[an], b)); return

        # ---- Comparison (2 args -> 1 bool) ----
        cmp_ops = {28: '==', 42: '!=', 32: '>', 30: '>=', 39: '<', 37: '<='}
        if an in cmp_ops:
            b = self._pop(); a = self._pop()
            self._push("(%s %s %s)" % (a, cmp_ops[an], b)); return

        # ---- Unary ----
        if an == 43:  # neg
            a = self._pop(); self._push("(-%s)" % a); return
        if an == 45:  # not
            a = self._pop()
            # Merge protective null-check guard: if a do_pop=0 if targeted
            # this not instruction, combine guard && operand before negating.
            # Pattern: is_object(x); if_jnz(no_pop)->L_not; pop; eval; L_not: not
            # => !(is_object(x) && eval)  (compiler expansion of negated guard)
            if ni in self._guard_pending:
                gcond = self._guard_pending.pop(ni)
                a = "(%s && %s)" % (gcond, a)
            self._push("(!%s)" % a); return
        if an == 74:  # rev
            a = self._pop(); self._push("(~%s)" % a); return

        # ---- Assignment ----
        if an == 3:  # assign
            # Bytecode push order: val, ptr (val first, ptr last on top)
            # Pop order: ptr (top), then val (below)
            ptr = self._pop(); val = self._pop()
            var = ptr.lstrip('&') if ptr.startswith('&') else ptr
            # Patch 8: assign-as-expression. The compiler emits pop
            # right after assign when the value is discarded (statement).
            # When the next non-nop instruction consumes the value, keep
            # the assignment inline: strlen(btime = ctime(btime, 3)),
            # is_string(account = ob->query(..)), !(x = f()).
            # Call/wrap consumers embed the value as a bare argument;
            # binop-operand contexts need parentheses for precedence.
            _ins = getattr(self, 'insts', None)
            _k = ni + 1
            while _ins is not None and _k < len(_ins) and _ins[_k][0] in (0, 29, 46):
                _k += 1
            if (_ins is not None and _k < len(_ins)
                    and _ins[_k][0] not in (50, 51)):
                if _ins[_k][0] in (5, 6, 7, 8, 36, 45):
                    self._push("%s = %s" % (var, val))
                else:
                    self._push("(%s = %s)" % (var, val))
                return
            self.lines.append("%s%s = %s;" % (self._ind(), var, val))
            self._push(var); return

        if an == 4:  # assign_rev (reverse assignment: ptr = val, but val pushed last)
            # Reverse push order: ptr, val (ptr first, val last on top)
            # Pop order: val (top), then ptr (below)
            val = self._pop(); ptr = self._pop()
            var = ptr.lstrip('&') if ptr.startswith('&') else ptr
            self.lines.append("%s%s = %s;" % (self._ind(), var, val))
            self._push(var); return

        if an == 47:  # op_eq (compound assignment)
            ops = {2: "+=", 3: "-=", 4: "*=", 5: "/=", 6: "%=",
                   7: "|=", 8: "^=", 9: ">>=", 10: "<<="}
            op = ops.get(p16, "?=")
            # Bytecode push order: ptr, val (ptr first, val last on top)
            # Pop order: val (top), then ptr (below)
            val = self._pop(); ptr = self._pop()
            var = ptr.lstrip('&') if ptr.startswith('&') else ptr
            self.lines.append("%s%s %s %s;" % (self._ind(), var, op, val))
            self._push(var); return

        if an == 34:  # inc
            # vm_incValue reads the target pointer from the stack top and uses
            # p16 as the increment amount (verified via IDA @ 0x808DEA4).
            ptr = self._pop() if self.stack else "&<?>"
            var = ptr[1:] if ptr.startswith('&') else ptr
            amt = p16 if p16 < 0x8000 else p16 - 0x10000
            # Postfix pattern: push_ptr(X) + to_value leaves X's old value
            # on the stack; an immediately following push_ptr(X) + inc is
            # the compiler's encoding of X++ / X-- (matrix_list[i++] = ...).
            # Replace the pending old value with the postfix expression
            # instead of emitting a standalone prefix statement.
            if amt in (1, -1):
                _ins = getattr(self, 'insts', None)
                _p = ni - 1
                while _ins is not None and _p >= 0 and _ins[_p][0] in (0, 29, 46):
                    _p -= 1
                if (_ins is not None and _p >= 0 and _ins[_p][0] in (54, 57, 58, 62)
                        and self.stack and self.stack[-1] == var):
                    self.stack[-1] = "%s%s" % (var, "++" if amt == 1 else "--")
                    return
            if amt == 1:
                self.lines.append("%s++%s;%s" % (self._ind(), var, "" if self.suppress_trailing_comments else "  // increment"))
            elif amt == -1:
                self.lines.append("%s--%s;%s" % (self._ind(), var, "" if self.suppress_trailing_comments else "  // decrement"))
            else:
                self.lines.append("%s%s += %d;" % (self._ind(), var, amt))
            return

        # ---- Pop ----
        if an == 51:  # pop
            # Skip the pop that follows a do_pop=0 if (condition already consumed)
            if self._pending_if_pop:
                self._pending_if_pop = False
                return
            # Patch 37: removed _sc_left dead code block - the _sc_left attribute
            # is never assigned anywhere in the file (only _sc_pending is used),
            # so hasattr(self, '_sc_left') was always False and this branch never
            # triggered. Removed to avoid confusion; the short-circuit pop-skip is
            # handled via _pending_if_pop above.
            if self.stack:
                val = self._pop()
                if val and self._has_side_effect(val):
                    # Strip "(expr != 0)" / "(expr == 0)" wrapper when the
                    # comparison result is discarded: the compiler emits
                    # call_result != 0 to convert to bool, but the call's
                    # side effect is all that matters.
                    m = re.match(r'^\((.+) (?:!=|==) 0\)$', val)
                    if m and self._has_side_effect(m.group(1)):
                        val = m.group(1)
                    # Patch 30: mapping merge (m1 << m2) - strip outer
                    # parens and use dedicated comment.
                    mm = re.match(r'^\((.+) << (.+)\)$', val)
                    if mm:
                        self.lines.append("%s%s << %s;%s" % (self._ind(), mm.group(1), mm.group(2), "" if self.suppress_trailing_comments else "  // mapping merge"))
                    else:
                        self.lines.append("%s%s;%s" % (self._ind(), val, "" if self.suppress_trailing_comments else "  // call (discard result)"))
            return
        if an == 50:  # pop_n
            for _ in range(p16):
                if self.stack:
                    val = self._pop()
                    if val and self._has_side_effect(val):
                        # Patch 30: mapping merge (m1 << m2) - strip outer
                        # parens and use dedicated comment.
                        mm = re.match(r'^\((.+) << (.+)\)$', val)
                        if mm:
                            self.lines.append("%s%s << %s;%s" % (self._ind(), mm.group(1), mm.group(2), "" if self.suppress_trailing_comments else "  // mapping merge"))
                        else:
                            self.lines.append("%s%s;%s" % (self._ind(), val, "" if self.suppress_trailing_comments else "  // call (discard result)"))
            return

        # ---- Return ----
        if an == 73:  # ret
            # Skip ret that is the exception path of a suppressed
            # catch-check if. This ret is only reached when an exception
            # was caught; normal flow falls through to L_normal.
            if ni in self._catch_ret_skip:
                self._catch_ret_skip.discard(ni)
                self._returned = False  # don't mark as returned
                self.stack.clear()
                return
            # Check if this ret is the target of a short-circuit pattern
            # (A; if !A goto L; pop; B; L: ret => return A && B)
            if ni in self._short_circuit:
                right = self._pop() if self.stack else "<?>"
                pending = self._sc_pending.pop(ni, [])
                val = right
                for sc_op, sc_left in reversed(pending):
                    val = "(%s %s %s)" % (sc_left, sc_op, val)
                if pending:
                    val = self._simplify_expr(val)
                self.lines.append("%sreturn %s;" % (self._ind(), val))
                self._returned = True
                self.stack.clear()
                return
            if self.stack:
                val = self._pop()
                self.lines.append("%sreturn %s;" % (self._ind(), val))
                self._returned = True
            elif self._ret_type == 'void' and self.indent <= 1:
                # Check if this is a placeholder ret at function start
                # (followed by real code that is not the function end).
                # If so, emit return; but do NOT set _returned, so the
                # following code is still decompiled.
                _has_following_code = False
                try:
                    _fwd = ni + 1
                    while _fwd < len(self.insts):
                        _fa = self.insts[_fwd][0]
                        if _fa in (0, 29, 46):  # nop
                            _fwd += 1
                            continue
                        if _fa == 73 or _fa == 72:  # ret / ret_fake
                            break  # another ret, no real code after
                        _has_following_code = True
                        break
                except Exception:
                    pass
                if _has_following_code and not self._prev_real_line_is_return():
                    self.lines.append("%sreturn;" % self._ind())
                    # do NOT set _returned; following code is reachable
                else:
                    pass  # suppress trailing return; in void functions
                    self._returned = True
            elif not self._prev_real_line_is_return():
                # suppress dead-code empty return after another return
                self.lines.append("%sreturn;" % self._ind())
                self._returned = True
            else:
                # dead return suppressed; remove preceding label comment if any
                self._cleanup_trailing_label()
                self._returned = True
            self.stack.clear()
            return
        if an == 72:  # ret_fake
            if self._ret_type == 'void' and self.indent <= 1:
                pass  # suppress trailing return; in void functions
            elif not self._prev_real_line_is_return():
                self.lines.append("%sreturn;" % self._ind())
            else:
                self._cleanup_trailing_label()
            self._returned = True
            self.stack.clear()
            return

        # ---- Function calls ----
        if an == 6:  # call_efun
            nargs = p8
            args = [self._pop() for _ in range(nargs)]
            fname = efun_name(p16)
            call_str = "%s(%s)" % (fname, ", ".join(args))
            self._push(call_str); return

        if an == 7:  # call_user
            nargs = p8 & 0x3F
            is_global = (p8 >> 7) & 1
            args = [self._pop() for _ in range(nargs)]
            # Patch 39: per IDA vm_callInUserFun @ 0x08094080, is_global=1 means
            # vm_pGlobalProgram->pFunctions[v6] (global program function table),
            # NOT efun/sefun. For start_gs.o (the global program), these are in
            # the file's own func_table. Check func_table FIRST when is_global=1.
            if is_global:
                if p16 in self.func_table:
                    fname = self.func_table[p16]
                else:
                    fname = EFUN_TABLE.get(p16) or SEFUN_TABLE.get(p16, "global_fn_%d" % p16)
            elif p16 in self.func_table:
                fname = self.func_table[p16]
                # Patch 9 (fixes Patch 17): inherited-slot call_user.
                # Overridden func (own same-name slot exists): a plain
                # call compiles to the OWN slot, so the inherited-slot
                # reference encodes ::func(). The runtime has no
                # "path"::fname syntax. Not overridden: the inherited
                # slot is the function's only slot -> plain call.
                i2p = getattr(self, '_inherit_idx_to_path', {})
                if p16 in i2p:
                    if any(v == fname and k not in i2p
                           for k, v in self.func_table.items()):
                        fname = '::%s' % fname
            elif p16 in getattr(self, '_inherit_idx_to_path', {}):
                # Parent .o loaded but function name unknown (e.g. parent's
                # own inherited funcs). Reference with parent path.
                parent_path = self._inherit_idx_to_path[p16]
                if not self.suppress_parent_path:
                    fname = '"%s"::local_fn_%d' % (parent_path, p16)
                else:
                    fname = 'local_fn_%d' % p16
            elif getattr(self, '_inherit_ranges', None):
                # Patch 24: p16 not in func_table - find which parent's idx
                # range covers p16 (handles parent's own inherited functions
                # that aren't merged into this child's func_table).
                matched_path = None
                for start, end, iname in self._inherit_ranges:
                    if start <= p16 <= end:
                        matched_path = iname
                        break
                if matched_path:
                    if not self.suppress_parent_path:
                        fname = '"%s"::local_fn_%d' % (matched_path, p16)
                    else:
                        fname = 'local_fn_%d' % p16
                else:
                    fname = "local_fn_%d" % p16
            else:
                fname = "local_fn_%d" % p16
            self._push("%s(%s)" % (fname, ", ".join(args))); return

        if an == 8:  # call_other - obj->func(args)
            # Stack layout (verified via IDA vm_callInOtherFun @ 0x8094cd8):
            #   vm_stack[sp]   = obj (top, last pushed)
            #   vm_stack[sp+1] = funcname (below, pushed before obj)
            #   vm_stack[sp+2..] = args (pushed first)
            # Bytecode push order: args..., funcname, obj
            # Pop order: obj (top), funcname, args
            nargs = p8
            obj = self._pop()
            fn_val = self._pop()
            args = [self._pop() for _ in range(nargs)]
            if isinstance(fn_val, str) and fn_val.startswith('"'):
                fname = fn_val.strip('"')
            else:
                fname = str(fn_val)
            # Wrap obj in parentheses when it is a complex expression
            # (array subscript, member access, call result, binary op, etc.)
            # so the '->' operator binds to the whole object expression.
            # Simple identifiers / string literals / this_object() need no parens.
            if _needs_paren_for_member(obj):
                obj = "(%s)" % obj
            self._push("%s->%s(%s)" % (obj, fname, ", ".join(args))); return

        if an == 5:  # call_ptr
            nargs = p8
            args = [self._pop() for _ in range(nargs)]
            funcptr = args[0] if args else "<?>"
            call_args = args[1:]
            # Patch 27: Use (*funcptr)(args) uniformly. LPC supports (*expr)(args)
            # for arbitrary expressions (verified by user), so no need to distinguish
            # simple vs complex, nor allocate temp vars. Supersedes Patch 25/26.
            result = "(*%s)(%s)" % (funcptr, ", ".join(call_args))
            self._push(result); return

        if an == 9:  # call_os
            nargs = p8
            args = [self._pop() for _ in range(nargs)]
            self._push("os_%d(%s)" % (p16, ", ".join(args))); return

        # ---- Array/Mapping creation ----
        if an == 22:  # new_array
            if p16 == 0:
                self._push("({})")
            else:
                # Patch 42: removed args.reverse() and fixed comment.
                # Verified via IDA vm_createArray @ 0x80918c8: compiler pushes
                # elements in REVERSE source order (last element pushed first),
                # so stack top = first source element. VM reads
                # vm_stack[v4+vm_sp] with v4 from 0, so array[0]=stack top.
                # Popping yields the CORRECT source order; reverse was wrong.
                args = [self._pop() for _ in range(p16)]
                self._push("({ %s })" % ", ".join(args))
            return
        if an == 24:  # new_mapping
            if p16 == 0:
                self._push("([ ])")
            else:
                # VM push order is: value first, key last (per vm_createMapping:
                # vm_stack[v6] is key, vm_stack[v6+1] is value, v6 starts at
                # stack top and grows toward stack bottom). After reverse,
                # args[0] is the first pushed = value, args[1] = key, etc.
                # So pair is (args[j+1]=key : args[j]=value).
                args = [self._pop() for _ in range(p16)][::-1]
                pairs = []
                for j in range(0, len(args), 2):
                    if j + 1 < len(args):
                        pairs.append("%s: %s" % (args[j+1], args[j]))
                    else:
                        pairs.append(args[j])
                self._push("([ %s ])" % ", ".join(pairs))
            return

        # ---- Member access (IDA: vm_queryMember @ 0x808fa60) ----
        # p16 flag: bit0=idx negative (from end, i.e. length - idx)
        if an == 67:  # member - obj[idx]
            idx = self._pop(); obj = self._pop()
            if p16 & 1:
                idx = "<%s" % idx  # from_end
            # Wrap $< (function args) in parentheses for subscript
            if obj == "$<":
                obj = "($<)"
            self._push("%s[%s]" % (obj, idx)); return
        if an == 68:  # member_ptr - &obj[idx]
            idx = self._pop(); obj = self._pop()
            if p16 & 1:
                idx = "<%s" % idx  # from_end
            # When obj already starts with & (nested member_ptr), don't add
            # another &. The & applies to the entire subscript expression:
            # &_cost["count"] + [expr] = &_cost["count"][expr]
            if obj.startswith('&'):
                self._push("%s[%s]" % (obj, idx))
            else:
                self._push("&%s[%s]" % (obj, idx))
            return

        # ---- Cast ----
        if an == 13:
            t = {2: "int", 3: "real", 4: "string", 5: "buffer",
                 6: "array", 7: "mapping"}
            a = self._pop()
            self._push("(%s)%s" % (t.get(p16, "T%d" % p16), a)); return

        # ---- Typeof ----
        if an == 36:
            is_fn = IS_TYPE_MAP.get(p16)
            if is_fn and self.stack:
                self.stack[-1] = "%s(%s)" % (is_fn, self.stack[-1])
            elif self.stack:
                tn = TYPE_CODES.get(p16, "T%d" % p16)
                self.stack[-1] = "(typeof(%s) == %s)" % (self.stack[-1], tn)
            return

        # ---- Sizeof ----
        if an == 79:
            a = self._pop()
            self._push("sizeof(%s)" % a); return

        # ---- Control flow: IF ----
        if an == 33:  # if
            is_jz = (p8 & 1) != 0
            do_pop = (p8 & 2) != 0

            # Do-while loop back-edge: emit "} while(cond);"
            # vm_if semantics: is_jz=1 => jump if true (non-zero);
            #                   is_jz=0 => jump if false (zero)
            # do { body } while(cond) means: loop back when cond is true
            #   => is_jz=1 => while(cond);  is_jz=0 => while(!(cond));
            if ni in self._do_while_ifs:
                head_ni, exit_ni = self._do_while_ifs[ni]
                if do_pop:
                    cond = self._pop()
                else:
                    cond = self.stack[-1] if self.stack else "TOS"
                if is_jz:
                    while_cond = cond
                else:
                    while_cond = "!(%s)" % cond
                # Close any inner if blocks first (only those opened inside the do-while)
                while self._if_stack and self._if_stack[-1][1] > ni and self._if_stack[-1][0] >= head_ni:
                    _, _, block_type = self._if_stack.pop()
                    self.indent -= 1
                    self.lines.append("%s}" % self._ind())
                self.indent -= 1
                self.lines.append("%s} while (%s);" % (self._ind(), self._simplify_expr(while_cond)))
                if self._while_stack:
                    self._while_stack.pop()
                self._returned = False  # do-while doesn't end function
                return

            # Loop condition: this if is a while-loop exit test
            if ni in self._loop_conds:
                exit_ni = self._loop_conds[ni]
                if do_pop:
                    cond = self._pop()
                else:
                    cond = self.stack[-1] if self.stack else "TOS"
                # If this loop condition is also a short-circuit terminal if,
                # merge the saved left conditions (e.g. A && B where A was
                # stored by the first-if and B is the right side on stack now).
                if ni in self._short_circuit:
                    pending = self._sc_pending.pop(ni, [])
                    for sc_op, sc_left in reversed(pending):
                        cond = "(%s %s %s)" % (sc_left, sc_op, cond)
                    if pending:
                        cond = self._simplify_expr(cond)
                self.lines.append("%swhile (%s) {" % (self._ind(), self._simplify_expr(cond)))
                self.indent += 1
                self._while_stack.append((ni, exit_ni))
                return

            # Short-circuit OR/AND: this is the SECOND if of the pattern.
            # Collect all first-if conditions that point to this ni (stored in
            # _sc_pending[ni]) and merge them right-associatively with this
            # if's right condition. Supports multi-segment chains where
            # multiple first-ifs point to the same terminal (e.g. A||B||(C&&(D||E))).
            if ni in self._short_circuit:
                if do_pop:
                    right = self._pop()
                else:
                    # do_pop=0: condition value stays on stack. Pop it now
                    # and set flag to skip the following pop instruction.
                    right = self._pop() if self.stack else "TOS"
                    self._pending_if_pop = True
                pending = self._sc_pending.pop(ni, [])
                cond = right
                for sc_op, sc_left in reversed(pending):
                    cond = "(%s %s %s)" % (sc_left, sc_op, cond)
                if pending:
                    cond = self._simplify_expr(cond)
                # Check if this ni is also the first-if of another short-circuit
                # pattern (middle node in a multi-condition chain). If so,
                # forward the merged condition to the next target's pending list
                # and don't emit the if yet.
                is_middle = False
                for sc_target, entries in self._short_circuit.items():
                    for (sc_op, sc_first) in entries:
                        if sc_first == ni:
                            self._sc_pending.setdefault(sc_target, []).append((sc_op, cond))
                            is_middle = True
                            break
                    if is_middle:
                        break
                if is_middle:
                    return
                target = jt()
                # is_jz=0 (jump if false) => if (cond) { ... } target:
                # is_jz=1 (jump if true) => if (!(cond)) { ... } target:
                prefix = "} else " if self._pending_else_if else ""
                if self._pending_else_if:
                    self._pending_else_if = False
                if not is_jz:
                    self.lines.append("%s%sif (%s) {" % (self._ind(), prefix, self._simplify_expr(cond)))
                else:
                    self.lines.append("%s%sif (!(%s)) {" % (self._ind(), prefix, self._simplify_expr(cond)))
                self.indent += 1
                self._if_stack.append((ni, target, "if"))
                return

            # Short-circuit OR/AND: this is the FIRST if of the pattern.
            # Save the left condition to _sc_pending[target] (the terminal
            # second-if will merge it). The condition is popped now and the
            # following pop instruction is skipped via _pending_if_pop.
            if hasattr(self, 'insts'):
                # check if this ni is the first-if of any short-circuit pattern
                for sc_target, entries in self._short_circuit.items():
                    for (sc_op, sc_first) in entries:
                        if sc_first == ni:
                            if do_pop:
                                cond = self._pop()
                            else:
                                cond = self._pop() if self.stack else "TOS"
                                self._pending_if_pop = True
                            self._sc_pending.setdefault(sc_target, []).append((sc_op, cond))
                            return  # skip emitting if; the following pop is skipped too

            if do_pop:
                cond = self._pop()
            else:
                # do_pop=0: condition value stays on stack. Pop it now and
                # set flag to skip the following pop instruction (which
                # would otherwise be emitted as a stray statement).
                cond = self._pop() if self.stack else "TOS"
                self._pending_if_pop = True
                self._if_with_cond_pop.add(ni)
            target = jt()
            # Short-circuit AND to assignment: do_pop=0 if targets an
            # assignment pointer instruction (push_local_ptr/push_global_ptr/
            # push_ovar_ptr). This is `var = A && B`; A stays on stack when
            # false (becomes the assigned value), pop A and push B when true.
            # Suppress the if block to avoid empty `if (A) {}`.
            if (not do_pop) and hasattr(self, 'insts') and target < len(self.insts):
                _t_an = self.insts[target][0]
                if _t_an in (57, 54, 62):  # push_local_ptr / push_global_ptr / push_ovar_ptr
                    # Save the left condition for merge at the
                    # assignment point. is_jz=0 (jump if false) =>
                    # AND: var = A && B; is_jz=1 (jump by true) =>
                    # OR: var = A || B.
                    self._sc_assign_pending[target] = (is_jz, cond)
                    self._pending_if_pop = True
                    return
            # Type-guard if: checkreftype(16) followed by if_jnz(pop) is a
            # compiler-internal type check. Suppress emission to avoid
            # `if (0) { ... }` or `if (/* unknown */) { ... }`.
            if ni in self._type_guard_ifs:
                self._type_guard_ifs.discard(ni)
                return
            # Catch-block exception check: compiler-generated if_jnz(pop)
            # testing the exception flag after end_catch. Suppress emission
            # to avoid `if (0) { return; }`. The ret is the exception path;
            # the normal path falls through to target.
            if ni in self._catch_check_skip:
                self._catch_check_skip.discard(ni)
                # The instruction after the if (typically ret) is the
                # exception path. Find it and mark for skip so the
                # normal code flow continues to L_normal (if target).
                try:
                    r_ni = ni + 1
                    while r_ni < len(self.insts) and self.insts[r_ni][0] in (0, 29, 46):
                        r_ni += 1
                    if r_ni < len(self.insts) and self.insts[r_ni][0] == 73:  # ret
                        self._catch_ret_skip.add(r_ni)
                except Exception:
                    pass
                return
            # Protective null-check guard: do_pop=0 if targets a `not`(45)
            # instruction. This is the compiler expansion of a guard wrapping
            # a member access inside a negation:
            #   is_object(x); if_jnz(no_pop) -> L_not; pop; eval x->...; L_not: not
            # The guard condition stays on stack when the if jumps (do_pop=0),
            # so we must pop it here and skip the following pop instruction.
            # Save the guard condition to _guard_pending[not_ni] so the `not`
            # handler can emit !(guard_cond && operand).
            if (not do_pop) and hasattr(self, 'insts') and target < len(self.insts) and self.insts[target][0] == 45:
                self._guard_pending[target] = cond
                self._pending_if_pop = True
                return
            # Handle empty stack (dead code or analysis gap)
            if cond == "<?>":
                cond = "/* unknown condition */"
            prefix = "} else " if self._pending_else_if else ""
            if self._pending_else_if:
                self._pending_else_if = False
            if is_jz:
                if cond.startswith('(!') and cond.endswith(')'):
                    inner = cond[2:-1]
                    self.lines.append("%s%sif (%s) {" % (self._ind(), prefix, inner))
                else:
                    self.lines.append("%s%sif (!(%s)) {" % (self._ind(), prefix, cond))
            else:
                self.lines.append("%s%sif (%s) {" % (self._ind(), prefix, cond))
            self.indent += 1
            self._if_stack.append((ni, target, "if"))
            # Save state for potential ternary detection at goto
            self._ternary_check = (len(self.lines), len(self.stack), cond, is_jz)
            return

        # ---- Control flow: GOTO (detect else pattern) ----
        if an == 31:  # goto
            # Skip gotos already consumed (e.g. emitted as break in else block)
            if ni in self._consumed_gotos:
                return
            if ni in self._catch_skip_gotos:
                self._catch_skip_gotos.discard(ni)
                return
            target = jt()
            # Skip redundant goto after end_catch: it jumps to catch body
            # (catch_target is the catch instruction, catch_target+1 is catch body)
            if hasattr(self, '_last_catch_target') and target == self._last_catch_target + 1:
                del self._last_catch_target
                return
            if hasattr(self, '_last_catch_target'):
                del self._last_catch_target
            # Dead-code goto: if the previous non-nop instruction is an
            # unconditional jump (goto/ret), this goto is unreachable and
            # must NOT be emitted (it would generate spurious "goto" lines
            # and could trigger false else detection). But only if this ni
            # is not itself a jump target (otherwise it's reachable via jump),
            # and not inside a switch body (case bodies often have consecutive
            # gotos that are all reachable from different case labels).
            if (ni not in self.all_targets
                    and not self._open_switches
                    and (not self._if_stack or self._if_stack[-1][2] != "ternary")):
                for prev_ni in range(ni - 1, -1, -1):
                    prev_an = self.insts[prev_ni][0]
                    if prev_an in (0, 29, 46):  # nop
                        continue
                    if prev_an in (31, 73):  # goto or ret
                        return
                    break
            # Close catch block if active (catch block ends with goto)
            if self._in_catch:
                self.indent -= 1
                self.lines.append("%s}" % self._ind())
                self._in_catch = False
            # Check for break statement: goto jumps to a loop exit point
            if self._while_stack:
                for _, loop_exit_ni in self._while_stack:
                    if target == loop_exit_ni:
                        # This goto is a break statement
                        # If inside an if block, emit break as the if body
                        if self._if_stack and self._if_stack[-1][2] == "if":
                            self.lines.append("%sbreak;" % self._ind())
                            # Don't close the if here - it will be closed
                            # when the block target is reached
                            # Mark that we emitted a break to prevent else detection
                            self._if_stack[-1] = (self._if_stack[-1][0],
                                                  self._if_stack[-1][1], "break")
                            self._skip_until_if_close = True
                            return
                        self.lines.append("%sbreak;" % self._ind())
                        return
            # Check for continue statement: goto jumps to a loop's for_step
            # back-edge (which then jumps to the loop head). The for_step
            # instruction is registered in _loop_exits. Also check direct
            # jumps to a loop head (foreach body target).
            if self._while_stack:
                matched_loop = False
                # Case 1: target is a for_step back-edge instruction
                if target in self._loop_exits:
                    head_ni, exit_ni = self._loop_exits[target]
                    for _, active_exit in self._while_stack:
                        if exit_ni == active_exit:
                            matched_loop = True
                            break
                # Case 2: target is a loop head directly (foreach body target)
                elif target in self._loop_heads:
                    exit_ni = self._loop_heads[target]
                    for _, active_exit in self._while_stack:
                        if exit_ni == active_exit:
                            matched_loop = True
                            break
                if matched_loop:
                    if self._if_stack and self._if_stack[-1][2] == "if":
                        self.lines.append("%scontinue;" % self._ind())
                        self._if_stack[-1] = (self._if_stack[-1][0],
                                              self._if_stack[-1][1], "break")
                        self._skip_until_if_close = True
                        return
                    self.lines.append("%scontinue;" % self._ind())
                    return
            # Check for break statement: goto jumps to a switch's end point or
            # fallthrough target. This is a "break" out of the switch (but
            # stays in the loop).
            if self._open_switches:
                for sw_start, sw_end in self._open_switches:
                    fallthru = self._switch_fallthru.get(sw_start)
                    if target == sw_end or (fallthru is not None and target == fallthru):
                        # This goto is a break out of the switch
                        if self._if_stack and self._if_stack[-1][2] == "if":
                            self.lines.append("%sbreak;" % self._ind())
                            self._if_stack[-1] = (self._if_stack[-1][0],
                                                  self._if_stack[-1][1], "break")
                            self._skip_until_if_close = True
                            return
                        self.lines.append("%sbreak;" % self._ind())
                        return
            if self._if_stack and self._if_stack[-1][2] == "if":
                prev_ni, prev_target, _ = self._if_stack[-1]
                # Patch 19: scan nop to check goto followed by if target
                # (there may be nop between goto and if target)
                _check_ni = ni + 1
                while _check_ni < prev_target and self.insts[_check_ni][0] in (0, 29, 46):
                    _check_ni += 1
                if _check_ni == prev_target:
                    # Skip else detection if this goto is the catch-block exit
                    # goto (it jumps past the if-block close target, but is
                    # not an else pattern).
                    if getattr(self, '_in_catch_block', False):
                        self._in_catch_block = False
                        return
                    # Dead-code goto: if the instruction immediately before
                    # this goto is also a goto, the previous goto already
                    # jumped away unconditionally, so this goto is dead code
                    # inside the if-block and must NOT trigger else detection.
                    # Otherwise the else block would wrongly swallow the code
                    # following the if-block (e.g. sibling case bodies).
                    # Patch 37: add all_targets guard (same as outer dead-code
                    # goto check at L3888). Without this, a goto that is BOTH a
                    # jump target AND immediately follows another goto would be
                    # wrongly suppressed inside an if-block tail.
                    if (ni - 1 > prev_ni and self.insts[ni - 1][0] == 31
                            and ni not in self.all_targets):
                        return
                    # Check for ternary pattern: only the "if (...) {" line was
                    # generated in if-block and exactly one value was pushed.
                    tc = getattr(self, '_ternary_check', None)
                    if tc and tc[1] + 1 == len(self.stack):
                        # Check if if-block has only label comments or empty lines
                        # beyond the "if (...) {" line (e.g. from catch(error(...)) pattern)
                        _extra = self.lines[tc[0]:]
                        _all_skip = all(
                            l.strip().startswith("// --- label") or l.strip() == ""
                            or (l.strip().startswith("catch(error(") and ("call (discard result)" in l or self.suppress_trailing_comments))
                            for l in _extra
                        )
                        if tc[0] == len(self.lines) or _all_skip:
                            # Save catch(error(...)) lines for re-emission
                            _catch_lines = [l for l in _extra if l.strip().startswith("catch(error(") and ("call (discard result)" in l or self.suppress_trailing_comments)]
                            # Remove all extra lines
                            while len(self.lines) > tc[0]:
                                self.lines.pop()
                            # Ternary: remove the "if (...) {" line, pop true value
                            self.lines.pop()
                            true_val = self._pop()
                            self._ternary_data = (tc[2], true_val, tc[3])
                            self._if_stack.pop()
                            self._if_stack.append((prev_ni, target, "ternary"))
                            # Re-emit catch(error(...)) as independent if block before ternary
                            # Reduce indent by 1 (undo the if-block open indent)
                            self.indent -= 1
                            for cl in _catch_lines:
                                self.lines.append("%sif (%s) {" % (self._ind(), tc[2]))
                                self.lines.append("%s    %s" % (self._ind(), cl.strip()))
                                self.lines.append("%s}" % self._ind())
                            self.indent += 1
                            return
                    # Check if the else block's first instruction is an if —
                    # if so, defer emission as "} else if (...)" (flattening
                    # nested else { if (...) { ... } } into else if (...) { ... }).
                    # prev_target = ni + 1 is the else block's start; scan past
                    # quiet (stack-only, non-line-emitting) opcodes to find the
                    # if instruction, since the else block typically starts with
                    # condition evaluation (push_local, push_i, compare, etc.)
                    # before the actual if instruction.
                    _QUIET_OPS = frozenset({
                        # nops / enter / checkargs
                        0, 15, 27, 29, 46,
                        # push operations
                        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
                        # binary ops: +, &, *, /, %, |, ^, <<, >>, -
                        1, 2, 25, 38, 40, 41, 48, 75, 81, 85,
                        # comparison ops: ==, >=, >, <=, <, !=
                        28, 30, 32, 37, 39, 42,
                        # unary ops: neg, not, rev
                        43, 45, 74,
                        # function calls (push result to stack)
                        5, 6, 7, 8, 9,
                        # array/mapping creation
                        22, 24,
                        # member access
                        67, 68,
                        # cast, typeof, sizeof
                        13, 36, 79,
                        # arg_array, to_ptr, to_value
                        19, 20, 21,
                        # resort, pickup_os, new_func
                        23, 49, 71,
                        # checkreftype, checkreftype_i
                        16, 17,
                        # range, range_ptr
                        69, 70,
                    })
                    else_start = prev_target
                    scan_limit = min(len(self.insts), prev_target + 200)
                    while (else_start < scan_limit
                           and self.insts[else_start][0] in _QUIET_OPS):
                        else_start += 1
                    es = self.insts[else_start] if else_start < len(self.insts) else None
                    # Only flatten plain ifs: do_pop=1 (self-popping), not a
                    # loop condition, not a do-while back-edge, not a catch-
                    # check, not a type-guard. do_pop=0 ifs (short-circuit
                    # first-if, null-check guard, AND-to-assignment) are
                    # excluded to avoid complex flag-lifetime issues.
                    # Also verify the if's target is within the else block
                    # (forward jump, not escaping the else block).
                    if (es is not None and es[0] == 33
                            and (es[2] & 2)  # do_pop
                            and else_start not in self._loop_conds
                            and else_start not in self._do_while_ifs
                            and else_start not in self._catch_check_skip
                            and else_start not in self._type_guard_ifs
                            and else_start not in self._short_circuit):
                        # Compute the if's jump target (where then-branch ends).
                        _if_p16 = es[1] if len(es) > 1 else 0
                        _if_target = else_start + 1 + (_if_p16 if _if_p16 < 0x8000 else _if_p16 - 0x10000)
                        if _if_target <= target:
                            # Determine the end of the if (or if-else) to verify
                            # it spans the ENTIRE else block — if there's code
                            # after the if, flattening would move it outside the
                            # else block, changing semantics.
                            # Scan backward from _if_target past nops to find
                            # the last non-nop instruction (end of then-branch).
                            _scan = _if_target - 1
                            while _scan > else_start and self.insts[_scan][0] in (0, 29, 46):
                                _scan -= 1
                            _if_end = _if_target  # default: if has no else
                            if _scan > else_start and self.insts[_scan][0] == 31:
                                # Goto at end of then-branch => if has an else.
                                # The if-else ends at the goto's target.
                                _g16 = self.insts[_scan][1]
                                _if_end = _scan + 1 + (_g16 if _g16 < 0x8000 else _g16 - 0x10000)
                            # Check: no non-nop instruction between _if_end
                            # and target (else block's end). Only nops are OK.
                            _has_code_after = False
                            for _cn in range(_if_end, target):
                                if _cn < len(self.insts) and self.insts[_cn][0] not in (0, 29, 46):
                                    _has_code_after = True
                                    break
                            if not _has_code_after:
                                # Flatten: if (or if-else) spans entire else block.
                                self._pending_else_if = True
                                self.indent -= 1
                                self._if_stack.pop()
                                return
                    self.indent -= 1
                    self._if_stack.pop()
                    self._if_stack.append((prev_ni, target, "else"))
                    self.lines.append("%s} else {" % self._ind())
                    self.indent += 1
                    return
            self.lines.append("%sgoto L%04d;" % (self._ind(), target))
            return

        # ---- For loop ----
        if an == 35:  # for_init
            # vm_initForLoop pops 3 values: step_type(top), container/to_value, loop_var_ptr
            # step_type: 1=forward, -1=backward, 997=foreach(array/mapping)
            step = self._pop() if self.stack else "1"
            second = self._pop() if self.stack else "<?>"
            loop_var = self._pop() if self.stack else "<?>"
            # Strip & from pointer
            if isinstance(loop_var, str) and loop_var.startswith('&'):
                loop_var = loop_var[1:]
            if isinstance(second, str) and second.startswith('&'):
                second = second[1:]
            # Save loop info for the next for_cond
            # step 997 = foreach, step 1 = forward for, step -1 = backward for
            self._pending_foreach = (loop_var, second, step)
            return
        if an == 18:  # for_cond (for_loop_jmp)
            target = jt()
            # Check if this is a recognized foreach loop condition
            if ni in self._loop_conds:
                exit_ni = self._loop_conds[ni]
                if self._pending_foreach is not None:
                    loop_var, second, step = self._pending_foreach
                    self._pending_foreach = None
                    self._foreach_info[ni] = (loop_var, second, step)
                    if step == "997":
                        # loop (loop_var in container)
                        # NOTE: "loop" is the correct LPC syntax for this project, NOT "foreach".
                        # Verified by user: loop (var in container) is the valid form.
                        self.lines.append("%sloop (%s in %s) {" % (self._ind(), loop_var, second))
                    else:
                        # Patch 45: merge for-loop init into header.
                        # Compiler emits `i = 0;` as a standalone assign stmt
                        # before for_cond. Check if last line is `var = value;`
                        # where var == loop_var, and merge it into the for header.
                        init_expr = ""
                        cur_ind = self._ind()
                        if self.lines:
                            last = self.lines[-1]
                            # Match: <indent>var = value;  (simple assignment only)
                            pat = r'^' + re.escape(cur_ind) + r'(' + re.escape(loop_var) + r') = (.+);$'
                            m = re.match(pat, last)
                            if m and m.group(1) == loop_var:
                                init_expr = "%s = %s" % (loop_var, m.group(2))
                                self.lines.pop()  # remove the standalone init stmt
                        if step == "-1":
                            # backward for: for (init; loop_var >= to; loop_var--)
                            if init_expr:
                                self.lines.append("%sfor (%s; %s >= %s; %s--) {" % (self._ind(), init_expr, loop_var, second, loop_var))
                            else:
                                self.lines.append("%sfor (; %s >= %s; %s--) {" % (self._ind(), loop_var, second, loop_var))
                        else:
                            # forward for: for (init; loop_var <= to; loop_var++)
                            if init_expr:
                                self.lines.append("%sfor (%s; %s <= %s; %s++) {" % (self._ind(), init_expr, loop_var, second, loop_var))
                            else:
                                self.lines.append("%sfor (; %s <= %s; %s++) {" % (self._ind(), loop_var, second, loop_var))
                else:
                    cond = self._pop() if self.stack else "1"
                    self.lines.append("%swhile (%s) {" % (self._ind(), self._simplify_expr(cond)))
                self.indent += 1
                self._while_stack.append((ni, exit_ni))
                return
            # Fallback: not a recognized loop condition
            cond = self._pop() if self.stack else "1"
            self.lines.append("%s// loop: while (%s) {  // exits->L%04d" % (self._ind(), cond, target))
            self._loop_stack.append(target)
            return
        if an == 80:  # for_step
            # If this is a recognized loop back-edge, the generate() loop
            # handles closing the loop body. Just skip emission here.
            if ni in self._loop_exits:
                return
            target = jt()
            self.lines.append("%s// } end_loop, continue->L%04d" % (self._ind(), target))
            if self._loop_stack:
                self._loop_stack.pop()
            return

        # ---- Try/Catch ----
        # 指令语义 (IDA验证):
        #   an=44 (newThreadCatch): try块开始, 参数=catch偏移, 注册catch handler
        #   an=26 (endThreadCatch): try块正常结束, 清理catch handler
        #   an=14 (caughtError): catch块入口, 异常时跳到这里
        # 字节码模式:
        #   [try: catch->L_catch]  ...try body...  [end_catch] [goto L_body]
        #   L_catch: [catch]  L_body: ...catch body... [goto L_exit]
        if an == 44:  # try - try块开始
            catch_target = jt()
            self._last_catch_target = catch_target  # 记录catch目标, 用于跳过多余goto
            self.lines.append("%stry {" % self._ind())
            self.indent += 1
            return
        if an == 26:  # end_catch - try块结束, 输出单行catch(catch body通常只有清理操作)
            self.indent -= 1
            # Detect catch(error(...)) pattern: try block contains only error() call.
            # If matched, remove try/error lines and push catch(error(...)) as
            # expression value on stack (for ternary detection). This avoids
            # generating multi-line try/catch code that breaks ternary detection.
            if len(self.lines) >= 2:
                _l1 = self.lines[-1].strip()
                _l2 = self.lines[-2].strip()
                if (_l2 == "try {" and _l1.startswith("error(")
                        and ("call (discard result)" in _l1 or self.suppress_trailing_comments)):
                    _start = _l1.index("error(") + 6
                    _end = _l1.rindex(");")
                    _err_arg = _l1[_start:_end]
                    self.lines.pop()  # remove error(...);
                    self.lines.pop()  # remove try {
                    # Generate catch(error(...)) statement (not stack value)
                    self.lines.append("%scatch(error(%s));%s" % (self._ind(), _err_arg, "" if self.suppress_trailing_comments else "  // call (discard result)"))
                    self._in_catch = False
                    return
            self.lines.append("%s} catch { /* error handled */ }" % self._ind())
            self._in_catch = False
            # Look for the catch-block exception check pattern:
            #   [push_i(0)] [end_catch] [goto L_body] L_catch: [catch]
            #   L_body: [if_jnz(pop) -> L_normal] [ret] L_normal: ...
            # push_i(0) is the default exception flag (no exception);
            # caughtError (an=14) pushes 1 on the exception path. The if
            # tests the flag: exception (1) -> ret; normal (0) -> L_body.
            # This is compiler-generated catch dispatch, not user code.
            # Suppress the if emission to avoid `if (0) { return; }`.
            # NOTE: jump targets are signed 16-bit relative offsets:
            #   target_ni = ni + 1 + (p16 if p16 < 0x8000 else p16 - 0x10000)
            try:
                def _rel_target(instr_ni, p16):
                    return instr_ni + 1 + (p16 if p16 < 0x8000 else p16 - 0x10000)
                # Scan backwards from end_catch for push_i(59)
                p = ni - 1
                while p >= 0 and self.insts[p][0] in (0, 29, 46):
                    p -= 1
                if p >= 0 and self.insts[p][0] == 59:
                    # Scan forwards from end_catch for goto(31)
                    g = ni + 1
                    while g < len(self.insts) and self.insts[g][0] in (0, 29, 46):
                        g += 1
                    if g < len(self.insts) and self.insts[g][0] == 31:
                        gp16 = self.insts[g][1]
                        l_body = _rel_target(g, gp16)
                        # L_body = l_body; just before it should be catch(14)
                        if 0 < l_body <= len(self.insts) and self.insts[l_body - 1][0] == 14:
                            # This goto is the compiler-generated normal-path jump that
                            # skips over the catch handler. It is redundant after we
                            # emit `} catch { ... }`, so suppress it.
                            self._catch_skip_gotos.add(g)
                            # At L_body: if_jnz(pop) -> L_normal
                            if l_body < len(self.insts) and self.insts[l_body][0] == 33:
                                if_p8 = self.insts[l_body][2] if len(self.insts[l_body]) > 2 else 0
                                if (if_p8 & 2) != 0:  # do_pop=1 (pop variant)
                                    if_p16 = self.insts[l_body][1]
                                    l_normal = _rel_target(l_body, if_p16)
                                    # L_normal-1 (skip nops) should be ret(73)
                                    r = l_normal - 1
                                    while r >= 0 and self.insts[r][0] in (0, 29, 46):
                                        r -= 1
                                    if r >= 0 and self.insts[r][0] == 73:
                                        self._catch_check_skip.add(l_body)
            except Exception:
                pass
            return
        if an == 14:  # catch
            # Mark that we are now in the catch block; the next goto is the
            # catch-block exit goto and must NOT be treated as an else pattern.
            # But only if the catch block actually has a goto as its next
            # non-nop instruction. If the catch block is empty (next non-nop
            # is pop_n or other non-goto), the following goto belongs to the
            # outer then-branch, not the catch block, and should NOT set
            # _in_catch_block (otherwise it blocks else/ternary detection).
            _next = ni + 1
            while _next < len(self.insts) and self.insts[_next][0] in (0, 29, 46):
                _next += 1
            if _next < len(self.insts) and self.insts[_next][0] == 31:  # goto
                self._in_catch_block = True
            return

        # ---- Switch/Case ----
        # case_num (an=10) and case_str (an=11) are complex instructions:
        #   metaIdx = p16 | (p8<<16) -> metaData[metaIdx]=offset, metaData[metaIdx+4]=value
        # default (an=12) is non-complex: p16 = direct offset
        # The switch expression is on top of stack when the first case runs.
        # We emit the switch header at the first case; case labels are emitted
        # at body targets (handled in generate() loop via _switch_cases).
        if an == 10:  # case_num
            if hasattr(self, '_switch_starts') and ni in self._switch_starts:
                # First case in a switch run: pop switch expression, emit header
                val = self._pop() if self.stack else "switch_expr"
                self.lines.append("%sswitch (%s) {" % (self._ind(), val))
                self.indent += 1
                end_ni = self._switch_ends.get(ni, ni + 1)
                self._open_switches.append((ni, end_ni))
            # case label is emitted at body target; nothing to do here
            return
        if an == 11:  # case_str
            if hasattr(self, '_switch_starts') and ni in self._switch_starts:
                val = self._pop() if self.stack else "switch_expr"
                self.lines.append("%sswitch (%s) {" % (self._ind(), val))
                self.indent += 1
                end_ni = self._switch_ends.get(ni, ni + 1)
                self._open_switches.append((ni, end_ni))
            return
        if an == 12:  # default
            # default label is emitted at body target; nothing to do here
            return

        # ---- Set operations ----
        if an == 76:  # set_global
            val = self._pop()
            self.lines.append("%s%s = %s;" % (self._ind(), self._global_var_name(p16), val))
            return
        if an == 77:  # set_local
            val = self._pop()
            self.lines.append("%s%s = %s;" % (self._ind(), self._arg_name(p16), val))
            return
        if an == 78:  # set_ovar
            val = self._pop()
            self.lines.append("%s%s = %s;" % (self._ind(), self.var_names.ovar_name(p16), val))
            return

        # ---- Misc ----
        if an == 21:  # arg_array
            # vm_createArgumentArray @ 0x8091fb4: packs all actual
            # arguments (vm_argumentInThisContext @ 0x8093400) into
            # an array and pushes to stack.
            # LPC syntax: $< (verified via yacc rule 188, RHS = [$ <])
            # yylex returns bare $ as token 91 when not followed by $;
            # < is token 75. No set_local_var follows arg_array - the
            # array is used directly on the stack.
            self._push("$<"); return
        if an == 19:  # to_ptr
            a = self._pop(); self._push("&%s" % a); return
        if an == 20:  # to_value
            a = self._pop()
            # *&expr -> expr: convertToValue on an address from push_*_ptr
            # simply loads the variable value (postfix old-value loads
            # depend on this simplified form).
            if a.startswith("&"):
                self._push(a[1:])
            else:
                self._push("(*%s)" % a)
            return
        if an == 71:  # resort
            return
        if an == 82:  # reserve
            self.lines.append("%sreserve(%d);%s" % (self._ind(), p16, "" if self.suppress_trailing_comments else "  // allocate stack space")); return
        if an == 83:  # unreserve
            self.lines.append("%sunreserve(%d);%s" % (self._ind(), p16, "" if self.suppress_trailing_comments else "  // free stack space")); return
        if an == 84:  # trace
            self.lines.append("%strace(%d);%s" % (self._ind(), p16, "" if self.suppress_trailing_comments else "  // debug tracepoint")); return
        if an == 49:  # pickup_os
            self._push("os_var_%d" % p16); return
        if an == 23:  # new_func - create function pointer (closure)
            # vm_createFunctionValue(p16=funcIdx, p8=flags)
            # p8 bit7=isGlobal, bit6=funcType, bits0-5 (p8&0x3F)=num captured vars
            # vm_createFunctionValueEx pops (p8&0x3F) values from stack as
            # captured arguments. Verified via IDA @ 0x8091cd0.
            nargs = p8 & 0x3F
            is_global = (p8 >> 7) & 1
            if is_global:
                # Patch 39 (same as call_user): is_global=1 means the
                # global program function table = [efuns 0..N-1, sefuns N..].
                # Check func_table first (the global program's own table),
                # then fall back to the efun/sefun ranges like call_user.
                if p16 in self.func_table:
                    fname = self.func_table[p16]
                else:
                    fname = EFUN_TABLE.get(p16) or SEFUN_TABLE.get(p16, "global_func_%d" % p16)
            elif p16 in self.func_table:
                fname = self.func_table[p16]
                # Patch 8: closure over an inherited slot. When the slot
                # belongs to a parent (_inherit_idx_to_path) AND this
                # object overrides the function (own slot with the same
                # name), the closure references the parent version:
                # (: ::process_command :). A plain closure compiles to
                # the OWN slot; the inherited slot without '::' would
                # recurse infinitely when invoked.
                _i2p = getattr(self, '_inherit_idx_to_path', {})
                if p16 in _i2p and any(v == fname and k not in _i2p
                                       for k, v in self.func_table.items()):
                    fname = '::%s' % fname
            else:
                fname = "#%d" % p16
            if nargs > 0:
                # Patch 29: pop order = lambda param order (no reverse).
                # IDA vm_createFunctionValueEx @ 0x8091cd0: VM pops from
                # stack TOP into Memory[0..n-1]; Memory[0] is the first
                # function argument (vm_callFunPtr @ 0x809441c).
                args = [self._pop() for _ in range(nargs)]
                self._push("(: %s, %s :)" % (fname, ", ".join(args)))
            else:
                self._push("(: %s :)" % fname)
            return
        if an == 16:  # checkreftype
            # Detect type-guard pattern: checkreftype followed by if_jnz(pop)
            # which tests the type-check result. Mark the if for suppression.
            try:
                g = ni + 1
                while g < len(self.insts) and self.insts[g][0] in (0, 29, 46):
                    g += 1
                if g < len(self.insts) and self.insts[g][0] == 33:
                    gp8 = self.insts[g][2] if len(self.insts[g]) > 2 else 0
                    if (gp8 & 2) != 0:  # do_pop=1 (pop variant)
                        self._type_guard_ifs.add(g)
            except Exception:
                pass
            return
        if an == 17:  # checkreftype_i
            return
        if an == 69:  # range - container[idx1..idx2] (IDA: vm_queryRange)
            # Stack layout (IDA verified @ 0x808fd04):
            #   [container, idx1, idx2]  (idx2 on top)
            # p16 flag: bit0=idx2 negative (from end), bit1=idx1 negative
            # Net effect: pop 3, push container[idx1..idx2]
            idx2 = self._pop()
            idx1 = self._pop()
            obj = self._pop()
            if p16 & 1:
                idx2 = "<%s" % idx2
            if p16 & 2:
                idx1 = "<%s" % idx1
            if obj == "$<":
                obj = "($<)"
            self._push("%s[%s..%s]" % (obj, idx1, idx2)); return
        if an == 70:  # range_ptr - &container[idx1..idx2]
            idx2 = self._pop()
            idx1 = self._pop()
            obj = self._pop()
            if p16 & 1:
                idx2 = "<%s" % idx2
            if p16 & 2:
                idx1 = "<%s" % idx1
            if obj == "$<":
                obj = "($<)"
            self._push("&%s[%s..%s]" % (obj, idx1, idx2)); return

        # Unknown
        self.lines.append("%s/* unknown bytecode: op_%d(%d,%d) */" % (self._ind(), an, p16, p8))


# ============================================================================
# Disassembly Formatter (v37 enhanced)
# ============================================================================
def fmt_disasm_v37(an, p16, p8, ni, nc, strings, local_vars, metadata=None,
                   func_local_vars=None, func_args=None):
    """Enhanced disassembly formatter with more context."""
    if an not in OPCODES:
        return "??? %d(p16=%d, p8=%d)" % (an, p16, p8)
    name, _, _, sem = OPCODES[an]
    if func_local_vars is None:
        func_local_vars = {}
    if func_args is None:
        func_args = {}

    def jt():
        t = ni + 1 + (p16 if p16 < 0x8000 else p16 - 0x10000)
        return max(0, min(t, nc-1))

    def lv(v):
        if v >= 0x8000: v = v - 0x10000
        return "local[%d]" % v

    def var_name(idx):
        """Convert bytecode local var index to readable name.
        Positive indices = function arguments (arg_0, arg_1, ...)
        Negative indices = local variables (local_1, local_2, ...)

        Verified via IDA vm_pushLocalVar @ 0x080914DC:
          v4 = v3->bp + a1   (bp = frame base, a1 = signed offset)
          argNum > a1 distinguishes args (a1>=0) from locals (a1<0).
        Local variable names are extracted from the .o file; the VM index
        (signed int16) is stored 12 bytes before each name's [len] field.
        See LPC反编译技术文档 25.5.
        """
        if idx >= 0x8000: idx = idx - 0x10000
        if idx < 0:
            if idx in func_local_vars:
                return func_local_vars[idx]
            return "local_%d" % abs(idx)
        if idx in func_args:
            return func_args[idx]
        return "arg_%d" % idx

    def ovar_name_dasm(idx):
        """Object variable name for disassembly - uses extracted names when possible.

        Verified via IDA vm_pushObjVar @ 0x08091224:
          v1 = &pObjectValues[valueNo + baseVarOffset]
        So p16 (valueNo) directly indexes the object variable array, and
        local_vars[idx] holds the name extracted from the .o file.
        See LPC反编译技术文档 25.4.
        """
        if 0 <= idx < len(local_vars):
            return local_vars[idx]
        return "this_var_%d" % idx

    if an in (0, 29, 46): return "nop"
    if an == 1:  return "add"
    if an == 2:  return "and"
    if an == 3:  return "assign"
    if an == 4:  return "assign_rev"
    if an == 5:  return "call_ptr(nargs=%d)" % p8
    if an == 6:
        en = efun_name(p16)
        doc = efun_doc(en)
        s = "call_efun(%s, nargs=%d)" % (en, p8)
        if doc: s += "  ; %s" % doc
        return s
    if an == 7:
        is_global = (p8 >> 7) & 1
        nargs = p8 & 0x3F
        prefix = "global_fn" if is_global else "user_fn"
        return "call_%s(#%d, nargs=%d)" % (prefix, p16, nargs)
    if an == 8:  return "call_other(flags=%d, nargs=%d)" % (p16, p8)
    if an == 9:  return "call_os(#%d, nargs=%d)" % (p16, p8)
    if an == 10:
        return "case_num(%d)" % p16
    if an == 11:
        return "case_str(#%d)" % p16
    if an == 12: return "default(offset=%d)" % p16
    if an == 13:
        t = {2: "int", 3: "real", 4: "string", 5: "buffer", 6: "array", 7: "mapping"}
        return "cast(%s)" % t.get(p16, str(p16))
    if an == 14: return "catch"
    if an == 15: return "checkargs(offset=%d)" % p16
    if an == 16: return "checkreftype(%d)" % p16
    if an == 17: return "checkreftype_i"
    if an == 18: return "for_cond: if_false goto L%04d" % jt()
    if an == 19: return "to_ptr"
    if an == 20: return "to_value"
    if an == 21: return "arg_array"
    if an == 22: return "new_array(elems=%d)" % p16
    if an == 23: return "new_func(%d, capture=%d)" % (p16, p8 & 0x3F)
    if an == 24: return "new_mapping(pairs=%d)" % p16
    if an == 25: return "div"
    if an == 26: return "end_catch"
    if an == 27: return "enter"
    if an == 28: return "eq"
    if an == 30: return "ge"
    if an == 31: return "goto L%04d" % jt()
    if an == 32: return "gt"
    if an == 33:
        jtype = "jz(if_true)" if (p8 & 1) else "jnz(if_false)"
        pop = ", pop" if (p8 & 2) else ""
        return "if_%s%s -> L%04d" % (jtype, pop, jt())
    if an == 34:
        amt = p16 if p16 < 0x8000 else p16 - 0x10000
        return "inc(amount=%d, *TOS)" % amt
    if an == 35: return "for_init(%d)" % p16
    if an == 36:
        is_fn = IS_TYPE_MAP.get(p16)
        if is_fn:
            return is_fn
        tn = TYPE_CODES.get(p16, "T%d" % p16)
        return "typeof(%s)" % tn
    if an == 37: return "le"
    if an == 38: return "lsh"
    if an == 39: return "lt"
    if an == 40: return "mod"
    if an == 41: return "mul"
    if an == 42: return "ne"
    if an == 43: return "neg"
    if an == 44: return "try: catch->L%04d" % jt()
    if an == 45: return "not"
    if an == 47:
        ops = {2: "+=", 3: "-=", 4: "*=", 5: "/=", 6: "%=",
               7: "|=", 8: "^=", 9: ">>=", 10: "<<="}
        return "op_eq(%s)" % ops.get(p16, str(p16))
    if an == 48: return "or"
    if an == 49: return "pickup_os(%d,%d)" % (p16, p8)
    if an == 50: return "pop_n(%d)" % p16
    if an == 51: return "pop"
    if an == 52: return "push_argnum"
    if an == 53: return "push_global(#%d)" % p16
    if an == 54: return "push_global_ptr(#%d)" % p16
    if an == 55: return "push_local(%s)" % var_name(p16)
    if an == 56: return "push_local_i"
    if an == 57: return "push_local_ptr(%s)" % var_name(p16)
    if an == 58: return "push_local_ptr_i"
    if an == 59: return "push_i(%d)" % p16
    if an == 60:
        # push_num is complex: p16|p8<<16 = byte offset into metaData
        meta_idx = p16 | (p8 << 16)
        if metadata is None or meta_idx + 4 > len(metadata):
            return "push_num(meta[%d]=?)" % meta_idx
        mv = struct.unpack_from('<i', metadata, meta_idx)[0]
        return "push_num(meta[%d]=%d)" % (meta_idx, mv)
    if an == 61: return "push_ovar(%s)" % ovar_name_dasm(p16)
    if an == 62: return "push_ovar_ptr(%s)" % ovar_name_dasm(p16)
    if an == 63:
        # push_real is complex: p16|p8<<16 = byte offset into metaData
        meta_idx = p16 | (p8 << 16)
        if metadata is None or meta_idx + 4 > len(metadata):
            return "push_real(meta[%d]=?)" % meta_idx
        raw = struct.unpack_from('<i', metadata, meta_idx)[0]
        v = struct.unpack('<f', struct.pack('<i', raw))[0]
        return "push_real(meta[%d]=%.4f)" % (meta_idx, v)
    if an == 64:
        # bytecode p16 is 0-based index into the .o string table
        # (vm_mapStrNoTo identity-maps when pStrOffsetMapping is NULL)
        idx = p16
        if strings and 0 <= idx < len(strings):
            s = strings[idx].replace('\n','\\n').replace('\r','\\r').replace('\x00','')
            if len(s) > 40:
                s = s[:40] + "...<trunc>"
            s = s.replace('"', '\\"')
            return 'push_s(#%d: "%s")' % (p16, s)
        return "push_s(#%d)" % p16
    if an == 65: return "push_this"
    if an == 66: return "push_0"
    if an == 67: return "member[%d]" % p16
    if an == 68: return "member_ptr[%d]" % p16
    if an == 69: return "range[%d]" % p16
    if an == 70: return "range_ptr[%d]" % p16
    if an == 71: return "resort"
    if an == 72: return "ret_fake"
    if an == 73: return "ret"
    if an == 74: return "rev"
    if an == 75: return "rsh"
    if an == 76: return "set_global(#%d)" % p16
    if an == 77: return "set_local(%s)" % var_name(p16)
    if an == 78: return "set_ovar(%s)" % ovar_name_dasm(p16)
    if an == 79: return "sizeof"
    if an == 80: return "for_step: goto L%04d" % jt()
    if an == 81: return "sub"
    if an == 82: return "reserve(%d)" % p16
    if an == 83: return "unreserve(%d)" % p16
    if an == 84: return "trace(%d)" % p16
    if an == 85: return "xor"
    return "%s(%d,%d)" % (sem, p16, p8)


# ============================================================================
# Main Decompiler v37 - Enhanced Output
# ============================================================================
def decompile_file_v37(filepath, out_path=None, mode='both', encoding='gb2312'):
    o = LPCOFile(filepath, encoding=encoding).parse()
    data = o.data
    prog = o.prog
    strings = o.str_table

    # Parse constants from FFFFFFFF entries (for files with many #defines)
    constants = o.parse_constants()
    const_stats = {}
    for name, val, ctype in constants:
        const_stats[ctype] = const_stats.get(ctype, 0) + 1

    segs = find_prologue_segments(prog)

    # If DLT parsing found authoritative function segments, use them to filter
    # out false positives from the heuristic scanner and add missed segments.
    dlt_segs = getattr(o, 'dlt_func_segs', {})
    if dlt_segs:
        # Only keep heuristic segments whose nodes_start matches DLT data
        segs = [s for s in segs if s['ns'] in dlt_segs]
        # Correct nodes_count from authoritative DLT data
        for s in segs:
            nc_dlt = dlt_segs.get(s['ns'])
            if nc_dlt and nc_dlt != s['nc']:
                s['nc'] = nc_dlt
                s['ne'] = s['ns'] + nc_dlt * 4
        # Add DLT segments that the heuristic scanner missed
        existing_ns = {s['ns'] for s in segs}
        for ns_dlt, nc_dlt in dlt_segs.items():
            if ns_dlt not in existing_ns and nc_dlt > 1:
                segs.append({'ns': ns_dlt, 'nc': nc_dlt,
                             'ne': ns_dlt + nc_dlt * 4,
                             'func_name': None, 'name_pos': 0})
                existing_ns.add(ns_dlt)

    # Patch 44: Pre-set func_name from DLT-authoritative mapping.
    # DLT parsing (vm_loadIdentifiers @ 0x08087FB8) correctly identified each
    # function's code segment nodes_start and its name from the Vm_Fun_T name
    # field. This is authoritative and should NOT be overridden by the
    # heuristic match_func_names (which finds closest preceding [len][name\0]
    # pattern and can be fooled by local variable names that share names with
    # functions, e.g. local var 'a' inside function 'b').
    dlt_func_names = getattr(o, 'dlt_func_names_by_seg', {})
    if dlt_func_names:
        for s in segs:
            nm = dlt_func_names.get(s['ns'])
            if nm:
                s['func_name'] = nm
                s['name_pos'] = -1  # marker: DLT-authoritative (not from prog search)
    # Use all symbol names (including constants) for code segment matching,
    # but filtered user_funcs for the "declared functions" output section.
    # Pass None for match_names/match_name_bytes so match_func_names only
    # fills in segments not already set by DLT.
    match_names = getattr(o, 'all_symbol_names', o.user_funcs)
    match_name_bytes = getattr(o, 'all_name_bytes', None)
    # Build a list of segments that still need matching (func_name is None)
    segs_to_match = [s for s in segs if not s.get('func_name')]
    segs_to_match = match_func_names(prog, match_names, segs_to_match, match_name_bytes, encoding=encoding)
    # merge matched results back into segs
    matched_dict = {s['ns']: s for s in segs_to_match}
    for s in segs:
        if not s.get('func_name') and s['ns'] in matched_dict:
            s['func_name'] = matched_dict[s['ns']].get('func_name')
            s['name_pos'] = matched_dict[s['ns']].get('name_pos', 0)

    # Add HIDDEN_FUNS segments that weren't found by find_prologue_segments.
    # Hidden functions are loaded via vm_loadFun and have proper checkargs
    # prologues, but if find_prologue_segments missed them, add them explicitly.
    existing_ns = {p['ns'] for p in segs}
    for hf in getattr(o, 'hidden_funcs', []):
        if hf['nodes_start'] not in existing_ns and hf['nodes_count'] > 1:
            segs.append({
                'ns': hf['nodes_start'],
                'nc': hf['nodes_count'],
                'ne': hf['nodes_start'] + hf['nodes_count'] * 4,
                'func_name': hf['name'],
                'name_pos': 0,
            })
            existing_ns.add(hf['nodes_start'])

    matched = {}
    for p in segs:
        fname = p['func_name']
        if fname and not fname.startswith('internal_') and fname not in matched:
            matched[fname] = (p['ns'], p['nc'])

    # Post-filter user_funcs: names that didn't match any segment are
    # functions without code (inherited, forward declarations, or function
    # pointer references). _parse_var_names already removed PROG_VARS
    # variable names, so remaining unmatched names are functions, not vars.
    real_funcs = [f for f in o.user_funcs if f in matched]
    no_code_funcs = [f for f in o.user_funcs if f not in matched]
    # Update user_funcs to only include real functions (with code segments)
    o.user_funcs = real_funcs

    lines = []
    # ---- File Header ----
    lines.append("/" + "*" * 70)
    lines.append(" * LPC Decompiled Source (v37 - Readability Enhanced)")
    lines.append(" * File: %s" % o.filename)
    lines.append(" * Source: %s" % (o.source_file or '(unknown)'))
    if o.inherited_func:
        lines.append(" * Inherits: %s()" % o.inherited_func)
    else:
        lines.append(" * Inherits: (none)")
    if getattr(o, 'inherit_names', None):
        lines.append(" * Inherited programs: %s" % ", ".join("%s()" % n for n in o.inherit_names))
    if constants:
        lines.append(" * Constants: %d (HEX:%d INT:%d STR:%d CODE:%d REF:%d)" % (
            len(constants), const_stats.get('HEX', 0), const_stats.get('INT', 0),
            const_stats.get('STRING', 0), const_stats.get('CODE', 0), const_stats.get('REF', 0)))
    lines.append(" * Functions: %d names, %d code segments found" % (len(o.user_funcs), len(segs)))
    lines.append(" * Strings: %d, Variables: %d" % (len(o.str_table), len(o.local_vars)))
    lines.append(" " + "*" * 69 + "/")
    lines.append("")

    # ---- Inherit Statements ----
    # Always emit `inherit "/path/parent";` so the source structure is
    # visible even when no #define constants are present.
    if getattr(o, 'inherit_names', None):
        for iname in o.inherit_names:
            lines.append('inherit "%s";' % iname)
        lines.append("")

    # ---- Constant Definitions (for files like start_aaa.o) ----
    if constants:
        # Separate constants by type
        hex_consts = [(n, v) for n, v, t in constants if t == 'HEX']
        int_consts = [(n, v) for n, v, t in constants if t == 'INT']
        str_consts = [(n, v) for n, v, t in constants if t == 'STRING']
        code_consts = [(n, v) for n, v, t in constants if t == 'CODE']
        ref_consts = [(n, v) for n, v, t in constants if t == 'REF']

        # Normalize hex values: lowercase x
        def _norm_hex(val):
            v = val.strip()
            if v.startswith('0X'):
                v = '0x' + v[2:]
            if v.startswith('$$') and v[2:].isdigit():
                return '##%s' % v[2:]
            return v

        if hex_consts or int_consts or str_consts or code_consts or ref_consts:
            lines.append("/* ================================================================")
            lines.append("   CONSTANT DEFINITIONS (#define)")
            lines.append("   Total: %d (HEX:%d INT:%d STR:%d CODE:%d REF:%d)" % (
                len(constants), len(hex_consts), len(int_consts),
                len(str_consts), len(code_consts), len(ref_consts)))
            lines.append("   ================================================================ */")
            lines.append("")
            # inherit statements are now emitted in the file header
            lines.append("")

        if hex_consts:
            lines.append("// --- Protocol IDs / Hex Constants (%d) ---" % len(hex_consts))
            for name, val in sorted(hex_consts, key=lambda x: _norm_hex(x[1])):
                padding = max(1, 55 - len(name))
                lines.append("#define %-s%s%s" % (name, " " * padding, _norm_hex(val)))
            lines.append("")

        if int_consts:
            lines.append("// --- Integer Constants (%d) ---" % len(int_consts))
            for name, val in int_consts:
                padding = max(1, 55 - len(name))
                lines.append("#define %-s%s%s" % (name, " " * padding, val.strip()))
            lines.append("")

        if str_consts:
            lines.append("// --- String Constants (%d) ---" % len(str_consts))
            for name, val in str_consts:
                lines.append("#define %-s%s%s" % (name, " " * max(1, 55 - len(name)), val.strip()))
            lines.append("")

        if ref_consts:
            lines.append("// --- Reference Constants (%d) ---" % len(ref_consts))
            for name, val in ref_consts:
                padding = max(1, 55 - len(name))
                lines.append("#define %-s%s%s" % (name, " " * padding, val.strip()))
            lines.append("")

        if code_consts:
            lines.append("// --- Code Macros (%d) ---" % len(code_consts))
            for name, val in code_consts:
                lines.append("#define %-s%s%s" % (name, " " * max(1, 55 - len(name)), val.strip()))
            lines.append("")


    # ---- String Table ----
    if o.user_strings and mode in ('disasm', 'both'):
        lines.append("/* ================================================================")
        lines.append("   String Literals Reference")
        lines.append("   ================================================================ */")
        shown_strings = []
        for i, s in enumerate(o.str_table):
            if not s or '\x00' in s:
                continue
            # Skip strings with no printable content (e.g. lone control chars
            # like 0x10/DLE). Allow common whitespace (\t \n \r) and ESC (0x1b
            # for ANSI escape sequences) but require at least one printable
            # character so pure-control-character strings are filtered out.
            if not any(c.isprintable() for c in s):
                continue
            esc = s.replace('\n','\\n').replace('\r','\\r').replace('"','\\"')
            if len(esc) > 70:
                esc = esc[:67] + "...<trunc>"
            lines.append("   [%3d] \"%s\"" % (i, esc))
            shown_strings.append(i)
        lines.append("")

    # ---- Variable Table ----
    if o.local_vars and mode in ('disasm', 'pseudo', 'both'):
        lines.append("/* ================================================================")
        lines.append("   Global/Object Variables")
        lines.append("   ================================================================ */")
        # Build name -> type mapping from var_types (PROG_VARS)
        name_to_type = {}
        if getattr(o, 'var_types', None):
            type_names = {0:'undefined',1:'void',2:'int',3:'float',4:'string',
                          5:'buffer',6:'array',7:'mapping',8:'pointer',
                          9:'function',10:'object',20:'loop_ctrl',255:'mixed'}
            for vn, tb in o.var_types.items():
                if 0 <= vn < len(o.local_vars):
                    nm = o.local_vars[vn]
                    if nm and not nm.startswith('global_'):
                        name_to_type[nm] = type_names.get(tb, 'mixed')
        unique_vars = sorted(set(o.local_vars))
        for v in unique_vars:
            if v in o.user_funcs:
                continue
            # Use actual type from PROG_VARS when available
            actual_type = name_to_type.get(v)
            if actual_type:
                lines.append("   %s %s;" % (actual_type, v))
                continue
            # Fallback: infer type hint based on naming conventions
            hint = ""
            if v.endswith('_url') or v.endswith('_name') or v.endswith('_file'):
                hint = "string"
            elif v.endswith('_id') or v.endswith('_count') or v.startswith('cur_'):
                hint = "int"
            elif v.startswith('is_') or v.startswith('has_'):
                hint = "int  // bool"
            if hint:
                lines.append("   %s %s;" % (hint.split('  ')[0], v))
            else:
                lines.append("   mixed %s;" % v)
        lines.append("")

    # ---- Global Variable Initializations (from init code segment) ----
    # Computed here but emitted later (after #define and forward declarations)
    init_lines = parse_init_code_segment(o)

    # ---- Function Definitions ----
    # First pass: collect all function signatures and bodies
    decoded_count = 0
    unresolved_count = 0

    all_funcs = []
    for p in sorted(segs, key=lambda x: x['ns']):
        all_funcs.append((p['func_name'], p['ns'], p['nc']))

    func_declarations = []  # forward declarations
    func_bodies = []        # full function output lines

    for fname, ns, nc in all_funcs:
        insts = []
        for ni in range(nc):
            off_b = ns + ni*4
            if off_b + 4 > len(prog): break
            b0, b1, b2, b3 = prog[off_b:off_b+4]
            insts.append((b3, b0|(b1<<8), b2))

        # Extract per-function metaData (for case_num/case_str complex instructions)
        func_meta = extract_func_metadata(prog, ns, nc)

        # Get per-function local variables and args from DLT parsing.
        # Falls back to the legacy global dict if DLT parsing didn't find this seg.
        seg_local_vars = o.func_local_vars_by_seg.get(ns, o.func_local_vars)
        seg_args = o.func_args_by_seg.get(ns)
        seg_arg_types = o.func_arg_types_by_seg.get(ns, {})
        seg_ret_type = o.func_ret_types_by_seg.get(ns)
        seg_attrib = o.func_attribs_by_seg.get(ns, 0)
        seg_min_arg_no = o.func_min_arg_nos_by_seg.get(ns)
        seg_max_arg_no = o.func_max_arg_nos_by_seg.get(ns)
        # Convert seg_args list [(vm_idx, name), ...] to dict {vm_idx: name}
        seg_args_dict = dict(seg_args) if seg_args else {}

        if len(insts) < 2:
            unresolved_count += 1
            lines.append("/* === Unresolved code segment: %s @ offset %d (too few instructions: %d) === */" % (fname, ns, len(insts)))
            lines.append("")
            continue

        # First instruction must be checkargs (an=15). enter (an=27) should
        # appear within the first few instructions (some functions have
        # push/sizesize before enter).
        if insts[0][0] != 15:
            unresolved_count += 1
            lines.append("/* === Unresolved code segment: %s @ offset %d (first op=%d, expected checkargs=15) === */" % (fname, ns, insts[0][0]))
            lines.append("")
            continue
        # Patch 22: extend enter search range to 30 for multi-arg functions
        has_checkargs = any(insts[j][0] == 27 for j in range(min(len(insts), 30)))
        if not has_checkargs:
            unresolved_count += 1
            lines.append("/* === Unresolved code segment: %s @ offset %d (no enter opcode within first 30 instructions) === */" % (fname, ns))
            lines.append("")
            continue

        decoded_count += 1

        # Determine argument count
        nargs = 0
        for an, p16, p8 in insts[2:]:
            if an in (55, 57):
                if p16 >= 0 and p16 < 100:
                    nargs = max(nargs, p16 + 1)
            elif an in (77,):
                if p16 >= 0 and p16 < 100:
                    nargs = max(nargs, p16 + 1)
        # v37+: Override nargs with DLT arg_count when DLT declares more args
        # than bytecode scanning found. This handles functions that accept
        # extra parameters not referenced in the function body (e.g. apply_status
        # in status objects receives 4 args but only uses 'me').
        dlt_arg_count = len(seg_args) if seg_args else 0
        if dlt_arg_count > nargs:
            nargs = dlt_arg_count

        # Return type inference (enhanced with bytecode analysis)
        # Type values: see vm_valueType @ 0x0808A13C
        _type_names = {0:'undefined',1:'void',2:'int',3:'float',4:'string',
                       5:'buffer',6:'array',7:'mapping',8:'pointer',
                       9:'function',10:'object',20:'loop_ctrl',255:'mixed'}
        ret = "void"
        if fname.startswith('query_') or fname.startswith('get_'): ret = "mixed"
        elif fname.startswith('is_') or fname.startswith('has_') or fname.startswith('can_'): ret = "int"
        elif fname.startswith('set_'): ret = "void"
        elif fname.startswith('check_') or fname.startswith('verify_'): ret = "int"
        elif fname.startswith('count_') or fname.startswith('num_'): ret = "int"
        elif fname.startswith('find_') or fname.startswith('search_'): ret = "mixed"
        elif fname.startswith('load_') or fname.startswith('read_'): ret = "mixed"
        elif fname.startswith('save_') or fname.startswith('write_'): ret = "void"
        elif fname.startswith('fill_') or fname.startswith('append_'): ret = "void"
        elif fname.startswith('handle_') or fname.startswith('do_'): ret = "void"
        elif fname.startswith('init_') or fname == 'create': ret = "void"
        elif fname.startswith('to_') or fname.startswith('parse_'): ret = "mixed"
        elif fname.startswith('format_') or fname.startswith('build_'): ret = "string"
        # Bytecode-based return type inference: check for ret with value on stack
        # ret (an=73) returns a value if the preceding instruction produces one
        VALUE_PRODUCING = {
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,  # push_*
            5, 6, 7, 8, 9,          # calls (produce return value)
            67, 68, 69, 70,         # member/range access
            1, 2, 25, 28, 30, 32, 37, 38, 39, 40, 41, 42, 48, 75, 81, 85,  # binary ops
            43, 45, 74, 79,         # unary ops (neg, not, rev, sizeof)
            3, 4,                   # assign (pushes assigned value)
            13,                     # cast
        }
        has_return_value = False
        for i, (an, p16, p8) in enumerate(insts):
            if an == 73:  # ret (returnFromFun)
                # Look backwards past nop instructions
                j = i - 1
                while j >= 0 and insts[j][0] in (0, 46):
                    j -= 1
                if j >= 0 and insts[j][0] in VALUE_PRODUCING:
                    has_return_value = True
                    break
        if has_return_value and ret == "void":
            ret = "mixed"

        # Override return type with real type from DLT (Vm_Fun_T.retType, offset 13)
        # Type values: see vm_valueType @ 0x0808A13C
        if seg_ret_type is not None:
            _ret_name = _type_names.get(seg_ret_type)
            if _ret_name and _ret_name != 'undefined':
                ret = _ret_name

        # Infer purpose
        purpose = infer_purpose(fname, insts)

        # Infer argument names and types from function name and usage.
        # Prefer real argument names extracted from DLT when available.
        arg_names = infer_arg_names(fname, nargs, insts, o.str_table)
        arg_types = infer_arg_types(fname, nargs, insts, o.str_table)
        if seg_args:
            # seg_args is a list of (vm_idx, name); vm_idx is 0-based arg index
            for vm_idx, aname in seg_args:
                if 0 <= vm_idx < nargs and aname:
                    arg_names[vm_idx] = aname
        # Override arg types with real types from DLT (Vm_Fun_Arg_T offset 5)
        # Type values: see vm_valueType @ 0x0808A13C
        # DLT type byte is authoritative; override for any valid type including
        # 'mixed'(255). Only skip 'undefined'(0)/'void'(1) which are not valid
        # parameter types. (Previously 'mixed' was skipped, leaving wrong
        # name-pattern hints like 'check_' -> 'string' even when DLT said mixed.)
        if seg_arg_types:
            for vm_idx, tb in seg_arg_types.items():
                if 0 <= vm_idx < nargs:
                    tname = _type_names.get(tb, 'mixed')
                    if tname not in ('undefined', 'void'):
                        arg_types[vm_idx] = tname

        # Patch 31: emit `type &name` for by-reference parameters.
        seg_arg_by_refs = {}
        if hasattr(o, 'func_arg_by_refs_by_seg'):
            seg_arg_by_refs = o.func_arg_by_refs_by_seg.get(ns, {})
        # Extract optional-argument default values from the function
        # header bytecode (push instructions between checkargs and enter).
        # Each pushed constant is the default for arg[min_arg + i].
        seg_defaults = extract_default_args(insts, seg_min_arg_no, nargs, o.str_table)
        params_parts = []  # body params (no default values)
        decl_params_parts = []  # forward declaration params (with defaults)
        for i in range(nargs):
            t = arg_types[i]
            n = arg_names[i]
            suffix = " = %s" % seg_defaults[i] if i in seg_defaults else ""
            if seg_arg_by_refs.get(i):
                params_parts.append("%s &%s" % (t, n))
                decl_params_parts.append("%s &%s%s" % (t, n, suffix))
            else:
                params_parts.append("%s %s" % (t, n))
                decl_params_parts.append("%s %s%s" % (t, n, suffix))
        params = ", ".join(params_parts)
        decl_params = ", ".join(decl_params_parts)

        # Build function modifiers from Vm_Fun_T.attrib (offset 8-9). Verified bits:
        #   bit 0 (0x001) = varargs   (vm_disassemble: MaxArg "+")
        #   bit 1 (0x002) = primitive (vm_disassemble; file-level `override`
        #                   directive sets it, not a per-function source keyword)
        #   bit 2 (0x004) = private   (vm_disassemble; vm_yyparse)
        #   bit 3 (0x008) = override  (vm_yyparse @ 0x8103DA1: test [edi+8], 8)
        #   bit 4 (0x010) = nomask    (vm_yyparse: bit 0x10 -> "can't be overrided")
        #   bit 8 (0x100) = hidden    (vm_disassemble: *(end+9) & 1)
        # Only real source-level modifiers are emitted (primitive/hidden are
        # driver-internal flags, not written in source).
        modifiers = []
        if seg_attrib & 0x8:
            modifiers.append("override")
        if seg_attrib & 0x10:
            modifiers.append("nomask")
        if seg_attrib & 0x4:
            modifiers.append("private")
        if seg_attrib & 0x1:
            modifiers.append("varargs")
        mod_prefix = (" ".join(modifiers) + " ") if modifiers else ""

        # Build forward declaration line
        decl_line = "%s%s %s(%s);" % (mod_prefix, ret, fname, decl_params)
        func_declarations.append(decl_line)

        # Collect function body output into a temporary list
        body_lines = []
        # Function separator and signature
        body_lines.append("// " + "-" * 68)
        if purpose:
            body_lines.append("// @brief  %s" % purpose)
        body_lines.append("// @code  nc=%d, nargs=%d, insts=%d" % (nc, nargs, len(insts)))
        body_lines.append("// " + "-" * 68)
        body_lines.append("%s%s %s(%s) {" % (mod_prefix, ret, fname, params))

        if mode in ('disasm', 'both'):
            body_lines.append("")
            body_lines.append("    /* ---- Bytecode Disassembly ---- */")
            targets = set()
            for ni, (an, p16, p8) in enumerate(insts):
                if an in (18, 31, 33, 44, 80):
                    t = ni + 1 + (p16 if p16 < 0x8000 else p16 - 0x10000)
                    if 0 <= t < nc: targets.add(t)
            for ni, (an, p16, p8) in enumerate(insts):
                if ni in targets:
                    body_lines.append("  L%04d:" % ni)
                body_lines.append("    [%3d] %s" % (ni, fmt_disasm_v37(an, p16, p8, ni, nc, strings, o.local_vars, func_meta, seg_local_vars, seg_args_dict)))
            body_lines.append("")

        if mode in ('pseudo', 'both'):
            body_lines.append("    /* ---- Enhanced Pseudocode ---- */")
            # Use full local_vars (same as disassembly) for consistent var name indices
            gen = PseudoGenV37(strings, o.local_vars, o.user_funcs, func_meta, ret_type=ret,
                               func_local_vars=seg_local_vars, func_table=o.func_table, func_args=seg_args_dict,
                               var_types=getattr(o, 'var_types', None),
                               inherit_idx_to_path=getattr(o, '_inherit_idx_to_path', None),
                               inherit_failed=getattr(o, '_inherit_failed', None),
                               inherit_ranges=getattr(o, '_inherit_ranges', None),
                               suppress_parent_path=(mode == 'pseudo'),
                               suppress_trailing_comments=(mode == 'pseudo'))
            pseudo_lines = gen.generate(insts, nc)
            if pseudo_lines:
                # Detect max local variable index used
                max_local = 0
                for an, p16, p8 in insts:
                    if an in (55, 57, 77) and p16 >= 0x8000:
                        li = -(p16 - 0x10000)
                        if li > max_local:
                            max_local = li
                # Emit local variable declarations (use inferred types when available)
                if max_local > 0:
                    for li in range(1, max_local + 1):
                        # Use real variable name when available
                        vm_idx = -li
                        real_name = seg_local_vars.get(vm_idx)
                        var_name_str = real_name if real_name else "local_%d" % li
                        var_type = gen.var_types.get(var_name_str, 'mixed')
                        # Patch 32: <loop_N> is the compiler-generated foreach loop
                        # context var (vm_newLoopContext @ 0x80bc780). It is an
                        # internal helper not in source; emit as comment.
                        if var_name_str.startswith('<loop_'):
                            body_lines.append("    //%s %s;" % (var_type, var_name_str))
                        else:
                            body_lines.append("    %s %s;" % (var_type, var_name_str))
                    body_lines.append("")
                for pl in pseudo_lines:
                    body_lines.append(pl)
            else:
                body_lines.append("    // (no statements)")
            body_lines.append("")

        body_lines.append("}")
        body_lines.append("")
        func_bodies.append(body_lines)

    # ---- Forward Declarations ----
    if func_declarations:
        lines.append("/* ================================================================")
        lines.append("   FORWARD DECLARATIONS")
        lines.append("   ================================================================ */")
        lines.append("")
        for decl in func_declarations:
            lines.append(decl)
        lines.append("")

    # ---- Global Variable Initializations (from init code segment) ----
    if init_lines:
        lines.extend(init_lines)

    # ---- Function Definitions ----
    lines.append("/* ================================================================")
    lines.append("   FUNCTION DEFINITIONS")
    lines.append("   ================================================================ */")
    lines.append("")
    for body_lines in func_bodies:
        lines.extend(body_lines)

    # ---- Functions without code ----
    if no_code_funcs:
        lines.append("/* ================================================================")
        lines.append("   DECLARED FUNCTIONS (no bytecode - likely inherited or forward decl)")
        lines.append("   ================================================================ */")
        lines.append("/*")
        for fname in no_code_funcs:
            decl = o.func_decl_info_by_name.get(fname)
            if decl:
                ret = _type_names.get(decl['ret_type'], 'mixed')
                if ret == 'undefined':
                    ret = 'mixed'
                seg_arg_types = decl.get('arg_types', {})
                seg_args = decl.get('args', [])
                nargs = len(seg_args)
                arg_names = ['arg%d' % i for i in range(nargs)]
                arg_types_list = ['mixed'] * nargs
                for vm_idx, aname in seg_args:
                    if 0 <= vm_idx < nargs and aname:
                        arg_names[vm_idx] = aname
                for vm_idx, tb in seg_arg_types.items():
                    if 0 <= vm_idx < nargs:
                        tname = _type_names.get(tb, 'mixed')
                        if tname != 'undefined':
                            arg_types_list[vm_idx] = tname
                # Patch 31: emit `type &name` for by-reference parameters.
                decl_arg_by_refs = decl.get('arg_by_refs', {})
                params_parts = []
                for i in range(nargs):
                    t = arg_types_list[i]
                    n = arg_names[i]
                    if decl_arg_by_refs.get(i):
                        params_parts.append("%s &%s" % (t, n))
                    else:
                        params_parts.append("%s %s" % (t, n))
                params = ", ".join(params_parts)
                seg_attrib = decl.get('attrib', 0)
                modifiers = []
                if seg_attrib & 0x8:
                    modifiers.append("override")
                if seg_attrib & 0x10:
                    modifiers.append("nomask")
                if seg_attrib & 0x4:
                    modifiers.append("private")
                if seg_attrib & 0x1:
                    modifiers.append("varargs")
                mod_prefix = (" ".join(modifiers) + " ") if modifiers else ""
                lines.append("   %s%s %s(%s);  // inherited from %s or forward declaration" % (
                    mod_prefix, ret, fname, params, o.inherited_func or 'parent'))
            else:
                lines.append("   %s();  // inherited from %s or forward declaration" % (
                    fname, o.inherited_func or 'parent'))
        lines.append("*/")
        lines.append("")

    # ---- Summary ----
    lines.append("/* ================================================================")
    lines.append("   DECOMPILATION SUMMARY")
    lines.append("   ================================================================")
    if constants:
        lines.append("   Constants extracted:         %d (HEX:%d INT:%d STR:%d CODE:%d REF:%d)" % (
            len(constants), const_stats.get('HEX', 0), const_stats.get('INT', 0),
            const_stats.get('STRING', 0), const_stats.get('CODE', 0), const_stats.get('REF', 0)))
    lines.append("   Total function names:        %d" % len(o.user_funcs))
    lines.append("   Code segments found:         %d" % len(segs))
    lines.append("   Successfully decoded:        %d" % decoded_count)
    lines.append("   Unresolved/minimal:          %d" % unresolved_count)
    lines.append("   Functions without code:      %d" % len(no_code_funcs))
    if getattr(o, 'hidden_funcs', None):
        lines.append("   Hidden functions parsed:     %d" % len(o.hidden_funcs))
        for hf in o.hidden_funcs:
            lines.append("      - %s (nodes=%d @ offset %d)" % (
                hf['name'], hf['nodes_count'], hf['nodes_start']))
    if getattr(o, 'inherit_names', None):
        lines.append("   Inherited programs:          %d" % len(o.inherit_names))
        for iname in o.inherit_names:
            lines.append("      - %s()" % iname)
    if len(no_code_funcs) > 0:
        lines.append("")
        lines.append("   Note: Functions without code are usually inherited from the")
        lines.append("   parent object (%s), forward declarations, or" % o.inherited_func)
        lines.append("   simple property accessor methods that the compiler optimizes away.")
    lines.append("   ================================================================ */")

    # ---- Post-process: comment out unused global_N placeholder variables ----
    # Placeholder names like global_0 arise when DLT has no name entry for a
    # variable slot (common in global programs where slots 0/1 are reserved).
    # If such a placeholder is never referenced anywhere in the output, emit
    # its declaration as a comment: /* mixed global_0; */
    # Single pass over the lines to count references (O(n) instead of O(n^2)).
    try:
        _ph_decl_re = re.compile(r'^(\s+)([A-Za-z_]\w*)\s+(global_\d+);\s*$')
        _ph_decls = {}
        for _li, _line in enumerate(lines):
            _m = _ph_decl_re.match(_line)
            if _m:
                _ph_decls[_li] = (_m.group(1), _m.group(2), _m.group(3))
        if _ph_decls:
            _ref_counts = {}
            for _line in lines:
                for _vn in re.findall(r'\bglobal_\d+\b', _line):
                    _ref_counts[_vn] = _ref_counts.get(_vn, 0) + 1
            for _li, (_ind, _vt, _vn) in _ph_decls.items():
                # Only the declaration line itself references the placeholder
                # (count == 1), so it is unused and can be commented out.
                if _ref_counts.get(_vn, 0) <= 1:
                    lines[_li] = "%s/* %s %s; */" % (_ind, _vt, _vn)
    except Exception:
        pass

    # Write output
    if out_path is None:
        out_path = filepath.rsplit('.', 1)[0] + '_v37.c'
    os.makedirs(os.path.dirname(out_path) if os.path.dirname(out_path) else '.', exist_ok=True)
    # 默认输出 GBK (gb2312), 保持与 .o 文件原始编码一致
    # 使用 surrogateescape 保证原始字节无损往返：解码时无法识别的字节保留为
    # 孤立 surrogate，编码回 gb2312 时自动还原为原始字节。
    if encoding == 'utf-8':
        # -u: UTF-8 BOM 输出 (符合文档规则: 如必须用 utf8 则必须用 utf8 bom)
        with open(out_path, 'w', encoding='utf-8-sig', errors='surrogateescape') as f:
            f.write('\n'.join(lines))
    else:
        with open(out_path, 'w', encoding='gb2312', errors='surrogateescape') as f:
            f.write('\n'.join(lines))

    return decoded_count, len(o.user_funcs)


def build_sefun_table(filepath, encoding='gb2312'):
    """Parse a startup .o file (e.g. start_aaa.o) and extract its function
    table as a sefun (simulated efun) name mapping.

    Returns {idx: name} for all function table indices >= len(EFUN_NAMES).
    Returns empty dict if parsing fails.
    """
    try:
        o = LPCOFile(filepath, encoding)
        o.parse()
        return {idx: name for idx, name in o.func_table.items() if idx >= len(EFUN_NAMES)}
    except Exception as e:
        print("Warning: failed to parse sefun file %s: %s" % (filepath, e))
        return {}


def build_global_var_table(filepath, encoding='gb2312'):
    """Parse a startup .o file (e.g. start_aaa.o) and extract its variable
    table as a global variable name mapping.

    Returns {idx: name} for all named variables (placeholder names like
    global_<digits> are skipped). Returns empty dict if parsing fails.
    """
    try:
        o = LPCOFile(filepath, encoding)
        o.parse()
        lvars = o.local_vars
        out = {}
        for idx, name in enumerate(lvars):
            if name and not re.match(r'^global_\d+$', name):
                out[idx] = name
        return out
    except Exception as e:
        print("Warning: failed to parse global var file %s: %s" % (filepath, e))
        return {}


# ============================================================================
# Batch mode: multiprocessing support
# ============================================================================
def _batch_worker_init(variant, sefun_table, global_var_table):
    """Per-worker initialization: set EFUN variant and reuse parsed tables.

    Workers are separate processes (spawn on Windows), so module-level globals
    are (re)initialized here. The sefun/global-var tables are parsed once in
    main() and passed in, avoiding a duplicate startup-file parse per worker.
    """
    global EFUN_NAMES, EFUN_TABLE, SEFUN_TABLE, GLOBAL_VAR_TABLE, _PARENT_TABLE_CACHE
    if variant != 'v37':
        EFUN_NAMES = _load_efun_names(variant)
        EFUN_TABLE = {i: name for i, name in enumerate(EFUN_NAMES)}
    SEFUN_TABLE = sefun_table or {}
    GLOBAL_VAR_TABLE = global_var_table or {}
    # Each worker keeps its own parent-parse cache for the whole batch.
    _PARENT_TABLE_CACHE.clear()


# Workers are long-lived, so run a periodic gc per worker to bound memory.
_BATCH_TASK_COUNTER = 0


def _batch_decompile_one(task):
    """Decompile a single .o file in a worker process.

    Returns a tuple: ('ok', fp, out_path, decoded, func_cnt) on success or
    ('err', fp, out_path, err_msg, traceback) on failure. Exceptions are
    caught here so one bad file never aborts the whole batch.
    """
    global _BATCH_TASK_COUNTER
    fp, out_path, mode, encoding = task
    try:
        decoded, func_cnt = decompile_file_v37(fp, out_path, mode, encoding)
        _BATCH_TASK_COUNTER += 1
        if _BATCH_TASK_COUNTER % 50 == 0:
            gc.collect()
        return ('ok', fp, out_path, decoded, func_cnt)
    except Exception as e:
        import traceback
        return ('err', fp, out_path, str(e), traceback.format_exc())


def main():
    import argparse
    ap = argparse.ArgumentParser(description='LPC .o Decompiler v37 - Readability Enhanced')
    ap.add_argument('path', help='Path to .o file or directory')
    ap.add_argument('-o', '--output', default=None,
                    help='Output file (single .o input) or output directory (directory input)')
    ap.add_argument('-r', '--recursive', action='store_true', help='Recursive directory scan')
    ap.add_argument('-m', '--mode', default='pseudo', choices=['disasm', 'pseudo', 'both'],
                    help='Output mode: disasm (bytecode only), pseudo (default), or both')
    ap.add_argument('-u', '--utf8', action='store_true',
                    help='Use UTF-8 (with BOM) for reading and writing instead of default GBK (gb2312)')
    ap.add_argument('-q', '--sefun', default=None,
                    help='Startup .o file (e.g. start_aaa.o) to build sefun name table; '
                         'when absent or not found, global_fn_<idx> is used')
    ap.add_argument('--old', action='store_true',
                    help='Use the legacy (magic_Linux32) EFUN table from efun_table.json '
                         '(old variant, 561 entries) instead of the v37 (572 entries) table')
    ap.add_argument('-j', '--jobs', type=int, default=0,
                    help='Number of parallel worker processes for batch (directory) mode; '
                         '0 = auto (default: min(2, CPU count))')
    args = ap.parse_args()
    out_encoding = 'utf-8' if args.utf8 else 'gb2312'

    global SEFUN_TABLE, GLOBAL_VAR_TABLE, EFUN_NAMES, EFUN_TABLE
    if args.old:
        EFUN_NAMES = _load_efun_names('old')
        EFUN_TABLE = {i: name for i, name in enumerate(EFUN_NAMES)}
    print("EFUN: %d entries loaded from efun_table.json (%s variant)" %
          (len(EFUN_NAMES), 'old' if args.old else 'v37'), flush=True)
    if args.sefun:
        if os.path.exists(args.sefun):
            SEFUN_TABLE = build_sefun_table(args.sefun, out_encoding)
            GLOBAL_VAR_TABLE = build_global_var_table(args.sefun, out_encoding)
            print("SEFUN: %d entries loaded from %s" % (len(SEFUN_TABLE), args.sefun), flush=True)
            print("GLOBAL_VAR: %d entries loaded from %s" % (len(GLOBAL_VAR_TABLE), args.sefun), flush=True)
        else:
            SEFUN_TABLE = {}
            GLOBAL_VAR_TABLE = {}
            print("SEFUN: startup file not found: %s (using global_fn_<idx>)" % args.sefun, flush=True)
            print("GLOBAL_VAR: startup file not found: %s (using global_<idx>)" % args.sefun, flush=True)

    if not os.path.exists(args.path):
        print("Path not found: %s" % args.path)
        return 1

    files_processed = 0
    total_decoded = 0
    total_funcs = 0

    if os.path.isfile(args.path):
        # Single file mode: -o is output FILE path (not a directory)
        if args.output:
            if os.path.isdir(args.output) or args.output.endswith(os.sep) or args.output.endswith('/'):
                base = os.path.splitext(os.path.basename(args.path))[0]
                out_path = os.path.join(args.output, base + '_v37.c')
            else:
                out_path = args.output
        else:
            out_path = args.path.rsplit('.', 1)[0] + '_v37.c'
        try:
            decoded, func_cnt = decompile_file_v37(args.path, out_path, args.mode, out_encoding)
            total_decoded = decoded
            total_funcs = func_cnt
            files_processed = 1
            print("[%s] %d/%d functions decoded -> %s" % (os.path.basename(args.path), decoded, func_cnt, out_path))
        except Exception as e:
            import traceback
            print("[%s] ERROR: %s" % (os.path.basename(args.path), e))
            traceback.print_exc()
    else:
        # Directory mode: -o is base output directory, mirror source structure
        files = []
        if args.recursive:
            for dp, _, fns in os.walk(args.path):
                for f in fns:
                    if f.endswith('.o'): files.append(os.path.join(dp, f))
        else:
            for f in os.listdir(args.path):
                if f.endswith('.o'): files.append(os.path.join(args.path, f))

        if not files:
            print("No .o files found")
            return 1

        out_dir = args.output or os.path.join(args.path, 'decompiled_v37')
        os.makedirs(out_dir, exist_ok=True)

        n_files = len(files)
        # Parallel worker count: -j wins, else min(2, CPU count) for a balanced
        # CPU/elapsed-time tradeoff. Each worker reuses its parent-parse cache
        # across the whole batch.
        jobs = args.jobs if args.jobs and args.jobs > 0 else min(2, os.cpu_count() or 1)
        print("Batch: %d .o file(s) -> %s (jobs=%d)" % (n_files, out_dir, jobs), flush=True)
        import time as _time
        batch_start = _time.time()

        tasks = []
        for fp in files:
            # Mirror relative path structure to prevent filename collisions
            rel = os.path.relpath(fp, args.path)
            base = os.path.splitext(rel)[0]
            tasks.append((fp, os.path.join(out_dir, base + '_v37.c'), args.mode, out_encoding))

        import multiprocessing as mp
        # Workers are long-lived (no maxtasksperchild) to avoid repeated
        # spawn + table-copy overhead; each worker runs a periodic gc to bound
        # per-process memory. chunksize batches tasks to reduce IPC overhead.
        with mp.Pool(processes=jobs, initializer=_batch_worker_init,
                     initargs=('old' if args.old else 'v37',
                               SEFUN_TABLE, GLOBAL_VAR_TABLE)) as pool:
            for done, res in enumerate(pool.imap_unordered(_batch_decompile_one, tasks, chunksize=8), 1):
                if res[0] == 'ok':
                    _, fp, out_path, decoded, func_cnt = res
                    total_decoded += decoded
                    total_funcs += func_cnt
                    files_processed += 1
                else:
                    _, fp, out_path, err, tb = res
                    print("[%d/%d] %s ERROR: %s" % (done, n_files, os.path.basename(fp), err), flush=True)
                    print(tb, flush=True)
                # Print progress periodically instead of per file: with fast
                # per-file tasks, per-file console output throttles the workers.
                if done % 25 == 0 or done == n_files:
                    elapsed = _time.time() - batch_start
                    print("[%d/%d] processed (%.0fs, %d/%d funcs decoded)" % (
                        done, n_files, elapsed, total_decoded, total_funcs), flush=True)

    print("\n=== Summary ===")
    print("Files processed: %d" % files_processed)
    print("Total functions declared: %d" % total_funcs)
    print("Total decoded: %d" % total_decoded)
    if total_funcs > 0:
        rate = min(100, total_decoded * 100 // max(1, total_funcs))
        print("Decode rate: %d%%" % rate)


if __name__ == '__main__':
    main()
