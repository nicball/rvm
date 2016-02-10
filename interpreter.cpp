#include "interpreter.h"
#include <functional>
#include <stdlib.h>

using namespace rvm;
using namespace rvm::interpreter;

uint8_t Interpreter::read_uint8() {
    return function_table[current_function].code[program_counter++];
}

Instruction Interpreter::read_instruction() {
    return static_cast<Instruction>(read_uint8());
}

OperandType Interpreter::read_operand_type() {
    return static_cast<OperandType>(read_uint8());
}

uint32_t Interpreter::read_uint32() {
    uint32_t re = 0;
    re |= read_uint8() << 24;
    re |= read_uint8() << 16;
    re |= read_uint8() << 8;
    re |= read_uint8() << 0;
    return re;
}

uint32_t Interpreter::arg_offset(uint32_t idx) {
    return frames.top()
         - 2
         - function_table[current_function].num_args
         + idx;
}

uint32_t Interpreter::local_offset(uint32_t idx) {
    return frames.top() + idx;
}

void Interpreter::enter(uint32_t idx) {
    switch (function_table[idx].type) {
        case FunctionType::managed:
        {
            operand_stack.push(Operand{current_function});
            operand_stack.push(Operand{program_counter});
            frames.push(operand_stack.size());
            operand_stack.resize(operand_stack.size()
                + function_table[idx].num_locals);
            current_function = idx;
            program_counter = 0;
            break;
        }
        case FunctionType::native:
        {
            auto offset = operand_stack.size() - function_table[idx].num_args;
            auto retval = function_table[idx].native_function(&operand_stack[offset]);
            operand_stack.resize(offset);
            operand_stack.push(retval);
            break;
        }
    }
}

void Interpreter::leave() {
    auto retval = operand_stack.pop();
    if (frames.size() != 1) {
        operand_stack.resize(frames.top());
        auto old_pc = operand_stack.pop().uint32;
        auto old_func = operand_stack.pop().uint32;
        operand_stack.resize(operand_stack.size()
                           - function_table[current_function].num_args);
        operand_stack.push(retval);
        frames.pop();
        current_function = old_func;
        program_counter = old_pc;
    }
    else {
        running = false;
    }
}

template <class Func>
void Interpreter::arithmetic_binop(Func f) {
    switch (read_operand_type()) {
        case OperandType::uint8:
        {
            auto y = operand_stack.pop().uint8;
            auto x = operand_stack.pop().uint8;
            operand_stack.push(Operand{static_cast<uint8_t>(f(x, y))});
            break;
        }
        case OperandType::uint32:
        {
            auto y = operand_stack.pop().uint32;
            auto x = operand_stack.pop().uint32;
            operand_stack.push(Operand{static_cast<uint32_t>(f(x, y))});
            break;
        }
    }
}

template <class Func>
void Interpreter::logic_binop(Func f) {
    switch (read_operand_type()) {
        case OperandType::uint8:
        {
            auto y = operand_stack.pop().uint8;
            auto x = operand_stack.pop().uint8;
            operand_stack.push(Operand{static_cast<uint8_t>(f(x, y))});
            break;
        }
        case OperandType::uint32:
        {
            auto y = operand_stack.pop().uint32;
            auto x = operand_stack.pop().uint32;
            operand_stack.push(Operand{static_cast<uint8_t>(f(x, y))});
            break;
        }
    }
}

