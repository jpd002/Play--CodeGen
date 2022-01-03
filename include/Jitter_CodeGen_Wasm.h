#pragma once

#include <stack>
#include "Jitter_CodeGen.h"
#include "MemStream.h"

class CWasmModuleBuilder;

namespace Jitter
{
	class CWasmFunctionRegistry
	{
	public:
		struct WASM_FUNCTION_INFO
		{
			int32 id = 0;
			std::string signature;
		};

		static void RegisterFunction(uintptr_t, const char*, const char*);
		static const WASM_FUNCTION_INFO* FindFunction(uintptr_t);

	private:
		static std::map<uintptr_t, WASM_FUNCTION_INFO> m_functions;
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
			LABEL_FLOW_BLOCK,
			LABEL_FLOW_IF,
			LABEL_FLOW_ELSE,
			LABEL_FLOW_ENDIF,
		};

		typedef void (CCodeGen_Wasm::*ConstCodeEmitterType)(const STATEMENT&);

		typedef std::function<void(void)> ParamEmitterFunction;
		typedef std::stack<ParamEmitterFunction> ParamStack;

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
		static CONSTMATCHER g_64ConstMatchers[];
		static CONSTMATCHER g_fpuConstMatchers[];
		static CONSTMATCHER g_mdConstMatchers[];

		void BuildLabelFlows(const StatementList&);
		void PrepareSignatures(CWasmModuleBuilder&, const StatementList&);
		void RegisterSignature(CWasmModuleBuilder&, std::string);
		void PrepareLocalVars(const StatementList&);

		uint32 GetTemporaryLocation(CSymbol*) const;

		void PushContext();

		void PushRelativeAddress(CSymbol*);
		void PushRelative(CSymbol*);

		void PushTemporary(CSymbol*);
		void PullTemporary(CSymbol*);

		void PushRelativeRefAddress(CSymbol*);
		void PushRelativeRef(CSymbol*);

		void PushTemporaryRef(CSymbol*);
		void PullTemporaryRef(CSymbol*);

		void PushRelative64(CSymbol*);

		void PushTemporary64(CSymbol*);
		void PullTemporary64(CSymbol*);

		void PushRelativeSingle(CSymbol*);

		void PushTemporarySingle(CSymbol*);
		void PullTemporarySingle(CSymbol*);

		void PushRelative128(CSymbol*);

		void PushTemporary128(CSymbol*);
		void PullTemporary128(CSymbol*);

		void PrepareSymbolUse(CSymbol*);
		void PrepareSymbolDef(CSymbol*);
		void CommitSymbol(CSymbol*);

		void MarkLabel(const STATEMENT&);

		void Emit_Nop(const STATEMENT&);
		void Emit_Break(const STATEMENT&);

		void Emit_Mov_VarAny(const STATEMENT&);

		void Emit_RelToRef_VarCst(const STATEMENT&);

		void Emit_AddRef_AnyAnyAny(const STATEMENT&);
		void Emit_IsRefNull_VarVar(const STATEMENT&);

		void Emit_LoadFromRef_VarVar(const STATEMENT&);
		void Emit_LoadFromRef_Ref_VarVar(const STATEMENT&);
		void Emit_LoadFromRefIdx_VarVarAny(const STATEMENT&);
		void Emit_Load8FromRef_MemVar(const STATEMENT&);
		void Emit_Load16FromRef_MemVar(const STATEMENT&);

		void Emit_StoreAtRef_VarAny(const STATEMENT&);
		void Emit_StoreAtRefIdx_VarAnyAny(const STATEMENT&);
		void Emit_Store8AtRef_VarAny(const STATEMENT&);
		void Emit_Store16AtRef_VarAny(const STATEMENT&);

		void Emit_Param_Ctx(const STATEMENT&);
		void Emit_Param_Any(const STATEMENT&);

		void Emit_Call(const STATEMENT&);
		void Emit_RetVal_Tmp(const STATEMENT&);

		void Emit_ExternJmp(const STATEMENT&);

		void Emit_Jmp(const STATEMENT&);
		void Emit_CondJmp_AnyAny(const STATEMENT&);

		void Emit_Cmp_AnyAnyAny(const STATEMENT&);

