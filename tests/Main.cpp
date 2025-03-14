#include "Jitter_CodeGenFactory.h"

#include "Crc32Test.h"
#include "CursorTest.h"
#include "MultTest.h"
#include "DivTest.h"
#include "RandomAluTest.h"
#include "RandomAluTest2.h"
#include "RandomAluTest3.h"
#include "ShiftTest.h"
#include "LogicTest.h"
#include "SelectTest.h"
#include "LoopTest.h"
#include "AliasTest.h"
#include "AliasTest2.h"
#include "FpSingleTest.h"
#include "FpIntMixTest.h"
#include "FpRoundModeTest.h"
#include "FpClampTest.h"
#include "SimpleMdTest.h"
#include "MdLogicTest.h"
#include "MdTest.h"
#include "MdAddTest.h"
#include "MdSubTest.h"
#include "MdCmpTest.h"
#include "MdMinMaxTest.h"
#include "MdUnpackTest.h"
#include "MdFpTest.h"
#include "MdFpFlagTest.h"
#include "MdCallTest.h"
#include "MdMemAccessTest.h"
#include "MdManipTest.h"
#include "MdShiftTest.h"
#include "CompareTest.h"
#include "CompareTest2.h"
#include "RegAllocTest.h"
#include "RegAllocTempTest.h"
#include "ReorderAddTest.h"
#include "MemAccessTest.h"
#include "MemAccessIdxTest.h"
#include "MemAccess8Test.h"
#include "MemAccess16Test.h"
#include "MemAccessRefTest.h"
#include "GotoTest.h"
#include "HugeJumpTest.h"
#include "HugeJumpTestLiteral.h"
#include "Alu64Test.h"
#include "ConditionTest.h"
#include "Cmp64Test.h"
#include "Shift64Test.h"
#include "Logic64Test.h"
#include "Call64Test.h"
#include "Merge64Test.h"
#include "MemAccess64Test.h"
#include "LzcTest.h"
#include "NestedIfTest.h"
#include "ExternJumpTest.h"

typedef std::function<CTest*()> TestFactoryFunction;

