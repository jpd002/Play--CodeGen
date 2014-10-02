#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CSimpleMdTest : public CTest
{
public:
	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		ALIGN16

		uint32			op1[4];
		uint32			op2[4];

		uint32			result[4];
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
