#include "assembly.h"

#define assert(c) if(!(c)) throw InvalidBytecodeError{}

using namespace rvm;
using namespace rvm::assembly;

namespace {

void dump(uint8_t i, std::ostream& out) {
    out.put(i);
}

void dump(uint16_t i, std::ostream& out) {
    dump(static_cast<uint8_t>((i & 0xFF00) >> 8), out);
    dump(static_cast<uint8_t>((i & 0x00FF) >> 0), out);
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
    for (auto&& x : xs) {
        dump(x, out);
    }
}

void dump(const ConstructorInfo& c, std::ostream& out) {
    dump(c.num_fields, out);
}

void dump(const ConstantInfo&, std::ostream&);

void dump(const AdtConstant& a, std::ostream& out) {
    dump(a.adt_table_index, out);
    dump(a.constructor_index, out);
    dump(a.num_fields, out);
    for (int32_t i = 0; i < a.num_fields; ++i) {
        dump(a.fields[i], out);
    }
}

void dump(const ConstantInfo& c, std::ostream& out) {
    dump(static_cast<uint8_t>(c.type), out);
    switch (c.type) {
        case ConstantType::int8:
            dump(static_cast<uint8_t>(c.int8), out);
            break;
        case ConstantType::int32:
            dump(static_cast<uint32_t>(c.int32), out);
            break;
        case ConstantType::adt:
            dump(c.adt, out);
            break;
    }
}

void dump(Operation t, std::ostream& out) {
    dump(static_cast<uint8_t>(t), out);
}

void dump(const Instruction& i, std::ostream& out) {
    dump(i.op, out);
    dump(i.index, out);
    dump(i.index2, out);
}

void dump(const FunctionInfo& f, std::ostream& out) {
    dump(f.num_args, out);
    dump(f.num_locals, out);
    dump(f.code, out);
}

uint8_t& parse(uint8_t* i, std::istream& in) {
    auto re = in.get();
    if (re == std::istream::traits_type::eof()) {
        throw ParseError{};
    }
    *i = re;
    return *i;
}

uint16_t& parse(uint16_t* i, std::istream& in) {
    auto re = uint16_t{};
    auto tmp = uint8_t{};
    re |= parse(&tmp, in) << 8;
    re |= parse(&tmp, in) << 0;
    *i = re;
    return *i;
}

uint32_t& parse(uint32_t* i, std::istream& in) {
    auto re = uint32_t{};
    auto tmp = uint8_t{};
    re |= parse(&tmp, in) << 24;
    re |= parse(&tmp, in) << 16;
    re |= parse(&tmp, in) << 8;
    re |= parse(&tmp, in) << 0;
    *i = re;
    return *i;
}

template <class T>
std::vector<T>& parse(std::vector<T>* v, std::istream& in) {
    auto size = uint32_t{};
    parse(&size, in);
    std::vector<T> re(size);
    for (auto&& i : re) {
        parse(&i, in);
    }
    *v = re;
    return *v;
}

ConstructorInfo& parse(ConstructorInfo* c, std::istream& in) {
    parse(&c->num_fields, in);
    return *c;
}

ConstantType& parse(ConstantType* c, std::istream& in) {
    auto re = uint8_t{};
    parse(&re, in);
    *c = static_cast<ConstantType>(re);
    return *c;
}

ConstantInfo& parse(ConstantInfo*, std::istream&);

AdtConstant& parse(AdtConstant* a, std::istream& in) {
    auto re = AdtConstant{};
    parse(&re.adt_table_index, in);
    parse(&re.constructor_index, in);
    parse(&re.num_fields, in);
    re.fields = new ConstantInfo[re.num_fields];
    for (auto i = re.num_fields; i != 0; --i) {
        parse(&re.fields[re.num_fields - i], in);
    }
    *a = re;
    return *a;
}

ConstantInfo& parse(ConstantInfo* c, std::istream& in) {
    auto re = ConstantInfo{};
    parse(&re.type, in);
    switch (re.type) {
        case ConstantType::int8:
            parse(reinterpret_cast<uint8_t*>(&re.int8), in);
            break;
        case ConstantType::int32:
            parse(reinterpret_cast<uint32_t*>(&re.int32), in);
            break;
        case ConstantType::adt:
            parse(&re.adt, in);
            break;
    }
    *c = re;
    return *c;
}

Operation& parse(Operation* i, std::istream& in) {
    parse(reinterpret_cast<uint8_t*>(i), in);
    return *i;
}

Instruction& parse(Instruction* i, std::istream& in) {
    auto re = Instruction{};
    parse(&re.op, in);
    parse(&re.index, in);
    parse(&re.index2, in);
    *i = re;
    return *i;
}

FunctionInfo& parse(FunctionInfo* f, std::istream& in) {
    auto re = FunctionInfo{};
    parse(&re.num_args, in);
    parse(&re.num_locals, in);
    parse(&re.code, in);
    *f = re;
    return *f;
}

void check_operand_type(rvm::OperandType t) {
    auto code = static_cast<int8_t>(t);
    assert(code >= 1 && code <= 4);
}

}

