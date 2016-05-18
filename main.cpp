#include "rvm.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>

rvm::interpreter::Operand native_print_int32(rvm::interpreter::Operand* v) {
    std::cout << v[0].int32 << std::endl;
    return rvm::interpreter::Operand{0};
}

int main() {
    using namespace rvm;
    auto main_bc = assembly::Bytecode{
        Instruction{Operation::ldc, 0},
        Instruction{Operation::stloc, 0},   // i = 1;
        Instruction{Operation::ldloc, 0},   // loop:
        Instruction{Operation::callnative, 0},
        Instruction{Operation::drop},       // native_print_int32(i);
        Instruction{Operation::ldloc, 0},
        Instruction{Operation::ldc, 0},
        Instruction{Operation::add, OperandType::int32},
        Instruction{Operation::stloc, 0},   // i = i + 1;
        Instruction{Operation::ldloc, 0},
        Instruction{Operation::ldc, 1},
        Instruction{Operation::tlt, OperandType::int32},
        Instruction{Operation::brtrue, 2}, // if (i < 10) goto loop;
        Instruction{Operation::ldloc, 0},
        Instruction{Operation::callnative, 0},
        Instruction{Operation::ret}       // return native_print_int32(i);
    };
    auto functab = assembly::FunctionTable{
        assembly::FunctionInfo{0, 1, main_bc}
    };
    auto consttab = assembly::ConstantTable{
        assembly::ConstantInfo{1},
        assembly::ConstantInfo{10}
    };
    auto adttab = assembly::AdtTable{};
    auto assfile = assembly::Assembly{adttab, consttab, functab};

    auto ofs = std::ofstream{"1.rbc", std::ios_base::out | std::ios_base::trunc | std::ios_base::binary};
    assembly::dump(assfile, ofs);
    ofs.close();

    auto ifs = std::ifstream{"1.rbc", std::ios_base::in | std::ios_base::binary};
    auto newassfile = assembly::Assembly::parse(ifs);
    assembly::validate(newassfile);

    auto vm = interpreter::Interpreter{newassfile};
    vm.add_native_function({native_print_int32, 1});
    vm.run();

    auto a = '\0';
    std::cin >> a;

    return 0;
}