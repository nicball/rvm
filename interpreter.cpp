#include "interpreter.h"
#include <functional>
#include <stdlib.h>

using namespace rvm;
using namespace rvm::interpreter;
using namespace rvm::assembly;

namespace {
    Operand pop(std::vector<Operand>& v) {
        auto re = v.back();
        v.pop_back();
        return re;
    }
}

FunctionInfo& Interpreter::current_function() {
    return assembly.function_table[current_function_index];
}

index_t Interpreter::arg_offset(index_t idx) {
    return frames.top()
         - 2
         - current_function().num_args
         + idx;
}

index_t Interpreter::local_offset(index_t idx) {
    return frames.top() + idx;
}

void Interpreter::enter(index_t idx) {
    operand_stack.push_back(Operand{current_function_index});
    operand_stack.push_back(Operand{program_counter});
    frames.push(operand_stack.size());
    operand_stack.resize(operand_stack.size()
        + assembly.function_table[idx].num_locals);
    current_function_index = idx;
    program_counter = 0;
}

void Interpreter::call_native(index_t idx) {
    auto&& ni = native_table[idx];
    auto re = ni.func(&operand_stack[operand_stack.size() - ni.num_args]);
    operand_stack.resize(operand_stack.size() - ni.num_args);
    operand_stack.push_back(re);
}

void Interpreter::leave() {
    auto retval = pop(operand_stack);
    if (frames.size() != 1) {
        operand_stack.resize(frames.top());
        auto old_pc = pop(operand_stack).int32;
        auto old_func = pop(operand_stack).int32;
        operand_stack.resize(operand_stack.size()
                           - current_function().num_args);
        operand_stack.push_back(retval);
        frames.pop();
        current_function_index = old_func;
        program_counter = old_pc;
    }
    else {
        running = false;
    }
}

template <class Func>
void Interpreter::arithmetic_binop(Func f) {
    switch (current_function().code[program_counter].type) {
        case OperandType::int8:
        {
            auto y = pop(operand_stack).int8;
            auto x = pop(operand_stack).int8;
            operand_stack.push_back(Operand{static_cast<int8_t>(f(x, y))});
            break;
        }
        case OperandType::int32:
        {
            auto y = pop(operand_stack).int32;
            auto x = pop(operand_stack).int32;
            operand_stack.push_back(Operand{static_cast<int32_t>(f(x, y))});
            break;
        }
    }
}

template <class Func>
void Interpreter::logic_binop(Func f) {
    switch (current_function().code[program_counter].type) {
        case OperandType::int8:
        {
            auto y = pop(operand_stack).int8;
            auto x = pop(operand_stack).int8;
            operand_stack.push_back(Operand{static_cast<int8_t>(f(x, y))});
            break;
        }
        case OperandType::int32:
        {
            auto y = pop(operand_stack).int32;
            auto x = pop(operand_stack).int32;
            operand_stack.push_back(Operand{static_cast<int8_t>(f(x, y))});
            break;
        }
    }
}

template <class Func>
void Interpreter::logic_binop_signed(Func f) {
    switch (current_function().code[program_counter].type) {
        case OperandType::int8:
        {
            auto y = pop(operand_stack).int8;
            auto x = pop(operand_stack).int8;
            operand_stack.push_back(Operand{static_cast<int8_t>(f(x, y))});
            break;
        }
        case OperandType::int32:
        {
            auto y = pop(operand_stack).int32;
            auto x = pop(operand_stack).int32;
            operand_stack.push_back(Operand{static_cast<int8_t>(f(x, y))});
            break;
        }
    }
}

void Interpreter::run() {
    while (running) {
        step();
    }
}

