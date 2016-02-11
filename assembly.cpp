#include "assembly.h"

#define assert(c) if(!(c)) throw Assembly::InvalidBytecodeError{}

using namespace rvm::assembly;

namespace {

void dump(uint8_t i, std::ostream& out) {
    out.put(i);
}

void dump(uint32_t i, std::ostream& out) {
    dump(static_cast<uint8_t>((i & 0xFF000000) >> 24), out);
    dump(static_cast<uint8_t>((i & 0x00FF0000) >> 16), out);
    dump(static_cast<uint8_t>((i & 0x0000FF00) >>  8), out);
    dump(static_cast<uint8_t>((i & 0x000000FF) >>  0), out);
}

template <class T>
void dump(const std::vector<T>& xs, std::ostream& out) {
    dump(xs.size(), out);
    for (auto& x : xs) {
        dump(x, out);
    }
}

void dump(const ConstructorInfo& c, std::ostream& out) {
    dump(c.num_fields, out);
}

void dump(const ConstantInfo&, std::ostream&);

void dump(const Adt& a, std::ostream& out) {
    dump(a.adt_table_index, out);
    dump(a.constructor_index, out);
    dump(a.num_fields, out);
    for (uint32_t i = 0; i < a.num_fields; ++i) {
        dump(a.fields[i], out);
    }
}

void dump(const ConstantInfo& c, std::ostream& out) {
    dump(static_cast<uint8_t>(c.type), out);
    switch (c.type) {
        case ConstantType::uint8:
            dump(c.uint8, out);
            break;
        case ConstantType::uint32:
            dump(c.uint32, out);
            break;
        case ConstantType::adt:
            dump(c.adt, out);
            break;
    }
}

void dump(const FunctionInfo& f, std::ostream& out) {
    dump(f.num_args, out);
    dump(f.num_locals, out);
    dump(f.code, out);
}

void parse(uint8_t* i, std::istream& in) {
    auto re = in.get();
    if (re == std::istream::traits_type::eof()) {
        throw Assembly::ParseError{};
    }
    *i = re;
}

void parse(uint32_t* i, std::istream& in) {
    auto re = uint32_t{};
    auto tmp = uint8_t{};
    re |= (parse(&tmp, in), tmp) << 24;
    re |= (parse(&tmp, in), tmp) << 16;
    re |= (parse(&tmp, in), tmp) << 8;
    re |= (parse(&tmp, in), tmp) << 0;
    *i = re;
}

template <class T>
void parse(std::vector<T>* v, std::istream& in) {
    auto size = uint32_t{};
    parse(&size, in);
    std::vector<T> re(size);
    for (auto i = uint32_t{0}; i < size; ++i) {
        parse(&re[i], in);
    }
    *v = re;
}

void parse(ConstructorInfo* c, std::istream& in) {
    parse(&c->num_fields, in);
}

void parse(ConstantType* c, std::istream& in) {
    auto re = uint8_t{};
    parse(&re, in);
    *c = static_cast<ConstantType>(re);
}

void parse(ConstantInfo*, std::istream&);

void parse(Adt* a, std::istream& in) {
    auto re = Adt{};
    parse(&re.adt_table_index, in);
    parse(&re.adt_table_index, in);
    parse(&re.num_fields, in);
    re.fields = new ConstantInfo[re.num_fields];
    for (uint32_t i = 0; i < re.num_fields; ++i) {
        parse(&re.fields[i], in);
    }
    *a = re;
}

void parse(ConstantInfo* c, std::istream& in) {
    auto re = ConstantInfo{};
    parse(&re.type, in);
    switch (re.type) {
        case ConstantType::uint8:
            parse(&re.uint8, in);
            break;
        case ConstantType::uint32:
            parse(&re.uint32, in);
            break;
        case ConstantType::adt:
            parse(&re.adt, in);
            break;
    }
    *c = re;
}

void parse(FunctionInfo* f, std::istream& in) {
    auto re = FunctionInfo{};
    parse(&re.num_args, in);
    parse(&re.num_locals, in);
    parse(&re.code, in);
    *f = re;
}

void check_operand_type(rvm::OperandType t) {
    auto code = static_cast<uint8_t>(t);
    assert(code >= 1 && code <= 4);
}

}

void Assembly::dump(std::ostream& out) {
    ::dump(MAGIC_NUMBER, out);
    ::dump(adt_table, out);
    ::dump(constant_table, out);
    ::dump(function_table, out);
}

Assembly Assembly::parse(std::istream& in) {
    auto magic = uint32_t{};
    ::parse(&magic, in);
    if (magic != MAGIC_NUMBER) throw ParseError{};
    auto re = Assembly{};
    ::parse(&re.adt_table, in);
    ::parse(&re.constant_table, in);
    ::parse(&re.function_table, in);
    return re;
}

void Assembly::validate() {
    for (auto& this_func : function_table) {
        auto pc = BytecodeIterator{this_func.code};
        while (!pc.finished()) {
            switch (pc.read_instruction()) {
                case Instruction::add:
                case Instruction::sub:
                case Instruction::mul:
                case Instruction::div:
                case Instruction::rem:
                case Instruction::band:
                case Instruction::bor:
                case Instruction::bxor:
                case Instruction::bnot:
                    check_operand_type(pc.read_operand_type());
                    break;
                case Instruction::dup:
                case Instruction::drop:
                    break;
                case Instruction::ldc:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < constant_table.size());
                    break;
                }
                case Instruction::ldloc:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < this_func.num_locals);
                    break;
                }
                case Instruction::stloc:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < this_func.num_locals);
                    break;
                }
                case Instruction::ldarg:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < this_func.num_args);
                    break;
                }
                case Instruction::starg:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < this_func.num_args);
                    break;
                }
                case Instruction::call:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < function_table.size());
                    break;
                }
                case Instruction::ret:
                    break;
                case Instruction::ldloca:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < this_func.num_locals);
                    break;
                }
                case Instruction::ldarga:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < this_func.num_args);
                    break;
                }
                case Instruction::ldfuna:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < function_table.size());
                    break;
                }
                case Instruction::calla:
                case Instruction::ldind:
                case Instruction::stind:
                    break;
                case Instruction::teq:
                case Instruction::tne:
                case Instruction::tlt:
                case Instruction::tlt_s:
                case Instruction::tle:
                case Instruction::tle_s:
                case Instruction::tgt:
                case Instruction::tgt_s:
                case Instruction::tge:
                case Instruction::tge_s:
                    check_operand_type(pc.read_operand_type());
                    break;
                case Instruction::br:
                case Instruction::brtrue:
                {
                    auto idx = pc.read_uint32();
                    assert(idx < this_func.code.size());
                    break;
                }
                case Instruction::mkadt:
                {
                    auto idx = pc.read_uint32();
                    auto ctor = pc.read_uint32();
                    assert(idx < adt_table.size());
                    assert(ctor < adt_table[idx].size());
                    break;
                }
                case Instruction::dladt:
                case Instruction::ldctor:
                case Instruction::ldfld:
                case Instruction::stfld:
                    break;
            }
        }
    }
}
