#pragma once
#include <functional>
#include <stdint.h>
#include <vector>
#include <stack>
#include "instruction.h"
#include "assembly.h"

namespace rvm {
namespace interpreter {

struct Adt;
struct Operand {
    union {
        int8_t int8;
        int32_t int32;
        Adt* adt;
    };

    Operand() = default;
    explicit Operand(int8_t i): int8{i} {}
    explicit Operand(int32_t i): int32{i} {}
    explicit Operand(Adt* a): adt{a} {}
    explicit Operand(assembly::ConstantInfo&);
    void free_adt() {
        free(adt);
    }
};
struct Adt {
    index_t adt_table_index;
    sindex_t constructor_index;
    Operand fields[1];
};

struct NativeInfo {
    std::function<Operand(Operand[])> func;
    index_t num_args;
};

class Interpreter {
public:
    struct IndexOutOfBoundError {};
    struct StackUnderflowError {};

    Interpreter(const assembly::Assembly& a): assembly(a) {
        enter(assembly::MAIN_FUNCTION_INDEX);
    }
    void run();
    void step();
    void add_native_function(NativeInfo f) {
        native_table.push_back(f);
    }

private:
    assembly::Assembly assembly{};
    std::vector<NativeInfo> native_table{};

    std::vector<Operand> operand_stack{};
    std::stack<int32_t> frames{{}};
    index_t current_function_index{0};
    index_t program_counter;
    bool running{true};

    assembly::FunctionInfo& current_function();
    index_t arg_offset(index_t);
    index_t local_offset(index_t);
    void enter(index_t);
    void call_native(index_t);
    void leave();
    template <class Func>
    void arithmetic_binop(Func);
    template <class Func>
    void logic_binop(Func);
    template <class Func>
    void logic_binop_un(Func);
};

inline Operand::Operand(assembly::ConstantInfo& c) {
    switch (c.type) {
        case assembly::ConstantType::int8:
            int8 = c.int8;
            break;
        case assembly::ConstantType::int32:
            int32 = c.int32;
            break;
        case assembly::ConstantType::adt:
            adt = (Adt*) malloc(sizeof(Adt) + c.adt.num_fields - 1);
            for (auto i = c.adt.num_fields; i != 0; --i) {
                adt->fields[i - 1] = Operand{c.adt.fields[i - 1]};
            }
            break;
    }
}

}
}
