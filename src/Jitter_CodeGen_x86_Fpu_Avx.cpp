#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_Avx_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	((m_assembler).*(FPUOP::OpEdAvx()))(CX86Assembler::xMM0, CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.VmovssEd(MakeMemoryFpSingleSymbolAddress(dst), CX86Assembler::xMM0);
}

template <typename FPUOP>
void CCodeGen_x86::Emit_Fpu_Avx_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = CX86Assembler::xMM0;
	auto src1Register = CX86Assembler::xMM1;

	m_assembler.VmovssEd(src1Register, MakeMemoryFpSingleSymbolAddress(src1));
	((m_assembler).*(FPUOP::OpEdAvx()))(dstRegister, src1Register, MakeMemoryFpSingleSymbolAddress(src2));
	m_assembler.VmovssEd(MakeMemoryFpSingleSymbolAddress(dst), dstRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_Rsqrt_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;
	auto sqrtRegister = CX86Assembler::xMM1;

	m_assembler.VsqrtssEd(sqrtRegister, CX86Assembler::xMM0, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.VmovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.VdivssEd(resultRegister, resultRegister, CX86Assembler::MakeXmmRegisterAddress(sqrtRegister));
	m_assembler.VmovssEd(MakeMemoryFpSingleSymbolAddress(dst), resultRegister);
}

void CCodeGen_x86::Emit_Fp_Avx_Rcpl_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpIntRegister = CX86Assembler::rAX;
	auto resultRegister = CX86Assembler::xMM0;

	m_assembler.MovId(tmpIntRegister, 0x3F800000);
	m_assembler.VmovdVo(resultRegister, CX86Assembler::MakeRegisterAddress(tmpIntRegister));
	m_assembler.VdivssEd(resultRegister, resultRegister, MakeMemoryFpSingleSymbolAddress(src1));
	m_assembler.VmovssEd(MakeMemoryFpSingleSymbolAddress(dst), resultRegister);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_fpuAvxConstMatchers[] = 
{
	{ OP_FP_ADD, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, &CCodeGen_x86::Emit_Fpu_Avx_MemMemMem<FPUOP_ADD> },
	{ OP_FP_SUB, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, &CCodeGen_x86::Emit_Fpu_Avx_MemMemMem<FPUOP_SUB> },
	{ OP_FP_MUL, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, &CCodeGen_x86::Emit_Fpu_Avx_MemMemMem<FPUOP_MUL> },
	{ OP_FP_DIV, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, &CCodeGen_x86::Emit_Fpu_Avx_MemMemMem<FPUOP_DIV> },

	{ OP_FP_SQRT,  MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fpu_Avx_MemMem<FPUOP_SQRT> },
	{ OP_FP_RSQRT, MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Rsqrt_MemMem        },
	{ OP_FP_RCPL,  MATCH_MEMORY_FP_SINGLE, MATCH_MEMORY_FP_SINGLE, MATCH_NIL, &CCodeGen_x86::Emit_Fp_Avx_Rcpl_MemMem         },

	{ OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, NULL },
};
