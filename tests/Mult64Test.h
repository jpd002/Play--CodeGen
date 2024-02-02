#pragma once

#include "Test.h"

class CMult64Test : public CTest
{
public:
	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		uint64			relValue1;
		uint64			relValue2;

		uint64			relLoResult;
		uint64			cstLoResult;
	};

	CONTEXT				m_context;
	FunctionType		m_function;
};
