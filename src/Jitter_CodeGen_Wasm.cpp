#include "Jitter_CodeGen_Wasm.h"
#include "StdStream.h"
#include "StdStreamUtils.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

void CCodeGen_Wasm::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	CWasmModuleBuilder moduleBuilder;
	moduleBuilder.WriteModule(*m_stream);
}

void CCodeGen_Wasm::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
}

void CCodeGen_Wasm::RegisterExternalSymbols(CObjectFile*) const
{
}

unsigned int CCodeGen_Wasm::GetAvailableRegisterCount() const
{
	return 0;
}

unsigned int CCodeGen_Wasm::GetAvailableMdRegisterCount() const
{
	return 0;
}

bool CCodeGen_Wasm::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

uint32 CCodeGen_Wasm::GetPointerSize() const
{
	return 4;
}
