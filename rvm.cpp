#include "rvm.h"
#include <functional>
#include <stdlib.h>

using namespace rvm;

inline uint8_t vm_state::read_u8() {
    return function_table[current_function].code[program_counter++];
}

inline instruction vm_state::read_instruction() {
    return static_cast<instruction>(read_u8());
}

inline operand_type vm_state::read_type() {
    return static_cast<operand_type>(read_u8());
}

inline uint32_t vm_state::read_index() {
    uint32_t re = 0;
    re |= read_u8() << 24;
    re |= read_u8() << 16;
    re |= read_u8() << 8;
    re |= read_u8() << 0;
    return re;
}

inline uint32_t vm_state::arg_offset(uint32_t idx) {
    return frames.top()
         - 2
         - function_table[current_function].num_args
         + idx;
}

inline uint32_t vm_state::local_offset(uint32_t idx) {
    return frames.top() + idx;
}

inline void vm_state::enter(uint32_t idx) {
    switch (function_table[idx].type) {
        case function_type::managed:
        {
            operand_stack.push(rt_value{current_function});
            operand_stack.push(rt_value{program_counter});
            frames.push(operand_stack.size());
            operand_stack.resize(operand_stack.size()
                + function_table[idx].num_locals);
            current_function = idx;
            program_counter = 0;
            break;
        }
        case function_type::native:
        {
            auto offset = operand_stack.size() - function_table[idx].num_args;
            auto retval = function_table[idx].pnf(&operand_stack[offset]);
            operand_stack.resize(offset);
            operand_stack.push(retval);
            break;
        }
    }
}

