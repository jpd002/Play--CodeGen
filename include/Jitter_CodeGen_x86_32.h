#ifndef _JITTER_CODEGEN_X86_32_H_
#define _JITTER_CODEGEN_X86_32_H_

#include "Jitter_CodeGen_x86.h"
#include <deque>

namespace Jitter
{
	class CCodeGen_x86_32 : public CCodeGen_x86
	{
	public:
											CCodeGen_x86_32();
		virtual								~CCodeGen_x86_32();

		unsigned int						GetAvailableRegisterCount() const;

	protected:
		virtual void						Emit_Prolog(const StatementList&, unsigned int, uint32);
		virtual void						Emit_Epilog(unsigned int, uint32);

		//PARAM
		void								Emit_Param_Ctx(const STATEMENT&);
		void								Emit_Param_Mem(const STATEMENT&);
		void								Emit_Param_Cst(const STATEMENT&);
		void								Emit_Param_Reg(const STATEMENT&);
		void								Emit_Param_Mem64(const STATEMENT&);
		void								Emit_Param_Cst64(const STATEMENT&);
		void								Emit_Param_Mem128(const STATEMENT&);
		
		//PARAM_RET
		void								Emit_ParamRet_Mem128(const STATEMENT&);

		//CALL
		void								Emit_Call(const STATEMENT&);

		//RETURNVALUE
		void								Emit_RetVal_Tmp(const STATEMENT&);
		void								Emit_RetVal_Reg(const STATEMENT&);
		void								Emit_RetVal_Mem64(const STATEMENT&);

		//MOV
		void								Emit_Mov_Mem64Mem64(const STATEMENT&);
		void								Emit_Mov_Rel64Cst64(const STATEMENT&);

		//ADD64
		void								Emit_Add64_MemMemMem(const STATEMENT&);
		void								Emit_Add64_RelRelCst(const STATEMENT&);

		//SUB64
		void								Emit_Sub64_RelRelRel(const STATEMENT&);
		void								Emit_Sub64_RelCstRel(const STATEMENT&);

		//AND64
		void								Emit_And64_RelRelRel(const STATEMENT&);

		//SRL64
		void								Emit_Srl64_MemMemVar(const STATEMENT&, CX86Assembler::REGISTER);
		void								Emit_Srl64_MemMemReg(const STATEMENT&);
		void								Emit_Srl64_MemMemMem(const STATEMENT&);
		void								Emit_Srl64_RelRelCst(const STATEMENT&);

		//SRA64
		void								Emit_Sra64_MemMemVar(const STATEMENT&, CX86Assembler::REGISTER);
		void								Emit_Sra64_MemMemReg(const STATEMENT&);
		void								Emit_Sra64_MemMemMem(const STATEMENT&);
		void								Emit_Sra64_RelRelCst(const STATEMENT&);

		//SLL64
		void								Emit_Sll64_MemMemVar(const STATEMENT&, CX86Assembler::REGISTER);
		void								Emit_Sll64_MemMemReg(const STATEMENT&);
		void								Emit_Sll64_MemMemMem(const STATEMENT&);
		void								Emit_Sll64_RelRelCst(const STATEMENT&);

		//CMP64
		void								Cmp64_Equal(const STATEMENT&);
		template <typename> void			Cmp64_Order(const STATEMENT&);
		void								Cmp64_GenericRel(const STATEMENT&);
		void								Emit_Cmp64_RegRelRel(const STATEMENT&);
		void								Emit_Cmp64_RelRelRel(const STATEMENT&);
		void								Emit_Cmp64_RegRelCst(const STATEMENT&);
		void								Emit_Cmp64_RelRelCst(const STATEMENT&);
		void								Emit_Cmp64_TmpRelRoc(const STATEMENT&);

		//RELTOREF
		void								Emit_RelToRef_TmpCst(const STATEMENT&);

		//ADDREF
		void								Emit_AddRef_MemMemReg(const STATEMENT&);
		void								Emit_AddRef_MemMemCst(const STATEMENT&);

		//LOADFROMREF
		void								Emit_LoadFromRef_RegTmp(const STATEMENT&);
		void								Emit_LoadFromRef_MemTmp(const STATEMENT&);

		//STOREATREF
		void								Emit_StoreAtRef_TmpReg(const STATEMENT&);
		void								Emit_StoreAtRef_TmpMem(const STATEMENT&);
		void								Emit_StoreAtRef_TmpCst(const STATEMENT&);

	private:
		typedef std::function<uint32 (uint32)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;
		
		typedef void (CCodeGen_x86_32::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION				op;
			MATCHTYPE				dstType;
			MATCHTYPE				src1Type;
			MATCHTYPE				src2Type;
			ConstCodeEmitterType	emitter;
		};

		uint32								WriteCtxParam(uint32);
		uint32								WriteRegParam(uint32, CX86Assembler::REGISTER);
		uint32								WriteMemParam(uint32, CSymbol*);
		uint32								WriteCstParam(uint32, uint32);
		uint32								WriteMem64Param(uint32, CSymbol*);
		uint32								WriteCst64Param(uint32, CSymbol*);
		uint32								WriteMem128Param(uint32, const CX86Assembler::CAddress&);
		
		static CONSTMATCHER					g_constMatchers[];
		static CX86Assembler::REGISTER		g_registers[];
		
		ParamStack							m_params;
		uint32								m_paramAreaSize;
		bool								m_hasImplicitRetValueParam;
	};
}

#endif
