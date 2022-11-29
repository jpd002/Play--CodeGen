#pragma once

#include "Test.h"
#include "MemoryFunction.h"

class CRandomAluTest3 : public CTest
{
public:
						CRandomAluTest3(bool);

	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct CONTEXT
	{
		uint32 number1;
		uint32 number2;
	};

	bool				m_useConstant;
	CONTEXT				m_context;
	CMemoryFunction		m_function;
};
