#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CAlu64Test : public CTest
{
public:
	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	struct CONTEXT
	{
		uint64			value0;
		uint64			value1;
		uint64			value2;
		uint64			value3;

		uint64			resultAdd0;
		uint64			resultAdd1;
		uint64			resultAddCst;
		uint64			resultSub0;
		uint64			resultSub1;
		uint64			resultSubCst0;
		uint64			resultSubCst1;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
