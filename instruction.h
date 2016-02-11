#pragma once
#include <stdint.h>

namespace rvm {

enum class OperandType: uint8_t {
    uint8 = 1,
    uint32,
    pointer,
    adt
};

enum class Instruction: uint8_t {
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

}
