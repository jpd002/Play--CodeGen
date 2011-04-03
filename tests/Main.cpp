#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>

#include "Jitter_CodeGenFactory.h"

#include "Crc32Test.h"
#include "MultTest.h"
#include "RandomAluTest.h"
#include "RandomAluTest2.h"
#include "RandomAluTest3.h"
#include "AliasTest.h"
#include "FpuTest.h"
#include "FpIntMixTest.h"
#include "MdTest.h"
#include "MdFpTest.h"
#include "MdFpFlagTest.h"
#include "CompareTest.h"
#include "RegAllocTest.h"
#include "MemAccessTest.h"
#include "HalfMultTest.h"
#include "HugeJumpTest.h"

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#ifndef TARGET_CPU_ARM
#define HAS_ADVANCED_OPS
#endif

typedef boost::function<CTest* ()> TestFactoryFunction;

TestFactoryFunction s_factories[] =
{
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CCompareTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRegAllocTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest>(), true)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest>(), false)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest2>(), true)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest2>(), false)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest3>(), true)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CRandomAluTest3>(), false)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CCrc32Test>(), "Hello World!", 0x67FCDACC)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CMultTest>(), true)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CMultTest>(), false)),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CMemAccessTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CHugeJumpTest>())),
#ifdef HAS_ADVANCED_OPS
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CHalfMultTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CAliasTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CFpuTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CFpIntMixTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CMdTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CMdFpTest>())),
	TestFactoryFunction(boost::lambda::bind(boost::lambda::new_ptr<CMdFpFlagTest>())),
#endif
	TestFactoryFunction(),
};

int main(int argc, char** argv)
{
	Jitter::CJitter jitter(Jitter::CreateCodeGen());
	TestFactoryFunction* currentTestFactory = s_factories;
	while(!currentTestFactory->empty())
	{
		CTest* test = (*currentTestFactory)();
		test->Compile(jitter);
		test->Run();
		delete test;
		currentTestFactory++;
	}
	return 0;
}