// clang-format off
static const TestFactoryFunction s_factories[] =
{
	[] () { return new CCompareTest(); },
	[] () { return new CCompareTest2(false, false, 0xFEDCBA98U, 0x012389ABULL); },
	[] () { return new CCompareTest2(false, true,  0xFEDCBA98U, 0x012389ABULL); },
	[] () { return new CCompareTest2(true,  false, 0xFEDCBA98U, 0x012389ABULL); },
	[] () { return new CCompareTest2(true,  true,  0xFEDCBA98U, 0x012389ABULL); },
	[] () { return new CCompareTest2(false, false, 0xF6543210U, 0xF567CDEFU); },
	[] () { return new CCompareTest2(false, true,  0xF6543210U, 0xF567CDEFU); },
	[] () { return new CCompareTest2(true,  true,  0xF6543210U, 0xF567CDEFU); },
	[] () { return new CCompareTest2(false, true,  0xF6543210U, 0); },
	[] () { return new CCompareTest2(false, false, 0x10000, 0x10000); },
	[] () { return new CCompareTest2(false, true,  0x10000, 0x10000); },
	[] () { return new CCompareTest2(true , false, 0x10000, 0x10000); },
	[] () { return new CCompareTest2(true , true,  0x10000, 0x10000); },
	[] () { return new CCompareTest2(false, true,  0, 0x80U); },
	[] () { return new CCompareTest2(false, true,  0, 0xFFFFFF80U); },
	[] () { return new CCompareTest2(true,  true,  0, 0xFFFFFF80U); },
	[] () { return new CRegAllocTest(); },
	[] () { return new CRegAllocTempTest(); },
	[] () { return new CRandomAluTest(true); },
	[] () { return new CRandomAluTest(false); },
	[] () { return new CRandomAluTest2(true); },
	[] () { return new CRandomAluTest2(false); },
	[] () { return new CRandomAluTest3(true); },
	[] () { return new CRandomAluTest3(false); },
	[] () { return new CShiftTest(0); },
	[] () { return new CShiftTest(12); },
	[] () { return new CShiftTest(31); },
	[] () { return new CShiftTest(32); },
	[] () { return new CShiftTest(44); },
	[] () { return new CReorderAddTest(); },
	[] () { return new CCrc32Test("Hello World!", 0x67FCDACC); },
	[] () { return new CCursorTest(); },
	[] () { return new CLogicTest(0, false, ~0, false); },
	[] () { return new CLogicTest(0, false, ~0, true); },
	[] () { return new CLogicTest(0, true, ~0, false); },
	[] () { return new CLogicTest(0, true, ~0, true); },
	[] () { return new CLogicTest(0x01234567, false,  0x8000, true); },
	[] () { return new CLogicTest(0x01234567, false, ~0x8000, true); },
	[] () { return new CLogicTest(0x89ABCDEF, false, 0x01234567, true); },
	[] () { return new CSelectTest(false, false); },
	[] () { return new CSelectTest(true, false); },
	[] () { return new CSelectTest(false, true); },
	[] () { return new CSelectTest(true, true); },
	[] () { return new CMultTest(true); },
	[] () { return new CMultTest(false); },
	[] () { return new CDivTest(true); },
	[] () { return new CDivTest(false); },
	[] () { return new CMemAccessTest(); },
	[] () { return new CMemAccessIdxTest(true); },
	[] () { return new CMemAccessIdxTest(false); },
	[] () { return new CMemAccess8Test(true); },
	[] () { return new CMemAccess8Test(false); },
	[] () { return new CMemAccess16Test(true); },
	[] () { return new CMemAccess16Test(false); },
	[] () { return new CMemAccessRefTest(); },
	[] () { return new CGotoTest(); },
	[] () { return new CHugeJumpTest(); },
	[] () { return new CHugeJumpTestLiteral(); },
	[] () { return new CLoopTest(); },
	[] () { return new CNestedIfTest(); },
	[] () { return new CLzcTest(); },
	[] () { return new CAliasTest(); },
	[] () { return new CAliasTest2(); },
	[] () { return new CFpRoundModeTest(); },
	[] () { return new CFpSingleTest(); },
	[] () { return new CFpIntMixTest(); },
	[] () { return new CSimpleMdTest(); },
	[] () { return new CMdTest(); },
	[] () { return new CMdLogicTest(); },
	[] () { return new CMdAddTest(); },
	[] () { return new CMdSubTest(); },
	[] () { return new CMdUnpackTest(); },
	[] () { return new CMdCmpTest(); },
	[] () { return new CMdMinMaxTest(); },
	[] () { return new CMdFpTest(); },
	[] () { return new CMdFpFlagTest(); },
	[] () { return new CMdCallTest(); },
	[] () { return new CMdMemAccessTest(); },
	[] () { return new CMdManipTest(); },
	[] () { return new CMdShiftTest(0); },
	[] () { return new CMdShiftTest(15); },
	[] () { return new CMdShiftTest(16); },
	[] () { return new CMdShiftTest(31); },
	[] () { return new CMdShiftTest(32); },
	[] () { return new CMdShiftTest(38); },
	[] () { return new CFpClampTest(); },
	[] () { return new CAlu64Test(); },
	//negative / positive
	[] () { return new CConditionTest(false, 0xFFFFFFFE, 0xFFFFFFFE); },
	[] () { return new CConditionTest(false, 0x00000002, 0xFFFFFFFE); },
	[] () { return new CConditionTest(false, 0xFFFFFFFE, 0x00000002); },
	[] () { return new CConditionTest(false, 0x00000002, 0x00000002); },
	[] () { return new CConditionTest(true,  0xFFFFFFFE, 0xFFFFFFFE); },
	[] () { return new CConditionTest(true,  0x00000002, 0xFFFFFFFE); },
	[] () { return new CConditionTest(true,  0xFFFFFFFE, 0x00000002); },
	[] () { return new CConditionTest(true,  0x00000002, 0x00000002); },
	//negative / negative
	[]() { return new CConditionTest(false, 0xFFFFFFF0, 0xFFFFFFF0); },
	[]() { return new CConditionTest(false, 0xFFFFFF00, 0xFFFFFFF0); },
	[]() { return new CConditionTest(false, 0xFFFFFFF0, 0xFFFFFF00); },
	[]() { return new CConditionTest(false, 0xFFFFFF00, 0xFFFFFF00); },
	[]() { return new CConditionTest(true,  0xFFFFFFF0, 0xFFFFFFF0); },
	[]() { return new CConditionTest(true,  0xFFFFFF00, 0xFFFFFFF0); },
	[]() { return new CConditionTest(true,  0xFFFFFFF0, 0xFFFFFF00); },
	[]() { return new CConditionTest(true,  0xFFFFFF00, 0xFFFFFF00); },
	//positive / positive
	[]() { return new CConditionTest(false, 0x0000000F, 0x0000000F); },
	[]() { return new CConditionTest(false, 0x000000FF, 0x0000000F); },
	[]() { return new CConditionTest(false, 0x0000000F, 0x000000FF); },
	[]() { return new CConditionTest(false, 0x000000FF, 0x000000FF); },
	[]() { return new CConditionTest(true,  0x0000000F, 0x0000000F); },
	[]() { return new CConditionTest(true,  0x000000FF, 0x0000000F); },
	[]() { return new CConditionTest(true,  0x0000000F, 0x000000FF); },
	[]() { return new CConditionTest(true,  0x000000FF, 0x000000FF); },
	[] () { return new CCmp64Test(false, false, 0xFEDCBA9876543210ULL, 0x012389AB4567CDEFULL); },
	[] () { return new CCmp64Test(false, true,  0xFEDCBA9876543210ULL, 0x012389AB4567CDEFULL); },
	[] () { return new CCmp64Test(true,  true,  0xFEDCBA9876543210ULL, 0x012389AB4567CDEFULL); },
	[] () { return new CCmp64Test(false, false, 0xFFFFFFFFF6543210ULL, 0xFFFFFFFFF567CDEFULL); },
	[] () { return new CCmp64Test(false, true,  0xFFFFFFFFF6543210ULL, 0xFFFFFFFFF567CDEFULL); },
	[] () { return new CCmp64Test(true,  true,  0xFFFFFFFFF6543210ULL, 0xFFFFFFFFF567CDEFULL); },
	[] () { return new CCmp64Test(false, true,  0xFFFFFFFFF6543210ULL, 0); },
	[] () { return new CCmp64Test(false, false, 0x100000000, 0x100000000); },
	[] () { return new CCmp64Test(false, true,  0x100000000, 0x100000000); },
	[] () { return new CCmp64Test(true , true,  0x100000000, 0x100000000); },
	[] () { return new CCmp64Test(false, true,  0, 0x80ULL); },
	[] () { return new CCmp64Test(false, true,  0, 0xFFFFFFFFFFFFFF80ULL); },
	[] () { return new CCmp64Test(true,  true,  0, 0xFFFFFFFFFFFFFF80ULL); },
	[] () { return new CLogic64Test(); },
	[] () { return new CShift64Test(0); },
	[] () { return new CShift64Test(12); },
	[] () { return new CShift64Test(32); },
	[] () { return new CShift64Test(52); },
	[] () { return new CShift64Test(63); },
	[] () { return new CShift64Test(64); },
	[] () { return new CShift64Test(76); },
	[] () { return new CMerge64Test(); },
	[] () { return new CMemAccess64Test(false); },
	[] () { return new CMemAccess64Test(true); },
	[] () { return new CCall64Test(); },
	[] () { return new CExternJumpTest(); }
};
// clang-format on

void PrepareExternalFunctions()
{
	CCrc32Test::PrepareExternalFunctions();
	CCall64Test::PrepareExternalFunctions();
	CRegAllocTempTest::PrepareExternalFunctions();
}

int main(int argc, const char** argv)
{
	PrepareExternalFunctions();
	Jitter::CJitter jitter(Jitter::CreateCodeGen());
	for(const auto& factory : s_factories)
	{
		auto test = factory();
		test->Compile(jitter);
		test->Run();
		delete test;
	}
	return 0;
}

#ifdef __ANDROID__

#include <jni.h>

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_codegentestsuite_NativeInterop_start(JNIEnv* env, jobject obj)
{
	main(0, nullptr);
}

#endif
