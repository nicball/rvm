#pragma once
#include <stdint.h>

namespace rvm {

enum class OperandType: int8_t {
    int8 = 1,
    int32,
    pointer,
    adt
};

enum class Operation: int8_t {
    add = 1,    // <type>
    sub,    // <type>
    mul,    // <type>
    div,    // <type>
    rem,    // <type>
    band,   // <type>
    bor,    // <type>
    bxor,   // <type>
    bnot,   // <type>

    dup,
    drop,
    ldc,    // #constant_table
    ldloc,  // #locals
    stloc,  // #locals
    ldarg,  // #args
    starg,  // #args
    call,   // #function_table
    callnative, // #native_table
    ret,
    ldloca, // #locals
    ldarga, // #args
    ldfuna, // #function_table
    calla,
    ldind,
    stind,

    teq,    // <type>
    tne,    // <type>
    tlt,    // <type>
    tlt_s,  // <type>
    tle,    // <type>
    tle_s,  // <type>
    tgt,    // <type>
    tgt_s,  // <type>
    tge,    // <type>
    tge_s,  // <type>

    br,     // #code
    brtrue, // #code

    mkadt,  // #adt_table, #constructor
    dladt,
    ldctor,
    ldfld,  // field_index
    stfld,  // field_index
};

using index_t = uint16_t;
using sindex_t = uint8_t;
struct Instruction {
    Operation op;
    union {
        index_t index{0};
        OperandType type;
    };
    sindex_t index2{0};

    Instruction() = default;
    explicit Instruction(Operation o): op{o} {}
    Instruction(Operation o, OperandType t): op{o}, type{t} {}
    Instruction(Operation o, index_t d): op{o}, index{d} {}
    Instruction(Operation o, index_t d, sindex_t s): op{o}, index{d}, index2{s} {}
};


}
