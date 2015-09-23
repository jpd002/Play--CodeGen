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

		void            GenerateCode(const StatementList&, unsigned int) override;
		void            SetStream(Framework::CStream*) override;
		void            RegisterExternalSymbols(CObjectFile*) const override;
		unsigned int    GetAvailableRegisterCount() const override;
		unsigned int    GetAvailableMdRegisterCount() const override;
		bool            CanHold128BitsReturnValueInRegisters() const override;

	private:
		typedef std::map<uint32, CAArch64Assembler::LABEL> LabelMapType;
		typedef void (CCodeGen_AArch64::*ConstCodeEmitterType)(const STATEMENT&);

		struct PARAM_STATE
		{
			bool prepared = false;
			unsigned int index = 0;
		};

		typedef std::function<void (PARAM_STATE&)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;

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
		
		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterDef(CSymbol*, CAArch64Assembler::REGISTER32);
		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterUse(CSymbol*, CAArch64Assembler::REGISTER32);
		void                             CommitSymbolRegister(CSymbol*, CAArch64Assembler::REGISTER32);
		
		CAArch64Assembler::REGISTER32    PrepareParam(PARAM_STATE&);
		CAArch64Assembler::REGISTER64    PrepareParam64(PARAM_STATE&);
		void                             CommitParam(PARAM_STATE&);
		void                             CommitParam64(PARAM_STATE&);
		
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
		
		struct LOGICOP_XOR : public LOGICOP_BASE
		{
			static OpRegType    OpReg()    { return &CAArch64Assembler::Eor; }
		};
		
		//ADDSUBOP ----------------------------------------------------------
		struct ADDSUBOP_BASE
		{
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, uint16 imm, CAArch64Assembler::ADDSUB_IMM_SHIFT_TYPE);
		};
		
		struct ADDSUBOP_SUB : public ADDSUBOP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Sub; }
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

		void    Emit_Prolog(uint32);
		void    Emit_Epilog(uint32);
		
		CAArch64Assembler::LABEL GetLabel(uint32);
		void                     MarkLabel(const STATEMENT&);
		
		void    Emit_Mov_MemAny(const STATEMENT&);
		void    Emit_Mov_VarAny(const STATEMENT&);
		
		void    Emit_Not_VarVar(const STATEMENT&);
		
		void    Emit_Mov_Mem64Mem64(const STATEMENT&);
		
		void    Emit_ExtLow64VarMem64(const STATEMENT&);
		void    Emit_ExtHigh64VarMem64(const STATEMENT&);
		
		void    Emit_Param_Ctx(const STATEMENT&);
		void    Emit_Param_Mem(const STATEMENT&);
		
		void    Emit_Call(const STATEMENT&);
		void    Emit_RetVal_Tmp(const STATEMENT&);
		
		void    Emit_Jmp(const STATEMENT&);
		
		void    Emit_CondJmp(const STATEMENT&);
		void    Emit_CondJmp_VarCst(const STATEMENT&);
		
		//ADDSUB
		template <typename> void    Emit_AddSub_VarVarCst(const STATEMENT&);
		
		//SHIFT
		template <typename> void    Emit_Shift_VarAnyVar(const STATEMENT&);
		template <typename> void    Emit_Shift_VarVarCst(const STATEMENT&);

		//LOGIC
		template <typename> void    Emit_Logic_VarAnyVar(const STATEMENT&);
		template <typename> void    Emit_Logic_VarVarCst(const STATEMENT&);

		//DIV
		template <bool> void Emit_Div_Tmp64AnyAny(const STATEMENT&);
		
		//SHIFT64
		template <typename> void    Emit_Shift64_MemMemVar(const STATEMENT&);
		template <typename> void    Emit_Shift64_MemMemCst(const STATEMENT&);
		
		static CONSTMATCHER    g_constMatchers[];
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
	};
};
