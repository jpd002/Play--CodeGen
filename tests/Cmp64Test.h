#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CCmp64Test : public CTest
{
public:
	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	struct CONTEXT
	{
		uint64			value0;
		uint64			value1;

		uint32			resultBl;
		uint32			resultLt;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
