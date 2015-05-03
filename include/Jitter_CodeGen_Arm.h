#pragma once

#include "Jitter_CodeGen.h"
#include "ArmAssembler.h"
#include <deque>
#include <functional>

namespace Jitter
{
	class CCodeGen_Arm : public CCodeGen
	{
	public:
												CCodeGen_Arm();
		virtual									~CCodeGen_Arm();

		void									GenerateCode(const StatementList&, unsigned int) override;
		void									SetStream(Framework::CStream*) override;
		void									RegisterExternalSymbols(CObjectFile*) const override;
		unsigned int							GetAvailableRegisterCount() const override;
		unsigned int							GetAvailableMdRegisterCount() const override;
		bool									CanHold128BitsReturnValueInRegisters() const override;

	private:
		typedef std::map<uint32, CArmAssembler::LABEL> LabelMapType;
		typedef void (CCodeGen_Arm::*ConstCodeEmitterType)(const STATEMENT&);

		struct PARAM_STATE
		{
			bool prepared = false;
			unsigned int index = 0;
		};

		class CTempRegisterContext
		{
		public:
			CArmAssembler::REGISTER Allocate()
			{
				return static_cast<CArmAssembler::REGISTER>(m_nextRegister++);
			}

			void Release(CArmAssembler::REGISTER reg)
			{
				m_nextRegister--;
				assert(reg == m_nextRegister);
			}

		private:
			uint8 m_nextRegister = CArmAssembler::r0;
		};

		typedef std::function<void (PARAM_STATE&)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;
		
		enum MAX_PARAM_REGS
		{
			MAX_PARAM_REGS = 4,
		};
		
		enum
		{
			MAX_REGISTERS = 6,
		};

		struct CONSTMATCHER
		{
			OPERATION							op;
			MATCHTYPE							dstType;
			MATCHTYPE							src1Type;
			MATCHTYPE							src2Type;
			ConstCodeEmitterType				emitter;
		};

		static uint16							GetSavedRegisterList(uint32);

		void									Emit_Prolog(unsigned int, uint16);
		void									Emit_Epilog(unsigned int, uint16);

		CArmAssembler::LABEL					GetLabel(uint32);
		void									MarkLabel(const STATEMENT&);

		void									LoadMemoryInRegister(CArmAssembler::REGISTER, CSymbol*);
		void									StoreRegisterInMemory(CSymbol*, CArmAssembler::REGISTER);

		void									LoadRelativeInRegister(CArmAssembler::REGISTER, CSymbol*);
		void									StoreRegisterInRelative(CSymbol*, CArmAssembler::REGISTER);
		
		void									LoadTemporaryInRegister(CArmAssembler::REGISTER, CSymbol*);
		void									StoreRegisterInTemporary(CSymbol*, CArmAssembler::REGISTER);
		
		void									LoadRelativeReferenceInRegister(CArmAssembler::REGISTER, CSymbol*);

		void									LoadTemporaryReferenceInRegister(CArmAssembler::REGISTER, CSymbol*);
		void									StoreInRegisterTemporaryReference(CSymbol*, CArmAssembler::REGISTER);
		
		void									LoadMemory64LowInRegister(CArmAssembler::REGISTER, CSymbol*);
		void									LoadMemory64HighInRegister(CArmAssembler::REGISTER, CSymbol*);
		void									LoadMemory64InRegisters(CArmAssembler::REGISTER, CArmAssembler::REGISTER, CSymbol*);

		void									StoreRegisterInMemory64Low(CSymbol*, CArmAssembler::REGISTER);
		void									StoreRegisterInMemory64High(CSymbol*, CArmAssembler::REGISTER);
		void									StoreRegistersInMemory64(CSymbol*, CArmAssembler::REGISTER, CArmAssembler::REGISTER);

		void									LoadMemoryFpSingleInRegister(CTempRegisterContext&, CArmAssembler::SINGLE_REGISTER, CSymbol*);
		void									StoreRegisterInMemoryFpSingle(CTempRegisterContext&, CSymbol*, CArmAssembler::SINGLE_REGISTER);

		void									LoadRelativeFpSingleInRegister(CTempRegisterContext&, CArmAssembler::SINGLE_REGISTER, CSymbol*);
		void									StoreRelativeFpSingleInRegister(CTempRegisterContext&, CSymbol*, CArmAssembler::SINGLE_REGISTER);

