#pragma once

#include "Test.h"
#include "Align16.h"
#include "MemoryFunction.h"

class CMdCallTest : public CTest
{
public:
						CMdCallTest();
	virtual				~CMdCallTest();

	void				Compile(Jitter::CJitter&) override;
	void				Run() override;

private:
	struct uint128
	{
		union
		{
			struct
			{
				uint32 nV[4];
			};
			struct
			{
				uint32 nV0;
				uint32 nV1;
				uint32 nV2;
				uint32 nV3;
			};
			struct
			{
				uint64 nD0;
				uint64 nD1;
			};
		};
	};

	struct CONTEXT
	{
		ALIGN16

		uint128			value0;
		uint128			value1;

		uint32			result0;
		uint32			result1;

		ALIGN16

		uint128			result2;
	};

	static uint32		MdInputFunction(const uint128&, uint32);
	static uint128		MdOutputFunction(uint32);

	CMemoryFunction		m_function;
};
