cmake_minimum_required(VERSION 3.18)
project(CodeGen)

set(CODEGEN_LIBS)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)

include(Header)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release"
		FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../Framework/build_cmake/Framework
		${CMAKE_CURRENT_BINARY_DIR}/Framework
	)
endif()

if(ANDROID)
	if(NOT TARGET cpufeatures)
		add_library(cpufeatures STATIC ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
	endif()
	include_directories("${ANDROID_NDK}/sources/android/cpufeatures")
	list(APPEND CODEGEN_LIBS cpufeatures)
endif()

# Détection iOS
if(APPLE)
	if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
		set(TARGET_PLATFORM_IOS TRUE)
	endif()
endif()

# Configuration iOS JIT
if(TARGET_PLATFORM_IOS)
	message(STATUS "Configuring Play--CodeGen for iOS with JIT support")

	# Ajouter lwmem
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/lwmem/CMakeLists.txt")
		add_subdirectory(deps/lwmem/lwmem)
		list(APPEND CODEGEN_LIBS lwmem)
	endif()
endif()

add_library(CodeGen 
	src/AArch32Assembler.cpp
	src/AArch64Assembler.cpp
	src/CoffObjectFile.cpp
	src/Jitter_CodeGen_AArch32.cpp
	src/Jitter_CodeGen_AArch32_64.cpp
	src/Jitter_CodeGen_AArch32_Div.h
	src/Jitter_CodeGen_AArch32_Fpu.cpp
	src/Jitter_CodeGen_AArch32_Md.cpp
	src/Jitter_CodeGen_AArch64.cpp
	src/Jitter_CodeGen_AArch64_64.cpp
	src/Jitter_CodeGen_AArch64_Fpu.cpp
	src/Jitter_CodeGen_AArch64_Md.cpp
	src/Jitter_CodeGen_x86_32.cpp
	src/Jitter_CodeGen_x86_64.cpp
	src/Jitter_CodeGen_x86.cpp
	src/Jitter_CodeGen_x86_Alu.h
	src/Jitter_CodeGen_x86_Div.h
	src/Jitter_CodeGen_x86_Fpu.cpp
	src/Jitter_CodeGen_x86_Fpu_Avx.cpp
	src/Jitter_CodeGen_x86_Fpu_Sse.cpp
	src/Jitter_CodeGen_x86_Md.cpp
	src/Jitter_CodeGen_x86_Md_Avx.cpp
	src/Jitter_CodeGen_x86_Md_Sse.cpp
	src/Jitter_CodeGen_x86_Mul.h
	src/Jitter_CodeGen_x86_Shift.h
	src/Jitter_CodeGen_Wasm.cpp
	src/Jitter_CodeGen_Wasm_64.cpp
	src/Jitter_CodeGen_Wasm_Fpu.cpp
	src/Jitter_CodeGen_Wasm_LoadStore.h
	src/Jitter_CodeGen_Wasm_Md.cpp
	src/Jitter_CodeGen.cpp
	src/Jitter_CodeGenFactory.cpp
	src/Jitter.cpp
	src/Jitter_Optimize.cpp
	src/Jitter_RegAlloc.cpp
	src/Jitter_Statement.cpp
	src/Jitter_SymbolTable.cpp
	src/LiteralPool.cpp
	src/MachoObjectFile.cpp
	src/MemoryFunction.cpp
	src/ObjectFile.cpp
	src/WasmModuleBuilder.cpp
	$<$<BOOL:${TARGET_PLATFORM_IOS}>:src/MemoryUtil_iOS.cpp>
	$<$<BOOL:${TARGET_PLATFORM_IOS}>:src/MemoryUtil_iOS_Legacy.cpp>
	$<$<BOOL:${TARGET_PLATFORM_IOS}>:src/MemoryUtil_iOS_LuckNoTXM.cpp>
	$<$<BOOL:${TARGET_PLATFORM_IOS}>:src/MemoryUtil_iOS_LuckTXM.cpp>
	$<$<BOOL:${TARGET_PLATFORM_IOS}>:src/JITMemoryTracker.cpp>
	src/X86Assembler.cpp
	src/X86Assembler_Avx.cpp
	src/X86Assembler_Fpu.cpp
	src/X86Assembler_Sse.cpp
	src/X86CpuFeatures.cpp

	include/AArch32Assembler.h
	include/AArch64Assembler.h
	include/ArrayStack.h
	include/CoffDefs.h
	include/CoffObjectFile.h
	include/Jitter_CodeGen_AArch32.h
	include/Jitter_CodeGen_AArch64.h
	include/Jitter_CodeGen_Wasm.h
	include/Jitter_CodeGen_x86_32.h
	include/Jitter_CodeGen_x86_64.h
	include/Jitter_CodeGen_x86.h
	include/Jitter_CodeGen.h
	include/Jitter_CodeGenFactory.h
	include/Jitter_Statement.h
	include/Jitter_Symbol.h
	include/Jitter_SymbolRef.h
	include/Jitter_SymbolTable.h
	include/Jitter.h
	include/Literal128.h
	include/LiteralPool.h
	include/MachoDefs.h
	include/MachoObjectFile.h
	include/MemoryFunction.h
	$<$<BOOL:${TARGET_PLATFORM_IOS}>:include/MemoryUtil.h>
	$<$<BOOL:${TARGET_PLATFORM_IOS}>:include/JITMemoryTracker.h>
	include/ObjectFile.h
	include/WasmDefs.h
	include/WasmModuleBuilder.h
	include/X86Assembler.h
	include/X86CpuFeatures.h
)

target_link_libraries(CodeGen PUBLIC Framework ${CODEGEN_LIBS})
target_include_directories(CodeGen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Configuration supplémentaire iOS
if(TARGET_PLATFORM_IOS)
	# Include directories pour lwmem
	if(TARGET lwmem)
		target_include_directories(CodeGen PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/deps/lwmem/lwmem/src/include
		)
	endif()

	# Définitions du compilateur
	target_compile_definitions(CodeGen PRIVATE
		IPHONEOS=1
		TARGET_OS_IOS=1
	)
endif()

enable_testing()

set(CodeGenTest_SRC
	tests/AliasTest.cpp
	tests/AliasTest.h
	tests/AliasTest2.cpp
	tests/AliasTest2.h
	tests/Alu64Test.cpp
	tests/Alu64Test.h
	tests/Call64Test.cpp
	tests/Call64Test.h
	tests/Cmp64Test.cpp
	tests/Cmp64Test.h
	tests/ConditionTest.cpp
	tests/ConditionTest.h
	tests/CompareTest.cpp
	tests/CompareTest.h
	tests/CompareTest2.cpp
	tests/CompareTest2.h
	tests/Crc32Test.cpp
	tests/Crc32Test.h
	tests/CursorTest.cpp
	tests/CursorTest.h
	tests/DivTest.cpp
	tests/DivTest.h
	tests/ExternJumpTest.cpp
	tests/ExternJumpTest.h
	tests/FpClampTest.cpp
	tests/FpClampTest.h
	tests/FpIntMixTest.cpp
	tests/FpIntMixTest.h
	tests/FpRoundModeTest.cpp
	tests/FpRoundModeTest.h
	tests/FpSingleTest.cpp
	tests/FpSingleTest.h
	tests/GotoTest.cpp
	tests/GotoTest.h
	tests/HugeJumpTest.cpp
	tests/HugeJumpTest.h
	tests/HugeJumpTestLiteral.cpp
	tests/HugeJumpTestLiteral.h
	tests/LogicTest.cpp
	tests/LogicTest.h
	tests/Logic64Test.cpp
	tests/Logic64Test.h
	tests/LoopTest.cpp
	tests/LoopTest.h
	tests/LzcTest.cpp
	tests/LzcTest.h
	tests/Main.cpp
	tests/MdAddTest.cpp
	tests/MdAddTest.h
	tests/MdCallTest.cpp
	tests/MdCallTest.h
	tests/MdCmpTest.cpp
	tests/MdCmpTest.h
	tests/MdFpFlagTest.cpp
	tests/MdFpFlagTest.h
	tests/MdFpTest.cpp
	tests/MdFpTest.h
	tests/MdLogicTest.cpp
	tests/MdLogicTest.h
	tests/MdManipTest.cpp
	tests/MdManipTest.h
	tests/MdMemAccessTest.cpp
	tests/MdMemAccessTest.h
	tests/MdMinMaxTest.cpp
	tests/MdMinMaxTest.h
	tests/MdShiftTest.cpp
	tests/MdShiftTest.h
	tests/MdSubTest.cpp
	tests/MdSubTest.h
	tests/MdTest.cpp
	tests/MdTest.h
	tests/MdUnpackTest.cpp
	tests/MdUnpackTest.h
	tests/MemAccessTest.cpp
	tests/MemAccessTest.h
	tests/MemAccess8Test.cpp
	tests/MemAccess8Test.h
	tests/MemAccess16Test.cpp
	tests/MemAccess16Test.h
	tests/MemAccess64Test.cpp
	tests/MemAccess64Test.h
	tests/MemAccessIdxTest.cpp
	tests/MemAccessIdxTest.h
	tests/MemAccessRefTest.cpp
	tests/MemAccessRefTest.h
	tests/Merge64Test.cpp
	tests/Merge64Test.h
	tests/MultTest.cpp
	tests/MultTest.h
	tests/NestedIfTest.cpp
	tests/NestedIfTest.h
	tests/RandomAluTest2.cpp
	tests/RandomAluTest2.h
	tests/RandomAluTest3.cpp
	tests/RandomAluTest3.h
	tests/RandomAluTest.cpp
	tests/RandomAluTest.h
	tests/RegAllocTest.cpp
	tests/RegAllocTest.h
	tests/RegAllocTempTest.cpp
	tests/RegAllocTempTest.h
	tests/ReorderAddTest.cpp
	tests/ReorderAddTest.h
	tests/SelectTest.cpp
	tests/SelectTest.h
	tests/Shift64Test.cpp
	tests/Shift64Test.h
	tests/ShiftTest.cpp
	tests/ShiftTest.h
	tests/SimpleMdTest.cpp
	tests/SimpleMdTest.h
	tests/Test.h
	tests/uint128.h
)

set(TESTSUITE_LIBS)
if(ANDROID)
	add_library(CodeGenTestSuite SHARED ${CodeGenTest_SRC})
	list(APPEND TESTSUITE_LIBS log)
else()
	add_executable(CodeGenTestSuite ${CodeGenTest_SRC})
	add_test(NAME CodeGenTestSuite COMMAND CodeGenTestSuite)
endif()

target_link_libraries(CodeGenTestSuite PRIVATE CodeGen Framework ${TESTSUITE_LIBS})

if(EMSCRIPTEN)
	target_link_options(CodeGenTestSuite PRIVATE "--bind")
	target_link_options(CodeGenTestSuite PRIVATE "-sMODULARIZE=1")
	target_link_options(CodeGenTestSuite PRIVATE "-sEXPORT_NAME=CodeGenTestSuite")
	target_link_options(CodeGenTestSuite PRIVATE "-sASSERTIONS=2")
	target_link_options(CodeGenTestSuite PRIVATE "-sWASM_BIGINT")
	target_link_options(CodeGenTestSuite PRIVATE "-sEXPORTED_FUNCTIONS=['_main', '_CCrc32Test_GetNextByte', '_CCrc32Test_GetTableValue', '_CCall64Test_Add64', '_CCall64Test_Sub64', '_CCall64Test_AddMul64', '_CCall64Test_AddMul64_2', '_RegAllocTempTest_DummyFunction']")
	target_link_options(CodeGenTestSuite PRIVATE "-sALLOW_TABLE_GROWTH")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()
