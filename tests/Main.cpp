#include "Jitter_CodeGenFactory.h"

#include "Crc32Test.h"
#include "MultTest.h"
#include "RandomAluTest.h"
#include "RandomAluTest2.h"
#include "RandomAluTest3.h"
#include "AliasTest.h"
#include "FpuTest.h"
#include "FpIntMixTest.h"
#include "SimpleMdTest.h"
#include "MdLogicTest.h"
#include "MdTest.h"
#include "MdAddTest.h"
#include "MdSubTest.h"
#include "MdCmpTest.h"
#include "MdUnpackTest.h"
#include "MdFpTest.h"
#include "MdFpFlagTest.h"
#include "MdCallTest.h"
#include "MdMemAccessTest.h"
#include "MdManipTest.h"
#include "CompareTest.h"
#include "RegAllocTest.h"
#include "MemAccessTest.h"
#include "HalfMultTest.h"
#include "HugeJumpTest.h"
#include "Alu64Test.h"
#include "Cmp64Test.h"
#include "Shift64Test.h"
#include "Logic64Test.h"
#include "Call64Test.h"
#include "Merge64Test.h"
#include "LzcTest.h"
#include "NestedIfTest.h"

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#if !TARGET_CPU_ARM
#define HAS_ADVANCED_OPS
#endif

typedef std::function<CTest* ()> TestFactoryFunction;

static const TestFactoryFunction s_factories[] =
{
	[] () { return new CCompareTest(); },
	[] () { return new CRegAllocTest(); },
	[] () { return new CRandomAluTest(true); },
	[] () { return new CRandomAluTest(false); },
	[] () { return new CRandomAluTest2(true); },
	[] () { return new CRandomAluTest2(false); },
	[] () { return new CRandomAluTest3(true); },
	[] () { return new CRandomAluTest3(false); },
	[] () { return new CCrc32Test("Hello World!", 0x67FCDACC); },
	[] () { return new CMultTest(true); },
	[] () { return new CMultTest(false); },
	[] () { return new CMemAccessTest(); },
	[] () { return new CHugeJumpTest(); },
	[] () { return new CNestedIfTest(); },
#ifdef HAS_ADVANCED_OPS
	[] () { return new CLzcTest(); },
	[] () { return new CHalfMultTest(); },
	[] () { return new CAliasTest(); },
	[] () { return new CFpuTest(); },
	[] () { return new CFpIntMixTest(); },
	[] () { return new CSimpleMdTest(); },
	[] () { return new CMdTest(); },
	[] () { return new CMdLogicTest(); },
	[] () { return new CMdAddTest(); },
	[] () { return new CMdSubTest(); },
	[] () { return new CMdUnpackTest(); },
	[] () { return new CMdCmpTest(); },
	[] () { return new CMdFpTest(); },
	[] () { return new CMdFpFlagTest(); },
	[] () { return new CMdCallTest(); },
	[] () { return new CMdMemAccessTest(); },
	[] () { return new CMdManipTest(); },
	[] () { return new CAlu64Test(); },
	[] () { return new CCmp64Test(false,	0xFEDCBA9876543210ULL, 0x012389AB4567CDEFULL); },
	[] () { return new CCmp64Test(true,		0xFEDCBA9876543210ULL, 0x012389AB4567CDEFULL); },
	[] () { return new CCmp64Test(false,	0xFFFFFFFFF6543210ULL, 0xFFFFFFFFF567CDEFULL); },
	[] () { return new CCmp64Test(true,		0xFFFFFFFFF6543210ULL, 0xFFFFFFFFF567CDEFULL); },
	[] () { return new CCmp64Test(false,	0x100000000, 0x100000000); },
	[] () { return new CCmp64Test(true,		0x100000000, 0x100000000); },
	[] () { return new CLogic64Test(); },
	[] () { return new CShift64Test(0); },
	[] () { return new CShift64Test(12); },
	[] () { return new CShift64Test(32); },
	[] () { return new CShift64Test(52); },
	[] () { return new CMerge64Test(); },
	[] () { return new CCall64Test(); },
#endif
};

int main(int argc, const char** argv)
{
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