		void									LoadTemporaryFpSingleInRegister(CTempRegisterContext&, CArmAssembler::SINGLE_REGISTER, CSymbol*);
		void									StoreTemporaryFpSingleInRegister(CTempRegisterContext&, CSymbol*, CArmAssembler::SINGLE_REGISTER);

		void									LoadMemory128AddressInRegister(CArmAssembler::REGISTER, CSymbol*);
		void									LoadRelative128AddressInRegister(CArmAssembler::REGISTER, CSymbol*);
		void									LoadTemporary128AddressInRegister(CArmAssembler::REGISTER, CSymbol*);

		void									LoadTemporary256ElementAddressInRegister(CArmAssembler::REGISTER, CSymbol*, uint32);

		CArmAssembler::REGISTER					PrepareSymbolRegisterDef(CSymbol*, CArmAssembler::REGISTER);
		CArmAssembler::REGISTER					PrepareSymbolRegisterUse(CSymbol*, CArmAssembler::REGISTER);
		void									CommitSymbolRegister(CSymbol*, CArmAssembler::REGISTER);

		typedef std::pair<CArmAssembler::REGISTER, CArmAssembler::REGISTER> ParamRegisterPair;

		CArmAssembler::REGISTER					PrepareParam(PARAM_STATE&);
		ParamRegisterPair						PrepareParam64(PARAM_STATE&);
		void									CommitParam(PARAM_STATE&);
		void									CommitParam64(PARAM_STATE&);

		CArmAssembler::AluLdrShift				GetAluShiftFromSymbol(CArmAssembler::SHIFT shiftType, CSymbol* symbol, CArmAssembler::REGISTER preferedRegister);

		static uint32							RotateRight(uint32);
		static uint32							RotateLeft(uint32);
		bool									TryGetAluImmediateParams(uint32, uint8&, uint8&);
		void									LoadConstantInRegister(CArmAssembler::REGISTER, uint32);
		void									LoadConstantPtrInRegister(CArmAssembler::REGISTER, uintptr_t);

		//ALUOP ----------------------------------------------------------
		struct ALUOP_BASE
		{
			typedef void (CArmAssembler::*OpImmType)(CArmAssembler::REGISTER, CArmAssembler::REGISTER, const CArmAssembler::ImmediateAluOperand&);
			typedef void (CArmAssembler::*OpRegType)(CArmAssembler::REGISTER, CArmAssembler::REGISTER, CArmAssembler::REGISTER);
			
			typedef void (CArmAssembler::*OpImmNegType)(CArmAssembler::REGISTER, CArmAssembler::REGISTER, const CArmAssembler::ImmediateAluOperand&);
			typedef void (CArmAssembler::*OpImmNotType)(CArmAssembler::REGISTER, CArmAssembler::REGISTER, const CArmAssembler::ImmediateAluOperand&);
		};
		
		struct ALUOP_ADD : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CArmAssembler::Add; }
			static OpRegType	OpReg()		{ return &CArmAssembler::Add; }
			
