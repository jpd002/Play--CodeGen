#ifndef _REGALLOCTEST_H_
#define _REGALLOCTEST_H_

#include "Test.h"
#include "MemoryFunction.h"

class CRegAllocTest : public CTest
{
public:
						CRegAllocTest();
	virtual				~CRegAllocTest();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	enum MAX_VARS
	{
		MAX_VARS = 16,
	};

	struct CONTEXT
	{
		uint32 number[MAX_VARS];
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};

#endif
