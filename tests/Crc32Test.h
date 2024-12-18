#pragma once

#include "Test.h"
#include <string>

extern "C" uint32 CCrc32Test_GetNextByte(void*);
extern "C" uint32 CCrc32Test_GetTableValue(uint32);

class CCrc32Test : public CTest
{
public:
	CCrc32Test(const char*, uint32);

	static void PrepareExternalFunctions();

	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	friend uint32(::CCrc32Test_GetNextByte)(void*);
	friend uint32(::CCrc32Test_GetTableValue)(uint32);

	enum STATE
	{
		STATE_TEST,
		STATE_COMPUTE,
		STATE_DONE,
	};

	struct CONTEXT
	{
		uint32 nextByte;
		uint32 currentCrc;
		uint32 state;
		CCrc32Test* testCase;
	};

	void CompileTestFunction(Jitter::CJitter&);
	void CompileComputeFunction(Jitter::CJitter&);

	uint32 GetNextByteImpl();

	CONTEXT m_context;
	FunctionType m_testFunction;
	FunctionType m_computeFunction;

	static void BuildTable();

	static bool m_tableBuilt;
	static uint32 m_table[0x100];

	std::string m_input;
	unsigned int m_inputPtr;
	uint32 m_result;
};
