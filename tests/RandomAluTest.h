#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CRandomAluTest : public CTest
{
public:
						CRandomAluTest(bool);

	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		uint32 number;
	};

	bool				m_useConstant;
	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
