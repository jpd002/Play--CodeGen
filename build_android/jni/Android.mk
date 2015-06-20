LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/ExternalDependencies.mk

include $(CLEAR_VARS)

LOCAL_MODULE			:= libFramework
LOCAL_SRC_FILES 		:= $(FRAMEWORK_PATH)/build_android/obj/local/$(TARGET_ARCH_ABI)/libFramework.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE		:= libCodeGen
LOCAL_SRC_FILES		:=	../../src/ArmAssembler.cpp \
						../../src/Jitter.cpp \
						../../src/Jitter_CodeGen.cpp \
						../../src/Jitter_CodeGen_Arm.cpp \
						../../src/Jitter_CodeGen_Arm_64.cpp \
						../../src/Jitter_CodeGen_Arm_Fpu.cpp \
						../../src/Jitter_CodeGen_Arm_Md.cpp \
						../../src/Jitter_CodeGenFactory.cpp \
						../../src/Jitter_Optimize.cpp \
						../../src/Jitter_RegAlloc.cpp \
						../../src/Jitter_Statement.cpp \
						../../src/Jitter_SymbolTable.cpp \
						../../src/MemoryFunction.cpp \
						../../src/ObjectFile.cpp
LOCAL_CFLAGS		:= -Wno-extern-c-compat
LOCAL_C_INCLUDES	:= $(FRAMEWORK_PATH)/include $(LOCAL_PATH)/../../include
LOCAL_CPP_FEATURES	:= exceptions rtti

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE			:= libCodeGenTestSuite
LOCAL_SRC_FILES			:=	../../tests/AliasTest.cpp \
							../../tests/Alu64Test.cpp \
							../../tests/Call64Test.cpp \
							../../tests/Cmp64Test.cpp \
							../../tests/CompareTest.cpp \
							../../tests/Crc32Test.cpp \
							../../tests/DivTest.cpp \
							../../tests/FpuTest.cpp \
							../../tests/FpIntMixTest.cpp \
							../../tests/HugeJumpTest.cpp \
							../../tests/Logic64Test.cpp \
							../../tests/LzcTest.cpp \
							../../tests/Main.cpp \
							../../tests/MdAddTest.cpp \
							../../tests/MdCallTest.cpp \
							../../tests/MdCmpTest.cpp \
							../../tests/MdFpFlagTest.cpp \
							../../tests/MdFpTest.cpp \
							../../tests/MdLogicTest.cpp \
							../../tests/MdManipTest.cpp \
							../../tests/MdMemAccessTest.cpp \
							../../tests/MdMinMaxTest.cpp \
							../../tests/MdShiftTest.cpp \
							../../tests/MdSubTest.cpp \
							../../tests/MdTest.cpp \
							../../tests/MdUnpackTest.cpp \
							../../tests/MemAccessTest.cpp \
							../../tests/Merge64Test.cpp \
							../../tests/MultTest.cpp \
							../../tests/NestedIfTest.cpp \
							../../tests/RandomAluTest.cpp \
							../../tests/RandomAluTest2.cpp \
							../../tests/RandomAluTest3.cpp \
							../../tests/RegAllocTest.cpp \
							../../tests/ShiftTest.cpp \
							../../tests/Shift64Test.cpp \
							../../tests/SimpleMdTest.cpp
LOCAL_CFLAGS			:= -Wno-extern-c-compat
LOCAL_C_INCLUDES		:= $(FRAMEWORK_PATH)/include $(LOCAL_PATH)/../../include
LOCAL_CPP_FEATURES		:= exceptions rtti
LOCAL_LDLIBS 			:= -landroid
LOCAL_STATIC_LIBRARIES	:= libCodeGen libFramework

include $(BUILD_SHARED_LIBRARY)
