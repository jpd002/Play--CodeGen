#ifndef _MDCALLTEST_H_
#define _MDCALLTEST_H_

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdCallTest : public CTest
{
public:
						CMdCallTest();
	virtual				~CMdCallTest();

	void				Compile(Jitter::CJitter&);
	void				Run();

private:
	struct uint128
	{
		float v[4];
	};

	struct CONTEXT
	{
		ALIGN16

		uint128			value0;
		uint128			value1;

		float			result0;
		float			result1;

		ALIGN16

		uint128			result2;
	};

	static uint32		MdInputFunction(const uint128&, uint32);
	static uint128		MdOutputFunction(uint32);

	CMemoryFunction*	m_function;
};

#endif
