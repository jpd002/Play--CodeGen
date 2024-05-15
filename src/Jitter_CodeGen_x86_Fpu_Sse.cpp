#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

template <typename FPUOP>
void CCodeGen_x86::Emit_Fp32_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(FPUOP::OpEd()))(CX86Assembler::xMM0, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fp32_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovssEd(CX86Assembler::xMM0, MakeMemoryFp32SymbolAddress(src1));
	((m_assembler).*(FPUOP::OpEd()))(CX86Assembler::xMM0, MakeMemoryFp32SymbolAddress(src2));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Fp_CmpS_VarMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);

	auto conditionCode = GetSseConditionCode(statement.jmpCondition);
	m_assembler.MovssEd(CX86Assembler::xMM0, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.CmpssEd(CX86Assembler::xMM0, MakeMemoryFp32SymbolAddress(src2), conditionCode);
	m_assembler.MovdVo(CX86Assembler::MakeRegisterAddress(dstReg), CX86Assembler::xMM0);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Fp_CmpS_VarMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto conditionCode = GetSseConditionCode(statement.jmpCondition);
	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);
	auto src1Reg = CX86Assembler::xMM0;
	auto src2Reg = CX86Assembler::xMM1;

	if(src2->m_valueLow == 0)
	{
		m_assembler.PxorVo(src2Reg, CX86Assembler::MakeXmmRegisterAddress(src2Reg));
	}
	else
	{
		auto cstReg = CX86Assembler::rDX;
		assert(dstReg != cstReg);
		m_assembler.MovId(cstReg, src2->m_valueLow);
		m_assembler.MovdVo(src2Reg, CX86Assembler::MakeRegisterAddress(cstReg));
	}

	m_assembler.MovssEd(src1Reg, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.CmpssEd(src1Reg, CX86Assembler::MakeXmmRegisterAddress(src2Reg), conditionCode);
	m_assembler.MovdVo(CX86Assembler::MakeRegisterAddress(dstReg), src1Reg);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Fp_RsqrtS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;
	auto sqrtRegister = CX86Assembler::xMM1;

	m_assembler.SqrtssEd(sqrtRegister, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.DivssEd(resultRegister, CX86Assembler::MakeXmmRegisterAddress(sqrtRegister));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp_RcplS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.DivssEd(resultRegister, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp_ClampS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovssEd(resultRegister, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.PminsdVo(resultRegister, MakeConstant128Address(g_fpClampMask1));
	m_assembler.PminudVo(resultRegister, MakeConstant128Address(g_fpClampMask2));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp_ToSingleI32_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.Cvtsi2ssEd(CX86Assembler::xMM0, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.MovssEd(MakeMemoryFp32SymbolAddress(dst), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Fp_ToInt32TruncS_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.Cvttss2siEd(CX86Assembler::rAX, MakeMemoryFp32SymbolAddress(src1));
	m_assembler.MovGd(MakeMemoryFp32SymbolAddress(dst), CX86Assembler::rAX);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_fpuSseConstMatchers[] = 
{
	{ OP_FP_ADD_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_MemMemMem<FP32OP_ADD> },
	{ OP_FP_SUB_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_MemMemMem<FP32OP_SUB> },
	{ OP_FP_MUL_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_MemMemMem<FP32OP_MUL> },
	{ OP_FP_DIV_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_MemMemMem<FP32OP_DIV> },
	{ OP_FP_MAX_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_MemMemMem<FP32OP_MAX> },
	{ OP_FP_MIN_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_MemMemMem<FP32OP_MIN> },

	{ OP_FP_CMP_S, MATCH_VARIABLE, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, &CCodeGen_x86::Emit_Fp_CmpS_VarMemMem },
	{ OP_FP_CMP_S, MATCH_VARIABLE, MATCH_FP_MEMORY32, MATCH_CONSTANT,    MATCH_NIL, &CCodeGen_x86::Emit_Fp_CmpS_VarMemCst },

	{ OP_FP_SQRT_S,  MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp32_MemMem<FP32OP_SQRT> },
	{ OP_FP_RSQRT_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_RsqrtS_MemMem         },
	{ OP_FP_RCPL_S,  MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_RcplS_MemMem          },

	{ OP_FP_CLAMP_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ClampS_MemMem },

	{ OP_FP_TOSINGLE_I32,    MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ToSingleI32_MemMem   },
	{ OP_FP_TOINT32_TRUNC_S, MATCH_FP_MEMORY32, MATCH_FP_MEMORY32, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ToInt32TruncS_MemMem },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
