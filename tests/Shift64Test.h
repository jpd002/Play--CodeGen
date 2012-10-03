#ifndef _SHIFT64TEST_H_
#define _SHIFT64TEST_H_

#include "Test.h"
#include "MemoryFunction.h"

class CShift64Test : public CTest
{
public:
						CShift64Test();
	virtual				~CShift64Test();
			
	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	struct CONTEXT
	{
		uint64			value0;
		uint64			value1;

		uint32			shiftAmount0;
		uint32			shiftAmount1;

		uint64			resultSra0;
		uint64			resultSra1;

		uint64			resultSraVar0;
		uint64			resultSraVar1;

		uint64			resultShlVar0;
		uint64			resultShlVar1;

		uint64			resultSrlVar0;
		uint64			resultSrlVar1;
	};

	CONTEXT				m_context;
	CMemoryFunction*	m_function;
};

#endif
