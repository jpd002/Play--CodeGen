#pragma once

#include "Test.h"
#include "MemoryFunction.h"
#include "Align16.h"

class CAliasTest2 : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		ALIGN16

		float value1[4];
		float value2[4];
		float value3[4];
	};

	CONTEXT         m_context;
	CMemoryFunction m_function;
};
