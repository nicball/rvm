#pragma once
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include "instruction.h"

namespace rvm {
namespace assembly {

static constexpr int32_t MAGIC_NUMBER = 0xBADDCAFE;
static constexpr index_t MAIN_FUNCTION_INDEX = 0;

struct ConstructorInfo {
    sindex_t num_fields;
};
using AdtInfo = std::vector<ConstructorInfo>;
using AdtTable = std::vector<AdtInfo>;

enum class ConstantType: sindex_t {
    int8, int32, adt
};
struct ConstantInfo;
struct AdtConstant {
    index_t adt_table_index;
    sindex_t constructor_index;
    index_t num_fields;
    ConstantInfo* fields;
};
struct ConstantInfo {
    ConstantType type;
    union {
        int8_t int8;
        int32_t int32;
        AdtConstant adt;
    };
    ConstantInfo() = default;
    ConstantInfo(int8_t i): type{ConstantType::int8}, int8{i} {}
    ConstantInfo(int32_t i): type{ConstantType::int32}, int32{i} {}
    ConstantInfo(AdtConstant a): type{ConstantType::adt}, adt{a} {}
};
using ConstantTable = std::vector<ConstantInfo>;

using Bytecode = std::vector<Instruction>;
struct FunctionInfo {
    index_t num_args;
    index_t num_locals;
    Bytecode code;
};
using FunctionTable = std::vector<FunctionInfo>;

struct ParseError {};
struct InvalidBytecodeError {};
struct Assembly {
    AdtTable adt_table;
    ConstantTable constant_table;
    FunctionTable function_table;

    static Assembly parse(std::istream&);
};

void validate(Assembly&);
void dump(Assembly&, std::ostream&);

}
}
