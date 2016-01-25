#pragma once
#include <stdint.h>
#include <vector>
#include <stack>
#include "instruction.h"

namespace rvm {
namespace interpreter {

struct Value;

struct Pointer {
    uint32_t index;
};

struct Adt {
    uint32_t adt_table_index;
    uint32_t constructor_index;
    Value* data;
};

struct Value {
    union {
        uint8_t uint8;
        uint32_t uint32;
        Pointer pointer;
        Adt adt;
    };
    Value(): adt{0} {}
    explicit Value(uint8_t u): Value{} { uint8 = u; }
    explicit Value(uint32_t u): Value{} { uint32 = u; }
    explicit Value(Pointer p): Value{} { pointer = p; }
    explicit Value(Adt a): Value{} { adt = a; }
};

class Stack: private std::vector<Value> {
public:
    struct UnderflowError {};

    Stack() = default;
    ~Stack() = default;
    using std::vector<Value>::operator[];
    using std::vector<Value>::size;
    using std::vector<Value>::resize;

    void push(Value v) {
        this->push_back(v);
    }
    Value pop() {
        if (this->size() == 0) throw UnderflowError{};
        auto re = this->back();
        this->pop_back();
        return re;
    }
};

class Bytecode: private std::vector<uint8_t> {
public:
    using std::vector<uint8_t>::vector;
    using std::vector<uint8_t>::operator[];
    using std::vector<uint8_t>::size;

    Bytecode& emit(Instruction i) {
        this->push_back(static_cast<uint8_t>(i));
        return *this;
    }
    Bytecode& emit(Instruction i, OperandType t) {
        emit(i);
        this->push_back(static_cast<uint8_t>(t));
        return *this;
    }
    Bytecode& emit(Instruction i, uint32_t idx) {
        emit(i);
        this->push_back((idx & 0xFF000000) >> 24);
        this->push_back((idx & 0x00FF0000) >> 16);
        this->push_back((idx & 0x0000FF00) >> 8);
        this->push_back((idx & 0x000000FF) >> 0);
        return *this;
    }
};

enum class FunctionType {
    managed,
    native
};

struct FunctionInfo {

    using native_function = Value(*)(Value*);
    FunctionType type;
    uint32_t num_args;
    uint32_t num_locals;
    Bytecode code;
    native_function pnf;

    FunctionInfo(uint32_t a, uint32_t l, Bytecode b):
        type{FunctionType::managed},
        num_args{a},
        num_locals{l},
        code{std::move(b)} {}
    FunctionInfo(uint32_t a, native_function p):
        type{FunctionType::native},
        num_args{a},
        num_locals{0},
        code{},
        pnf{p} {}
};

using FunctionTable = std::vector<FunctionInfo>;

struct ConstructorInfo {
    uint32_t num_fields;
};

using AdtInfo = std::vector<ConstructorInfo>;

using AdtTable = std::vector<AdtInfo>;

using ConstantTable = std::vector<Value>;

class Interpreter {
public:
    struct IndexOutOfBoundError {};

    Interpreter(AdtTable a, ConstantTable c, FunctionTable f):
        adt_table{a}, constant_table{c}, function_table{f} {
        enter(MAIN_FUNCTION_INDEX);
        if (frames.size() == 0) running = false;
    }
    ~Interpreter() = default;
    void run();
    void step();

private:
    AdtTable adt_table;
    ConstantTable constant_table;
    FunctionTable function_table;

    Stack operand_stack;
    std::stack<uint32_t> frames;
    uint32_t current_function;
    uint32_t program_counter;
    bool running{true};

    uint8_t read_u8();
    Instruction read_instruction();
    OperandType read_type();
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
}