		void Emit_Sll_AnyAnyAny(const STATEMENT&);
		void Emit_Srl_AnyAnyAny(const STATEMENT&);
		void Emit_Sra_AnyAnyAny(const STATEMENT&);

		void Emit_Not_AnyAny(const STATEMENT&);
		void Emit_Lzc_AnyAny(const STATEMENT&);
		void Emit_And_AnyAnyAny(const STATEMENT&);
		void Emit_Or_AnyAnyAny(const STATEMENT&);
		void Emit_Xor_AnyAnyAny(const STATEMENT&);

		void Emit_Add_AnyAnyAny(const STATEMENT&);
		void Emit_Sub_AnyAnyAny(const STATEMENT&);

		template <uint32> void Emit_Shift64_MemAnyAny(const STATEMENT&);

		void Emit_Mov64_MemAny(const STATEMENT&);
		void Emit_Add64_MemAnyAny(const STATEMENT&);
		void Emit_Sub64_MemAnyAny(const STATEMENT&);
		void Emit_And64_MemAnyAny(const STATEMENT&);
		void Emit_Cmp64_MemAnyAny(const STATEMENT&);

		void Emit_Load64FromRef_MemMem(const STATEMENT&);
		void Emit_Store64AtRef_MemAny(const STATEMENT&);

		void Emit_RetVal_Tmp64(const STATEMENT&);

		//MUL
		template <bool>
		void Emit_Mul_Tmp64AnyAny(const STATEMENT&);

		//DIV
		template <bool>
		void Emit_Div_Tmp64AnyAny(const STATEMENT&);

		void Emit_ExtLow64VarMem64(const STATEMENT&);
		void Emit_ExtHigh64VarMem64(const STATEMENT&);
		void Emit_MergeTo64_Mem64AnyAny(const STATEMENT&);

		//FPU
		template <uint32> void Emit_Fpu_MemMem(const STATEMENT&);
		template <uint32> void Emit_Fpu_MemMemMem(const STATEMENT&);
		void Emit_Fp_Cmp_AnyMemMem(const STATEMENT&);
		void Emit_Fp_Rcpl_MemMem(const STATEMENT&);
		void Emit_Fp_Rsqrt_MemMem(const STATEMENT&);
		void Emit_Fp_Mov_MemSRelI32(const STATEMENT&);
		void Emit_Fp_ToIntTrunc_MemMem(const STATEMENT&);
		void Emit_Fp_LdCst_TmpCst(const STATEMENT&);
		
		//MD
		template <uint32> void Emit_Md_MemMem(const STATEMENT&);
		template <uint32> void Emit_Md_MemMemMem(const STATEMENT&);
		template <uint32> void Emit_Md_Shift_MemMemCst(const STATEMENT&);
		template <const uint8*> void Emit_Md_Unpack_MemMemMemRev(const STATEMENT&);
		void Emit_Md_Mov_MemMem(const STATEMENT&);
		void Emit_Md_AddUSW_MemMemMem(const STATEMENT&);
		void Emit_Md_MakeSz_MemMem(const STATEMENT&);
		void Emit_Md_LoadFromRef_MemMem(const STATEMENT&);
		void Emit_Md_StoreAtRef_MemMem(const STATEMENT&);
		void Emit_Md_MovMasked_MemMemMem(const STATEMENT&);
		void Emit_Md_Expand_MemAny(const STATEMENT&);
		void Emit_Md_Srl256_MemMemVar(const STATEMENT&);
		void Emit_Md_Srl256_MemMemCst(const STATEMENT&);
		void Emit_MergeTo256_MemMemMem(const STATEMENT&);

		typedef std::pair<SYM_TYPE, uint32> TemporaryInstance;

		Framework::CStream* m_stream = nullptr;
		Framework::CMemStream m_functionStream;
		std::map<uint32, LABEL_FLOW> m_labelFlows;
		std::map<std::string, uint32> m_signatures;
		std::map<TemporaryInstance, uint32> m_temporaryLocations;
		uint32 m_localI32Count = 0;
		uint32 m_localI64Count = 0;
		uint32 m_localF32Count = 0;
		uint32 m_localV128Count = 0;
		bool m_isInsideBlock = false;
		uint32 m_currentBlockDepth = 0;
		ParamStack m_params;
	};
}
