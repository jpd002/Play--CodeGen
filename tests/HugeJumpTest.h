#ifndef _HUGEJUMPTEST_H_
#define _HUGEJUMPTEST_H_

#include "Test.h"
#include "MemoryFunction.h"

class CHugeJumpTest : public CTest
{
public:
						CHugeJumpTest();
	virtual				~CHugeJumpTest();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	enum MAX_VARS
	{
		MAX_VARS = 32,
	};

	struct CONTEXT
	{
		uint32	condition;
		uint32	number[MAX_VARS];
	};

	CONTEXT				m_context;
	CMemoryFunction*	m_function;
};

#endif
