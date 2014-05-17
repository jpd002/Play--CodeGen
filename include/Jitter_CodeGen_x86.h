#pragma once

#include "Jitter_CodeGen.h"
#include "X86Assembler.h"

namespace Jitter
{
	class CCodeGen_x86 : public CCodeGen
	{
	public:
						CCodeGen_x86();
		virtual			~CCodeGen_x86();

		void			GenerateCode(const StatementList&, unsigned int) override;
		void			SetStream(Framework::CStream*) override;
		void			RegisterExternalSymbols(CObjectFile*) const override;

	protected:
		typedef std::map<uint32, CX86Assembler::LABEL> LabelMapType;
		typedef std::vector<std::pair<void*, CX86Assembler::LABEL>> SymbolReferenceLabelArray;

		//ALUOP ----------------------------------------------------------
		struct ALUOP_BASE
		{
			typedef void (CX86Assembler::*OpIdType)(const CX86Assembler::CAddress&, uint32);
			typedef void (CX86Assembler::*OpEdType)(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);
		};

		struct ALUOP_ADD : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::AddId; }
			static OpEdType OpEd() { return &CX86Assembler::AddEd; }
		};

		struct ALUOP_AND : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::AndId; }
			static OpEdType OpEd() { return &CX86Assembler::AndEd; }
		};

		struct ALUOP_SUB : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::SubId; }
			static OpEdType OpEd() { return &CX86Assembler::SubEd; }
		};

		struct ALUOP_OR : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::OrId; }
			static OpEdType OpEd() { return &CX86Assembler::OrEd; }
		};

		struct ALUOP_XOR : public ALUOP_BASE
		{
			static OpIdType OpId() { return &CX86Assembler::XorId; }
			static OpEdType OpEd() { return &CX86Assembler::XorEd; }
		};

		//SHIFTOP -----------------------------------------------------------
		struct SHIFTOP_BASE
		{
			typedef void (CX86Assembler::*OpCstType)(const CX86Assembler::CAddress&, uint8);
			typedef void (CX86Assembler::*OpVarType)(const CX86Assembler::CAddress&);
		};

		struct SHIFTOP_SRL : public SHIFTOP_BASE
		{
			static OpCstType OpCst() { return &CX86Assembler::ShrEd; }
			static OpVarType OpVar() { return &CX86Assembler::ShrEd; }
		};

		struct SHIFTOP_SRA : public SHIFTOP_BASE
		{
			static OpCstType OpCst() { return &CX86Assembler::SarEd; }
			static OpVarType OpVar() { return &CX86Assembler::SarEd; }
		};

		struct SHIFTOP_SLL : public SHIFTOP_BASE
		{
			static OpCstType OpCst() { return &CX86Assembler::ShlEd; }
			static OpVarType OpVar() { return &CX86Assembler::ShlEd; }
		};

		//FPUOP -----------------------------------------------------------
		struct FPUOP_BASE
		{
			typedef void (CX86Assembler::*OpEdType)(CX86Assembler::XMMREGISTER, const CX86Assembler::CAddress&);
		};

		struct FPUOP_ADD : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::AddssEd; }
		};

		struct FPUOP_SUB : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::SubssEd; }
		};

		struct FPUOP_MUL : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::MulssEd; }
		};

		struct FPUOP_DIV : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::DivssEd; }
		};
		
		struct FPUOP_MAX : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::MaxssEd; }
		};

		struct FPUOP_MIN : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::MinssEd; }
		};

		struct FPUOP_SQRT : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::SqrtssEd; }
		};

		struct FPUOP_RSQRT : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::RsqrtssEd; }
		};

		struct FPUOP_RCPL : public FPUOP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::RcpssEd; }
		};

		//MDOP -----------------------------------------------------------
		struct MDOP_BASE
		{
			typedef void (CX86Assembler::*OpVoType)(CX86Assembler::XMMREGISTER, const CX86Assembler::CAddress&);
		};

		struct MDOP_ADDB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PaddbVo; }
		};

		struct MDOP_ADDUSB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PaddusbVo; }
		};

		struct MDOP_ADDH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PaddwVo; }
		};

		struct MDOP_ADDW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PadddVo; }
		};

		struct MDOP_SUBB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsubbVo; }
		};

		struct MDOP_SUBW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsubdVo; }
		};

		struct MDOP_CMPEQB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpeqbVo; }
		};

		struct MDOP_CMPEQH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpeqwVo; }
		};

		struct MDOP_CMPEQW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpeqdVo; }
		};

		struct MDOP_CMPGTH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpgtwVo; }
		};

		struct MDOP_MINH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PminswVo; }
		};

		struct MDOP_MINW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PminsdVo; }
		};

		struct MDOP_MAXH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PmaxswVo; }
		};

		struct MDOP_MAXW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PmaxsdVo; }
		};

		struct MDOP_AND : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PandVo; }
		};

		struct MDOP_OR : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PorVo; }
		};

		struct MDOP_XOR : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PxorVo; }
		};

		struct MDOP_UNPACK_LOWER_BH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpcklbwVo; }
		};

		struct MDOP_UNPACK_LOWER_HW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpcklwdVo; }
		};

		struct MDOP_UNPACK_LOWER_WD : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpckldqVo; }
		};

		struct MDOP_UNPACK_UPPER_BH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpckhbwVo; }
		};

		struct MDOP_UNPACK_UPPER_WD : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpckhdqVo; }
		};

		struct MDOP_ADDS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::AddpsVo; }
		};

		struct MDOP_SUBS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::SubpsVo; }
		};

		struct MDOP_MULS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::MulpsVo; }
		};

		struct MDOP_DIVS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::DivpsVo; }
		};

		struct MDOP_MINS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::MinpsVo; }
		};

		struct MDOP_MAXS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::MaxpsVo; }
		};

		struct MDOP_TOWORD_TRUNCATE : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::Cvttps2dqVo; }
		};

		struct MDOP_TOSINGLE : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::Cvtdq2psVo; }
		};

		//MDOP SHIFT -----------------------------------------------------
		struct MDOP_SHIFT_BASE
		{
			typedef void (CX86Assembler::*OpVoType)(CX86Assembler::XMMREGISTER, uint8);
		};
		
		struct MDOP_SRLH : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsrlwVo; }
		};

		struct MDOP_SRAH : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsrawVo; }
		};

		struct MDOP_SLLH : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsllwVo; }
		};

		struct MDOP_SRLW : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsrldVo; }
		};

		struct MDOP_SRAW : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsradVo; }
		};

		struct MDOP_SLLW : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PslldVo; }
		};

		virtual void				Emit_Prolog(const StatementList&, unsigned int, uint32) = 0;
		virtual void				Emit_Epilog(unsigned int, uint32) = 0;

		CX86Assembler::LABEL		GetLabel(uint32);

		CX86Assembler::CAddress		MakeRelativeSymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeTemporarySymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeMemorySymbolAddress(CSymbol*);

		CX86Assembler::CAddress		MakeRelativeReferenceSymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeTemporaryReferenceSymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeMemoryReferenceSymbolAddress(CSymbol*);

		CX86Assembler::CAddress		MakeRelative64SymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeRelative64SymbolLoAddress(CSymbol*);
		CX86Assembler::CAddress		MakeRelative64SymbolHiAddress(CSymbol*);
		CX86Assembler::CAddress		MakeTemporary64SymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeTemporary64SymbolLoAddress(CSymbol*);
		CX86Assembler::CAddress		MakeTemporary64SymbolHiAddress(CSymbol*);
		CX86Assembler::CAddress		MakeMemory64SymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeMemory64SymbolLoAddress(CSymbol*);
		CX86Assembler::CAddress		MakeMemory64SymbolHiAddress(CSymbol*);

		CX86Assembler::CAddress		MakeRelativeFpSingleSymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeTemporaryFpSingleSymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeMemoryFpSingleSymbolAddress(CSymbol*);

		CX86Assembler::CAddress		MakeRelative128SymbolElementAddress(CSymbol*, unsigned int);
		CX86Assembler::CAddress		MakeTemporary128SymbolElementAddress(CSymbol*, unsigned int);

		CX86Assembler::CAddress		MakeMemory128SymbolAddress(CSymbol*);
		CX86Assembler::CAddress		MakeMemory128SymbolElementAddress(CSymbol*, unsigned int);

		CX86Assembler::CAddress		MakeTemporary256SymbolElementAddress(CSymbol*, unsigned int);

		//LABEL
		void						MarkLabel(const STATEMENT&);

		//NOP
		void						Emit_Nop(const STATEMENT&);

		//ALU
		template <typename> void	Emit_Alu_RegRegReg(const STATEMENT&);
		template <typename> void	Emit_Alu_RegRegMem(const STATEMENT&);
		template <typename> void	Emit_Alu_RegRegCst(const STATEMENT&);
		template <typename> void	Emit_Alu_RegMemReg(const STATEMENT&);
		template <typename> void	Emit_Alu_RegMemMem(const STATEMENT&);
		template <typename> void	Emit_Alu_RegMemCst(const STATEMENT&);
		template <typename> void	Emit_Alu_RegCstReg(const STATEMENT&);
		template <typename> void	Emit_Alu_RegCstMem(const STATEMENT&);

		template <typename> void	Emit_Alu_MemRegReg(const STATEMENT&);
		template <typename> void	Emit_Alu_MemRegMem(const STATEMENT&);
		template <typename> void	Emit_Alu_MemRegCst(const STATEMENT&);
		template <typename> void	Emit_Alu_MemMemReg(const STATEMENT&);
		template <typename> void	Emit_Alu_MemMemCst(const STATEMENT&);
		template <typename> void	Emit_Alu_MemMemMem(const STATEMENT&);
		template <typename> void	Emit_Alu_MemCstReg(const STATEMENT&);
		template <typename> void	Emit_Alu_MemCstMem(const STATEMENT&);

		//SHIFT
		template <typename> void	Emit_Shift_RegRegReg(const STATEMENT&);
		template <typename> void	Emit_Shift_RegRegMem(const STATEMENT&);
		template <typename> void	Emit_Shift_RegRegCst(const STATEMENT&);
		template <typename> void	Emit_Shift_RegMemReg(const STATEMENT&);
		template <typename> void	Emit_Shift_RegMemMem(const STATEMENT&);
		template <typename> void	Emit_Shift_RegMemCst(const STATEMENT&);
		template <typename> void	Emit_Shift_RegCstReg(const STATEMENT&);
		template <typename> void	Emit_Shift_RegCstMem(const STATEMENT&);

		template <typename> void	Emit_Shift_MemRegReg(const STATEMENT&);
		template <typename> void	Emit_Shift_MemRegMem(const STATEMENT&);
		template <typename> void	Emit_Shift_MemRegCst(const STATEMENT&);
		template <typename> void	Emit_Shift_MemMemReg(const STATEMENT&);
		template <typename> void	Emit_Shift_MemMemMem(const STATEMENT&);
		template <typename> void	Emit_Shift_MemMemCst(const STATEMENT&);
		template <typename> void	Emit_Shift_MemCstReg(const STATEMENT&);
		template <typename> void	Emit_Shift_MemCstMem(const STATEMENT&);

		//NOT
		void						Emit_Not_RegReg(const STATEMENT&);
		void						Emit_Not_RegRel(const STATEMENT&);
		void						Emit_Not_RegTmp(const STATEMENT&);
		void						Emit_Not_RelReg(const STATEMENT&);
		void						Emit_Not_RelTmp(const STATEMENT&);

		//LZC
		void						Lzc_RegMem(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);
		void						Emit_Lzc_RegMem(const STATEMENT&);
		void						Emit_Lzc_MemMem(const STATEMENT&);

		//CMP
		void						Cmp_GetFlag(const CX86Assembler::CAddress&, CONDITION);
		void						Emit_Cmp_RegRegReg(const STATEMENT&);
		void						Emit_Cmp_RegRegMem(const STATEMENT&);
		void						Emit_Cmp_RegRegCst(const STATEMENT&);
		void						Emit_Cmp_RegRelRel(const STATEMENT&);
		void						Emit_Cmp_RegRelCst(const STATEMENT&);
		void						Emit_Cmp_RelRegReg(const STATEMENT&);
		void						Emit_Cmp_RelRegRel(const STATEMENT&);
		void						Emit_Cmp_RelRegCst(const STATEMENT&);
		void						Emit_Cmp_RelRelRel(const STATEMENT&);
		void						Emit_Cmp_RelRelCst(const STATEMENT&);

		//MUL/MULS
		template<bool> void			Emit_MulTmp64RegRel(const STATEMENT&);
		template<bool> void			Emit_MulTmp64RegCst(const STATEMENT&);
		template<bool> void			Emit_MulTmp64RegReg(const STATEMENT&);
		template<bool> void			Emit_MulTmp64RelRel(const STATEMENT&);
		template<bool> void			Emit_MulTmp64RelCst(const STATEMENT&);

		//DIV/DIVS
		template <bool> void		Emit_DivTmp64RegReg(const STATEMENT&);
		template <bool> void		Emit_DivTmp64RegMem(const STATEMENT&);
		template <bool> void		Emit_DivTmp64RegCst(const STATEMENT&);
		template <bool> void		Emit_DivTmp64RelReg(const STATEMENT&);
		template <bool> void		Emit_DivTmp64MemMem(const STATEMENT&);
		template <bool> void		Emit_DivTmp64MemCst(const STATEMENT&);
		template <bool> void		Emit_DivTmp64CstReg(const STATEMENT&);
		template <bool> void		Emit_DivTmp64CstMem(const STATEMENT&);

		//MOV
		void						Emit_Mov_RegRel(const STATEMENT&);
		void						Emit_Mov_RegReg(const STATEMENT&);
		void						Emit_Mov_RegCst(const STATEMENT&);
		void						Emit_Mov_RegTmp(const STATEMENT&);
		void						Emit_Mov_RelReg(const STATEMENT&);
		void						Emit_Mov_RelRel(const STATEMENT&);
		void						Emit_Mov_RelCst(const STATEMENT&);
		void						Emit_Mov_RelTmp(const STATEMENT&);
		void						Emit_Mov_TmpReg(const STATEMENT&);
		void						Emit_Mov_TmpRel(const STATEMENT&);

		//JMP
		void						Emit_Jmp(const STATEMENT&);

		//CONDJMP
		void						CondJmp_JumpTo(CX86Assembler::LABEL, Jitter::CONDITION);
		void						Emit_CondJmp_RegReg(const STATEMENT&);
		void						Emit_CondJmp_RegMem(const STATEMENT&);
		void						Emit_CondJmp_RegCst(const STATEMENT&);
		void						Emit_CondJmp_RelRel(const STATEMENT&);
		void						Emit_CondJmp_MemCst(const STATEMENT&);

		//MULSH
		void						Emit_MulSHL(const CX86Assembler::CAddress&, const CX86Assembler::CAddress&, const CX86Assembler::CAddress&);
		void						Emit_MulSHH(const CX86Assembler::CAddress&, const CX86Assembler::CAddress&, const CX86Assembler::CAddress&);
		
		struct MULTSHOP_BASE
		{
			typedef void (CCodeGen_x86::*OpType)(const CX86Assembler::CAddress&, const CX86Assembler::CAddress&, const CX86Assembler::CAddress&);
		};

		struct MULTSHOP_LOW : public MULTSHOP_BASE
		{
			static OpType Op() { return &CCodeGen_x86::Emit_MulSHL; }
		};

		struct MULTSHOP_HIGH : public MULTSHOP_BASE
		{
			static OpType Op() { return &CCodeGen_x86::Emit_MulSHH; }
		};

		template <typename>	void	Emit_MulSH_RegRegReg(const STATEMENT&);
		template <typename>	void	Emit_MulSH_RegRegMem(const STATEMENT&);
		template <typename>	void	Emit_MulSH_RegMemMem(const STATEMENT&);
		template <typename>	void	Emit_MulSH_MemRegMem(const STATEMENT&);
		template <typename>	void	Emit_MulSH_MemRegReg(const STATEMENT&);
		template <typename>	void	Emit_MulSH_MemMemReg(const STATEMENT&);
		template <typename> void	Emit_MulSH_MemMemMem(const STATEMENT&);

		//MERGETO64
		void						Emit_MergeTo64_Tmp64RegReg(const STATEMENT&);
		void						Emit_MergeTo64_Tmp64RegMem(const STATEMENT&);
		void						Emit_MergeTo64_Tmp64MemMem(const STATEMENT&);
		void						Emit_MergeTo64_Tmp64CstReg(const STATEMENT&);
		void						Emit_MergeTo64_Tmp64CstMem(const STATEMENT&);

		//EXTLOW64
		void						Emit_ExtLow64RegTmp64(const STATEMENT&);
		void						Emit_ExtLow64RelTmp64(const STATEMENT&);
		void						Emit_ExtLow64TmpTmp64(const STATEMENT&);

		//EXTHIGH64
		void						Emit_ExtHigh64RegTmp64(const STATEMENT&);
		void						Emit_ExtHigh64RelTmp64(const STATEMENT&);
		void						Emit_ExtHigh64TmpTmp64(const STATEMENT&);

		//FPUOP
		template <typename> void	Emit_Fpu_MemMem(const STATEMENT&);
		template <typename> void	Emit_Fpu_MemMemMem(const STATEMENT&);

		//FPCMP
		CX86Assembler::SSE_CMP_TYPE	GetSseConditionCode(Jitter::CONDITION);
		void						Emit_Fp_Cmp_MemMem(CX86Assembler::REGISTER, const STATEMENT&);
		void						Emit_Fp_Cmp_MemCst(CX86Assembler::REGISTER, const STATEMENT&);

		void						Emit_Fp_Cmp_SymMemMem(const STATEMENT&);
		void						Emit_Fp_Cmp_SymMemCst(const STATEMENT&);

		//FPABS
		void						Emit_Fp_Abs_MemMem(const STATEMENT&);

		//FPNEG
		void						Emit_Fp_Neg_MemMem(const STATEMENT&);

		//FP_MOV
		void						Emit_Fp_Mov_RelSRelI32(const STATEMENT&);

		//FP_TOINT_TRUNC
		void						Emit_Fp_ToIntTrunc_RelRel(const STATEMENT&);

		//FP_LDCST
		void						Emit_Fp_LdCst_MemCst(const STATEMENT&);

		//MDOP
		template <typename> void	Emit_Md_MemMem(const STATEMENT&);
		template <typename> void	Emit_Md_MemMemMem(const STATEMENT&);
		template <typename> void	Emit_Md_MemMemMemRev(const STATEMENT&);
		template <typename> void	Emit_Md_Shift_MemMemCst(const STATEMENT&);
		void						Emit_Md_AddSSW_MemMemMem(const STATEMENT&);
		void						Emit_Md_AddUSW_MemMemMem(const STATEMENT&);
		void						Emit_Md_PackHB_MemMemMem(const STATEMENT&);
		void						Emit_Md_PackWH_MemMemMem(const STATEMENT&);
		void						Emit_Md_Not_MemMem(const STATEMENT&);
		void						Emit_Md_Mov_MemMem(const STATEMENT&);
		void						Emit_Md_MovMasked_MemMemCst(const STATEMENT&);
		void						Emit_Md_Abs_MemMem(const STATEMENT&);
		void						Emit_Md_IsNegative_RegMem(const STATEMENT&);
		void						Emit_Md_IsNegative_MemMem(const STATEMENT&);
		void						Emit_Md_IsZero_RegMem(const STATEMENT&);
		void						Emit_Md_IsZero_MemMem(const STATEMENT&);
		void						Emit_Md_Expand_MemReg(const STATEMENT&);
		void						Emit_Md_Expand_MemMem(const STATEMENT&);
		void						Emit_Md_Expand_MemCst(const STATEMENT&);

		void						Emit_MergeTo256_MemMemMem(const STATEMENT&);

		void						Emit_Md_Srl256_MemMem(CSymbol*, CSymbol*, const CX86Assembler::CAddress&);
		void						Emit_Md_Srl256_MemMemReg(const STATEMENT&);
		void						Emit_Md_Srl256_MemMemMem(const STATEMENT&);
		void						Emit_Md_Srl256_MemMemCst(const STATEMENT&);

		void						Emit_Md_IsZero(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);
		void						Emit_Md_IsNegative(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);

		CX86Assembler				m_assembler;
		CX86Assembler::REGISTER*	m_registers;
		LabelMapType				m_labels;
		SymbolReferenceLabelArray	m_symbolReferenceLabels;
		uint32						m_stackLevel;
		
	private:
		typedef void (CCodeGen_x86::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION				op;
			MATCHTYPE				dstType;
			MATCHTYPE				src1Type;
			MATCHTYPE				src2Type;
			ConstCodeEmitterType	emitter;
		};

		static CONSTMATCHER			g_constMatchers[];
		static CONSTMATCHER			g_fpuConstMatchers[];
		static CONSTMATCHER			g_mdConstMatchers[];
	};
}
