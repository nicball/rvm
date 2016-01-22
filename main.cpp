#include "rvm.h"
#include <iostream>

rvm::rt_value native_print_uint32(rvm::rt_value* v) {
    std::cout << v->uint32 << std::endl;
    return rvm::rt_value{(uint32_t)0};
}

int main() {
    rvm::bytecode main_bc;
    main_bc
        .emit(rvm::instruction::ldc, 1)
        .emit(rvm::instruction::stloc, 0)   // i = 1;
        .emit(rvm::instruction::ldloc, 0)   // loop:
        .emit(rvm::instruction::call, 1)
        .emit(rvm::instruction::drop)       // native_print_uint32(i);
        .emit(rvm::instruction::ldloc, 0)
        .emit(rvm::instruction::ldc, 1)
        .emit(rvm::instruction::add, rvm::operand_type::uint32)
        .emit(rvm::instruction::stloc, 0)   // i = i + 1;
        .emit(rvm::instruction::ldloc, 0)
        .emit(rvm::instruction::ldc, 2)
        .emit(rvm::instruction::tlt, rvm::operand_type::uint32)
        .emit(rvm::instruction::brtrue, 10) // if (i < 100) goto loop;
        .emit(rvm::instruction::ldloc, 0)
        .emit(rvm::instruction::call, 1)    // native_print_uint32(i);
        .emit(rvm::instruction::ret);       // return
    rvm::function_table functab{
        rvm::function_info{0, 1, main_bc},
        rvm::function_info{1, native_print_uint32}
    };
    rvm::constant_table consttab{
        rvm::rt_value{static_cast<uint32_t>(123u)},
        rvm::rt_value{static_cast<uint32_t>(1u)},
        rvm::rt_value{static_cast<uint32_t>(10u)}
    };
    rvm::adt_table adttab{};

    rvm::vm_state vm{adttab, consttab, functab};
    vm.run();

    char a;
    std::cin >> a;

    return 0;
}