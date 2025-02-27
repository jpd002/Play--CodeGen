#pragma once

#include <array>
#include "Jitter_CodeGen.h"
#include "X86Assembler.h"
#include "X86CpuFeatures.h"
#include "Literal128.h"

namespace Jitter
{
	class CCodeGen_x86 : public CCodeGen
	{
	public:
		CCodeGen_x86(CX86CpuFeatures);
		virtual ~CCodeGen_x86() = default;

		void GenerateCode(const StatementList&, unsigned int) override;
		void SetStream(Framework::CStream*) override;
		void RegisterExternalSymbols(CObjectFile*) const override;
		bool Has128BitsCallOperands() const override;
		bool SupportsExternalJumps() const override;
		bool SupportsCmpSelect() const override;

	protected:
		typedef std::map<uint32, CX86Assembler::LABEL> LabelMapType;
		typedef std::vector<std::pair<uintptr_t, CX86Assembler::LABEL>> SymbolReferenceLabelArray;

		// clang-format off
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

		//FP32OP -----------------------------------------------------------
		struct FP32OP_BASE
		{
			typedef void (CX86Assembler::*OpEdType)(CX86Assembler::XMMREGISTER, const CX86Assembler::CAddress&);
			typedef void (CX86Assembler::*OpEdAvxType)(CX86Assembler::XMMREGISTER, CX86Assembler::XMMREGISTER, const CX86Assembler::CAddress&);
		};

