# -*- coding: utf-8 -*-
"""Add LPCVM_OPFN=<substr>: per-instruction trace (operand + stack top) for
matching LPC functions -- to see what all_templates[task_class] evaluates to
inside taskd::query_task_tip."""
import io

P = 'src/original_vm.cpp'
d = io.open(P, 'rb').read()


def B(s):
    return s.replace('\n', '\r\n').encode('ascii')


OLD = B("""            const ActionInfo& ai = actionInfo(in.op);
            int32_t m0 = 0, m1 = 0;
            bool fromMeta = (ai.attrib & 0x80000000u) != 0;
            if (fromMeta) {
                uint32_t off = static_cast<uint32_t>(in.a) | (static_cast<uint32_t>(in.b) << 16);
                m0 = readI32(*meta_, off);
                if (ai.parameters == 2) m1 = readI32(*meta_, off + 4);
            }
""")

NEW = B("""            const ActionInfo& ai = actionInfo(in.op);
            int32_t m0 = 0, m1 = 0;
            bool fromMeta = (ai.attrib & 0x80000000u) != 0;
            if (fromMeta) {
                uint32_t off = static_cast<uint32_t>(in.a) | (static_cast<uint32_t>(in.b) << 16);
                m0 = readI32(*meta_, off);
                if (ai.parameters == 2) m1 = readI32(*meta_, off + 4);
            }
            // LPCVM_OPFN=<substr>: per-instruction trace (operand + stack top)
            // of matching LPC functions.  Diagnostic only; capped per function.
            {
                static const bool opFnOn = (std::getenv("LPCVM_OPFN") != nullptr);
                if (opFnOn && !frames_.empty() && frames_.back().fn) {
                    static const char* const opFnFlt = std::getenv("LPCVM_OPFN");
                    const std::string& tfn = frames_.back().fn->name;
                    if (opFnFlt && *opFnFlt && tfn.find(opFnFlt) != std::string::npos) {
                        static std::map<std::string, int> opFnCount;
                        int& oc = opFnCount[tfn];
                        if (oc < 400) {
                            ++oc;
                            std::string topS = "<empty>";
                            if (!stack_.empty())
                                topS = "t" + std::to_string(stack_.back().value.index()) + ":" +
                                       to_string(stack_.back().value, false).substr(0, 70);
                            std::cerr << "[op] fn=" << tfn << " ip=" << ip_ << " op=" << in.op
                                      << " a=" << in.a << " b=" << in.b;
                            if (fromMeta) std::cerr << " m0=" << m0 << " m1=" << m1;
                            std::cerr << " sp=" << stack_.size() << " top=" << topS << "\\n";
                        }
                    }
                }
            }
""")
assert d.count(OLD) == 1, 'anchor not found (%d)' % d.count(OLD)
d = d.replace(OLD, NEW)
io.open(P, 'wb').write(d)
print('patched; CRLF', d.count(b'\r\n'), 'bareLF', d.count(b'\n') - d.count(b'\r\n'))
