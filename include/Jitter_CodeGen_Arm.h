#ifndef _JITTER_CODEGEN_ARM_H_
#define _JITTER_CODEGEN_ARM_H_

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

		void									GenerateCode(const StatementList&, unsigned int);
		void									SetStream(Framework::CStream*);
		unsigned int							GetAvailableRegisterCount() const;

	private:
		typedef std::map<uint32, CArmAssembler::LABEL> LabelMapType;
		typedef void (CCodeGen_Arm::*ConstCodeEmitterType)(const STATEMENT&);

		typedef std::tr1::function<void (CArmAssembler::REGISTER)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;

		enum MAX_PARAMS
		{
			MAX_PARAMS = 4,
		};
		
		enum
		{
			MAX_REGISTERS = 6,
		};

		enum
		{
			LITERAL_POOL_SIZE = 0x40,
		};
		
		struct LITERAL_POOL_REF
		{
			unsigned int			poolPtr;
			CArmAssembler::REGISTER	dstRegister;
			unsigned int			offset;
		};

		typedef std::list<LITERAL_POOL_REF> LiteralPoolRefList;

		struct CONSTMATCHER
		{
			OPERATION							op;
			MATCHTYPE							dstType;
			MATCHTYPE							src1Type;
			MATCHTYPE							src2Type;
			ConstCodeEmitterType				emitter;
		};

		CArmAssembler::LABEL					GetLabel(uint32);
		void									MarkLabel(const STATEMENT&);

		uint32									RotateRight(uint32);
		uint32									RotateLeft(uint32);
		bool									TryGetAluImmediateParams(uint32, uint8&, uint8&);
		void									LoadConstantInRegister(CArmAssembler::REGISTER, uint32);
		void									DumpLiteralPool();

		//ALUOP ----------------------------------------------------------
		struct ALUOP_BASE
		{
			typedef void (CArmAssembler::*OpImmType)(CArmAssembler::REGISTER, CArmAssembler::REGISTER, const CArmAssembler::ImmediateAluOperand&);
			typedef void (CArmAssembler::*OpRegType)(CArmAssembler::REGISTER, CArmAssembler::REGISTER, CArmAssembler::REGISTER);
		};
		
		struct ALUOP_ADD : public ALUOP_BASE
		{
			static OpImmType OpImm() { return &CArmAssembler::Add; }
			static OpRegType OpReg() { return &CArmAssembler::Add; }
		};
		
		struct ALUOP_SUB : public ALUOP_BASE
		{
			static OpImmType OpImm() { return &CArmAssembler::Sub; }
			static OpRegType OpReg() { return &CArmAssembler::Sub; }
		};
		
		struct ALUOP_AND : public ALUOP_BASE
		{
			static OpImmType OpImm() { return &CArmAssembler::And; }
			static OpRegType OpReg() { return &CArmAssembler::And; }
		};

		struct ALUOP_XOR : public ALUOP_BASE
		{
			static OpImmType OpImm() { return &CArmAssembler::Eor; }
			static OpRegType OpReg() { return &CArmAssembler::Eor; }
		};
		
		//FPUOP -----------------------------------------------------------
		struct FPUOP_BASE
		{
			typedef void (CArmAssembler::*OpRegType)(CArmAssembler::VFP_REGISTER, CArmAssembler::VFP_REGISTER, CArmAssembler::VFP_REGISTER);
		};

		struct FPUOP_ADD : public FPUOP_BASE
		{
			static OpRegType OpReg() { return &CArmAssembler::Fadds; }
		};

		//ALUOP
		template <typename> void				Emit_Alu_RegRegReg(const STATEMENT&);
		template <typename> void				Emit_Alu_RegRegCst(const STATEMENT&);
		template <typename> void				Emit_Alu_RegCstReg(const STATEMENT&);
		
		//SHIFTOP
		template <CArmAssembler::SHIFT> void	Emit_Shift_RegRegReg(const STATEMENT&);
		template <CArmAssembler::SHIFT> void	Emit_Shift_RegRegCst(const STATEMENT&);

		//PARAM
		void									Emit_Param_Ctx(const STATEMENT&);
		void									Emit_Param_Reg(const STATEMENT&);
		
		//CALL
		void									Emit_Call(const STATEMENT&);
		
		//RETVAL
		void									Emit_RetVal_Reg(const STATEMENT&);
		
		//MUL/MULS
		template<bool> void						Emit_MulTmp64RegReg(const STATEMENT&);

		//DIV/DIVS
		template<bool> void						Emit_DivTmp64RegCst(const STATEMENT&);

		//MOV
		void									Emit_Mov_RegReg(const STATEMENT&);
		void									Emit_Mov_RegRel(const STATEMENT&);
		void									Emit_Mov_RegCst(const STATEMENT&);
		void									Emit_Mov_RelReg(const STATEMENT&);

		//EXTLOW64
		void									Emit_ExtLow64RegTmp64(const STATEMENT&);

		//EXTHIGH64
		void									Emit_ExtHigh64RegTmp64(const STATEMENT&);

		//CMP
		void									Cmp_GetFlag(CArmAssembler::REGISTER, CONDITION);
		void									Emit_Cmp_RegRegReg(const STATEMENT&);
		void									Emit_Cmp_RegRegCst(const STATEMENT&);

		//JMP
		void									Emit_Jmp(const STATEMENT&);

		//CONDJMP
		void									Emit_CondJmp_RegCst(const STATEMENT&);
		
		//NOT
		void									Emit_Not_RegReg(const STATEMENT&);
		
		//FPUOP
		template <typename> void				Emit_Fpu_RelRel(const STATEMENT&);
		template <typename> void				Emit_Fpu_RelRelRel(const STATEMENT&);

		static CONSTMATCHER						g_constMatchers[];
		static CONSTMATCHER						g_fpuConstMatchers[];
		static CArmAssembler::REGISTER			g_registers[MAX_REGISTERS];
		static CArmAssembler::REGISTER			g_paramRegs[MAX_PARAMS];		
		static CArmAssembler::REGISTER			g_baseRegister;

		Framework::CStream*						m_stream;
		CArmAssembler							m_assembler;
		LabelMapType							m_labels;
		ParamStack								m_params;		
		uint32*									m_literalPool;
		unsigned int							m_lastLiteralPtr;
		LiteralPoolRefList						m_literalPoolRefs;
		uint32									m_stackLevel;
	};
};

#endif
