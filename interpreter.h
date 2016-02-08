#pragma once
#include <stdint.h>
#include <vector>
#include <stack>
#include "instruction.h"
#include "assembly.h"

namespace rvm {
namespace interpreter {

struct Operand;
struct Pointer {
    uint32_t index;
};
struct Adt {
    uint32_t adt_table_index;
    uint32_t constructor_index;
    Operand* fields;
};
struct Operand {
    union {
        uint8_t uint8;
        uint32_t uint32;
        Pointer pointer;
        Adt adt;
    };
    Operand(): adt{0} {}
    explicit Operand(uint8_t u): Operand{} { uint8 = u; }
    explicit Operand(uint32_t u): Operand{} { uint32 = u; }
    explicit Operand(Pointer p): Operand{} { pointer = p;  }
    explicit Operand(Adt a): Operand{} { adt = a; }
    explicit Operand(const assembly::ConstantInfo& v) {
        switch (v.type) {
            case assembly::ConstantType::uint8:
                uint8 = v.uint8;
                break;
            case assembly::ConstantType::uint32:
                uint32 = v.uint32;
                break;
            case assembly::ConstantType::adt:
            {
                adt.adt_table_index = v.adt.adt_table_index;
                adt.constructor_index = v.adt.constructor_index;
                adt.fields = new Operand[v.adt.num_fields];
                for (auto i = 0u; i < v.adt.num_fields; ++i) {
                    new(&adt.fields[i]) Operand{v.adt.fields[i]};
                }
                break;
            }
        }
    }
};

class Stack: private std::vector<Operand> {
public:
    struct UnderflowError {};

    Stack() = default;
    ~Stack() = default;
    using std::vector<Operand>::operator[];
    using std::vector<Operand>::size;
    using std::vector<Operand>::resize;

    void push(Operand v) {
        this->push_back(v);
    }
    Operand pop() {
        if (this->size() == 0) throw UnderflowError{};
        auto re = this->back();
        this->pop_back();
        return re;
    }
};

using assembly::AdtTable;
using assembly::ConstantTable;
enum class FunctionType {
    managed, native
};
using NativeFunction = Operand (*)(Operand*);
struct NativeFunctionInfo {
    uint32_t num_args;
    NativeFunction native_function;
};
struct FunctionInfo {
    FunctionType type;
    uint32_t num_args;
    uint32_t num_locals;
    assembly::Bytecode code;
    NativeFunction native_function;

    FunctionInfo() = default;
    explicit FunctionInfo(const assembly::FunctionInfo& f):
        type{FunctionType::managed},
        num_args{f.num_args},
        num_locals{f.num_locals},
        code{f.code}  {}
    FunctionInfo(uint32_t a, uint32_t l, assembly::Bytecode c):
        type{FunctionType::managed},
        num_args{a},
        num_locals{l},
        code{c} {}
    FunctionInfo(NativeFunctionInfo fi):
        type{FunctionType::native},
        num_args{fi.num_args},
        native_function{fi.native_function} {}
};
using FunctionTable = std::vector<FunctionInfo>;

class Interpreter {
public:
    struct IndexOutOfBoundError {};

    Interpreter(const assembly::Assembly& a):
        adt_table{a.adt_table},
        constant_table{a.constant_table},
        function_table{a.function_table.size()}  {
        for (size_t i = 0; i < a.function_table.size(); ++i) {
            new(&function_table[i]) FunctionInfo{a.function_table[i]};
        }
        enter(assembly::MAIN_FUNCTION_INDEX);
    }
    ~Interpreter() = default;
    void run();
    void step();
    void set_function(uint32_t idx, NativeFunctionInfo f) {
        function_table[idx] = f;
    }

private:
    AdtTable adt_table;
    ConstantTable constant_table;
    FunctionTable function_table;

    Stack operand_stack{};
    std::stack<uint32_t> frames{{}};
    uint32_t current_function = assembly::MAIN_FUNCTION_INDEX;
    uint32_t program_counter = 0;
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
