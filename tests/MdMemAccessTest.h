#pragma once

#include "Test.h"
#include "Align16.h"
#include "uint128.h"

class CMdMemAccessTest : public CTest
{
public:
	void Run() override;
	void Compile(Jitter::CJitter&) override;

private:
	static constexpr size_t ITEM_COUNT = 0x10;

	struct CONTEXT
	{
		ALIGN16

		uint128 op;
		uint128 loadResult0;
		uint128 loadResult1;

		uint32 loadFromIdx1;
		uint32 storeAtIdx1;

		uint128* array;
	};

	CONTEXT m_context;
	uint128 m_memory[ITEM_COUNT];
	FunctionType m_function;
};