template <class Func>
void Interpreter::logic_binop_signed(Func f) {
    switch (read_operand_type()) {
        case OperandType::uint8:
        {
            auto y = static_cast<int8_t>(operand_stack.pop().uint8);
            auto x = static_cast<int8_t>(operand_stack.pop().uint8);
            operand_stack.push(Operand{static_cast<uint8_t>(f(x, y))});
            break;
        }
        case OperandType::uint32:
        {
            auto y = static_cast<int32_t>(operand_stack.pop().uint32);
            auto x = static_cast<int32_t>(operand_stack.pop().uint32);
            operand_stack.push(Operand{static_cast<uint8_t>(f(x, y))});
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
    printf("step(): %d [", function_table[current_function].code[program_counter]);
    for (auto i = 0u; i < operand_stack.size(); ++i) {
        printf("%d ", operand_stack[i].uint32);
    }
    printf("]\n");
    switch (read_instruction()) {
        case Instruction::add:
            arithmetic_binop(std::plus<>{});
            break;
        case Instruction::sub:
            arithmetic_binop(std::minus<>{});
            break;
        case Instruction::mul:
            arithmetic_binop(std::multiplies<>{});
            break;
        case Instruction::div:
            arithmetic_binop(std::divides<>{});
            break;
        case Instruction::rem:
            arithmetic_binop(std::modulus<>{});
            break;
        case Instruction::band:
            arithmetic_binop(std::bit_and<>{});
            break;
        case Instruction::bor:
            arithmetic_binop(std::bit_or<>{});
            break;
        case Instruction::bxor:
            arithmetic_binop(std::bit_xor<>{});
            break;
        case Instruction::bnot:
        {
            switch (read_operand_type()) {
                case OperandType::uint32:
                {
                    auto x = operand_stack.pop().uint32;
                    operand_stack.push(Operand{static_cast<uint32_t>(~x)});
                    break;
                }
                case OperandType::uint8:
                {
                    auto x = operand_stack.pop().uint8;
                    operand_stack.push(Operand{static_cast<uint8_t>(~x)});
                    break;
                }
            }
            break;
        }
        case Instruction::dup:
        {
            auto x = operand_stack.pop();
            operand_stack.push(x);
            operand_stack.push(x);
            break;
        }
        case Instruction::drop:
        {
            if (operand_stack.size() == frames.top() + function_table[current_function].num_locals) {
                --program_counter;
                throw Stack::UnderflowError{};
            }
            else {
                operand_stack.pop();
            }
            break;
        }
        case Instruction::ldc:
        {
            auto idx = read_uint32();
            operand_stack.push(Operand{constant_table[idx]});
            break;
        }
        case Instruction::ldloc:
        {
            auto idx = read_uint32();
            auto offset = local_offset(idx);
            operand_stack.push(operand_stack[offset]);
            break;
        }
        case Instruction::stloc:
        {
            auto idx = read_uint32();
            auto v = operand_stack.pop();
            auto offset = local_offset(idx);
            operand_stack[offset] = v;
            break;
        }
        case Instruction::ldarg:
        {
            auto idx = read_uint32();
            auto offset = arg_offset(idx);
            operand_stack.push(operand_stack[offset]);
            break;
        }
        case Instruction::starg:
        {
            auto idx = read_uint32();
            auto v = operand_stack.pop();
            auto offset = arg_offset(idx);
            operand_stack[offset] = v;
            break;
        }
        case Instruction::call:
        {
            auto idx = read_uint32();
            enter(idx);
            break;
        }
        case Instruction::ret:
            leave();
            break;
        case Instruction::ldloca:
        {
            auto idx = read_uint32();
            operand_stack.push(Operand{local_offset(idx)});
            break;
        }
        case Instruction::ldarga:
        {
            auto idx = read_uint32();
            operand_stack.push(Operand{arg_offset(idx)});
            break;
        }
        case Instruction::ldfuna:
        {
            auto idx = read_uint32();
            operand_stack.push(Operand{idx});
            break;
        }
        case Instruction::calla:
        {
            auto idx = operand_stack.pop().uint32;
            if (idx >= function_table.size()) {
                --program_counter;
                throw IndexOutOfBoundError{};
            }
            enter(idx);
        }
        case Instruction::ldind:
        {
            auto idx = operand_stack.pop().pointer.index;
            operand_stack.push(operand_stack[idx]);
            break;
        }
        case Instruction::stind:
        {
            auto v = operand_stack.pop();
            auto idx = operand_stack.pop().pointer.index;
            operand_stack[idx] = v;
            break;
        }
        case Instruction::teq:
        {
            auto y = operand_stack.pop();
            auto x = operand_stack.pop();
            auto a = static_cast<uint8_t>(memcmp(&x, &y, sizeof(Operand)) == 0 ? 1 : 0);
            operand_stack.push(Operand{a});
            break;
        }
        case Instruction::tne:
        {
            auto y = operand_stack.pop();
            auto x = operand_stack.pop();
            auto a = static_cast<uint8_t>(memcmp(&x, &y, sizeof(Operand)) == 0 ? 0 : 1);
            operand_stack.push(Operand{a});
            break;
        }
        case Instruction::tlt:
            logic_binop(std::less<>{});
            break;
        case Instruction::tlt_s:
            logic_binop_signed(std::less<>{});
            break;
        case Instruction::tle:
            logic_binop(std::less_equal<>{});
            break;
        case Instruction::tle_s:
            logic_binop_signed(std::less_equal<>{});
            break;
        case Instruction::tgt:
            logic_binop(std::less<>{});
            break;
        case Instruction::tgt_s:
            logic_binop_signed(std::less<>{});
            break;
        case Instruction::tge:
            logic_binop(std::less_equal<>{});
            break;
        case Instruction::tge_s:
            logic_binop_signed(std::less_equal<>{});
            break;
        case Instruction::br:
        {
            auto idx = read_uint32();
            program_counter = idx;
            break;
        }
        case Instruction::brtrue:
        {
            auto idx = read_uint32();
            if (operand_stack.pop().uint8 != 0) {
                program_counter = idx;
            }
            break;
        }
        case Instruction::mkadt:
        {
            auto idx = read_uint32();
            auto ctor = read_uint32();
            auto n = adt_table[idx][ctor].num_fields;
            auto* fields = new Operand[n];
            for (uint32_t i = 1; i <= n; ++i) {
                fields[n - i] = operand_stack.pop();
            }
            Adt adt{idx, ctor, fields};
            operand_stack.push(Operand{adt});
            break;
        }
        case Instruction::dladt:
        {
            auto adt = operand_stack.pop().adt;
            delete adt.fields;
            break;
        }
        case Instruction::ldctor:
        {
            auto adt = operand_stack.pop().adt;
            operand_stack.push(Operand{adt.constructor_index});
            break;
        }
        case Instruction::ldfld:
        {
            auto idx = read_uint32();
            auto adt = operand_stack.pop().adt;
            operand_stack.push(adt.fields[idx]);
            break;
        }
        case Instruction::stfld:
        {
            auto idx = read_uint32();
            auto adt = operand_stack.pop().adt;
            auto v = operand_stack.pop();
            adt.fields[idx] = v;
            break;
        }
    }
}
