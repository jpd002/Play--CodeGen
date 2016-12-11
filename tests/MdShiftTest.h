#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdShiftTest : public CTest
{
public:
						CMdShiftTest(uint32);
	virtual				~CMdShiftTest();
			
	void				Run() override;
	void				Compile(Jitter::CJitter&) override;

private:
	struct CONTEXT
	{
		ALIGN16

		uint8			value[16];

		uint16			resultSllH[8];
		uint16			resultSrlH[8];
		uint16			resultSraH[8];
		uint32			resultSrlW[4];
		uint32			resultSraW[4];
		uint32			resultSllW[4];
	};

	CONTEXT				m_context;
	CMemoryFunction		m_function;
	uint32				m_shiftAmount = 0;
};