void rvm::assembly::dump(const Assembly& a, std::ostream& out) {
    ::dump(MAGIC_NUMBER, out);
    ::dump(a.adt_table, out);
    ::dump(a.constant_table, out);
    ::dump(a.function_table, out);
}

Assembly Assembly::parse(std::istream& in) {
    auto re = Assembly{};
    auto magic = uint32_t{};
    ::parse(&magic, in);
    if (magic != MAGIC_NUMBER) throw ParseError{};
    ::parse(&re.adt_table, in);
    ::parse(&re.constant_table, in);
    ::parse(&re.function_table, in);
    return re;
}

void rvm::assembly::validate(const Assembly& a) {
    for (auto& this_func : a.function_table) {
        for (auto&& pc : this_func.code) {
            switch (pc.op) {
                case Operation::add:
                case Operation::sub:
                case Operation::mul:
                case Operation::div:
                case Operation::rem:
                case Operation::band:
                case Operation::bor:
                case Operation::bxor:
                case Operation::bnot:
                    check_operand_type(pc.type);
                    break;
                case Operation::dup:
                case Operation::drop:
                    break;
                case Operation::ldc:
                    assert(pc.index < a.constant_table.size());
                    break;
                case Operation::ldloc:
                    assert(pc.index < this_func.num_locals);
                    break;
                case Operation::stloc:
                    assert(pc.index < this_func.num_locals);
                    break;
                case Operation::ldarg:
                    assert(pc.index < this_func.num_args);
                    break;
                case Operation::starg:
                    assert(pc.index < this_func.num_args);
                    break;
                case Operation::call:
                    assert(pc.index < a.function_table.size());
                    break;
                case Operation::ret:
                    break;
                case Operation::ldloca:
                    assert(pc.index < this_func.num_locals);
                    break;
                case Operation::ldarga:
                    assert(pc.index < this_func.num_args);
                    break;
                case Operation::ldfuna:
                    assert(pc.index < a.function_table.size());
                    break;
                case Operation::calla:
                case Operation::ldind:
                case Operation::stind:
                    break;
                case Operation::teq:
                case Operation::tne:
                case Operation::tlt:
                case Operation::tlt_un:
                case Operation::tle:
                case Operation::tle_un:
                case Operation::tgt:
                case Operation::tgt_un:
                case Operation::tge:
                case Operation::tge_un:
                    check_operand_type(pc.type);
                    break;
                case Operation::br:
                case Operation::brtrue:
                    assert(pc.index < this_func.code.size());
                    break;
                case Operation::mkadt:
                    assert(pc.index < a.adt_table.size());
                    assert(pc.index2 < a.adt_table[pc.index].size());
                    break;
                case Operation::dladt:
                case Operation::ldctor:
                case Operation::ldfld:
                case Operation::stfld:
                    break;
            }
        }
    }
}
