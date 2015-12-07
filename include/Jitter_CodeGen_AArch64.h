#pragma once

#include <deque>
#include "Jitter_CodeGen.h"
#include "AArch64Assembler.h"

namespace Jitter
{
	class CCodeGen_AArch64 : public CCodeGen
	{
	public:
		           CCodeGen_AArch64();
		virtual    ~CCodeGen_AArch64();

		void            SetGenerateRelocatableCalls(bool);

		void            GenerateCode(const StatementList&, unsigned int) override;
		void            SetStream(Framework::CStream*) override;
		void            RegisterExternalSymbols(CObjectFile*) const override;
		unsigned int    GetAvailableRegisterCount() const override;
		unsigned int    GetAvailableMdRegisterCount() const override;
		bool            CanHold128BitsReturnValueInRegisters() const override;

	private:
		typedef std::map<uint32, CAArch64Assembler::LABEL> LabelMapType;
		typedef void (CCodeGen_AArch64::*ConstCodeEmitterType)(const STATEMENT&);

		struct ADDSUB_IMM_PARAMS
		{
			uint16                                      imm = 0;
			CAArch64Assembler::ADDSUB_IMM_SHIFT_TYPE    shiftType = CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0;
		};

		struct PARAM_STATE
		{
			bool prepared = false;
			unsigned int index = 0;
		};

		typedef std::function<void (PARAM_STATE&)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;

		enum
		{
			MAX_REGISTERS = 9,
		};

		enum MAX_PARAM_REGS
		{
			MAX_PARAM_REGS = 8,
		};

		enum MAX_TEMP_REGS
		{
			MAX_TEMP_REGS = 7,
		};

		struct CONSTMATCHER
		{
			OPERATION               op;
			MATCHTYPE               dstType;
			MATCHTYPE               src1Type;
			MATCHTYPE               src2Type;
			ConstCodeEmitterType    emitter;
		};

		CAArch64Assembler::REGISTER32    GetNextTempRegister();
		CAArch64Assembler::REGISTER64    GetNextTempRegister64();
		
		void    LoadMemoryInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		void    StoreRegisterInMemory(CSymbol*, CAArch64Assembler::REGISTER32);
		
		void    LoadMemory64InRegister(CAArch64Assembler::REGISTER64, CSymbol*);
		void    StoreRegisterInMemory64(CSymbol*, CAArch64Assembler::REGISTER64);
		
		void    LoadConstantInRegister(CAArch64Assembler::REGISTER32, uint32);
		void    LoadConstant64InRegister(CAArch64Assembler::REGISTER64, uint64);
		
		void    LoadMemory64LowInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		void    LoadMemory64HighInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		
		void    LoadSymbol64InRegister(CAArch64Assembler::REGISTER64, CSymbol*);

		void    LoadMemoryReferenceInRegister(CAArch64Assembler::REGISTER64, CSymbol*);
		void    StoreRegisterInTemporaryReference(CSymbol*, CAArch64Assembler::REGISTER64);
		
		void    LoadMemory128AddressInRegister(CAArch64Assembler::REGISTER64, CSymbol*, uint32 = 0);
		void    LoadRelative128AddressInRegister(CAArch64Assembler::REGISTER64, CSymbol*, uint32);
		void    LoadTemporary128AddressInRegister(CAArch64Assembler::REGISTER64, CSymbol*, uint32);
		
		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterDef(CSymbol*, CAArch64Assembler::REGISTER32);
		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterUse(CSymbol*, CAArch64Assembler::REGISTER32);
		void                             CommitSymbolRegister(CSymbol*, CAArch64Assembler::REGISTER32);
		
		CAArch64Assembler::REGISTER32    PrepareParam(PARAM_STATE&);
		CAArch64Assembler::REGISTER64    PrepareParam64(PARAM_STATE&);
		void                             CommitParam(PARAM_STATE&);
		void                             CommitParam64(PARAM_STATE&);
		
		bool TryGetAddSubImmParams(uint32, ADDSUB_IMM_PARAMS&);
		bool TryGetAddSub64ImmParams(uint64, ADDSUB_IMM_PARAMS&);
		
