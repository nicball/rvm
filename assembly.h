#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include "instruction.h"

namespace rvm {
namespace assembly {

static constexpr uint32_t MAGIC_NUMBER = 0xBADDCAFE;
static constexpr uint32_t MAIN_FUNCTION_INDEX = 0;

struct ConstructorInfo {
    uint32_t num_fields;
};
using AdtInfo = std::vector<ConstructorInfo>;
using AdtTable = std::vector<AdtInfo>;

enum class ConstantType: uint8_t {
    uint8, uint32, adt
};
struct ConstantInfo;
struct Adt {
    uint32_t adt_table_index;
    uint32_t constructor_index;
    uint32_t num_fields;
    ConstantInfo* fields;
};
struct ConstantInfo {
    ConstantType type;
    union {
        uint8_t uint8;
        uint32_t uint32;
        Adt adt;
    };
    ConstantInfo() = default;
    explicit ConstantInfo(uint8_t u): type{ConstantType::uint8}, uint8{u} {}
    explicit ConstantInfo(uint32_t u): type{ConstantType::uint32}, uint32{u} {}
    explicit ConstantInfo(Adt a): type{ConstantType::adt}, adt{a} {}
};
using ConstantTable = std::vector<ConstantInfo>;

using Bytecode = std::vector<uint8_t>;
struct FunctionInfo {
    uint32_t num_args;
    uint32_t num_locals;
    Bytecode code;
};
using FunctionTable = std::vector<FunctionInfo>;

struct Assembly {
    AdtTable adt_table;
    ConstantTable constant_table;
    FunctionTable function_table;

    struct ParseError {};
    struct InvalidBytecodeError {};

    void validate();
    void dump(std::ostream&);
    static Assembly parse(std::istream&);
};


class BytecodeBuilder {
    Bytecode code;
public:
    BytecodeBuilder& emit(Instruction i) {
        code.push_back(static_cast<uint8_t>(i));
        return *this;
    }
    BytecodeBuilder& emit(Instruction i, OperandType t) {
        emit(i);
        code.push_back(static_cast<uint8_t>(t));
        return *this;
    }
    BytecodeBuilder& emit(Instruction i, uint32_t idx) {
        emit(i);
        code.push_back((idx & 0xFF000000) >> 24);
        code.push_back((idx & 0x00FF0000) >> 16);
        code.push_back((idx & 0x0000FF00) >> 8);
        code.push_back((idx & 0x000000FF) >> 0);
        return *this;
    }
    Bytecode& get() {
        return code;
    }
};

class BytecodeIterator {
    const Bytecode* code;
    uint32_t pc;
public:
    BytecodeIterator(const Bytecode& c): code{&c}, pc{0} {}
    BytecodeIterator(const Bytecode& c, uint32_t p): code{&c}, pc{p} {}
    uint8_t read_uint8() {
        return (*code)[pc++];
    }

    Instruction read_instruction() {
        return static_cast<Instruction>(read_uint8());
    }

    OperandType read_operand_type() {
        return static_cast<OperandType>(read_uint8());
    }

    uint32_t read_uint32() {
        uint32_t re = 0;
        re |= read_uint8() << 24;
        re |= read_uint8() << 16;
        re |= read_uint8() << 8;
        re |= read_uint8() << 0;
        return re;
    }

    explicit operator uint32_t() {
        return pc;
    }

    BytecodeIterator& operator = (uint32_t p) {
        pc = p;
        return *this;
    }

    void move_back() {
        --pc;
    }

    bool finished() {
        return pc >= code->size();
    }
};

}
}
