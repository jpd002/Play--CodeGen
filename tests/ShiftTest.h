#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CShiftTest : public CTest
{
public:
						CShiftTest(uint32);
	virtual				~CShiftTest();
			
	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	struct CONTEXT
	{
		uint32			value0;
		uint32			value1;

		uint32			shiftAmount;

		uint32			resultSra0;
		uint32			resultSra1;

		uint32			resultSrl0;
		uint32			resultSrl1;

		uint32			resultShl0;
		uint32			resultShl1;

		uint32			resultSraVar0;
		uint32			resultSraVar1;

		uint32			resultSrlVar0;
		uint32			resultSrlVar1;

		uint32			resultShlVar0;
		uint32			resultShlVar1;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
	uint32				m_shiftAmount = 0;
};
