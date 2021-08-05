#pragma once

#include "Jitter_CodeGen.h"
#include "MemStream.h"

namespace Jitter
{
	class CWasmFunctionRegistry
	{
	public:
		static void RegisterFunction(uintptr_t, const char*, const char*);
		static int32 FindFunction(uintptr_t);

	private:
		static std::map<uintptr_t, int32> m_functions;
	};

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

		void PushContext();
		void PushRelativeAddress(CSymbol*);
		void PushTemporary(CSymbol*);
		void PullTemporary(CSymbol*);

		void MarkLabel(const STATEMENT&);

		void Emit_Mov_RelRel(const STATEMENT&);
		void Emit_Mov_RelTmp(const STATEMENT&);

		void Emit_Param_Ctx(const STATEMENT&);

		void Emit_Call(const STATEMENT&);
		void Emit_RetVal_Tmp(const STATEMENT&);

		void Emit_Sll_RelRelCst(const STATEMENT&);

		Framework::CStream* m_stream = nullptr;
		Framework::CMemStream m_functionStream;
	};
}
