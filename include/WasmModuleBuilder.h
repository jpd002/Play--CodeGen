#pragma once

#include "Stream.h"
#include "Types.h"
#include <vector>

class CWasmModuleBuilder
{
public:
	typedef std::vector<uint8> FunctionCode;
	struct FUNCTION
	{
		FunctionCode code;
		uint32 localI32Count = 0;
	};

	static void WriteSLeb128(Framework::CStream&, int32);
	static void WriteULeb128(Framework::CStream&, uint32);

	static uint32 GetULeb128Size(uint32);

	void AddFunction(FUNCTION);

	void WriteModule(Framework::CStream&);

private:
	void WriteSectionHeader(Framework::CStream&, uint8, uint32);

	std::vector<FUNCTION> m_functions;
};
