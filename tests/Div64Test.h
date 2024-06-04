#pragma once

#include "Test.h"

class CDiv64Test : public CTest
{
public:
						CDiv64Test(bool);

	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint64			relArg0;
		uint64			relArg1;

		uint64			relRelResult;
	};

	bool				m_isSigned;
	CONTEXT				m_context;
	FunctionType		m_function;
};
