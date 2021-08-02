#pragma once

#include "Jitter_CodeGen.h"
#include "MemStream.h"

namespace Jitter
{
	class CCodeGen_Wasm : public CCodeGen
	{
	public:
		CCodeGen_Wasm();

		void GenerateCode(const StatementList&, unsigned int) override;
		void SetStream(Framework::CStream*) override;
		void RegisterExternalSymbols(CObjectFile*) const override;

		unsigned int GetAvailableRegisterCount() const override;
		unsigned int GetAvailableMdRegisterCount() const override;
		bool CanHold128BitsReturnValueInRegisters() const override;
		uint32 GetPointerSize() const override;

	private:
		typedef void (CCodeGen_Wasm::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION op;
			MATCHTYPE dstType;
			MATCHTYPE src1Type;
			MATCHTYPE src2Type;
			MATCHTYPE src3Type;
			ConstCodeEmitterType emitter;
		};

		static CONSTMATCHER g_constMatchers[];

		void MarkLabel(const STATEMENT&);

		void Emit_Mov_RelRel(const STATEMENT&);

		Framework::CStream* m_stream = nullptr;
		Framework::CMemStream m_functionStream;
	};
}
