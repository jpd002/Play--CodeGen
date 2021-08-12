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
		enum LABEL_FLOW
		{
			LABEL_FLOW_IF,
			LABEL_FLOW_ELSE,
			LABEL_FLOW_END,
		};

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

		void BuildLabelFlows(const StatementList&);

		void PushContext();
		void PushRelativeAddress(CSymbol*);
		void PushRelative(CSymbol*);
		void PushTemporary(CSymbol*);
		void PullTemporary(CSymbol*);

		void PrepareSymbolUse(CSymbol*);
		void PrepareSymbolDef(CSymbol*);
		void CommitSymbol(CSymbol*);

		void MarkLabel(const STATEMENT&);

		void Emit_Mov_RelAny(const STATEMENT&);

		void Emit_Param_Ctx(const STATEMENT&);
		void Emit_Param_Tmp(const STATEMENT&);

		void Emit_Call(const STATEMENT&);
		void Emit_RetVal_Tmp(const STATEMENT&);

		void Emit_Jmp(const STATEMENT&);
		void Emit_CondJmp_RelCst(const STATEMENT&);

		void Emit_Cmp_AnyAnyAny(const STATEMENT&);

		void Emit_Sll_AnyAnyAny(const STATEMENT&);
		void Emit_Srl_AnyAnyAny(const STATEMENT&);
		void Emit_Sra_AnyAnyAny(const STATEMENT&);

		void Emit_Xor_AnyAnyAny(const STATEMENT&);

		Framework::CStream* m_stream = nullptr;
		Framework::CMemStream m_functionStream;
		std::map<uint32, LABEL_FLOW> m_labelFlows;
	};
}
