#include "rvm.h"
#include <iostream>

rvm::interpreter::Value native_print_uint32(rvm::interpreter::Value* v) {
    std::cout << v->uint32 << std::endl;
    return rvm::interpreter::Value{static_cast<uint32_t>(0)};
}

int main() {
    rvm::interpreter::Bytecode main_bc;
    main_bc
        .emit(rvm::Instruction::ldc, 0)
        .emit(rvm::Instruction::stloc, 0)   // i = 1;
        .emit(rvm::Instruction::ldloc, 0)   // loop:
        .emit(rvm::Instruction::call, 1)
        .emit(rvm::Instruction::drop)       // native_print_uint32(i);
        .emit(rvm::Instruction::ldloc, 0)
        .emit(rvm::Instruction::ldc, 0)
        .emit(rvm::Instruction::add, rvm::OperandType::uint32)
        .emit(rvm::Instruction::stloc, 0)   // i = i + 1;
        .emit(rvm::Instruction::ldloc, 0)
        .emit(rvm::Instruction::ldc, 1)
        .emit(rvm::Instruction::tlt, rvm::OperandType::uint32)
        .emit(rvm::Instruction::brtrue, 10) // if (i < 10) goto loop;
        .emit(rvm::Instruction::ldloc, 0)
        .emit(rvm::Instruction::call, 1)    // native_print_uint32(i);
        .emit(rvm::Instruction::ret);       // return
    rvm::interpreter::FunctionTable functab{
        rvm::interpreter::FunctionInfo{0, 1, main_bc},
        rvm::interpreter::FunctionInfo{1, native_print_uint32}
    };
    rvm::interpreter::ConstantTable consttab{
        rvm::interpreter::Value{static_cast<uint32_t>(1u)},
        rvm::interpreter::Value{static_cast<uint32_t>(10u)}
    };
    rvm::interpreter::AdtTable adttab{};

    rvm::interpreter::Interpreter vm{adttab, consttab, functab};
    vm.run();

    char a;
    std::cin >> a;

    return 0;
}