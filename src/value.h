#pragma once
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace lpc {
extern std::atomic<int64_t> g_mapInsertCount;
// LPCVM_MEMSTAT: exact live LPC mapping entries/buckets (O(1) per
// container, never per element) so the memory census can attribute the
// resident set to the game data rather than guessing at averages.
extern std::atomic<int64_t> g_liveMapEntries;
extern std::atomic<int64_t> g_liveMapBuckets;

// ---- [vpool] Value 对象线程本地自由链池 ------------------------------------
// 采样实证堆分配/释放占 VM 执行 ~40% (羽化 burst 每 tick 数万次
// ArrayValue/MappingValue/SharedString::Block 的 new+delete)。每个类型
// 一条 thread_local 自由链, 32 个一块地向 malloc 批量取; push/pop 是两次
// 指针读写。语义零变化: 对象照常构造/析构, 只换内存来源。跨线程把块
// 还到别的线程的链上也没问题 (内存就是内存)。
template <class T>
struct VmPool {
    static void*& list() {
        static thread_local void* fl = nullptr;
        return fl;
    }
    static T* pop() {
        void*& fl = list();
        if (!fl) {
            enum { CHUNK = 32 };
            constexpr size_t kStride =
                (sizeof(T) + alignof(T) - 1) / alignof(T) * alignof(T);
            char* base = static_cast<char*>(std::malloc(CHUNK * kStride));
            if (!base) throw std::bad_alloc();
            for (int i = 0; i < CHUNK; ++i) {
                void* p = base + i * kStride;
                *static_cast<void**>(p) =
                    (i + 1 < CHUNK) ? base + (i + 1) * kStride : nullptr;
            }
            fl = base;
        }
        void* p = fl;
        fl = *static_cast<void**>(p);
        return static_cast<T*>(p);
    }
    static void push(void* p) {
        void*& fl = list();
        *static_cast<void**>(p) = fl;
        fl = p;
    }
};

// ---- [vpool] end -----------------------------------------------------------

// ---- [ref] 侵入式引用计数 + 非原子智能指针 ----------------------------------
// 对齐原版 vm 的值模型: 原版 Vm_Value 是 24B POD, 计数是普通 ++/-- (原版
// vm_getVmMutex 单一大锁保证同刻只有一个线程碰值)。shared_ptr 的控制块
// 分配 + seq_cst 原子增减是采样实证的每指令 ~700ns 里的最大单项 (原版
// ~100ns)。这里换成: 计数内嵌对象 (零控制块分配) + 非原子增减。
//
// 线程纪律 (vmValueMutex, 对齐原版 vm_getVmMutex):
//   * 调度线程每次派发 (runProcess) 全程持 vmValueMutex;
//   * 外部线程 (网络/telnet/mysql) 只允许在持锁窗口内 拷贝/交接/销毁 Value
//     (execOnMain 的入参拷贝、结果移交、Registry 外部查询);
//   * execOnMain 型进程结束时必须先清空 pcb 携带的全部 Value 再唤醒等待者,
//     保证调用方 return 后析构自己那份时无人并发计数。
// 违反纪律 = 数据竞争; 编译器不拦, 靠上述两处锁纪律兜底。
std::recursive_mutex& vmValueMutex();

struct RefCountedBase {
    // [fix-atomic-refs] 容器计数原子化。原版是普通 ++/-- (vm_getVmMutex
    // 单一大锁保证同刻一个线程碰值); 本实现没有全局值锁, Value 跨线程
    // 流转 (网络收包线程 -> 调度器) 时非原子计数竞争 -> double release ->
    // 池 double push -> 自由链损坏 (gs 运行 14.5min 崩 VmPool<BufferValue>
    // ::pop 读到已释放页, 实锤)。SharedString::Block 同因早已原子化。
    std::atomic<int32_t> refs{1};   // 创建即持有 1 份 (adopt 语义)
    void retain() noexcept { refs.fetch_add(1, std::memory_order_relaxed); }
};
// Value 析构的类型化释放 —— 实现在 value.h 尾部 (容器 struct 完整定义后)。
void destroyValueObject(uint8_t t, RefCountedBase* p) noexcept;
// [map-bad] VT_MAP delete 取证: LPCVM_MAPDEL=1 时 dump this+符号化调用栈
// (dbghelp, main 已 prewarm)。定位悬空 mapping 的 delete 现场。
extern const bool g_mapDelTraceOn;
void mapDeleteTrace(const void* self) noexcept;

template <class T>
struct RefBase : RefCountedBase {
    void release() noexcept {
        if (refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete static_cast<T*>(this);
    }
protected:
    RefBase() = default;
    // 拷贝构造 = 新对象 (基类 refs 重置 1) —— 供派生类本体克隆
    // (new ClosureValue(*src) 等) 使用; atomic 成员需手写 (=default 不可拷贝)。
    RefBase(const RefBase&) noexcept : RefCountedBase() {
        refs.store(1, std::memory_order_relaxed);
    }
    RefBase& operator=(const RefBase&) = delete;
    ~RefBase() = default;
};

// 非原子侵入式指针。语义子集 = shared_ptr: 拷贝/移动/判空/->/*/get/
// use_count。没有 weak/别名/自定义删除器 (全仓审计无使用)。
template <class T>
class Ref {
public:
    Ref() noexcept = default;
    Ref(std::nullptr_t) noexcept {}
    static Ref adopt(T* p) noexcept {   // 接管一个 refs==1 的新对象
        Ref r;
        r.p_ = p;
        return r;
    }
    explicit Ref(T* p) noexcept : p_(p) {}   // adopt 形式的直接构造
    Ref(const Ref& o) noexcept : p_(o.p_) { if (p_) p_->retain(); }
    Ref(Ref&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
    Ref& operator=(const Ref& o) noexcept {
        if (p_ != o.p_) {
            if (o.p_) o.p_->retain();
            if (p_) p_->release();
            p_ = o.p_;
        }
        return *this;
    }
    Ref& operator=(Ref&& o) noexcept {
        if (p_ != o.p_) {
            if (p_) p_->release();
            p_ = o.p_;
            o.p_ = nullptr;
        }
        return *this;
    }
    ~Ref() { if (p_) p_->release(); }

    T* get() const noexcept { return p_; }
    T* operator->() const noexcept { return p_; }
    T& operator*() const noexcept { return *p_; }
    explicit operator bool() const noexcept { return p_ != nullptr; }
    bool operator!() const noexcept { return p_ == nullptr; }
    int32_t use_count() const noexcept {
        return p_ ? p_->refs.load(std::memory_order_relaxed) : 0;
    }
    void reset() noexcept { if (p_) { p_->release(); p_ = nullptr; } }
    template <class U> bool operator==(const Ref<U>& o) const noexcept { return p_ == o.get(); }
    template <class U> bool operator!=(const Ref<U>& o) const noexcept { return p_ != o.get(); }
    bool operator==(std::nullptr_t) const noexcept { return p_ == nullptr; }
    bool operator!=(std::nullptr_t) const noexcept { return p_ != nullptr; }
private:
    T* p_ = nullptr;
};

template <class T, class... A>
Ref<T> make_ref(A&&... a) {
    return Ref<T>::adopt(new T(std::forward<A>(a)...));
}

// ---- [ref] end -------------------------------------------------------------

// Recursion-depth guard for the recursive C++ value walkers (packet padding,
// deep copy, printing, save/restore, comparisons).  A CYCLIC LPC value (an
// array that contains itself, which shared_ptr semantics allow) makes them
// recurse until the C++ stack is exhausted and the whole service dies with
// 0xC00000FD (STACK_OVERFLOW) - observed while gs reconnected a character:
// userd::reconnected -> ask_fields->main -> me->send(0x211B, buf) and then a
// silent 0xc00000fd with no LPC frames involved (the LPC call-depth guard
// never fired).  Capping the depth turns that into one reported line plus a
// truncated value, so the service survives and the cycle is visible.
struct ValueDepthGuard {
    static int& depth() { static thread_local int d = 0; return d; }
    bool ok;
    explicit ValueDepthGuard(const char* what) : ok(depth() < 192) {
        if (!ok) {
            static std::atomic<int> reported{0};
            if (reported.fetch_add(1) < 20) {
                std::fprintf(stderr, "[cdepth] recursion >192 in %s - cyclic value? bailing out\n", what);
            }
        } else {
            ++depth();
        }
    }
    ~ValueDepthGuard() { if (ok) --depth(); }
    ValueDepthGuard(const ValueDepthGuard&) = delete;
    ValueDepthGuard& operator=(const ValueDepthGuard&) = delete;
};

// Ring of the last entered LPC function names.  The C++ [crash] handler prints
// it: a C++-level recursion (a cyclic value walker) never pushes an LPC frame,
// so the VM's frame chain alone cannot say where the process died.
constexpr int kLpcRingSize = 64;
extern const char* g_lpcRing[kLpcRingSize];
extern std::atomic<unsigned> g_lpcRingPos;
inline void lpcRingPush(const char* fn) {
    g_lpcRing[g_lpcRingPos.fetch_add(1, std::memory_order_relaxed) % kLpcRingSize] = fn;
}

struct ArrayValue;
struct MappingValue;
struct ObjectValue;
struct ClosureValue;
namespace vmcore { class VmObject; }
struct LoopCtrl;
struct BufferValue;
struct Program;

// ---------------------------------------------------------------------------
// SharedString: the string slot of a Value.
//
// The original driver stores strings as reference-counted shared objects
// (push/assign/copy of a string value only moves a pointer and bumps a
// count).  std::string's copy semantics deep-copied the whole buffer on every
// push/assignment/parameter/local read, which showed up as heavy malloc +
// memcpy + free churn wherever Values holding text were duplicated (mapping
// inserts, array pushes, stack traffic, save/serialize).
//
// SharedString reproduces the shared model: the text lives in a heap Block
// whose refcount is shared by every copy of the SharedString.  Copying a
// SharedString is one atomic increment; destroying one is one decrement.
// Strings are IMMUTABLE - there is no in-place mutation on this type.  Code
// that needs an independent, writable buffer copies explicitly (e.g. through
// the std::string materialization below, .substr(), ...).
//
// The Block stores a real std::string, which gives read sites a zero-copy
// window onto the text: str()/the implicit conversion to const std::string&
// bind straight to the block's string, so existing code that reads a string
// Value through a `const std::string&` keeps working without byte copies.
// Each freshly-created string costs one block allocation (small strings ride
// the std::string SSO inside the block).  The refcount is atomic because
// Values cross scheduler/telnet threads (a producer on one thread may still
// hold a copy while another thread destroys its own).
// ---------------------------------------------------------------------------
class SharedString {
public:
    struct Block {
        std::atomic<int32_t> refs;   // [bisect-A] 暂恢复原子, 定位悬空块来源
        std::string s;
        explicit Block(std::string&& v) : refs(1), s(std::move(v)) {}
        // [vpool] Block 高频分配走池 (所有 new/delete 站点自动接管)
        static void* operator new(std::size_t) { return VmPool<Block>::pop(); }
        static void operator delete(void* p) noexcept { VmPool<Block>::push(p); }
    };

    SharedString() noexcept : b_(nullptr) {}
    SharedString(const char* s) : b_(nullptr) {
        if (s && s[0]) b_ = new Block(std::string(s));
    }
    SharedString(const std::string& s) : b_(nullptr) {
        if (!s.empty()) b_ = new Block(std::string(s));
    }
    SharedString(std::string&& s) : b_(nullptr) {
        if (!s.empty()) b_ = new Block(std::move(s));
    }
    SharedString(const SharedString& o) noexcept : b_(o.b_) { retain(); }
    SharedString(SharedString&& o) noexcept : b_(o.b_) { o.b_ = nullptr; }
    SharedString& operator=(const SharedString& o) noexcept {
        if (this != &o) {
            SharedString t(o);
            release();
            b_ = t.b_;
            t.b_ = nullptr;
        }
        return *this;
    }
    SharedString& operator=(SharedString&& o) noexcept {
        if (this != &o) {
            release();
            b_ = o.b_;
            o.b_ = nullptr;
        }
        return *this;
    }
    ~SharedString() { release(); }

    // True when this slot does not hold a text payload.  (An empty string is
    // represented by a null block, exactly like an empty std::string slot.)
    bool empty() const noexcept { return b_ == nullptr || b_->s.empty(); }
    // True when both hold the SAME text block (identical payload, including
    // both empty).  Lets equality checks short-circuit before comparing
    // contents - LPC boots compare many strings that alias the same block.
    bool sameBlock(const SharedString& o) const noexcept { return b_ == o.b_; }
    // Reference count of the underlying text block (0 when empty/null).
    size_t blockRefs() const noexcept {
        return b_ ? static_cast<size_t>(b_->refs.load(std::memory_order_relaxed)) : 0;
    }
    // Append more text IN PLACE to this string's block.  Caller must ensure
    // the block is safe to mutate (single logical owner): use_count == 1
    // after the callee's own copy is accounted for (see op_eq_value += path).
    // Returns false when there is no block (empty string) - caller falls back
    // to the copy path.
    bool appendInPlace(const std::string& more) noexcept {
        if (!b_) return false;
        b_->s.append(more);
        return true;
    }
    size_t size() const noexcept { return b_ ? b_->s.size() : 0; }
    const char* c_str() const noexcept { return b_ ? b_->s.c_str() : kEmpty.c_str(); }
    const char* data() const noexcept { return b_ ? b_->s.data() : kEmpty.data(); }
    char operator[](size_t i) const noexcept { return str()[i]; }

    // Read-only view of the text.  Binds to the block's std::string with no
    // copy; do not keep the reference past this SharedString's last copy
    // being destroyed (same lifetime rule as today's variant slot).
    const std::string& str() const noexcept { return b_ ? b_->s : kEmpty; }

    // Implicit read interop: lets a SharedString flow into code that expects
    // const std::string& (function parameters, bindings, std::string member
    // calls) without copying bytes.  Copying INTO an owned std::string still
    // happens for std::string by-value targets, which is the explicit-copy
    // boundary for code that wants an independent buffer.
    operator const std::string&() const noexcept { return str(); }

    using const_iterator = std::string::const_iterator;
    const_iterator begin() const noexcept { return str().begin(); }
    const_iterator end() const noexcept { return str().end(); }

private:
    static const std::string kEmpty;

    void retain() noexcept {
        if (b_) b_->refs.fetch_add(1, std::memory_order_relaxed);
    }
    void release() noexcept {
        if (b_ && b_->refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete b_;
    }
    Block* b_;
};

// std::string's comparison operators are templates and cannot deduce through
// SharedString, so provide our own, comparing through the underlying text
// (str() binds the block's std::string with no copy).
inline bool operator==(const SharedString& a, const SharedString& b) noexcept {
    return a.sameBlock(b) || a.str() == b.str();
}
inline bool operator!=(const SharedString& a, const SharedString& b) noexcept {
    return !(a.sameBlock(b) || a.str() == b.str());
}
inline bool operator<(const SharedString& a, const SharedString& b) noexcept { return a.str() < b.str(); }
inline bool operator<=(const SharedString& a, const SharedString& b) noexcept { return a.str() <= b.str(); }
inline bool operator>(const SharedString& a, const SharedString& b) noexcept { return a.str() > b.str(); }
inline bool operator>=(const SharedString& a, const SharedString& b) noexcept { return a.str() >= b.str(); }
inline bool operator==(const SharedString& a, const char* b) noexcept { return a.str() == b; }
inline bool operator!=(const SharedString& a, const char* b) noexcept { return a.str() != b; }
inline bool operator<(const SharedString& a, const char* b) noexcept { return a.str() < b; }
inline bool operator<=(const SharedString& a, const char* b) noexcept { return a.str() <= b; }
inline bool operator>(const SharedString& a, const char* b) noexcept { return a.str() > b; }
inline bool operator>=(const SharedString& a, const char* b) noexcept { return a.str() >= b; }
inline bool operator==(const char* a, const SharedString& b) noexcept { return a == b.str(); }
inline bool operator!=(const char* a, const SharedString& b) noexcept { return a != b.str(); }
inline bool operator<(const char* a, const SharedString& b) noexcept { return a < b.str(); }
inline bool operator<=(const char* a, const SharedString& b) noexcept { return a <= b.str(); }
inline bool operator>(const char* a, const SharedString& b) noexcept { return a > b.str(); }
inline bool operator>=(const char* a, const SharedString& b) noexcept { return a >= b.str(); }
inline bool operator==(const SharedString& a, const std::string& b) noexcept { return a.str() == b; }
inline bool operator!=(const SharedString& a, const std::string& b) noexcept { return a.str() != b; }
inline bool operator<(const SharedString& a, const std::string& b) noexcept { return a.str() < b; }
inline bool operator<=(const SharedString& a, const std::string& b) noexcept { return a.str() <= b; }
inline bool operator>(const SharedString& a, const std::string& b) noexcept { return a.str() > b; }
inline bool operator>=(const SharedString& a, const std::string& b) noexcept { return a.str() >= b; }
inline bool operator==(const std::string& a, const SharedString& b) noexcept { return a == b.str(); }
inline bool operator!=(const std::string& a, const SharedString& b) noexcept { return a != b.str(); }
inline bool operator<(const std::string& a, const SharedString& b) noexcept { return a < b.str(); }
inline bool operator<=(const std::string& a, const SharedString& b) noexcept { return a <= b.str(); }
inline bool operator>(const std::string& a, const SharedString& b) noexcept { return a > b.str(); }
inline bool operator>=(const std::string& a, const SharedString& b) noexcept { return a >= b.str(); }

struct UndefinedTag {};

// A cross-VM object-variable reference passed as a call_other argument.
// The original driver's reference parameters (&var) point straight at the
// CALLER's instance variable; a callee's assignment writes through it.  Our
// registry-VM path passes deref'd VALUES, which works for shared_ptr-backed
// mappings (in-place mutation) but not for arrays REASSIGNED with `=`/`+=`
// (a fresh array is built and the caller's variable never changes) - see
// importd.create_data(file, &attrib_index, &attrib_list): attrib_list (a
// mapping) survives, attrib_index (an array) never gets written back, so
// import/npc.o's import() sees an empty attrib_index and raw NPCs (守护天神
// etc.) never receive their npc.list attributes.  Wrapping such arguments in
// a CrossVmRef makes every callee-side read/write forward to the caller's
// object variable (the slot keeps the CrossVmRef, so repeated `+=` all land
// in the caller's variable).
struct CrossVmRef : RefBase<CrossVmRef> {
    void* vm = nullptr;        // OriginalVm* of the caller (opaque to value.h)
    int objIndex = -1;         // index into the caller's objVars (or -1)
    int slotIndex = -1;        // index into the caller's stack (local slot; or -1)
};

// ---- [pvalue] 扁平 tagged union Value（替代 std::variant）--------------------
// 采样实证: variant 的 _Variant_raw_visit / 构造析构是每指令 ~700ns 的稳定
// 构成 —— 解释器每条指令 push/pop 2-4 次 Value。原版 Vm_Value 是 24B POD。
// 16B 平铺: 1B 类型码 + union, 类型码与原 variant 的 index 一一对应 (681 处
// .index() 判断编号不变)。计数手工管理:
//   VT_STR          -> SharedString 原地内嵌 (placement, 原子块计数)
//   VT_ARR..VT_XREF -> RefCountedBase* 裸指针 (非原子侵入式计数)
// vget<T> 取代 std::get<T>; vgetI<N> 取代 std::get<N>。
enum VTypeCode : uint8_t {
    VT_NIL = 0, VT_UNDEF = 1, VT_INT = 2, VT_FLOAT = 3, VT_STR = 4,
    VT_ARR = 5, VT_MAP = 6, VT_OBJ = 7, VT_CLO = 8, VT_LOOP = 9,
    VT_BUF = 10, VT_XREF = 11
};
template <class T> struct VTypeCodeOf;
template <> struct VTypeCodeOf<ArrayValue>  { static constexpr uint8_t code = VT_ARR; };
template <> struct VTypeCodeOf<MappingValue>{ static constexpr uint8_t code = VT_MAP; };
template <> struct VTypeCodeOf<ObjectValue> { static constexpr uint8_t code = VT_OBJ; };
template <> struct VTypeCodeOf<ClosureValue>{ static constexpr uint8_t code = VT_CLO; };
template <> struct VTypeCodeOf<LoopCtrl>    { static constexpr uint8_t code = VT_LOOP; };
template <> struct VTypeCodeOf<BufferValue> { static constexpr uint8_t code = VT_BUF; };
template <> struct VTypeCodeOf<CrossVmRef>  { static constexpr uint8_t code = VT_XREF; };

class Value {
public:
    Value() noexcept : t_(VT_NIL), i_(0) {}
    Value(std::monostate) noexcept : Value() {}
    Value(UndefinedTag) noexcept : t_(VT_UNDEF), i_(0) {}
    Value(int64_t v) noexcept : t_(VT_INT), i_(v) {}
    Value(double v) noexcept : t_(VT_FLOAT), d_(v) {}
    Value(SharedString s) noexcept : t_(VT_STR), i_(0) {
        // [fix-emptystr] 空串是合法字符串 (原版 type 4), 不得折叠成 NIL ——
        // 否则 mudlib 侧所有空串值 (read_ini 缺键/trim("")/空文件行) 全部
        // 变 undefined: as_string(undefined) 渲染 "(undefined)" 洗成真串,
        // strlen 变 11 —— dbd 把 "(undefined)" 拼进 mysql host, AAA 的
        // post_init 永不排空 (8001/8160 不开)。SharedString 空文本 =
        // b_=nullptr 槽, 拷贝/析构/str() 全路径安全。
        new (strSlot()) SharedString(std::move(s));
    }
    template <class T>
    Value(const lpc::Ref<T>& r) noexcept : t_(VTypeCodeOf<T>::code), i_(0) {
        // [fix-refown] 持有一份属于本 Value 的引用。原版(by value 形参 +
        // "直接接管") 从 lvalue Ref 构造时形参拷贝 retain 的一份会被形参
        // 析构 release 掉 —— Value 净获得 0 份却会在析构时 release 一次,
        // 净 -1: op22/op24 压栈的数组/mapping 天生少一份计数, 后续任意
        // 一次 release 提前 delete, 持有者悬空 (boot 期 insert/items 段错误
        // 实锤)。prvalue (make_* 工厂 return) 走 C++17 elision 才侥幸平账,
        // 单测因此没暴露。const& + retain: lvalue/prvalue/xvalue 全形态
        // 账均正确。
        if (r) { p_ = r.get(); p_->retain(); }
    }
    Value(const Value& o) noexcept : t_(o.t_) {
        switch (t_) {
        case VT_STR:
            new (strSlot()) SharedString(o.strRef());
            break;
        case VT_ARR: case VT_MAP: case VT_OBJ: case VT_CLO:
        default:
            i_ = o.i_;      // 全 8 字节复制 (int/double/对象指针槽统一)
            break;
        }
        if (t_ >= VT_ARR && p_) p_->retain();
    }
    Value(Value&& o) noexcept : t_(o.t_) {
        switch (t_) {
        case VT_STR:
            new (strSlot()) SharedString(std::move(o.strRef()));
            o.strRef().~SharedString();
            break;
        default:
            i_ = o.i_;      // 全 8 字节复制
            break;
        }
        o.t_ = VT_NIL;
        o.i_ = 0;
    }
    Value& operator=(const Value& o) noexcept {
        if (this != &o) {
            this->~Value();
            new (static_cast<void*>(this)) Value(o);
        }
        return *this;
    }
    Value& operator=(Value&& o) noexcept {
        if (this != &o) {
            this->~Value();
            new (static_cast<void*>(this)) Value(std::move(o));
        }
        return *this;
    }
    ~Value() noexcept {
        if (t_ == VT_STR) strRef().~SharedString();
        else if (t_ >= VT_ARR) destroyValueObject(t_, p_);
        t_ = VT_NIL;
        i_ = 0;
    }

    uint8_t index() const noexcept { return t_; }

private:
    uint8_t t_;
    union {
        int64_t i_;
        double d_;
        RefCountedBase* p_;
        std::aligned_storage_t<sizeof(void*), alignof(void*)> ss_;
    };
    SharedString* strSlot() noexcept { return reinterpret_cast<SharedString*>(&ss_); }
    const SharedString* strSlot() const noexcept { return reinterpret_cast<const SharedString*>(&ss_); }
    SharedString& strRef() noexcept { return *strSlot(); }
    const SharedString& strRef() const noexcept { return *strSlot(); }
    template <class T>
    static void* slotPtr(Value& v) noexcept {
        if constexpr (std::is_same_v<T, SharedString>) return &v.ss_;
        else return &v.p_;
    }
    template <class T>
    static const void* slotPtr(const Value& v) noexcept {
        if constexpr (std::is_same_v<T, SharedString>) return &v.ss_;
        else return &v.p_;
    }
    template <class T> friend T& vget(Value& v) noexcept;
    template <class T> friend const T& vget(const Value& v) noexcept;
};

template <class T>
inline T& vget(Value& v) noexcept {
    return *reinterpret_cast<T*>(Value::slotPtr<T>(v));
}
template <class T>
inline const T& vget(const Value& v) noexcept {
    return *reinterpret_cast<const T*>(Value::slotPtr<T>(v));
}
// variant 的按序号形态 std::get<N>(v) —— N 即 alternative 下标。
template <unsigned I> struct VAltOf;
template <> struct VAltOf<4>  { using type = SharedString; };
template <> struct VAltOf<5>  { using type = lpc::Ref<ArrayValue>; };
template <> struct VAltOf<6>  { using type = lpc::Ref<MappingValue>; };
template <> struct VAltOf<7>  { using type = lpc::Ref<ObjectValue>; };
template <> struct VAltOf<8>  { using type = lpc::Ref<ClosureValue>; };
template <> struct VAltOf<9>  { using type = lpc::Ref<LoopCtrl>; };
template <> struct VAltOf<10> { using type = lpc::Ref<BufferValue>; };
template <> struct VAltOf<11> { using type = lpc::Ref<CrossVmRef>; };
template <unsigned I>
inline typename VAltOf<I>::type& vgetI(Value& v) noexcept {
    return vget<typename VAltOf<I>::type>(v);
}
template <unsigned I>
inline const typename VAltOf<I>::type& vgetI(const Value& v) noexcept {
    return vget<typename VAltOf<I>::type>(v);
}

struct ArrayValue : RefBase<ArrayValue> {
    // [vpool] 高频分配走线程本地自由链池
    static void* operator new(std::size_t) { return VmPool<ArrayValue>::pop(); }
    static void operator delete(void* p) noexcept { VmPool<ArrayValue>::push(p); }
    std::vector<Value> items;
    ArrayValue();
    ~ArrayValue();
};

// Faithful port of the original driver's vm_mapping.c (gameshell):
//   - bucketed hash table; string keys hashed with vm_whashstr (T table),
//     int keys with (number >> 4), everything else by identity,
//   - new nodes are inserted at the HEAD of their bucket chain,
//   - an "unfilled" counter tracks (80% of buckets - empty-bucket inserts)
//     and triggers vm_growMapping when it hits 1,
//   - keys()/values()/foreach iterate buckets from tableSize DOWN TO 0 and
//     each chain head-first, which is what produces the original load order.
struct MappingValue : RefBase<MappingValue> {
    // [vpool] 高频分配走线程本地自由链池。[map-bad] 取证期曾临时去池化 +
    // 0xDD 毒化 delete 定位悬空引用 (refown 修复后已还原; 取证开关
    // LPCVM_MAPDEL 与 items 结构哨兵保留, 默认零成本)。
    static void* operator new(std::size_t) { return VmPool<MappingValue>::pop(); }
    static void operator delete(void* p) noexcept { VmPool<MappingValue>::push(p); }
    struct Node {
        Value key;
        Value value;
        int32_t next = -1;   // next node in the same bucket chain
    };
    std::vector<Node> nodes;        // node storage (indices are stable)
    std::vector<int32_t> table;     // bucket -> head node index (-1 = empty)
    uint32_t tableSize = 7;         // bucket count - 1 (always 2^k - 1)
    int32_t unfilled = 6;
    uint32_t cnt = 0;
    bool caseSensitive = true;      // attrib & 1

    MappingValue();
    explicit MappingValue(uint32_t sizeHint);
    ~MappingValue();

    size_t size() const { return cnt; }
    bool empty() const { return cnt == 0; }

    // Find the value stored for key (nullptr when absent).
    Value* find(const Value& key);
    const Value* find(const Value& key) const;

    // vm_safeFindForInsert: add or replace; reproduces original bucket order.
    Value& insert(const Value& key, const Value& value);

    // vm_mappingDelete.
    void erase(const Value& key);

    // vm_growMapping (internal).
    void grow();

    // vm_setCaseCheck: toggle case sensitivity and rehash (low->high buckets,
    // tail insertion; obsorb drops later duplicates when set).
    void setCaseCheck(bool cs, int obsorb);

    // (key, value) pairs in vm_mappingIndices order (bucket high->low).
    std::vector<std::pair<Value, Value>> items() const;
    // Loop-statement order (vm_cmpAndJmpInLoop ctrl 902): buckets ASCENDING.
    std::vector<std::pair<Value, Value>> itemsAscending() const;

    // Walk every LIVE entry (bucket chains only - erased nodes stay in the
    // nodes vector but are unreachable) without allocating a copy.  fn(key,
    // value) is invoked in vm_mappingIndices order (0x809e688: buckets from
    // tableSize DOWN TO 0, each chain head first) — the keys()/values()
    // order.  The loop statement uses a DIFFERENT (ascending) order — see
    // itemsAscending().
    template <typename F>
    void forEachActive(F&& fn) const {
        for (int32_t b = static_cast<int32_t>(tableSize); b >= 0; --b)
            for (int32_t ni = table[b]; ni != -1; ni = nodes[static_cast<size_t>(ni)].next)
                fn(nodes[static_cast<size_t>(ni)].key, nodes[static_cast<size_t>(ni)].value);
    }
};
struct ObjectValue : RefBase<ObjectValue> {
    std::string programName;
    // The INSTANCE the value refers to (原版: v.info = Vm_Object_T*, 对象值
    // 就是指针 — vm_callInOtherFun 0x8094cd8 零查找).  Null for values built
    // before vmcore registration (diagnostics only).
    vmcore::VmObject* vmObj = nullptr;
    // The program that created this object value (set by push_this_object).
    // When programName is "<this>" the value is an object-identity reference
    // captured at creation time; callers resolve it through this owner's
    // registry VM (handler.c's create calls call_other(module, ...) with a
    // captured (["module": object(<this>)])).
    const struct OriginalObject* owner = nullptr;
    // Shared with the registry VM that owns the object (nullptr when the value
    // was not built from a registry lookup).  Object values are created afresh
    // on every push_this_object/clone_object, so a stale copy can outlive the
    // object: `alive` is the flag unregisterDestructed() clears, and is_object
    // / the is_type opcode treat a cleared flag as "not an object" - the
    // original driver invalidates references when an object is really freed.
    std::shared_ptr<std::atomic<bool>> alive;
    std::unordered_map<std::string, Value> variables;
};
struct ClosureValue : RefBase<ClosureValue> {
    // [vpool] 高频分配走线程本地自由链池
    static void* operator new(std::size_t) { return VmPool<ClosureValue>::pop(); }
    static void operator delete(void* p) noexcept { VmPool<ClosureValue>::push(p); }
    std::string functionName;
    // Merged function-table index captured at closure-creation time.  A
    // closure created from create_function_value(n) must invoke THAT function
    // (e.g. handler_module's inherited process_command at merged index 32),
    // not whatever the current VM's findFunctionAny(name) happens to resolve
    // later (which would pick the OVERRIDING copy - accountd's own
    // process_command - and loop forever).  -1 = resolve by name only.
    int funcIndex = -1;
    // Exact function captured at closure-creation time.  call_fun_ptr invokes
    // THIS function directly, so a closure made by object A (merged index n)
    // still runs A's function when invoked from object B's VM (B's own index n
    // is a different function).  Null when only funcIndex/name are known.
    const struct OriginalFunction* func = nullptr;
    std::vector<Value> boundArgs;
    // Object that owns this closure (the program whose bytecode created it).
    // When the closure is invoked from another VM (e.g. the master's post_init
    // loop calling a daemon's do_init), call_fun_ptr resolves the function in
    // this owner instead of the current VM.  Null for efun closures.
    const struct OriginalObject* owner = nullptr;
    // True when this closure wraps a BUILTIN EFUN (create_function_value with
    // the global-table bit and no matching master function, e.g.
    // (: call_other, path, "fn" :) or (: create_thread, ... :)).  funcIndex is
    // then the EFUN's slot number, which COLLIDES with real function indexes in
    // a VM's merged table: call_fun_ptr's funcsByIndex_[funcIndex] fallback
    // would silently run that unrelated function (taskd's (*funcs["tip"])(...)
    // returned int 1 instead of instruction_1's tip string -> every task
    // log/tip empty).  Such closures must go to the efun-by-name fallback.
    bool efunClosure = false;
    // True when this closure wraps a GLOBAL-PROGRAM function (op23
    // create_function_value with bit7 set and a matching master function,
    // e.g. scand's (: find_basic_object_by_import_name :), funcIndex 1869).
    // 原版 vm_callInUserFunBody(bit7, ..., vm_pGlobalProgram): the BODY runs
    // from vm_pGlobalProgram (master bytecode + constant pool) with the
    // CURRENT object as context — never the creating object's program (whose
    // merged table has no such index and whose findFn by name misses ->
    // closure fell into the efun-by-name stub and returned 0 forever).
    bool globalFn = false;
    // Registry key of the EXACT VM that created this closure (e.g.
    // "clone:gs/clone/char/login.o#16960").  A clone shares its template's
    // OriginalObject, so several registered VMs report object() == owner and
    // object()-only resolution can bind the closure to an OLDER clone of the
    // same program: login.o process_command dispatches
    // (: _process_command, ... :) to threadd, and if the closure then ran on
    // clone #1 the do_login/ask_fields 0x211B reply went to clone #1's
    // (long-dead) client.  ownerKey pins resolution to the creating instance.
    // Empty for global/efun closures and for plain function-name closures.
    std::string ownerKey;
};
// Loop control block created by init_for_loop (original VM value type 20).
struct LoopCtrl : RefBase<LoopCtrl> {
    int type = 0;                     // 1 / -1 (int), 901 (array), 902 (mapping)
    int slotOfVar = -1;               // stack index of the loop variable
    // The loop variable's slot expressed as a FRAME-RELATIVE offset:
    //   slot = frame.bp - varOff      (varOff is -1 for `loop (f in arr)`,
    //                                  whose variable sits one slot ABOVE bp)
    // The absolute slot above is captured once and then lives inside a stack
    // slot, so it goes STALE whenever that stack is re-based - which
    // runDetached's SuspendSignal merge does (it appends the detached callee's
    // stack above the caller's and shifts every frame's bp).  The loop keeps
    // working (its own ctrl slot is looked up from the live frame) but every
    // setVar() kept writing the OLD slot while the body read the NEW one, so
    // `loop (f in _enter_world_funcs)` re-applied element 38 (vipd's
    // do_when_enter_world) for all 189 remaining hooks and the newbie story
    // never ran.  When the variable really is a stack slot, recompute
    // `frame.bp - varOff` at every step instead.  NOTE: this is NOT the
    // init_for_loop par16 - that operand addresses the hidden CONTROL slot
    // (-2 for the same loop), while the variable comes from the
    // push_local_vptr pushed just before it.
    int varOff = 0;                   // relative slot offset (valid if varIsSlot)
    bool varIsSlot = false;           // loop variable lives in a stack slot
    int64_t current = 0;
    int64_t to = 0;
    int done = 0;
    Ref<ArrayValue> arr;
    Ref<MappingValue> map;
    size_t iter = 0;                  // array iterator
    size_t mIdx = 0;                  // mapping iterator
    int mElementIndex = 0;
};
// Binary buffer (the original driver's Vm_Buffer_T).  Kept distinct from a
// text string so send_to_client/pkt_pad_buffer serialize it as raw bytes
// rather than as a length-prefixed string.
struct BufferValue : RefBase<BufferValue> {
    // [vpool] 高频分配走线程本地自由链池
    static void* operator new(std::size_t) { return VmPool<BufferValue>::pop(); }
    static void operator delete(void* p) noexcept { VmPool<BufferValue>::push(p); }
    std::string data;
};

// [pvalue] 类型化释放 (声明见 RefCountedBase 处)
// [map-bad] 取证声明已上移至 RefCountedBase 处 (MappingValue::operator
// delete 需要在定义点可见)。

inline void destroyValueObject(uint8_t t, RefCountedBase* p) noexcept {
    if (!p) return;
    switch (t) {
    case VT_ARR:  if (p->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) delete static_cast<ArrayValue*>(p);  break;
    case VT_MAP:  if (p->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                      if (g_mapDelTraceOn) mapDeleteTrace(p);
                      delete static_cast<MappingValue*>(p);
                  } break;
    case VT_OBJ:  if (p->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) delete static_cast<ObjectValue*>(p); break;
    case VT_CLO:  if (p->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) delete static_cast<ClosureValue*>(p); break;
    case VT_LOOP: if (p->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) delete static_cast<LoopCtrl*>(p);    break;
    case VT_BUF:  if (p->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) delete static_cast<BufferValue*>(p); break;
    case VT_XREF: if (p->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) delete static_cast<CrossVmRef*>(p);  break;
    default: break;
    }
}

Value make_int(int64_t v);
Value make_float(double v);
Value make_string(std::string s);
Value make_buffer(std::string data);
Value make_array();
Value make_mapping();
Value make_object(std::string programName);
Value make_object_owned(std::string programName, const struct OriginalObject* owner);
Value make_crossvm_ref(Ref<CrossVmRef> r);
Value make_closure(std::string name, std::vector<Value> args = {});

std::string value_type_name(const Value& v);
std::string to_string(const Value& v, bool quote_strings = true);
// Append v's unquoted textual form (exactly what to_string(v,false) yields) to
// out WITHOUT deep-copying a string Value first (its shared text is appended
// straight from the block).  Used by string concatenation points so a + b
// builds the result in one buffer instead of copying each operand twice.
void append_text(std::string& out, const Value& v);
bool is_truthy(const Value& v);
bool value_equal(const Value& a, const Value& b);
bool value_less(const Value& a, const Value& b);

// Live container census (ArrayValue/MappingValue alive + cumulative creates).
// Updated from their ctors/dtors; cheap atomics, used by the LPCVM_MEM2 heap
// diagnostics to attribute memory growth during boot plateaus.
void liveHeapSnapshot(long& arrLive, long& mapLive, long& arrCum, long& mapCum);

} // namespace lpc