inline void vm_state::leave() {
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
void vm_state::arithmetic_binop(Func f) {
    switch (read_type()) {
        case operand_type::uint8:
        {
            auto y = operand_stack.pop().uint8;
            auto x = operand_stack.pop().uint8;
            operand_stack.push(rt_value{static_cast<uint8_t>(f(x, y))});
            break;
        }
        case operand_type::uint32:
        {
            auto y = operand_stack.pop().uint32;
            auto x = operand_stack.pop().uint32;
            operand_stack.push(rt_value{static_cast<uint32_t>(f(x, y))});
            break;
        }
    }
}

template <class Func>
void vm_state::logic_binop(Func f) {
    switch (read_type()) {
        case operand_type::uint8:
        {
            auto y = operand_stack.pop().uint8;
            auto x = operand_stack.pop().uint8;
            operand_stack.push(rt_value{static_cast<uint32_t>(f(x, y))});
            break;
        }
        case operand_type::uint32:
        {
            auto y = operand_stack.pop().uint32;
            auto x = operand_stack.pop().uint32;
            operand_stack.push(rt_value{static_cast<uint32_t>(f(x, y))});
            break;
        }
    }
}

template <class Func>
void vm_state::logic_binop_signed(Func f) {
    switch (read_type()) {
        case operand_type::uint8:
        {
            auto y = static_cast<int8_t>(operand_stack.pop().uint8);
            auto x = static_cast<int8_t>(operand_stack.pop().uint8);
            operand_stack.push(rt_value{static_cast<uint32_t>(f(x, y))});
            break;
        }
        case operand_type::uint32:
        {
            auto y = static_cast<int32_t>(operand_stack.pop().uint32);
            auto x = static_cast<int32_t>(operand_stack.pop().uint32);
            operand_stack.push(rt_value{static_cast<uint32_t>(f(x, y))});
            break;
        }
    }
}

void vm_state::run() {
    while (running) {
        step();
    }
}

void vm_state::step() {
    printf("step(): %d [", function_table[current_function].code[program_counter]);
    for (auto i = 0u; i < operand_stack.size(); ++i) {
        printf("%d ", operand_stack[i].uint32);
    }
    printf("]\n");
    switch (read_instruction()) {
        case instruction::add:
            arithmetic_binop(std::plus<>{});
            break;
        case instruction::sub:
            arithmetic_binop(std::minus<>{});
            break;
        case instruction::mul:
            arithmetic_binop(std::multiplies<>{});
            break;
        case instruction::div:
            arithmetic_binop(std::divides<>{});
            break;
        case instruction::rem:
            arithmetic_binop(std::modulus<>{});
            break;
        case instruction::band:
            arithmetic_binop(std::bit_and<>{});
            break;
        case instruction::bor:
            arithmetic_binop(std::bit_or<>{});
            break;
        case instruction::bxor:
            arithmetic_binop(std::bit_xor<>{});
            break;
        case instruction::bnot:
        {
            switch (read_type()) {
                case operand_type::uint32:
                {
                    auto x = operand_stack.pop().uint32;
                    operand_stack.push(rt_value{static_cast<uint32_t>(~x)});
                    break;
                }
                case operand_type::uint8:
                {
                    auto x = operand_stack.pop().uint8;
                    operand_stack.push(rt_value{static_cast<uint8_t>(~x)});
                    break;
                }
            }
            break;
        }
        case instruction::dup:
        {
            auto x = operand_stack.pop();
            operand_stack.push(x);
            operand_stack.push(x);
            break;
        }
        case instruction::drop:
        {
            if (operand_stack.size() == frames.top() + function_table[current_function].num_locals) {
                --program_counter;
                throw opstack_underflow_error{};
            }
            else {
                operand_stack.pop();
            }
            break;
        }
        case instruction::ldc:
        {
            auto idx = read_index();
            operand_stack.push(constant_table[idx]);
            break;
        }
        case instruction::ldloc:
        {
            auto idx = read_index();
            auto offset = local_offset(idx);
            operand_stack.push(operand_stack[offset]);
            break;
        }
        case instruction::stloc:
        {
            auto idx = read_index();
            auto v = operand_stack.pop();
            auto offset = local_offset(idx);
            operand_stack[offset] = v;
            break;
        }
        case instruction::ldarg:
        {
            auto idx = read_index();
            auto offset = arg_offset(idx);
            operand_stack.push(operand_stack[offset]);
            break;
        }
        case instruction::starg:
        {
            auto idx = read_index();
            auto v = operand_stack.pop();
            auto offset = arg_offset(idx);
            operand_stack[offset] = v;
            break;
        }
        case instruction::call:
        {
            auto idx = read_index();
            enter(idx);
            break;
        }
        case instruction::ret:
            leave();
            break;
        case instruction::ldloca:
        {
            auto idx = read_index();
            operand_stack.push(rt_value{local_offset(idx)});
            break;
        }
        case instruction::ldarga:
        {
            auto idx = read_index();
            operand_stack.push(rt_value{arg_offset(idx)});
            break;
        }
        case instruction::ldfuna:
        {
            auto idx = read_index();
            operand_stack.push(rt_value{idx});
            break;
        }
        case instruction::calla:
        {
            auto idx = operand_stack.pop().uint32;
            if (idx >= function_table.size()) {
                --program_counter;
                throw index_out_of_bound_error{};
            }
            enter(idx);
        }
        case instruction::ldind:
        {
            auto idx = operand_stack.pop().pointer.index;
            operand_stack.push(operand_stack[idx]);
            break;
        }
        case instruction::stind:
        {
            auto v = operand_stack.pop();
            auto idx = operand_stack.pop().pointer.index;
            operand_stack[idx] = v;
            break;
        }
        case instruction::teq:
        {
            auto y = operand_stack.pop();
            auto x = operand_stack.pop();
            auto a = static_cast<uint8_t>(memcmp(&x, &y, sizeof(rt_value)) == 0 ? 1 : 0);
            operand_stack.push(rt_value{a});
            break;
        }
        case instruction::tne:
        {
            auto y = operand_stack.pop();
            auto x = operand_stack.pop();
            auto a = static_cast<uint8_t>(memcmp(&x, &y, sizeof(rt_value)) == 0 ? 0 : 1);
            operand_stack.push(rt_value{a});
            break;
        }
        case instruction::tlt:
            logic_binop(std::less<>{});
            break;
        case instruction::tlt_s:
            logic_binop_signed(std::less<>{});
            break;
        case instruction::tle:
            logic_binop(std::less_equal<>{});
            break;
        case instruction::tle_s:
            logic_binop_signed(std::less_equal<>{});
            break;
        case instruction::tgt:
            logic_binop(std::less<>{});
            break;
        case instruction::tgt_s:
            logic_binop_signed(std::less<>{});
            break;
        case instruction::tge:
            logic_binop(std::less_equal<>{});
            break;
        case instruction::tge_s:
            logic_binop_signed(std::less_equal<>{});
            break;
        case instruction::br:
        {
            auto idx = read_index();
            program_counter = idx;
            break;
        }
        case instruction::brtrue:
        {
            auto idx = read_index();
            if (operand_stack.pop().uint32 != 0) {
                program_counter = idx;
            }
            break;
        }
        case instruction::mkadt:
        {
            auto idx = read_index();
            auto ctor = read_index();
            auto n = adt_table[idx][ctor].num_fields;
            auto* data = new rt_value[n];
            for (uint32_t i = 1; i <= n; ++i) {
                data[n - i] = operand_stack.pop();
            }
            rt_adt adt{idx, ctor, data};
            operand_stack.push(rt_value{adt});
            break;
        }
        case instruction::dladt:
        {
            auto adt = operand_stack.pop().adt;
            delete adt.data;
            break;
        }
        case instruction::ldctor:
        {
            auto adt = operand_stack.pop().adt;
            operand_stack.push(rt_value{adt.constructor_index});
            break;
        }
        case instruction::ldfld:
        {
            auto idx = read_index();
            auto adt = operand_stack.pop().adt;
            operand_stack.push(adt.data[idx]);
            break;
        }
        case instruction::stfld:
        {
            auto idx = read_index();
            auto adt = operand_stack.pop().adt;
            auto v = operand_stack.pop();
            adt.data[idx] = v;
            break;
        }
    }
}
