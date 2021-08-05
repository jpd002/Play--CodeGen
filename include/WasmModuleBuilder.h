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

	void AddFunction(FUNCTION);

	void WriteModule(Framework::CStream&);

private:
	std::vector<FUNCTION> m_functions;
};
