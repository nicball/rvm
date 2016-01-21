#include "rvm.h"
#include <iostream>

rvm::rt_value native_print_uint32(rvm::rt_value* v) {
    std::cout << v->uint32 << std::endl;
    return rvm::rt_value{(uint32_t)0};
}

int main() {
    rvm::bytecode main_bc;
    main_bc.emit(rvm::instruction::ldc, 0)
        .emit(rvm::instruction::call, 1)
        .emit(rvm::instruction::ret);
    rvm::function_table functab{
        rvm::function_info{0, 0, main_bc},
        rvm::function_info{1, native_print_uint32}
    };
    rvm::constant_table consttab{
        rvm::rt_value{static_cast<uint32_t>(123u)}
    };
    rvm::adt_table adttab{};

    rvm::vm_state vm;
    vm.function_table = functab;
    vm.constant_table = consttab;
    vm.adt_table = adttab;
    vm.run();

    char a;
    std::cin >> a;

    return 0;
}
