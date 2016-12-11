LOCAL_PATH := $(call my-dir)

PROJECT_PATH := $(realpath $(LOCAL_PATH))/../../../../
FRAMEWORK_PATH := $(realpath $(LOCAL_PATH))/../../../../../Framework

include $(CLEAR_VARS)

LOCAL_MODULE			:= libFramework
LOCAL_SRC_FILES 		:= $(FRAMEWORK_PATH)/build_android/obj/local/$(TARGET_ARCH_ABI)/libFramework.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE		:= libCodeGen
LOCAL_SRC_FILES		:=	$(PROJECT_PATH)/src/AArch32Assembler.cpp \
						$(PROJECT_PATH)/src/AArch64Assembler.cpp \
						$(PROJECT_PATH)/src/Jitter.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_AArch32.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_AArch32_64.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_AArch32_Fpu.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_AArch32_Md.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_AArch64.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_AArch64_64.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_AArch64_Fpu.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_AArch64_Md.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_x86.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_x86_32.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_x86_64.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_x86_Fpu.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGen_x86_Md.cpp \
						$(PROJECT_PATH)/src/Jitter_CodeGenFactory.cpp \
						$(PROJECT_PATH)/src/Jitter_Optimize.cpp \
						$(PROJECT_PATH)/src/Jitter_RegAlloc.cpp \
						$(PROJECT_PATH)/src/Jitter_Statement.cpp \
						$(PROJECT_PATH)/src/Jitter_SymbolTable.cpp \
						$(PROJECT_PATH)/src/MemoryFunction.cpp \
						$(PROJECT_PATH)/src/ObjectFile.cpp \
						$(PROJECT_PATH)/src/X86Assembler.cpp \
						$(PROJECT_PATH)/src/X86Assembler_Fpu.cpp \
						$(PROJECT_PATH)/src/X86Assembler_Sse.cpp
LOCAL_CFLAGS		:= -Wno-extern-c-compat
LOCAL_C_INCLUDES	:= $(FRAMEWORK_PATH)/include $(PROJECT_PATH)/include $(NDK_ROOT)/sources/android/cpufeatures
LOCAL_CPP_FEATURES	:= exceptions rtti

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE			:= libCodeGenTestSuite
LOCAL_SRC_FILES			:=	$(PROJECT_PATH)/tests/AliasTest.cpp \
							$(PROJECT_PATH)/tests/AliasTest2.cpp \
							$(PROJECT_PATH)/tests/Alu64Test.cpp \
							$(PROJECT_PATH)/tests/Call64Test.cpp \
							$(PROJECT_PATH)/tests/ConditionTest.cpp \
							$(PROJECT_PATH)/tests/Cmp64Test.cpp \
							$(PROJECT_PATH)/tests/CompareTest.cpp \
							$(PROJECT_PATH)/tests/Crc32Test.cpp \
							$(PROJECT_PATH)/tests/DivTest.cpp \
							$(PROJECT_PATH)/tests/FpuTest.cpp \
							$(PROJECT_PATH)/tests/FpIntMixTest.cpp \
							$(PROJECT_PATH)/tests/HugeJumpTest.cpp \
							$(PROJECT_PATH)/tests/LogicTest.cpp \
							$(PROJECT_PATH)/tests/Logic64Test.cpp \
							$(PROJECT_PATH)/tests/LzcTest.cpp \
							$(PROJECT_PATH)/tests/Main.cpp \
							$(PROJECT_PATH)/tests/MdAddTest.cpp \
							$(PROJECT_PATH)/tests/MdCallTest.cpp \
							$(PROJECT_PATH)/tests/MdCmpTest.cpp \
							$(PROJECT_PATH)/tests/MdFpFlagTest.cpp \
							$(PROJECT_PATH)/tests/MdFpTest.cpp \
							$(PROJECT_PATH)/tests/MdLogicTest.cpp \
							$(PROJECT_PATH)/tests/MdManipTest.cpp \
							$(PROJECT_PATH)/tests/MdMemAccessTest.cpp \
							$(PROJECT_PATH)/tests/MdMinMaxTest.cpp \
							$(PROJECT_PATH)/tests/MdShiftTest.cpp \
							$(PROJECT_PATH)/tests/MdSubTest.cpp \
							$(PROJECT_PATH)/tests/MdTest.cpp \
							$(PROJECT_PATH)/tests/MdUnpackTest.cpp \
							$(PROJECT_PATH)/tests/MemAccessTest.cpp \
							$(PROJECT_PATH)/tests/Merge64Test.cpp \
							$(PROJECT_PATH)/tests/MultTest.cpp \
							$(PROJECT_PATH)/tests/NestedIfTest.cpp \
							$(PROJECT_PATH)/tests/RandomAluTest.cpp \
							$(PROJECT_PATH)/tests/RandomAluTest2.cpp \
							$(PROJECT_PATH)/tests/RandomAluTest3.cpp \
							$(PROJECT_PATH)/tests/RegAllocTest.cpp \
							$(PROJECT_PATH)/tests/ShiftTest.cpp \
							$(PROJECT_PATH)/tests/Shift64Test.cpp \
							$(PROJECT_PATH)/tests/SimpleMdTest.cpp
LOCAL_CFLAGS			:= -Wno-extern-c-compat
LOCAL_C_INCLUDES		:= $(FRAMEWORK_PATH)/include $(PROJECT_PATH)/include
LOCAL_CPP_FEATURES		:= exceptions rtti
LOCAL_LDLIBS 			:= -landroid
LOCAL_STATIC_LIBRARIES	:= libCodeGen libFramework cpufeatures

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)
