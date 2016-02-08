#include "rvm.h"
#include <iostream>

rvm::interpreter::Operand native_print_uint32(rvm::interpreter::Operand* v) {
    std::cout << v->uint32 << std::endl;
    return rvm::interpreter::Operand{static_cast<uint32_t>(0u)};
}

int main() {
    rvm::assembly::BytecodeBuilder main_bc;
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
        .emit(rvm::Instruction::call, 1)
        .emit(rvm::Instruction::ret);       // return native_print_uint32(i);
    rvm::assembly::FunctionTable functab{
        rvm::assembly::FunctionInfo{0, 1, main_bc.get()},
        rvm::assembly::FunctionInfo{0, 0, {}} // stub
    };
    rvm::assembly::ConstantTable consttab{
        rvm::assembly::ConstantInfo{static_cast<uint32_t>(1u)},
        rvm::assembly::ConstantInfo{static_cast<uint32_t>(10u)}
    };
    rvm::assembly::AdtTable adttab{};

    rvm::interpreter::Interpreter vm{rvm::assembly::Assembly{adttab, consttab, functab}};
    vm.set_function(1, rvm::interpreter::NativeFunctionInfo{1, native_print_uint32});
    vm.run();

    char a;
    std::cin >> a;

    return 0;
}