			static OpImmNegType OpImmNeg()	{ return &CArmAssembler::Sub; }
			static OpImmNotType OpImmNot()	{ return NULL; }
		};
		
		struct ALUOP_SUB : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CArmAssembler::Sub; }
			static OpRegType	OpReg()		{ return &CArmAssembler::Sub; }
			
			static OpImmNegType OpImmNeg()	{ return &CArmAssembler::Add; }
			static OpImmNotType OpImmNot()	{ return NULL; }
		};
		
		struct ALUOP_AND : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CArmAssembler::And; }
			static OpRegType	OpReg()		{ return &CArmAssembler::And; }
			
			static OpImmNegType OpImmNeg()	{ return NULL; }
			static OpImmNotType OpImmNot()	{ return &CArmAssembler::Bic; }
		};

		struct ALUOP_OR : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CArmAssembler::Or; }
			static OpRegType	OpReg()		{ return &CArmAssembler::Or; }
			
			static OpImmNegType OpImmNeg()	{ return NULL; }
			static OpImmNotType OpImmNot()	{ return NULL; }
		};
		
		struct ALUOP_XOR : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CArmAssembler::Eor; }
			static OpRegType	OpReg()		{ return &CArmAssembler::Eor; }
			
			static OpImmNegType OpImmNeg()	{ return NULL; }
			static OpImmNotType OpImmNot()	{ return NULL; }
		};
		
		//FPUOP -----------------------------------------------------------
		struct FPUOP_BASE2
		{
			typedef void (CArmAssembler::*OpRegType)(CArmAssembler::SINGLE_REGISTER, CArmAssembler::SINGLE_REGISTER);
		};

		struct FPUOP_BASE3
		{
			typedef void (CArmAssembler::*OpRegType)(CArmAssembler::SINGLE_REGISTER, CArmAssembler::SINGLE_REGISTER, CArmAssembler::SINGLE_REGISTER);
		};

		struct FPUMDOP_BASE2
		{
			typedef void (CArmAssembler::*OpRegType)(CArmAssembler::QUAD_REGISTER, CArmAssembler::QUAD_REGISTER);
		};

		struct FPUMDOP_BASE3
		{
			typedef void (CArmAssembler::*OpRegType)(CArmAssembler::QUAD_REGISTER, CArmAssembler::QUAD_REGISTER, CArmAssembler::QUAD_REGISTER);
		};

		struct FPUOP_ABS : public FPUOP_BASE2
		{
			static OpRegType OpReg() { return &CArmAssembler::Vabs_F32; }
		};

		struct FPUOP_NEG : public FPUOP_BASE2
		{
			static OpRegType OpReg() { return &CArmAssembler::Vneg_F32; }
		};

		struct FPUOP_SQRT : public FPUOP_BASE2
		{
			static OpRegType OpReg() { return &CArmAssembler::Vsqrt_F32; }
		};

		struct FPUOP_ADD : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vadd_F32; }
		};

		struct FPUOP_SUB : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vsub_F32; }
		};

		struct FPUOP_MUL : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vmul_F32; }
		};

		struct FPUOP_DIV : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vdiv_F32; }
		};

		struct FPUMDOP_RCPL : public FPUMDOP_BASE2
		{
			static OpRegType OpReg() { return &CArmAssembler::Vrecpe_F32; }
		};

		struct FPUMDOP_RSQRT : public FPUMDOP_BASE2
		{
			static OpRegType OpReg() { return &CArmAssembler::Vrsqrte_F32; }
		};

		struct FPUMDOP_MIN : public FPUMDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vmin_F32; }
		};

		struct FPUMDOP_MAX : public FPUMDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vmax_F32; }
		};

		//MDOP -----------------------------------------------------------
		struct MDOP_BASE2
		{
			typedef void (CArmAssembler::*OpRegType)(CArmAssembler::QUAD_REGISTER, CArmAssembler::QUAD_REGISTER);
		};

		struct MDOP_BASE3
		{
			typedef void (CArmAssembler::*OpRegType)(CArmAssembler::QUAD_REGISTER, CArmAssembler::QUAD_REGISTER, CArmAssembler::QUAD_REGISTER);
		};

		struct MDOP_ADDW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vadd_I32; }
		};

		struct MDOP_ADDBUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vqadd_U8; }
		};

		struct MDOP_ADDWUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vqadd_U32; }
		};

		struct MDOP_SUBB : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vsub_I8; }
		};

		struct MDOP_SUBW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vsub_I32; }
		};

		struct MDOP_CMPEQW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vceq_I32; }
		};

		struct MDOP_AND : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vand; }
		};

		struct MDOP_OR : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vorr; }
		};

		struct MDOP_XOR : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Veor; }
		};

		struct MDOP_ADDS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vadd_F32; }
		};

		struct MDOP_SUBS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vsub_F32; }
		};

		struct MDOP_MULS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CArmAssembler::Vmul_F32; }
		};

		struct MDOP_ABSS : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CArmAssembler::Vabs_F32; }
		};

		struct MDOP_TOSINGLE : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CArmAssembler::Vcvt_F32_S32; }
		};

		struct MDOP_TOWORD : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CArmAssembler::Vcvt_S32_F32; }
		};

		//ALUOP
		template <typename> void				Emit_Alu_GenericAnyAny(const STATEMENT&);
		template <typename> void				Emit_Alu_GenericAnyCst(const STATEMENT&);

		//SHIFT
		template <CArmAssembler::SHIFT> void	Emit_Shift_Generic(const STATEMENT&);

		//PARAM
		void									Emit_Param_Ctx(const STATEMENT&);
		void									Emit_Param_Reg(const STATEMENT&);
		void									Emit_Param_Mem(const STATEMENT&);
		void									Emit_Param_Cst(const STATEMENT&);
		void									Emit_Param_Mem64(const STATEMENT&);
		void									Emit_Param_Cst64(const STATEMENT&);
		void									Emit_Param_Mem128(const STATEMENT&);

		//PARAM_RET
		void									Emit_ParamRet_Tmp128(const STATEMENT&);

		//CALL
		void									Emit_Call(const STATEMENT&);

		//RETVAL
		void									Emit_RetVal_Reg(const STATEMENT&);
		void									Emit_RetVal_Tmp(const STATEMENT&);
		void									Emit_RetVal_Mem64(const STATEMENT&);

		//MUL/MULS
		template<bool> void						Emit_MulTmp64AnyAny(const STATEMENT&);

		//DIV/DIVS
		template<bool> void						Div_GenericTmp64RegReg_Quotient(CSymbol*);
		template<bool> void						Div_GenericTmp64RegReg_Remainder(CSymbol*);
		template<bool> void						Emit_DivTmp64RegReg(const STATEMENT&);
		template<bool> void						Emit_DivTmp64RegCst(const STATEMENT&);
		template<bool> void						Emit_DivTmp64MemCst(const STATEMENT&);

		//MOV
		void									Emit_Mov_RegReg(const STATEMENT&);
		void									Emit_Mov_RegMem(const STATEMENT&);
		void									Emit_Mov_RegCst(const STATEMENT&);
		void									Emit_Mov_MemReg(const STATEMENT&);
		void									Emit_Mov_MemMem(const STATEMENT&);
		void									Emit_Mov_MemCst(const STATEMENT&);

		//LZC
		void									Lzc_RegReg(CArmAssembler::REGISTER, CArmAssembler::REGISTER);
		void									Emit_Lzc_RegReg(const STATEMENT&);
		void									Emit_Lzc_MemReg(const STATEMENT&);

		//NOP
		void									Emit_Nop(const STATEMENT&);
		
		//EXTLOW64
		void									Emit_ExtLow64VarMem64(const STATEMENT&);

		//EXTHIGH64
		void									Emit_ExtHigh64VarMem64(const STATEMENT&);

		//MERGETO64
		void									Emit_MergeTo64_Mem64RegReg(const STATEMENT&);
		void									Emit_MergeTo64_Mem64RegMem(const STATEMENT&);
		void									Emit_MergeTo64_Mem64CstReg(const STATEMENT&);
		void									Emit_MergeTo64_Mem64CstMem(const STATEMENT&);

		//CMP
		void									Cmp_GetFlag(CArmAssembler::REGISTER, CONDITION);
		void									Cmp_GenericRegCst(CArmAssembler::REGISTER, uint32, CArmAssembler::REGISTER);
		void									Emit_Cmp_AnyAnyAny(const STATEMENT&);
		void									Emit_Cmp_AnyAnyCst(const STATEMENT&);

		//JMP
		void									Emit_Jmp(const STATEMENT&);

		//CONDJMP
		void									Emit_CondJmp(const STATEMENT&);
		void									Emit_CondJmp_VarVar(const STATEMENT&);
		void									Emit_CondJmp_VarCst(const STATEMENT&);
		
		//NOT
		void									Emit_Not_RegReg(const STATEMENT&);
		void									Emit_Not_MemReg(const STATEMENT&);
		void									Emit_Not_MemMem(const STATEMENT&);

		//ADDREF
		void									Emit_AddRef_TmpRelReg(const STATEMENT&);
		void									Emit_AddRef_TmpRelCst(const STATEMENT&);
		
		//LOADFROMREF
		void									Emit_LoadFromRef_RegTmp(const STATEMENT&);
		
		//STOREATREF
		void									Emit_StoreAtRef_TmpReg(const STATEMENT&);
		void									Emit_StoreAtRef_TmpRel(const STATEMENT&);
		void									Emit_StoreAtRef_TmpCst(const STATEMENT&);
		
		//MOV64
		void									Emit_Mov_Mem64Mem64(const STATEMENT&);
		void									Emit_Mov_Mem64Cst64(const STATEMENT&);

		//ADD64
		void									Emit_Add64_MemMemMem(const STATEMENT&);
		void									Emit_Add64_MemMemCst(const STATEMENT&);

		//SUB64
		void									Emit_Sub64_MemMemMem(const STATEMENT&);
		void									Emit_Sub64_MemCstMem(const STATEMENT&);

		//AND64
		void									Emit_And64_MemMemMem(const STATEMENT&);

		//SLL64
		void									Emit_Sl64Var_MemMem(CSymbol*, CSymbol*, CArmAssembler::REGISTER);
		void									Emit_Sll64_MemMemVar(const STATEMENT&);
		void									Emit_Sll64_MemMemCst(const STATEMENT&);

		//SR64
		void									Emit_Sr64Var_MemMem(CSymbol*, CSymbol*, CArmAssembler::REGISTER, CArmAssembler::SHIFT);
		void									Emit_Sr64Cst_MemMem(CSymbol*, CSymbol*, uint32, CArmAssembler::SHIFT);

		//SRL64
		void									Emit_Srl64_MemMemVar(const STATEMENT&);
		void									Emit_Srl64_MemMemCst(const STATEMENT&);

		//SRA64
		void									Emit_Sra64_MemMemVar(const STATEMENT&);
		void									Emit_Sra64_MemMemCst(const STATEMENT&);

		//CMP64
		void									Cmp64_RegSymLo(CArmAssembler::REGISTER, CSymbol*, CArmAssembler::REGISTER);
		void									Cmp64_RegSymHi(CArmAssembler::REGISTER, CSymbol*, CArmAssembler::REGISTER);
		void									Cmp64_Equal(const STATEMENT&);
		void									Cmp64_Order(const STATEMENT&);
		void									Emit_Cmp64_VarMemAny(const STATEMENT&);

		//FPUOP
		template <typename> void				Emit_Fpu_MemMem(const STATEMENT&);
		template <typename> void				Emit_Fpu_MemMemMem(const STATEMENT&);
		template <typename> void				Emit_FpuMd_MemMem(const STATEMENT&);
		template <typename> void				Emit_FpuMd_MemMemMem(const STATEMENT&);
		void									Emit_Fp_Cmp_AnyMemMem(const STATEMENT&);
		void									Emit_Fp_Mov_MemSRelI32(const STATEMENT&);
		void									Emit_Fp_ToIntTrunc_MemMem(const STATEMENT&);
		void									Emit_Fp_LdCst_TmpCst(const STATEMENT&);
		
		//MDOP
		template <typename> void				Emit_Md_MemMem(const STATEMENT&);
		template <typename> void				Emit_Md_MemMemMem(const STATEMENT&);
		void									Emit_Md_Mov_MemMem(const STATEMENT&);
		void									Emit_Md_Not_MemMem(const STATEMENT&);
		void									Emit_Md_Srl256_MemMemVar(const STATEMENT&);
		void									Emit_Md_Srl256_MemMemCst(const STATEMENT&);

		void									Emit_Md_MovMasked_MemMemMem(const STATEMENT&);
		void									Emit_Md_Expand_MemReg(const STATEMENT&);
		void									Emit_Md_Expand_MemMem(const STATEMENT&);
		void									Emit_Md_Expand_MemCst(const STATEMENT&);

		void									Emit_MergeTo256_MemMemMem(const STATEMENT&);

		static CONSTMATCHER						g_constMatchers[];
		static CONSTMATCHER						g_64ConstMatchers[];
		static CONSTMATCHER						g_fpuConstMatchers[];
		static CONSTMATCHER						g_mdConstMatchers[];
		static CArmAssembler::REGISTER			g_registers[MAX_REGISTERS];
		static CArmAssembler::REGISTER			g_paramRegs[MAX_PARAM_REGS];
		static CArmAssembler::REGISTER			g_baseRegister;
		static CArmAssembler::REGISTER			g_callAddressRegister;
		static CArmAssembler::REGISTER			g_tempParamRegister0;
		static CArmAssembler::REGISTER			g_tempParamRegister1;

		Framework::CStream*						m_stream;
		CArmAssembler							m_assembler;
		LabelMapType							m_labels;
		ParamStack								m_params;
		uint32									m_stackLevel;
	};
};