		//SHIFTOP ----------------------------------------------------------
		struct SHIFTOP_BASE
		{
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, uint8);
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32);
		};
		
		struct SHIFTOP_ASR : public SHIFTOP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Asr; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Asrv; }
		};
		
		struct SHIFTOP_LSL : public SHIFTOP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Lsl; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Lslv; }
		};

		struct SHIFTOP_LSR : public SHIFTOP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Lsr; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Lsrv; }
		};
		
		//LOGICOP ----------------------------------------------------------
		struct LOGICOP_BASE
		{
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32);
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, uint8, uint8);
		};
		
		struct LOGICOP_AND : public LOGICOP_BASE
		{
			static OpRegType    OpReg()    { return &CAArch64Assembler::And; }
			static OpImmType    OpImm()    { return &CAArch64Assembler::And; }
		};
		
		struct LOGICOP_OR : public LOGICOP_BASE
		{
			static OpRegType    OpReg()    { return &CAArch64Assembler::Orr; }
		};

		struct LOGICOP_XOR : public LOGICOP_BASE
		{
			static OpRegType    OpReg()    { return &CAArch64Assembler::Eor; }
		};
		
		//ADDSUBOP ----------------------------------------------------------
		struct ADDSUBOP_BASE
		{
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, uint16 imm, CAArch64Assembler::ADDSUB_IMM_SHIFT_TYPE);
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32);
		};
		
		struct ADDSUBOP_ADD : public ADDSUBOP_BASE
		{
			static OpImmType    OpImm()       { return &CAArch64Assembler::Add; }
			static OpRegType    OpReg()       { return &CAArch64Assembler::Add; }
			static OpImmType    OpImmRev()    { return &CAArch64Assembler::Sub; }
		};

		struct ADDSUBOP_SUB : public ADDSUBOP_BASE
		{
			static OpImmType    OpImm()       { return &CAArch64Assembler::Sub; }
			static OpRegType    OpReg()       { return &CAArch64Assembler::Sub; }
			static OpImmType    OpImmRev()    { return &CAArch64Assembler::Add; }
		};

		//SHIFT64OP ----------------------------------------------------------
		struct SHIFT64OP_BASE
		{
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER64, CAArch64Assembler::REGISTER64, uint8);
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTER64, CAArch64Assembler::REGISTER64, CAArch64Assembler::REGISTER64);
		};
		
		struct SHIFT64OP_ASR : public SHIFT64OP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Asr; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Asrv; }
		};
		
		struct SHIFT64OP_LSL : public SHIFT64OP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Lsl; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Lslv; }
		};

		struct SHIFT64OP_LSR : public SHIFT64OP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Lsr; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Lsrv; }
		};

		//MDOP -----------------------------------------------------------
		struct MDOP_BASE3
		{
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTERMD, CAArch64Assembler::REGISTERMD, CAArch64Assembler::REGISTERMD);
		};

		struct MDOP_ADDB : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Add_16b; }
		};
		
		struct MDOP_ADDH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Add_8h; }
		};
		
		struct MDOP_ADDW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Add_4s; }
		};

		struct MDOP_ADDBUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Uqadd_16b; }
		};
		
		struct MDOP_ADDWUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Uqadd_4s; }
		};
		
		struct MDOP_ADDHSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sqadd_8h; }
		};

		struct MDOP_ADDWSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sqadd_4s; }
		};
		
		struct MDOP_SUBB : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sub_16b; }
		};
		
		struct MDOP_SUBH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sub_8h; }
		};

		struct MDOP_SUBW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sub_4s; }
		};
		
		struct MDOP_SUBBUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Uqsub_16b; }
		};
		
		struct MDOP_SUBHUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Uqsub_8h; }
		};
		
		struct MDOP_SUBHSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sqsub_8h; }
		};

		struct MDOP_SUBWSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sqsub_4s; }
		};

		struct MDOP_AND : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::And_16b; }
		};
		
		struct MDOP_OR : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Orr_16b; }
		};
		
		struct MDOP_XOR : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Eor_16b; }
		};
		
		uint16    GetSavedRegisterList(uint32);
		void      Emit_Prolog(uint32, uint16);
		void      Emit_Epilog(uint32, uint16);
		
		CAArch64Assembler::LABEL GetLabel(uint32);
		void                     MarkLabel(const STATEMENT&);
		
		void    Emit_Nop(const STATEMENT&);
		
		void    Emit_Mov_MemAny(const STATEMENT&);
		void    Emit_Mov_VarAny(const STATEMENT&);
		
		void    Emit_Not_VarVar(const STATEMENT&);
		void    Emit_Lzc_VarVar(const STATEMENT&);
		
		void    Emit_Mov_Mem64Mem64(const STATEMENT&);
		void    Emit_Mov_Mem64Cst64(const STATEMENT&);
		
		void    Emit_ExtLow64VarMem64(const STATEMENT&);
		void    Emit_ExtHigh64VarMem64(const STATEMENT&);
		
		void    Emit_RelToRef_TmpCst(const STATEMENT&);
		void    Emit_AddRef_TmpMemAny(const STATEMENT&);
		void    Emit_LoadFromRef_VarMem(const STATEMENT&);
		void    Emit_StoreAtRef_MemAny(const STATEMENT&);
		
		void    Emit_Param_Ctx(const STATEMENT&);
		void    Emit_Param_Reg(const STATEMENT&);
		void    Emit_Param_Mem(const STATEMENT&);
		void    Emit_Param_Cst(const STATEMENT&);
		void    Emit_Param_Mem64(const STATEMENT&);
		void    Emit_Param_Cst64(const STATEMENT&);
		void    Emit_Param_Mem128(const STATEMENT&);
		
		void    Emit_Call(const STATEMENT&);
		void    Emit_RetVal_Reg(const STATEMENT&);
		void    Emit_RetVal_Tmp(const STATEMENT&);
		void    Emit_RetVal_Mem64(const STATEMENT&);
		void    Emit_RetVal_Mem128(const STATEMENT&);
		
		void    Emit_Jmp(const STATEMENT&);
		
		void    Emit_CondJmp(const STATEMENT&);
		void    Emit_CondJmp_AnyVar(const STATEMENT&);
		void    Emit_CondJmp_VarCst(const STATEMENT&);
		
		void    Cmp_GetFlag(CAArch64Assembler::REGISTER32, Jitter::CONDITION);
		void    Emit_Cmp_VarAnyVar(const STATEMENT&);
		void    Emit_Cmp_VarVarCst(const STATEMENT&);
		
		void    Emit_Add64_MemMemMem(const STATEMENT&);
		void    Emit_Add64_MemMemCst(const STATEMENT&);
		
		void    Emit_Sub64_MemAnyMem(const STATEMENT&);
		void    Emit_Sub64_MemMemCst(const STATEMENT&);
		
		void    Emit_Cmp64_VarAnyMem(const STATEMENT&);
		void    Emit_Cmp64_VarMemCst(const STATEMENT&);
		
		void    Emit_And64_MemMemMem(const STATEMENT&);
		
		//ADDSUB
		template <typename> void    Emit_AddSub_VarAnyVar(const STATEMENT&);
		template <typename> void    Emit_AddSub_VarVarCst(const STATEMENT&);
		
		//SHIFT
		template <typename> void    Emit_Shift_VarAnyVar(const STATEMENT&);
		template <typename> void    Emit_Shift_VarVarCst(const STATEMENT&);

		//LOGIC
		template <typename> void    Emit_Logic_VarAnyVar(const STATEMENT&);
		template <typename> void    Emit_Logic_VarVarCst(const STATEMENT&);

		//MUL
		template <bool> void Emit_Mul_Tmp64AnyAny(const STATEMENT&);
		
		//DIV
		template <bool> void Emit_Div_Tmp64AnyAny(const STATEMENT&);
		
		//SHIFT64
		template <typename> void    Emit_Shift64_MemMemVar(const STATEMENT&);
		template <typename> void    Emit_Shift64_MemMemCst(const STATEMENT&);
		
		//MD
		template <typename> void    Emit_Md_MemMemMem(const STATEMENT&);

		void    Emit_Md_Mov_MemMem(const STATEMENT&);
		void    Emit_Md_Not_MemMem(const STATEMENT&);
		
		void    Emit_Md_MovMasked_MemMemMem(const STATEMENT&);
		
		static CONSTMATCHER    g_constMatchers[];
		static CONSTMATCHER    g_64ConstMatchers[];
		static CONSTMATCHER    g_mdConstMatchers[];
		
		static CAArch64Assembler::REGISTER32    g_registers[MAX_REGISTERS];
		static CAArch64Assembler::REGISTER32    g_tempRegisters[MAX_TEMP_REGS];
		static CAArch64Assembler::REGISTER64    g_tempRegisters64[MAX_TEMP_REGS];
		static CAArch64Assembler::REGISTER32    g_paramRegisters[MAX_PARAM_REGS];
		static CAArch64Assembler::REGISTER64    g_paramRegisters64[MAX_PARAM_REGS];
		static CAArch64Assembler::REGISTER64    g_baseRegister;

		Framework::CStream*    m_stream = nullptr;
		CAArch64Assembler      m_assembler;
		LabelMapType           m_labels;
		ParamStack             m_params;
		uint32                 m_nextTempRegister = 0;

		bool    m_generateRelocatableCalls = false;
	};
};
