#pragma once

#include "Jitter_CodeGen.h"

namespace Jitter
{
	class CCodeGen_Wasm : public CCodeGen
	{
	public:
		void GenerateCode(const StatementList&, unsigned int) override;
		void SetStream(Framework::CStream*) override;
		void RegisterExternalSymbols(CObjectFile*) const override;

		unsigned int GetAvailableRegisterCount() const override;
		unsigned int GetAvailableMdRegisterCount() const override;
		bool CanHold128BitsReturnValueInRegisters() const override;
		uint32 GetPointerSize() const override;

	private:
		Framework::CStream* m_stream = nullptr;
	};
}