		struct FP32OP_ADD : public FP32OP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::AddssEd; }
			static OpEdAvxType OpEdAvx() { return &CX86Assembler::VaddssEd; }
		};

		struct FP32OP_SUB : public FP32OP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::SubssEd; }
			static OpEdAvxType OpEdAvx() { return &CX86Assembler::VsubssEd; }
		};

		struct FP32OP_MUL : public FP32OP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::MulssEd; }
			static OpEdAvxType OpEdAvx() { return &CX86Assembler::VmulssEd; }
		};

		struct FP32OP_DIV : public FP32OP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::DivssEd; }
			static OpEdAvxType OpEdAvx() { return &CX86Assembler::VdivssEd; }
		};
		
		struct FP32OP_MAX : public FP32OP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::MaxssEd; }
			static OpEdAvxType OpEdAvx() { return &CX86Assembler::VmaxssEd; }
		};

		struct FP32OP_MIN : public FP32OP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::MinssEd; }
			static OpEdAvxType OpEdAvx() { return &CX86Assembler::VminssEd; }
		};

		struct FP32OP_SQRT : public FP32OP_BASE
		{
			static OpEdType OpEd() { return &CX86Assembler::SqrtssEd; }
			static OpEdAvxType OpEdAvx() { return &CX86Assembler::VsqrtssEd; }
		};

		//MDOP -----------------------------------------------------------
		struct MDOP_BASE
		{
			typedef void (CX86Assembler::*OpVoType)(CX86Assembler::XMMREGISTER, const CX86Assembler::CAddress&);
			typedef void (CX86Assembler::*OpVoAvxType)(CX86Assembler::XMMREGISTER, CX86Assembler::XMMREGISTER, const CX86Assembler::CAddress&);
		};

		struct MDOP_ADDB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PaddbVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpaddbVo; }
		};

		struct MDOP_ADDH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PaddwVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpaddwVo; }
		};

		struct MDOP_ADDW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PadddVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpadddVo; }
		};

		struct MDOP_ADDSSB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PaddsbVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpaddsbVo; }
		};

		struct MDOP_ADDSSH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PaddswVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpaddswVo; }
		};

		struct MDOP_ADDUSB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PaddusbVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpaddusbVo; }
		};

		struct MDOP_ADDUSH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PadduswVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpadduswVo; }
		};

		struct MDOP_SUBB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsubbVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsubbVo; }
		};

		struct MDOP_SUBH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsubwVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsubwVo; }
		};

		struct MDOP_SUBW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsubdVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsubdVo; }
		};

		struct MDOP_SUBSSH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsubswVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsubswVo; }
		};

		struct MDOP_SUBUSB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsubusbVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsubusbVo; }
		};

		struct MDOP_SUBUSH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsubuswVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsubuswVo; }
		};

		struct MDOP_CMPEQB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpeqbVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpcmpeqbVo; }
		};

		struct MDOP_CMPEQH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpeqwVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpcmpeqwVo; }
		};

		struct MDOP_CMPEQW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpeqdVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpcmpeqdVo; }
		};

		struct MDOP_CMPGTB : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpgtbVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpcmpgtbVo; }
		};

		struct MDOP_CMPGTH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpgtwVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpcmpgtwVo; }
		};

		struct MDOP_CMPGTW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PcmpgtdVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpcmpgtdVo; }
		};

		struct MDOP_MINH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PminswVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpminswVo; }
		};

		struct MDOP_MINW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PminsdVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpminsdVo; }
		};

		struct MDOP_MAXH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PmaxswVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpmaxswVo; }
		};

		struct MDOP_MAXW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PmaxsdVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpmaxsdVo; }
		};

		struct MDOP_AND : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PandVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpandVo; }
		};

		struct MDOP_OR : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PorVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VporVo; }
		};

		struct MDOP_XOR : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PxorVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpxorVo; }
		};

		struct MDOP_UNPACK_LOWER_BH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpcklbwVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpunpcklbwVo; }
		};

		struct MDOP_UNPACK_LOWER_HW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpcklwdVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpunpcklwdVo; }
		};

		struct MDOP_UNPACK_LOWER_WD : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpckldqVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpunpckldqVo; }
		};

		struct MDOP_UNPACK_UPPER_BH : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpckhbwVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpunpckhbwVo; }
		};

		struct MDOP_UNPACK_UPPER_HW : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpckhwdVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpunpckhwdVo; }
		};

		struct MDOP_UNPACK_UPPER_WD : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PunpckhdqVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpunpckhdqVo; }
		};

		struct MDOP_ADDS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::AddpsVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VaddpsVo; }
		};

		struct MDOP_SUBS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::SubpsVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VsubpsVo; }
		};

		struct MDOP_MULS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::MulpsVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VmulpsVo; }
		};

		struct MDOP_DIVS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::DivpsVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VdivpsVo; }
		};

		struct MDOP_CMPLTS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::CmpltpsVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VcmpltpsVo; }
		};

		struct MDOP_CMPGTS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::CmpgtpsVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VcmpgtpsVo; }
		};

		struct MDOP_MINS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::MinpsVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VminpsVo; }
		};

		struct MDOP_MAXS : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::MaxpsVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VmaxpsVo; }
		};

		struct MDOP_TOWORD_TRUNCATE : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::Cvttps2dqVo; }
			static OpVoType OpVoAvx() { return &CX86Assembler::Vcvttps2dqVo; }
		};

		struct MDOP_TOSINGLE : public MDOP_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::Cvtdq2psVo; }
			static OpVoType OpVoAvx() { return &CX86Assembler::Vcvtdq2psVo; }
		};

		//MDOP SHIFT -----------------------------------------------------
		struct MDOP_SHIFT_BASE
		{
			typedef void (CX86Assembler::*OpVoType)(CX86Assembler::XMMREGISTER, uint8);
			typedef void (CX86Assembler::*OpVoAvxType)(CX86Assembler::XMMREGISTER, CX86Assembler::XMMREGISTER, uint8);
		};
		
		struct MDOP_SRLH : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsrlwVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsrlwVo; }
		};

		struct MDOP_SRAH : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsrawVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsrawVo; }
		};

		struct MDOP_SLLH : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsllwVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsllwVo; }
		};

		struct MDOP_SRLW : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsrldVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsrldVo; }
		};

		struct MDOP_SRAW : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PsradVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpsradVo; }
		};

		struct MDOP_SLLW : public MDOP_SHIFT_BASE
		{
			static OpVoType OpVo() { return &CX86Assembler::PslldVo; }
			static OpVoAvxType OpVoAvx() { return &CX86Assembler::VpslldVo; }
		};

		//MDOP SINGLEOP -------------------------------------------------
		struct MDOP_SINGLEOP_BASE
		{
			typedef void (CCodeGen_x86::*OpVrType)(CX86Assembler::XMMREGISTER);
		};

		struct MDOP_ABS : public MDOP_SINGLEOP_BASE
		{
			static OpVrType OpVr() { return &CCodeGen_x86::Emit_Md_Abs; }
		};

		struct MDOP_NEG : public MDOP_SINGLEOP_BASE
		{
			static OpVrType OpVr() { return &CCodeGen_x86::Emit_Md_Neg; }
		};

		struct MDOP_NOT : public MDOP_SINGLEOP_BASE
		{
			static OpVrType OpVr() { return &CCodeGen_x86::Emit_Md_Not; }
		};

		//MDOP FLAG -----------------------------------------------------
		struct MDOP_FLAG_BASE
		{
			typedef void (CCodeGen_x86::*OpEdType)(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);
		};
		// clang-format on

		virtual void Emit_Prolog(const StatementList&, unsigned int) = 0;
		virtual void Emit_Epilog() = 0;

		virtual CX86Assembler::CAddress MakeConstant128Address(const LITERAL128&) = 0;

		CX86Assembler::LABEL GetLabel(uint32);

		CX86Assembler::CAddress MakeRelativeSymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeTemporarySymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeMemorySymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeVariableSymbolAddress(CSymbol*);

		CX86Assembler::CAddress MakeRelativeReferenceSymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeTemporaryReferenceSymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeMemoryReferenceSymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeVariableReferenceSymbolAddress(CSymbol*);

		CX86Assembler::CAddress MakeRefBaseScaleSymbolAddress(CSymbol*, CX86Assembler::REGISTER, CSymbol*, CX86Assembler::REGISTER, uint8);

		CX86Assembler::CAddress MakeRelative64SymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeRelative64SymbolLoAddress(CSymbol*);
		CX86Assembler::CAddress MakeRelative64SymbolHiAddress(CSymbol*);
		CX86Assembler::CAddress MakeTemporary64SymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeTemporary64SymbolLoAddress(CSymbol*);
		CX86Assembler::CAddress MakeTemporary64SymbolHiAddress(CSymbol*);
		CX86Assembler::CAddress MakeMemory64SymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeMemory64SymbolLoAddress(CSymbol*);
		CX86Assembler::CAddress MakeMemory64SymbolHiAddress(CSymbol*);

		CX86Assembler::CAddress MakeRelativeFp32SymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeTemporaryFp32SymbolAddress(CSymbol*);

		CX86Assembler::CAddress MakeVariableFp32SymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeMemoryFp32SymbolAddress(CSymbol*);

		CX86Assembler::CAddress MakeRelative128SymbolElementAddress(CSymbol*, unsigned int);
		CX86Assembler::CAddress MakeTemporary128SymbolElementAddress(CSymbol*, unsigned int);

		CX86Assembler::CAddress MakeVariable128SymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeMemory128SymbolAddress(CSymbol*);
		CX86Assembler::CAddress MakeMemory128SymbolElementAddress(CSymbol*, unsigned int);

		CX86Assembler::CAddress MakeTemporary256SymbolElementAddress(CSymbol*, unsigned int);

		void MdSseBlendVariables(const CX86Assembler::CAddress&, const CX86Assembler::CAddress&, const CX86Assembler::CAddress&, uint8);

		//LABEL
		void MarkLabel(const STATEMENT&);

		//NOP
		void Emit_Nop(const STATEMENT&);

		//BREAK
		void Emit_Break(const STATEMENT&);

		//ALU
		template <typename>
		void Emit_Alu_RegRegReg(const STATEMENT&);
		template <typename>
		void Emit_Alu_RegRegMem(const STATEMENT&);
		template <typename>
		void Emit_Alu_RegRegCst(const STATEMENT&);
		template <typename>
		void Emit_Alu_RegMemReg(const STATEMENT&);
		template <typename>
		void Emit_Alu_RegMemMem(const STATEMENT&);
		template <typename>
		void Emit_Alu_RegMemCst(const STATEMENT&);
		template <typename>
		void Emit_Alu_RegCstReg(const STATEMENT&);
		template <typename>
		void Emit_Alu_RegCstMem(const STATEMENT&);

		template <typename>
		void Emit_Alu_MemRegReg(const STATEMENT&);
		template <typename>
		void Emit_Alu_MemRegMem(const STATEMENT&);
		template <typename>
		void Emit_Alu_MemRegCst(const STATEMENT&);
		template <typename>
		void Emit_Alu_MemMemReg(const STATEMENT&);
		template <typename>
		void Emit_Alu_MemMemCst(const STATEMENT&);
		template <typename>
		void Emit_Alu_MemMemMem(const STATEMENT&);
		template <typename>
		void Emit_Alu_MemCstReg(const STATEMENT&);
		template <typename>
		void Emit_Alu_MemCstMem(const STATEMENT&);

		//SHIFT
		template <typename>
		void Emit_Shift_RegRegReg(const STATEMENT&);
		template <typename>
		void Emit_Shift_RegRegMem(const STATEMENT&);
		template <typename>
		void Emit_Shift_RegRegCst(const STATEMENT&);
		template <typename>
		void Emit_Shift_RegMemReg(const STATEMENT&);
		template <typename>
		void Emit_Shift_RegMemMem(const STATEMENT&);
		template <typename>
		void Emit_Shift_RegMemCst(const STATEMENT&);
		template <typename>
		void Emit_Shift_RegCstReg(const STATEMENT&);
		template <typename>
		void Emit_Shift_RegCstMem(const STATEMENT&);

		template <typename>
		void Emit_Shift_MemRegReg(const STATEMENT&);
		template <typename>
		void Emit_Shift_MemRegMem(const STATEMENT&);
		template <typename>
		void Emit_Shift_MemRegCst(const STATEMENT&);
		template <typename>
		void Emit_Shift_MemMemReg(const STATEMENT&);
		template <typename>
		void Emit_Shift_MemMemMem(const STATEMENT&);
		template <typename>
		void Emit_Shift_MemMemCst(const STATEMENT&);
		template <typename>
		void Emit_Shift_MemCstReg(const STATEMENT&);
		template <typename>
		void Emit_Shift_MemCstMem(const STATEMENT&);

		//NOT
		void Emit_Not_RegReg(const STATEMENT&);
		void Emit_Not_RegMem(const STATEMENT&);
		void Emit_Not_MemReg(const STATEMENT&);
		void Emit_Not_MemMem(const STATEMENT&);

		//LZC
		void Emit_Lzc(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);
		void Emit_Lzc_RegVar(const STATEMENT&);
		void Emit_Lzc_MemVar(const STATEMENT&);

		//CMP
		void Cmp_GetFlag(const CX86Assembler::CAddress&, CONDITION);

		//MUL/MULS
		template <bool>
		void Emit_MulMem64VarVar(const STATEMENT&);
		template <bool>
		void Emit_MulMem64VarCst(const STATEMENT&);

		//DIV/DIVS
		template <bool>
		void Emit_DivMem64VarVar(const STATEMENT&);
		template <bool>
		void Emit_DivMem64VarCst(const STATEMENT&);
		template <bool>
		void Emit_DivMem64CstVar(const STATEMENT&);

		//MOV
		void Emit_Mov_RegReg(const STATEMENT&);
		void Emit_Mov_RegMem(const STATEMENT&);
		void Emit_Mov_RegCst(const STATEMENT&);
		void Emit_Mov_MemReg(const STATEMENT&);
		void Emit_Mov_MemMem(const STATEMENT&);
		void Emit_Mov_MemCst(const STATEMENT&);

		//JMP
		void Emit_Jmp(const STATEMENT&);

		//CONDJMP
		void CondJmp_JumpTo(CX86Assembler::LABEL, Jitter::CONDITION);
		void Emit_CondJmp_RegReg(const STATEMENT&);
		void Emit_CondJmp_RegMem(const STATEMENT&);
		void Emit_CondJmp_RegCst(const STATEMENT&);
		void Emit_CondJmp_MemMem(const STATEMENT&);
		void Emit_CondJmp_MemCst(const STATEMENT&);

		//SELECT
		void Select_Generic(Jitter::CONDITION, CSymbol*, CSymbol*, CSymbol*);
		void Emit_Select_VarVarAnyAny(const STATEMENT&);

		//CMPSELECT
		void Emit_CmpSelectP1_AnyVar(const STATEMENT&);
		void Emit_CmpSelectP2_VarAnyAny(const STATEMENT&);

		//MERGETO64
		void Emit_MergeTo64_Mem64RegReg(const STATEMENT&);
		void Emit_MergeTo64_Mem64RegMem(const STATEMENT&);
		void Emit_MergeTo64_Mem64RegCst(const STATEMENT&);
		void Emit_MergeTo64_Mem64MemReg(const STATEMENT&);
		void Emit_MergeTo64_Mem64MemMem(const STATEMENT&);
		void Emit_MergeTo64_Mem64CstReg(const STATEMENT&);
		void Emit_MergeTo64_Mem64CstMem(const STATEMENT&);

		//EXTLOW64
		void Emit_ExtLow64VarMem64(const STATEMENT&);

		//EXTHIGH64
		void Emit_ExtHigh64VarMem64(const STATEMENT&);

		//LOADFROMREF
		void Emit_LoadFromRef_VarVar(const STATEMENT&);
		void Emit_LoadFromRef_VarVarAny(const STATEMENT&);

		//LOAD8FROMREF
		void Emit_Load8FromRef_VarVar(const STATEMENT&);
		void Emit_Load8FromRef_VarVarAny(const STATEMENT&);

		//LOAD16FROMREF
		void Emit_Load16FromRef_VarVar(const STATEMENT&);
		void Emit_Load16FromRef_VarVarAny(const STATEMENT&);

		//STOREATREF
		void Emit_StoreAtRef_VarVar(const STATEMENT&);
		void Emit_StoreAtRef_VarCst(const STATEMENT&);
		void Emit_StoreAtRef_VarAnyVar(const STATEMENT&);
		void Emit_StoreAtRef_VarAnyCst(const STATEMENT&);

		//STORE8ATREF
		void Emit_Store8AtRef_VarCst(const STATEMENT&);
		void Emit_Store8AtRef_VarAnyCst(const STATEMENT&);

		//STORE16ATREF
		void Emit_Store16AtRef_VarVar(const STATEMENT&);
		void Emit_Store16AtRef_VarCst(const STATEMENT&);
		void Emit_Store16AtRef_VarAnyVar(const STATEMENT&);
		void Emit_Store16AtRef_VarAnyCst(const STATEMENT&);

		//FPUOP Generic
		CX86Assembler::SSE_CMP_TYPE GetSseConditionCode(Jitter::CONDITION);

		void Emit_Fp_AbsS_MemMem(const STATEMENT&);
		void Emit_Fp_NegS_MemMem(const STATEMENT&);
		void Emit_Fp32_LdCst_MemCst(const STATEMENT&);

		//FPUOP SSE
		template <typename>
		void Emit_Fp32_RegVar(const STATEMENT&);
		template <typename>
		void Emit_Fp32_MemVar(const STATEMENT&);
		template <typename>
		void Emit_Fp32_RegRegReg(const STATEMENT&);
		template <typename>
		void Emit_Fp32_RegMemReg(const STATEMENT&);
		template <typename>
		void Emit_Fp32_RegVarVar(const STATEMENT&);
		template <typename>
		void Emit_Fp32_MemVarVar(const STATEMENT&);
		template <typename>
		void Emit_Fp32_SingleOp_RegVar(const STATEMENT&);
		template <typename>
		void Emit_Fp32_SingleOp_MemVar(const STATEMENT&);

		void Emit_Fp32_Mov_RegMem(const STATEMENT&);
		void Emit_Fp32_Mov_MemReg(const STATEMENT&);
		void Emit_Fp32_LdCst_RegCst(const STATEMENT&);
		void Emit_Fp_Sse_SetRoundingMode_Cst(const STATEMENT&);

		void Emit_Fp_CmpS_VarVarVar(const STATEMENT&);
		void Emit_Fp_CmpS_VarMemCst(const STATEMENT&);
		void Emit_Fp_RsqrtS_VarVar(const STATEMENT&);
		void Emit_Fp_RcplS_RegVar(const STATEMENT&);
		void Emit_Fp_RcplS_MemVar(const STATEMENT&);
		void Emit_Fp_ClampS_RegVar(const STATEMENT&);
		void Emit_Fp_ClampS_MemVar(const STATEMENT&);
		void Emit_Fp_ToSingleI32_VarReg(const STATEMENT&);
		void Emit_Fp_ToSingleI32_VarMem(const STATEMENT&);
		void Emit_Fp_ToInt32TruncS_RegVar(const STATEMENT&);
		void Emit_Fp_ToInt32TruncS_MemVar(const STATEMENT&);

		//MDOP
		template <typename>
		void Emit_Md_RegVar(const STATEMENT&);
		template <typename>
		void Emit_Md_MemVar(const STATEMENT&);
		template <typename>
		void Emit_Md_RegRegReg(const STATEMENT&);
		template <typename>
		void Emit_Md_RegMemReg(const STATEMENT&);
		template <typename>
		void Emit_Md_RegVarVar(const STATEMENT&);
		template <typename>
		void Emit_Md_MemVarVar(const STATEMENT&);
		template <typename>
		void Emit_Md_VarVarVarRev(const STATEMENT&);
		template <typename, uint8>
		void
		Emit_Md_Shift_RegVarCst(const STATEMENT&);
		template <typename, uint8>
		void
		Emit_Md_Shift_MemVarCst(const STATEMENT&);
		template <typename>
		void Emit_Md_SingleOp_RegVar(const STATEMENT&);
		template <typename>
		void Emit_Md_SingleOp_MemVar(const STATEMENT&);
		void Emit_Md_AddSSW_VarVarVar(const STATEMENT&);
		void Emit_Md_AddUSW_VarVarVar(const STATEMENT&);
		void Emit_Md_SubSSW_VarVarVar(const STATEMENT&);
		void Emit_Md_SubUSW_VarVarVar(const STATEMENT&);
		void Emit_Md_MinW_VarVarVar(const STATEMENT&);
		void Emit_Md_MaxW_VarVarVar(const STATEMENT&);
		void Emit_Md_ClampS_RegVar(const STATEMENT&);
		void Emit_Md_ClampS_MemVar(const STATEMENT&);
		void Emit_Md_PackHB_VarVarVar(const STATEMENT&);
		void Emit_Md_PackWH_VarVarVar(const STATEMENT&);
		void Emit_Md_Mov_RegVar(const STATEMENT&);
		void Emit_Md_Mov_MemReg(const STATEMENT&);
		void Emit_Md_Mov_MemMem(const STATEMENT&);
		void Emit_Md_LoadFromRefMasked_VarVarAnyVar(const STATEMENT&);
		void Emit_Md_StoreAtRefMasked_VarAnyVar(const STATEMENT&);
		void Emit_Md_LoadFromRefMasked_Sse41_VarVarAnyVar(const STATEMENT&);
		void Emit_Md_StoreAtRefMasked_Sse41_VarAnyVar(const STATEMENT&);
		void Emit_Md_MovMasked_VarVarVar(const STATEMENT&);
		void Emit_Md_MovMasked_Sse41_VarVarVar(const STATEMENT&);
		void Emit_Md_Expand_VarReg(const STATEMENT&);
		void Emit_Md_Expand_VarMem(const STATEMENT&);
		void Emit_Md_Expand_VarCst(const STATEMENT&);
		void Emit_Md_Expand_VarVarCst(const STATEMENT&);

		void Emit_MergeTo256_MemVarVar(const STATEMENT&);

		void Emit_Md_Srl256_VarMem(CSymbol*, CSymbol*, const CX86Assembler::CAddress&);
		void Emit_Md_Srl256_VarMemVar(const STATEMENT&);
		void Emit_Md_Srl256_VarMemCst(const STATEMENT&);

		void Emit_Md_Abs(CX86Assembler::XMMREGISTER);
		void Emit_Md_Neg(CX86Assembler::XMMREGISTER);
		void Emit_Md_Not(CX86Assembler::XMMREGISTER);
		void Emit_Md_MakeSz(CX86Assembler::XMMREGISTER, const CX86Assembler::CAddress&);
		void Emit_Md_MakeSz_VarVar(const STATEMENT&);
		void Emit_Md_MakeSz_Ssse3_VarVar(const STATEMENT&);

		void Emit_Md_LoadFromRef_VarVar(const STATEMENT&);
		void Emit_Md_LoadFromRef_VarVarAny(const STATEMENT&);

		void Emit_Md_StoreAtRef_VarVar(const STATEMENT&);
		void Emit_Md_StoreAtRef_VarAnyVar(const STATEMENT&);

		//FPUOP AVX
		template <typename>
		void Emit_Fp32_Avx_VarVar(const STATEMENT&);
		template <typename>
		void Emit_Fp32_Avx_VarVarVar(const STATEMENT&);

		void Emit_Fp32_Avx_Mov_RegMem(const STATEMENT&);
		void Emit_Fp32_Avx_Mov_MemReg(const STATEMENT&);
		void Emit_Fp32_Avx_LdCst_RegCst(const STATEMENT&);
		void Emit_Fp_Avx_SetRoundingMode_Cst(const STATEMENT&);

		void Emit_Fp_Avx_CmpS_VarVarVar(const STATEMENT&);
		void Emit_Fp_Avx_RsqrtS_VarVar(const STATEMENT&);
		void Emit_Fp_Avx_RcplS_VarVar(const STATEMENT&);
		void Emit_Fp_Avx_AbsS_RegMem(const STATEMENT&);
		void Emit_Fp_Avx_AbsS_VarReg(const STATEMENT&);
		void Emit_Fp_Avx_NegS_RegMem(const STATEMENT&);
		void Emit_Fp_Avx_NegS_VarReg(const STATEMENT&);
		void Emit_Fp_Avx_ClampS_VarVar(const STATEMENT&);
		void Emit_Fp_Avx_ToSingleI32_VarReg(const STATEMENT&);
		void Emit_Fp_Avx_ToSingleI32_VarMem(const STATEMENT&);
		void Emit_Fp_Avx_ToInt32TruncS_RegVar(const STATEMENT&);
		void Emit_Fp_Avx_ToInt32TruncS_MemVar(const STATEMENT&);

		//MDOP AVX
		template <typename>
		void Emit_Md_Avx_VarVar(const STATEMENT&);
		template <typename>
		void Emit_Md_Avx_VarVarVar(const STATEMENT&);
		template <typename>
		void Emit_Md_Avx_VarVarVarRev(const STATEMENT&);
		template <typename, uint8>
		void
		Emit_Md_Avx_Shift_VarVarCst(const STATEMENT&);
		void Emit_Md_Avx_Mov_RegVar(const STATEMENT&);
		void Emit_Md_Avx_Mov_MemReg(const STATEMENT&);
		void Emit_Md_Avx_Mov_MemMem(const STATEMENT&);
		void Emit_Md_Avx_MovMasked_VarVarVar(const STATEMENT&);

		void Emit_Md_Avx_Not_VarVar(const STATEMENT&);
		void Emit_Md_Avx_Abs_VarVar(const STATEMENT&);
		void Emit_Md_Avx_Neg_VarVar(const STATEMENT&);
		void Emit_Md_Avx_AddSSW_VarVarVar(const STATEMENT&);
		void Emit_Md_Avx_AddUSW_VarVarVar(const STATEMENT&);
		void Emit_Md_Avx_SubSSW_VarVarVar(const STATEMENT&);
		void Emit_Md_Avx_SubUSW_VarVarVar(const STATEMENT&);
		void Emit_Md_Avx_ClampS_VarVar(const STATEMENT&);

		void Emit_Md_Avx_PackHB_VarVarVar(const STATEMENT&);
		void Emit_Md_Avx_PackWH_VarVarVar(const STATEMENT&);

		void Emit_Md_Avx_MakeClip_VarVarVar(const STATEMENT&);
		void Emit_Md_Avx_MakeSz_VarVar(const STATEMENT&);

		void Emit_Md_Avx_Expand_VarVar(const STATEMENT&);
		void Emit_Md_Avx_Expand_VarCst(const STATEMENT&);
		void Emit_Md_Avx_Expand_VarVarCst(const STATEMENT&);

		void Emit_Md_Avx2_Expand_VarReg(const STATEMENT&);
		void Emit_Md_Avx2_Expand_VarMem(const STATEMENT&);
		void Emit_Md_Avx2_Expand_VarCst(const STATEMENT&);
		void Emit_Md_Avx2_Expand_VarRegCst(const STATEMENT&);
		void Emit_Md_Avx2_Expand_VarMemCst(const STATEMENT&);

		void Emit_Avx_MergeTo256_MemVarVar(const STATEMENT&);

		void Emit_Md_Avx_Srl256_VarMemVar(const STATEMENT&);
		void Emit_Md_Avx_Srl256_VarMemCst(const STATEMENT&);

		void Emit_Md_Avx_LoadFromRef_VarVar(const STATEMENT&);
		void Emit_Md_Avx_LoadFromRef_VarVarAny(const STATEMENT&);

		void Emit_Md_Avx_StoreAtRef_VarVar(const STATEMENT&);
		void Emit_Md_Avx_StoreAtRef_VarAnyVar(const STATEMENT&);

		void Emit_Md_Avx_LoadFromRefMasked_VarVarAnyVar(const STATEMENT&);
		void Emit_Md_Avx_StoreAtRefMasked_VarAnyVar(const STATEMENT&);

		static CX86Assembler::REGISTER g_baseRegister;

		CX86Assembler::REGISTER PrepareSymbolRegisterDef(CSymbol*, CX86Assembler::REGISTER);
		CX86Assembler::REGISTER PrepareSymbolRegisterUse(CSymbol*, CX86Assembler::REGISTER);
		CX86Assembler::BYTEREGISTER PrepareSymbolByteRegisterUse(CSymbol*, CX86Assembler::REGISTER);
		void CommitSymbolRegister(CSymbol*, CX86Assembler::REGISTER);

		CX86Assembler::XMMREGISTER PrepareSymbolRegisterDefFp32(CSymbol*, CX86Assembler::XMMREGISTER);
		CX86Assembler::XMMREGISTER PrepareSymbolRegisterUseFp32Avx(CSymbol*, CX86Assembler::XMMREGISTER);
		void CommitSymbolRegisterFp32Sse(CSymbol*, CX86Assembler::XMMREGISTER);
		void CommitSymbolRegisterFp32Avx(CSymbol*, CX86Assembler::XMMREGISTER);

		CX86Assembler::XMMREGISTER PrepareSymbolRegisterDefMd(CSymbol*, CX86Assembler::XMMREGISTER);
		CX86Assembler::XMMREGISTER PrepareSymbolRegisterUseMdSse(CSymbol*, CX86Assembler::XMMREGISTER);
		CX86Assembler::XMMREGISTER PrepareSymbolRegisterUseMdAvx(CSymbol*, CX86Assembler::XMMREGISTER);
		void CommitSymbolRegisterMdSse(CSymbol*, CX86Assembler::XMMREGISTER);
		void CommitSymbolRegisterMdAvx(CSymbol*, CX86Assembler::XMMREGISTER);

		virtual CX86Assembler::REGISTER PrepareRefSymbolRegisterUse(CSymbol*, CX86Assembler::REGISTER) = 0;

		static const LITERAL128 g_makeClipShufflePattern;
		static const LITERAL128 g_makeSzShufflePattern;
		static const LITERAL128 g_fpCstOne;
		static const LITERAL128 g_fpClampMask1;
		static const LITERAL128 g_fpClampMask2;
		static const std::array<uint8, 4> g_mdExpandShufPatterns;
		static const std::array<uint32, 4> g_fpMxcsrRoundBits;
		static constexpr uint32 MXCSR_ROUND_MASK = 0x6000;

		CX86Assembler m_assembler;
		CX86Assembler::REGISTER* m_registers = nullptr;
		CX86Assembler::XMMREGISTER* m_mdRegisters = nullptr;
		LabelMapType m_labels;
		SymbolReferenceLabelArray m_symbolReferenceLabels;
		uint32 m_stackLevel = 0;
		uint32 m_registerUsage = 0;

		CX86CpuFeatures m_cpuFeatures;

	private:
		typedef void (CCodeGen_x86::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION op;
			MATCHTYPE dstType;
			MATCHTYPE src1Type;
			MATCHTYPE src2Type;
			MATCHTYPE src3Type;
			ConstCodeEmitterType emitter;
		};

		void InsertMatchers(const CONSTMATCHER*);

		static CONSTMATCHER g_constMatchers[];

		static CONSTMATCHER g_fpuConstMatchers[];
		static CONSTMATCHER g_fpuSseConstMatchers[];
		static CONSTMATCHER g_fpuAvxConstMatchers[];

		//SSE SIMD matchers
		static CONSTMATCHER g_mdSseConstMatchers[];

		static CONSTMATCHER g_mdNoSse41ConstMatchers[];
		static CONSTMATCHER g_mdSse41ConstMatchers[];

		static CONSTMATCHER g_mdNoSsse3ConstMatchers[];
		static CONSTMATCHER g_mdSsse3ConstMatchers[];

		//AVX SIMD matchers
		static CONSTMATCHER g_mdAvxConstMatchers[];

		static CONSTMATCHER g_mdNoAvx2ConstMatchers[];
		static CONSTMATCHER g_mdAvx2ConstMatchers[];
	};
}
