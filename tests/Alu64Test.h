#ifndef _ALU64TEST_H_
#define _ALU64TEST_H_

#include "Test.h"
#include "MemoryFunction.h"

class CAlu64Test : public CTest
{
public:
						CAlu64Test();
	virtual				~CAlu64Test();
			
	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	struct CONTEXT
	{
		uint64			value0;
		uint64			value1;

		uint64			resultAdd;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};

#endif
