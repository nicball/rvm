#include "assembly.h"

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
    uint32_t re = 0;
    uint8_t tmp;
    re |= (parse(&tmp, in), tmp) << 24;
    re |= (parse(&tmp, in), tmp) << 16;
    re |= (parse(&tmp, in), tmp) << 8;
    re |= (parse(&tmp, in), tmp) << 0;
    *i = re;
}

template <class T>
void parse(std::vector<T>* v, std::istream& in) {
    uint32_t size;
    parse(&size, in);
    std::vector<T> re(size);
    for (uint32_t i = 0; i < size; ++i) {
        parse(&re[i], in);
    }
    *v = re;
}

void parse(ConstructorInfo* c, std::istream& in) {
    parse(&c->num_fields, in);
}

void parse(ConstantType* c, std::istream& in) {
    uint8_t re;
    parse(&re, in);
    *c = static_cast<ConstantType>(re);
}

void parse(ConstantInfo*, std::istream&);

void parse(Adt* a, std::istream& in) {
    Adt re;
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
    ConstantInfo re;
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
    FunctionInfo re;
    parse(&re.num_args, in);
    parse(&re.num_locals, in);
    parse(&re.code, in);
    *f = re;
}

}

void Assembly::dump(std::ostream& out) {
    ::dump(MAGIC_NUMBER, out);
    ::dump(adt_table, out);
    ::dump(constant_table, out);
    ::dump(function_table, out);
}
Assembly Assembly::parse(std::istream& in) {
    uint32_t magic;
    ::parse(&magic, in);
    if (magic != MAGIC_NUMBER) throw ParseError{};
    Assembly re;
    ::parse(&re.adt_table, in);
    ::parse(&re.constant_table, in);
    ::parse(&re.function_table, in);
    return re;
}
