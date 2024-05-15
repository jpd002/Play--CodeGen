#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

template <typename FPUOP>
void CCodeGen_x86::Emit_Fp32_Avx_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(FPUOP::OpEdAvx()))(CX86Assembler::xMM0, CX86Assembler::xMM0, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.VmovssEd(MakeMemoryFp32SymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fp32_Avx_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = CX86Assembler::xMM0;
	auto src1Register = CX86Assembler::xMM1;

	m_assembler.VmovssEd(src1Register, MakeMemoryFp32SymbolAddress(src1));
	((m_assembler).*(FPUOP::OpEdAvx()))(dstRegister, src1Register, MakeMemoryFp32SymbolAddress(src2));
	m_assembler.VmovssEd(MakeMemoryFp32SymbolAddress(dst), dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_CmpS_VarMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);
	auto cmpReg = CX86Assembler::xMM0;
	auto resReg = CX86Assembler::xMM1;

	auto conditionCode = GetSseConditionCode(statement.jmpCondition);
	m_assembler.VmovssEd(cmpReg, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.VcmpssEd(resReg, cmpReg, MakeMemoryFp32SymbolAddress(src2), conditionCode);
	m_assembler.VmovdVo(CX86Assembler::MakeRegisterAddress(dstReg), resReg);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Fp_Avx_RsqrtS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;
	auto sqrtRegister = CX86Assembler::xMM1;

	m_assembler.VsqrtssEd(sqrtRegister, CX86Assembler::xMM0, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.VmovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.VdivssEd(resultRegister, resultRegister, CX86Assembler::MakeXmmRegisterAddress(sqrtRegister));
	m_assembler.VmovssEd(MakeMemoryFp32SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_RcplS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.VmovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.VdivssEd(resultRegister, resultRegister, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.VmovssEd(MakeMemoryFp32SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_ClampS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.VmovssEd(resultRegister, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.VpminsdVo(resultRegister, resultRegister, MakeConstant128Address(g_fpClampMask1));
	m_assembler.VpminudVo(resultRegister, resultRegister, MakeConstant128Address(g_fpClampMask2));
	m_assembler.VmovssEd(MakeMemoryFp32SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_ToSingleI32_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.Vcvtsi2ssEd(CX86Assembler::xMM0, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.VmovssEd(MakeMemoryFp32SymbolAddress(dst), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Fp_Avx_ToInt32TruncS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.Vcvttss2siEd(CX86Assembler::rAX, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.MovGd(MakeMemoryFp32SymbolAddress(dst), CX86Assembler::rAX);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_fpuAvxConstMatchers[] = 
{
	{ OP_FP_ADD_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_MemMemMem<FP32OP_ADD> },
	{ OP_FP_SUB_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_MemMemMem<FP32OP_SUB> },
	{ OP_FP_MUL_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_MemMemMem<FP32OP_MUL> },
	{ OP_FP_DIV_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_MemMemMem<FP32OP_DIV> },
	{ OP_FP_MAX_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_MemMemMem<FP32OP_MAX> },
	{ OP_FP_MIN_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_MemMemMem<FP32OP_MIN> },

	{ OP_FP_CMP_S, MATCH_VARIABLE, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_CmpS_VarMemMem },

	{ OP_FP_SQRT_S,  MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_Avx_MemMem<FP32OP_SQRT> },
	{ OP_FP_RSQRT_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_RsqrtS_MemMem         },
	{ OP_FP_RCPL_S,  MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_RcplS_MemMem          },

	{ OP_FP_CLAMP_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ClampS_MemMem },

	{ OP_FP_TOSINGLE_I32,    MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToSingleI32_MemMem },
	{ OP_FP_TOINT32_TRUNC_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_ToInt32TruncS_MemMem },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