void Interpreter::step() {
    switch (current_function().code[program_counter].op) {
        case Operation::add:
            arithmetic_binop(std::plus<>{});
            break;
        case Operation::sub:
            arithmetic_binop(std::minus<>{});
            break;
        case Operation::mul:
            arithmetic_binop(std::multiplies<>{});
            break;
        case Operation::div:
            arithmetic_binop(std::divides<>{});
            break;
        case Operation::rem:
            arithmetic_binop(std::modulus<>{});
            break;
        case Operation::band:
            arithmetic_binop(std::bit_and<>{});
            break;
        case Operation::bor:
            arithmetic_binop(std::bit_or<>{});
            break;
        case Operation::bxor:
            arithmetic_binop(std::bit_xor<>{});
            break;
        case Operation::bnot:
        {
            switch (current_function().code[program_counter].type) {
                case OperandType::int32:
                {
                    auto x = pop(operand_stack).int32;
                    operand_stack.push_back(Operand{static_cast<int32_t>(~x)});
                    break;
                }
                case OperandType::int8:
                {
                    auto x = pop(operand_stack).int8;
                    operand_stack.push_back(Operand{static_cast<int8_t>(~x)});
                    break;
                }
            }
            break;
        }
        case Operation::dup:
        {
            auto x = pop(operand_stack);
            operand_stack.push_back(x);
            operand_stack.push_back(x);
            break;
        }
        case Operation::drop:
        {
            if (operand_stack.size() == frames.top() + current_function().num_locals) {
                --program_counter;
                throw StackUnderflowError{};
            }
            else {
                pop(operand_stack);
            }
            break;
        }
        case Operation::ldc:
        {
            auto idx = current_function().code[program_counter].index;
            operand_stack.push_back(Operand{assembly.constant_table[idx]});
            break;
        }
        case Operation::ldloc:
        {
            auto idx = current_function().code[program_counter].index;
            auto offset = local_offset(idx);
            operand_stack.push_back(operand_stack[offset]);
            break;
        }
        case Operation::stloc:
        {
            auto idx = current_function().code[program_counter].index;
            auto v = pop(operand_stack);
            auto offset = local_offset(idx);
            operand_stack[offset] = v;
            break;
        }
        case Operation::ldarg:
        {
            auto idx = current_function().code[program_counter].index;
            auto offset = arg_offset(idx);
            operand_stack.push_back(operand_stack[offset]);
            break;
        }
        case Operation::starg:
        {
            auto idx = current_function().code[program_counter].index;
            auto v = pop(operand_stack);
            auto offset = arg_offset(idx);
            operand_stack[offset] = v;
            break;
        }
        case Operation::call:
        {
            auto idx = current_function().code[program_counter].index;
            enter(idx);
            break;
        }
        case Operation::callnative:
        {
            auto idx = current_function().code[program_counter].index;
            call_native(idx);
            break;
        }
        case Operation::ret:
            leave();
            break;
        case Operation::ldloca:
        {
            auto idx = current_function().code[program_counter].index;
            operand_stack.push_back(Operand{local_offset(idx)});
            break;
        }
        case Operation::ldarga:
        {
            auto idx = current_function().code[program_counter].index;
            operand_stack.push_back(Operand{arg_offset(idx)});
            break;
        }
        case Operation::ldfuna:
        {
            auto idx = current_function().code[program_counter].index;
            operand_stack.push_back(Operand{idx});
            break;
        }
        case Operation::calla:
        {
            auto idx = static_cast<index_t>(pop(operand_stack).int32);
            if (idx >= assembly.function_table.size()) {
                --program_counter;
                throw IndexOutOfBoundError{};
            }
            enter(idx);
            break;
        }
        case Operation::ldind:
        {
            auto idx = pop(operand_stack).int32;
            operand_stack.push_back(operand_stack[idx]);
            break;
        }
        case Operation::stind:
        {
            auto v = pop(operand_stack);
            auto idx = pop(operand_stack).int32;
            operand_stack[idx] = v;
            break;
        }
        case Operation::teq:
        {
            auto y = pop(operand_stack);
            auto x = pop(operand_stack);
            auto a = static_cast<int8_t>(memcmp(&x, &y, sizeof(Operand)) == 0 ? 1 : 0);
            operand_stack.push_back(Operand{a});
            break;
        }
        case Operation::tne:
        {
            auto y = pop(operand_stack);
            auto x = pop(operand_stack);
            auto a = static_cast<int8_t>(memcmp(&x, &y, sizeof(Operand)) == 0 ? 0 : 1);
            operand_stack.push_back(Operand{a});
            break;
        }
        case Operation::tlt:
            logic_binop(std::less<>{});
            break;
        case Operation::tlt_s:
            logic_binop_signed(std::less<>{});
            break;
        case Operation::tle:
            logic_binop(std::less_equal<>{});
            break;
        case Operation::tle_s:
            logic_binop_signed(std::less_equal<>{});
            break;
        case Operation::tgt:
            logic_binop(std::less<>{});
            break;
        case Operation::tgt_s:
            logic_binop_signed(std::less<>{});
            break;
        case Operation::tge:
            logic_binop(std::less_equal<>{});
            break;
        case Operation::tge_s:
            logic_binop_signed(std::less_equal<>{});
            break;
        case Operation::br:
        {
            auto idx = current_function().code[program_counter].index;
            program_counter = idx;
            break;
        }
        case Operation::brtrue:
        {
            auto idx = current_function().code[program_counter].index;
            if (pop(operand_stack).int8 != 0) {
                program_counter = idx - 1;
            }
            break;
        }
        case Operation::mkadt:
        {
            auto idx = current_function().code[program_counter].index;
            auto ctor = current_function().code[program_counter].index2;
            auto n = assembly.adt_table[idx][ctor].num_fields;
            auto adt = (Adt*) malloc(sizeof(Adt) + n);
            for (int32_t i = 1; i <= n; ++i) {
                adt->fields[n - i] = pop(operand_stack);
            }
            operand_stack.push_back(Operand{adt});
            break;
        }
        case Operation::dladt:
        {
            pop(operand_stack).free_adt();
            break;
        }
        case Operation::ldctor:
        {
            auto adt = pop(operand_stack).adt;
            operand_stack.push_back(Operand{adt->constructor_index});
            break;
        }
        case Operation::ldfld:
        {
            auto idx = current_function().code[program_counter].index;
            auto adt = pop(operand_stack).adt;
            if (idx >= assembly.adt_table[adt->adt_table_index][adt->constructor_index].num_fields) {
                throw IndexOutOfBoundError{};
            }
            operand_stack.push_back(adt->fields[idx]);
            break;
        }
        case Operation::stfld:
        {
            auto idx = current_function().code[program_counter].index;
            auto adt = pop(operand_stack).adt;
            auto v = pop(operand_stack);
            adt->fields[idx] = v;
            break;
        }
    }
    ++program_counter;
}
