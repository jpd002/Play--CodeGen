#include "Jitter_CodeGen_x86.h"
#include "Jitter.h"
#include <stdexcept>

using namespace Jitter;

CX86Assembler::XMMREGISTER CCodeGen_x86::PrepareSymbolRegisterUseFp32Avx(CSymbol* symbol, CX86Assembler::XMMREGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REGISTER32:
		return m_mdRegisters[symbol->m_valueLow];
		break;
	case SYM_FP_TEMPORARY32:
	case SYM_FP_RELATIVE32:
		m_assembler.VmovssEd(preferedRegister, MakeMemoryFp32SymbolAddress(symbol));
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_x86::CommitSymbolRegisterFp32Avx(CSymbol* symbol, CX86Assembler::XMMREGISTER usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REGISTER32:
		assert(usedRegister == m_mdRegisters[symbol->m_valueLow]);
		break;
	case SYM_FP_TEMPORARY32:
	case SYM_FP_RELATIVE32:
		m_assembler.VmovssEd(MakeMemoryFp32SymbolAddress(symbol), usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fp32_Avx_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);

	((m_assembler).*(FPUOP::OpEdAvx()))(dstRegister, CX86Assembler::xMM0, MakeVariableFp32SymbolAddress(src1));

	CommitSymbolRegisterFp32Avx(dst, dstRegister);
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fp32_Avx_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseFp32Avx(src1, CX86Assembler::xMM1);

	((m_assembler).*(FPUOP::OpEdAvx()))(dstRegister, src1Register, MakeVariableFp32SymbolAddress(src2));

	CommitSymbolRegisterFp32Avx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp32_Avx_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);

	m_assembler.VmovssEd(m_mdRegisters[dst->m_valueLow], MakeMemoryFp32SymbolAddress(src1));
}

void CCodeGen_x86::Emit_Fp32_Avx_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REGISTER32);

	m_assembler.VmovssEd(MakeMemoryFp32SymbolAddress(dst), m_mdRegisters[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Fp32_Avx_LdCst_RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);
	assert(src1->m_type == SYM_CONSTANT);

	auto tmpRegister = CX86Assembler::rAX;

	m_assembler.MovId(tmpRegister, src1->m_valueLow);
	m_assembler.VmovdVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(tmpRegister));
}

void CCodeGen_x86::Emit_Fp_Avx_CmpS_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);
	auto cmpReg = PrepareSymbolRegisterUseFp32Avx(src1, CX86Assembler::xMM0);
	auto resReg = CX86Assembler::xMM1;

	auto conditionCode = GetSseConditionCode(statement.jmpCondition);
	m_assembler.VcmpssEd(resReg, cmpReg, MakeVariableFp32SymbolAddress(src2), conditionCode);
	m_assembler.VmovdVo(CX86Assembler::MakeRegisterAddress(dstReg), resReg);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Fp_Avx_RsqrtS_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);
	auto sqrtRegister = CX86Assembler::xMM1;

	m_assembler.VsqrtssEd(sqrtRegister, CX86Assembler::xMM0, MakeVariableFp32SymbolAddress(src1));
	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.VmovdVo(dstRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.VdivssEd(dstRegister, dstRegister, CX86Assembler::MakeXmmRegisterAddress(sqrtRegister));

	CommitSymbolRegisterFp32Avx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_RcplS_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);
	auto oneRegister = CX86Assembler::xMM1;

	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.VmovdVo(oneRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.VdivssEd(dstRegister, oneRegister, MakeVariableFp32SymbolAddress(src1));

	CommitSymbolRegisterFp32Avx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_AbsS_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);

	auto tmpIntRegister = CX86Assembler::rAX;

	m_assembler.MovEd(tmpIntRegister, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.AndId(CX86Assembler::MakeRegisterAddress(tmpIntRegister), 0x7FFFFFFF);
	m_assembler.VmovdVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(tmpIntRegister));
}

void CCodeGen_x86::Emit_Fp_Avx_AbsS_VarReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REGISTER32);

	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);
	auto maskRegister = CX86Assembler::xMM1;

	m_assembler.VpcmpeqdVo(maskRegister, maskRegister, CX86Assembler::MakeXmmRegisterAddress(maskRegister));
	m_assembler.VpsrldVo(maskRegister, maskRegister, 1);
	m_assembler.VpandVo(dstRegister, maskRegister, CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src1->m_valueLow]));

	CommitSymbolRegisterFp32Avx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_NegS_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);

	auto tmpIntRegister = CX86Assembler::rAX;

	m_assembler.MovEd(tmpIntRegister, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.XorId(CX86Assembler::MakeRegisterAddress(tmpIntRegister), 0x80000000);
	m_assembler.VmovdVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(tmpIntRegister));
}

