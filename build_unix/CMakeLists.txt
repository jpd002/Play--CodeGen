cmake_minimum_required(VERSION 2.8)
project(CodeGen)

set(CMAKE_MODULE_PATH
	${CodeGen_SOURCE_DIR}/Modules
	${CMAKE_MODULE_PATH}
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT TARGET Framework)
	add_subdirectory(../../Framework/build_unix ${CodeGen_SOURCE_DIR}/../../Framework/build_unix/build)
endif()

include_directories(../include ../../Framework/include)

add_library(CodeGen 
	../src/ArmAssembler.cpp
	../src/Jitter_CodeGen_Arm_64.cpp
	../src/Jitter_CodeGen_Arm.cpp
	../src/Jitter_CodeGen_Arm_Div.h
	../src/Jitter_CodeGen_Arm_Fpu.cpp
	../src/Jitter_CodeGen_Arm_Md.cpp
	../src/X86Assembler.cpp
	../src/X86Assembler_Fpu.cpp
	../src/X86Assembler_Sse.cpp
	../src/Jitter_CodeGen_x86_32.cpp
	../src/Jitter_CodeGen_x86_64.cpp
	../src/Jitter_CodeGen_x86_Alu.h
	../src/Jitter_CodeGen_x86.cpp
	../src/Jitter_CodeGen_x86_Div.h
	../src/Jitter_CodeGen_x86_Fpu.cpp
	../src/Jitter_CodeGen_x86_Md.cpp
	../src/Jitter_CodeGen_x86_Mul.h
	../src/Jitter_CodeGen_x86_Shift.h
	../src/CoffObjectFile.cpp
	../src/Jitter_CodeGen.cpp
	../src/Jitter_CodeGenFactory.cpp
	../src/Jitter.cpp
	../src/Jitter_Optimize.cpp
	../src/Jitter_RegAlloc.cpp
	../src/Jitter_Statement.cpp
	../src/Jitter_SymbolTable.cpp
	../src/MachoObjectFile.cpp
	../src/MemoryFunction.cpp
	../src/ObjectFile.cpp
)

enable_testing()

add_executable(CodeGenTest
	../tests/AliasTest.cpp
	../tests/Alu64Test.cpp
	../tests/Call64Test.cpp
	../tests/Cmp64Test.cpp
	../tests/CompareTest.cpp
	../tests/Crc32Test.cpp
	../tests/DivTest.cpp
	../tests/FpIntMixTest.cpp
	../tests/FpuTest.cpp
	../tests/HugeJumpTest.cpp
	../tests/Logic64Test.cpp
	../tests/LzcTest.cpp
	../tests/Main.cpp
	../tests/MdAddTest.cpp
	../tests/MdCallTest.cpp
	../tests/MdCmpTest.cpp
	../tests/MdFpFlagTest.cpp
	../tests/MdFpTest.cpp
	../tests/MdLogicTest.cpp
	../tests/MdManipTest.cpp
	../tests/MdMemAccessTest.cpp
	../tests/MdMinMaxTest.cpp
	../tests/MdShiftTest.cpp
	../tests/MdSubTest.cpp
	../tests/MdTest.cpp
	../tests/MdUnpackTest.cpp
	../tests/MemAccessTest.cpp
	../tests/Merge64Test.cpp
	../tests/MultTest.cpp
	../tests/NestedIfTest.cpp
	../tests/RandomAluTest2.cpp
	../tests/RandomAluTest3.cpp
	../tests/RandomAluTest.cpp
	../tests/RegAllocTest.cpp
	../tests/Shift64Test.cpp
	../tests/ShiftTest.cpp
	../tests/SimpleMdTest.cpp
)
target_link_libraries(CodeGenTest CodeGen Framework)
add_test(CodeGenTest CodeGenTest)

