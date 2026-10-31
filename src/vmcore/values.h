// ===========================================================================
// vmcore/values.h — 值语义 (已实锋试错成果, 原样搬运)
//
// 这些函数的语义全部经过实机验证（32 位 int 环绕、float32 混合运算、
// mapping<< 原地合并、GBK 字节区间、(int)buffer 大端累加……）——
// 搬运自原实现，剥离诊断探测与 A/B 开关，语义一字不改。
// ===========================================================================
#pragma once
#include <cstdint>
#include <string>

#include "value.h"

namespace lpc {
namespace vmcore {

int64_t toInt(const Value& v);
double toReal(const Value& v);
// vm_isZero (0x808cae6): ONLY number-0, real-0.0, undefined(void) and nil are
// zero.  Arrays, mappings, buffers, strings and functions are ALWAYS non-zero.
bool isZero(const Value& v);
// our variant index -> original VM type number (is_type operand).
int vmTypeOf(const Value& v);

Value addValues(const Value& a, const Value& b);
Value subValues(const Value& a, const Value& b);
Value mulValues(const Value& a, const Value& b);
Value divValues(const Value& a, const Value& b);
Value modValues(const Value& a, const Value& b);
Value lshValues(const Value& a, const Value& b);
Value rshValues(const Value& a, const Value& b);
Value andValues(const Value& a, const Value& b);
Value orValues(const Value& a, const Value& b);
Value xorValues(const Value& a, const Value& b);
// '|' / '|=' on arrays = set union (load_ban_words relies on it).
Value lpcOrValues(const Value& a, const Value& b);
Value negValue(const Value& a);

int compareValues(const Value& a, const Value& b);
// vm_compareValue (0x80bbcb4): type-number order, string strtol rule,
// buffer memcmp, structures by pointer identity.
int compareValuesOriginal(const Value& a, const Value& b);

// vm_castType: 2 int, 3 real, 4 string, 5 buffer, 6 array, 7 mapping.
Value castValue(Value v, int targetType);

// vm_queryMember (0x808fa60): mapping/array/string/buffer member read.
Value queryMemberValue(const Value& container, const Value& key);
// vm_queryRange (0x808fd04): byte-based ranges (strings are BYTE sliced).
Value queryRangeValue(const Value& container, const Value& from, const Value& to,
                      int flag, bool asPtr);

} // namespace vmcore
} // namespace lpc
