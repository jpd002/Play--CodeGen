#ifndef _HALFMULTTEST_H_
#define _HALFMULTTEST_H_

#include "Test.h"
#include "MemoryFunction.h"

class CHalfMultTest : public CTest
{
public:
						CHalfMultTest();
	virtual				~CHalfMultTest();
			
	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	struct CONTEXT
	{
		uint32			relArg0;
		uint32			relArg1;

		uint32			multLoResult;
		uint32			multHiResult;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};

#endif
