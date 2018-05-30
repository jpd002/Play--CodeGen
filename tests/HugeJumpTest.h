#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CHugeJumpTest : public CTest
{
public:
	virtual				~CHugeJumpTest() = default;

	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

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
	CMemoryFunction		m_function;
};
