#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CCmp64Test : public CTest
{
public:
						CCmp64Test(bool, uint64, uint64);

	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	struct CONTEXT
	{
		uint64			value0;
		uint64			value1;

		uint32			resultEq;
		uint32			resultNe;
		uint32			resultBl;
		uint32			resultLt;
		uint32			resultGt;
	};

	bool				m_useConstant = false;
	uint64				m_value0 = 0;
	uint64				m_value1 = 0;
	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
