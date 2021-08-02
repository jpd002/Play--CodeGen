#pragma once

#include "Stream.h"
#include "Types.h"
#include <vector>

class CWasmModuleBuilder
{
public:
	typedef std::vector<uint8> FunctionCode;

	void AddFunction(FunctionCode);

	void WriteModule(Framework::CStream&);

private:
	std::vector<FunctionCode> m_functions;
};