void CCodeGen_x86::Emit_Fp_Avx_NegS_VarReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REGISTER32);

	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);
	auto signRegister = CX86Assembler::xMM1;

	m_assembler.VpcmpeqdVo(signRegister, signRegister, CX86Assembler::MakeXmmRegisterAddress(signRegister));
	m_assembler.VpslldVo(signRegister, signRegister, 31);
	m_assembler.VpxorVo(dstRegister, signRegister, CX86Assembler::MakeXmmRegisterAddress(m_mdRegisters[src1->m_valueLow]));

	CommitSymbolRegisterFp32Avx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_ClampS_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseFp32Avx(src1, CX86Assembler::xMM1);

	m_assembler.VpminsdVo(dstRegister, src1Register, MakeConstant128Address(g_fpClampMask1));
	m_assembler.VpminudVo(dstRegister, dstRegister, MakeConstant128Address(g_fpClampMask2));

	CommitSymbolRegisterFp32Avx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_ToSingleI32_VarReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REGISTER32);

	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);
	auto tmpIntRegister = CX86Assembler::rAX;

	m_assembler.VmovdVo(CX86Assembler::MakeRegisterAddress(tmpIntRegister), m_mdRegisters[src1->m_valueLow]);
	m_assembler.Vcvtsi2ssEd(dstRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));

	CommitSymbolRegisterFp32Avx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_ToSingleI32_VarMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefFp32(dst, CX86Assembler::xMM0);

	m_assembler.Vcvtsi2ssEd(dstRegister, MakeVariableFp32SymbolAddress(src1));

	CommitSymbolRegisterFp32Avx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_ToInt32TruncS_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REGISTER32);

	auto tmpIntRegister = CX86Assembler::rAX;

	m_assembler.Vcvttss2siEd(tmpIntRegister, MakeVariableFp32SymbolAddress(src1));
	m_assembler.VmovdVo(m_mdRegisters[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(tmpIntRegister));
}

void CCodeGen_x86::Emit_Fp_Avx_ToInt32TruncS_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;

	m_assembler.Vcvttss2siEd(tmpIntRegister, MakeVariableFp32SymbolAddress(src1));
	m_assembler.MovGd(MakeMemoryFp32SymbolAddress(dst), tmpIntRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_SetRoundingMode_Cst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	uint32 mxcsrRoundBits = g_fpMxcsrRoundBits[src1->m_valueLow];

	auto tempValueAddress = CX86Assembler::MakeIndRegAddress(CX86Assembler::rSP);

	//We push/pop eax to allocate stack space since size of rSP is dependent on
	//target bitness and we don't have the info here.

	m_assembler.Push(CX86Assembler::rAX);
	m_assembler.VstmxcsrGd(tempValueAddress);
	m_assembler.AndId(tempValueAddress, ~MXCSR_ROUND_MASK);
	if(mxcsrRoundBits != 0)
	{
		m_assembler.OrId(tempValueAddress, mxcsrRoundBits);
	}
	m_assembler.VldmxcsrGd(tempValueAddress);
	m_assembler.Pop(CX86Assembler::rAX);
}

// clang-format off
CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_fpuAvxConstMatchers[] = 
{
	{ OP_FP_ADD_S, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_VarVarVar<FP32OP_ADD> },
	{ OP_FP_SUB_S, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_VarVarVar<FP32OP_SUB> },
	{ OP_FP_MUL_S, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_VarVarVar<FP32OP_MUL> },
	{ OP_FP_DIV_S, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_VarVarVar<FP32OP_DIV> },
	{ OP_FP_MAX_S, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_VarVarVar<FP32OP_MAX> },
	{ OP_FP_MIN_S, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_VarVarVar<FP32OP_MIN> },

	{ OP_FP_CMP_S, MATCH_VARIABLE, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_CmpS_VarVarVar },

	{ OP_FP_SQRT_S,  MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_VarVar<FP32OP_SQRT> },
	{ OP_FP_RSQRT_S, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_RsqrtS_VarVar         },
	{ OP_FP_RCPL_S,  MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_RcplS_VarVar          },

	{ OP_FP_ABS_S,   MATCH_FP_REGISTER32, MATCH_FP_MEMORY32,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_AbsS_RegMem   },
	{ OP_FP_ABS_S,   MATCH_FP_VARIABLE32, MATCH_FP_REGISTER32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_AbsS_VarReg   },
	{ OP_FP_NEG_S,   MATCH_FP_REGISTER32, MATCH_FP_MEMORY32,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_NegS_RegMem   },
	{ OP_FP_NEG_S,   MATCH_FP_VARIABLE32, MATCH_FP_REGISTER32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_NegS_VarReg   },
	{ OP_FP_CLAMP_S, MATCH_FP_VARIABLE32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ClampS_VarVar },

	{ OP_FP_TOSINGLE_I32,    MATCH_FP_VARIABLE32, MATCH_FP_REGISTER32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToSingleI32_VarReg   },
	{ OP_FP_TOSINGLE_I32,    MATCH_FP_VARIABLE32, MATCH_FP_MEMORY32,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToSingleI32_VarMem   },
	{ OP_FP_TOINT32_TRUNC_S, MATCH_FP_REGISTER32, MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToInt32TruncS_RegVar },
	{ OP_FP_TOINT32_TRUNC_S, MATCH_FP_MEMORY32,   MATCH_FP_VARIABLE32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToInt32TruncS_MemVar },

	{ OP_MOV,      MATCH_FP_REGISTER32, MATCH_FP_MEMORY32,   MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_Mov_RegMem },
	{ OP_MOV,      MATCH_FP_MEMORY32,   MATCH_FP_REGISTER32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_Mov_MemReg },
	{ OP_FP_LDCST, MATCH_FP_REGISTER32, MATCH_CONSTANT,      MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_LdCst_RegCst },

	{ OP_FP_SETROUNDINGMODE, MATCH_NIL, MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_SetRoundingMode_Cst },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
// clang-format on
