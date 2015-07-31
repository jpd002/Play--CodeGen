cmake_minimum_required(VERSION 2.8)
project(CodeGen)

set(CMAKE_MODULE_PATH
	${Framework_SOURCE_DIR}/Modules
	${CMAKE_MODULE_PATH}
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
