#pragma once
#ifndef RVM_H

#include <vector>
#include <stack>
#include <stdint.h>
#include <utility>
#include <algorithm>

namespace rvm {

static constexpr uint32_t MAIN_FUNCTION_INDEX = 0;

struct invalid_bytecode_error {};
struct opstack_underflow_error {};
struct index_out_of_bound_error {};

enum class operand_type: uint8_t {
    uint8,
    uint32,
    pointer,
    adt
};

enum class instruction: uint8_t {
    add,    // <type>
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

struct rt_value;
struct rt_pointer {
    uint32_t index;
};
struct rt_adt {
    uint32_t adt_table_index;
    uint32_t constructor_index;
    rt_value* data;
};
struct rt_value {
    union {
        uint8_t uint8;
        uint32_t uint32;
        rt_pointer pointer;
        rt_adt adt;
    };
    rt_value(): adt{0} {}
    explicit rt_value(uint8_t u): uint8{u} {}
    explicit rt_value(uint32_t u): uint32{u} {}
    explicit rt_value(rt_pointer p): pointer(p) {}
    explicit rt_value(rt_adt a): adt(a) {}
};

class rt_stack: private std::vector<rt_value> {
public:
    rt_stack() = default;
    ~rt_stack() = default;

    using std::vector<rt_value>::operator[];
    using std::vector<rt_value>::size;
    using std::vector<rt_value>::resize;

    void push(rt_value v) {
        this->push_back(v);
    }
    rt_value pop() {
        if (this->size() == 0) throw opstack_underflow_error{};
        auto re = this->back();
        this->pop_back();
        return re;
    }
};

class bytecode: private std::vector<uint8_t> {
public:
    using std::vector<uint8_t>::vector;
    using std::vector<uint8_t>::operator[];
    using std::vector<uint8_t>::size;

    bytecode& emit(instruction i) {
        this->push_back(static_cast<uint8_t>(i));
        return *this;
    }
    bytecode& emit(instruction i, operand_type t) {
        emit(i);
        this->push_back(static_cast<uint8_t>(t));
        return *this;
    }
    bytecode& emit(instruction i, uint32_t idx) {
        emit(i);
        this->push_back((idx & 0xFF000000) >> 24);
        this->push_back((idx & 0x00FF0000) >> 16);
        this->push_back((idx & 0x0000FF00) >> 8);
        this->push_back((idx & 0x000000FF) >> 0);
        return *this;
    }
};

enum class function_type: uint8_t {
    managed,
    native
};
using native_function = rt_value (*)(rt_value*);
struct function_info {
    function_type type;
    uint32_t num_args;
    uint32_t num_locals;
    bytecode code;
    native_function pnf;

    function_info(uint32_t a, uint32_t l, bytecode b):
        type{function_type::managed},
        num_args{a},
        num_locals{l},
        code{std::move(b)} {}
    function_info(uint32_t a, native_function p):
        type{function_type::native},
        num_args{a},
        num_locals{0},
        code{},
        pnf{p} {}
};
using function_table = std::vector<function_info>;

struct constructor_info {
    uint32_t num_fields;
};
using adt_info = std::vector<constructor_info>;
using adt_table = std::vector<adt_info>;

using constant_table = std::vector<rt_value>;

class vm_state {
public:
    vm_state(adt_table a, constant_table c, function_table f):
        adt_table{a}, constant_table{c}, function_table{f} {
        enter(MAIN_FUNCTION_INDEX);
    }
    ~vm_state() = default;
    void run();
    void step();

private:
    adt_table adt_table;
    constant_table constant_table;
    function_table function_table;

    rt_stack operand_stack;
    std::stack<uint32_t> frames;
    uint32_t current_function;
    uint32_t program_counter;
    bool running{true};

    uint8_t read_u8();
    instruction read_instruction();
    operand_type read_type();
    uint32_t read_index();
    uint32_t arg_offset(uint32_t);
    uint32_t local_offset(uint32_t);
    void enter(uint32_t);
    void leave();
    template <class Func>
    void arithmetic_binop(Func);
    template <class Func>
    void logic_binop(Func);
    template <class Func>
    void logic_binop_signed(Func);
};

}

#endif
