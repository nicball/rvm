#include "rvm.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>

rvm::interpreter::Operand native_print_uint32(rvm::interpreter::Operand* v) {
    std::cout << v[0].uint32 << std::endl;
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
    rvm::assembly::Assembly assfile{adttab, consttab, functab};

    std::ofstream ofs("1.rbc", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    assfile.dump(ofs);
    ofs.close();

    std::ifstream ifs("1.rbc", std::ios_base::in | std::ios_base::binary);
    auto newassfile = rvm::assembly::Assembly::parse(ifs);

    rvm::interpreter::Interpreter vm{newassfile};
    vm.set_function(1, rvm::interpreter::NativeFunctionInfo{1, native_print_uint32});
    vm.run();

    char a;
    std::cin >> a;

    return 0;
}