#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CNestedIfTest : public CTest
{
public:
	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	struct CONTEXT
	{
		uint32		input;
		uint32		result;
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
