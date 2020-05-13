#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(FPUOP::OpEd()))(CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.MovssEd(MakeMemoryFpSingleSymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovssEd(CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src1));
	((m_assembler).*(FPUOP::OpEd()))(CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src2));
	m_assembler.MovssEd(MakeMemoryFpSingleSymbolAddress(dst), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Fp_Cmp_VarMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);

	auto conditionCode = GetSseConditionCode(statement.jmpCondition);
	m_assembler.MovssEd(CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.CmpssEd(CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src2), conditionCode);
	m_assembler.MovdVo(CX86Assembler::MakeRegisterAddress(dstReg), CX86Assembler::xMM0);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Fp_Cmp_VarMemCst(const STATEMENT& statement)
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

	m_assembler.MovssEd(src1Reg, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.CmpssEd(src1Reg, CX86Assembler::MakeXmmRegisterAddress(src2Reg), conditionCode);
	m_assembler.MovdVo(CX86Assembler::MakeRegisterAddress(dstReg), src1Reg);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_x86::Emit_Fp_Rsqrt_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;
	auto sqrtRegister = CX86Assembler::xMM1;

	m_assembler.SqrtssEd(sqrtRegister, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.DivssEd(resultRegister, CX86Assembler::MakeXmmRegisterAddress(sqrtRegister));
	m_assembler.MovssEd(MakeMemoryFpSingleSymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp_Rcpl_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.MovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.DivssEd(resultRegister, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.MovssEd(MakeMemoryFpSingleSymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp_Mov_RelSRelI32(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_REL_SINGLE);
	assert(src1->m_type == SYM_FP_REL_INT32);

	m_assembler.Cvtsi2ssEd(CX86Assembler::xMM0, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovssEd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::xMM0);
}

void CCodeGen_x86::Emit_Fp_ToIntTrunc_RelRel(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_REL_SINGLE);
	assert(src1->m_type == SYM_FP_REL_SINGLE);

	m_assembler.Cvttss2siEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, src1->m_valueLow));
	m_assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, dst->m_valueLow), CX86Assembler::rAX);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_fpuSseConstMatchers[] = 
{
	{ OP_FP_ADD, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_DIV> },
	{ OP_FP_MAX, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_MAX> },
	{ OP_FP_MIN, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_MemMemMem<FPUOP_MIN> },

	{ OP_FP_CMP, MATCH_VARIABLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Cmp_VarMemMem },
	{ OP_FP_CMP, MATCH_VARIABLE, MATCH_MEMORY_FP_SINGLE, MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_x86::Emit_Fp_Cmp_VarMemCst },

	{ OP_FP_SQRT,  MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_MemMem<FPUOP_SQRT> },
	{ OP_FP_RSQRT, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Rsqrt_MemMem        },
	{ OP_FP_RCPL,  MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Rcpl_MemMem         },

	{ OP_MOV,            MATCH_RELATIVE_FP_SINGLE, MATCH_RELATIVE_FP_INT32,  MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Mov_RelSRelI32    },
	{ OP_FP_TOINT_TRUNC, MATCH_RELATIVE_FP_SINGLE, MATCH_RELATIVE_FP_SINGLE, MATCH_NIL, MATCH_NIL, &CCodeGen_x86::Emit_Fp_ToIntTrunc_RelRel },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },
};
