#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

template <typename MDOP>
void CCodeGen_x86::Emit_Md_Avx_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto src1Register = PrepareSymbolRegisterUseMdAvx(src1, CX86Assembler::xMM1);

	((m_assembler).*(MDOP::OpVoAvx()))(dstRegister, src1Register, MakeVariable128SymbolAddress(src2));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

void CCodeGen_x86::Emit_Md_Avx_Mov_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.VmovapsVo(m_mdRegisters[dst->m_valueLow], MakeVariable128SymbolAddress(src1));
}

void CCodeGen_x86::Emit_Md_Avx_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.VmovapsVo(MakeMemory128SymbolAddress(dst), m_mdRegisters[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Md_Avx_Not_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDefMd(dst, CX86Assembler::xMM0);
	auto cstRegister = CX86Assembler::xMM1;

	assert(dstRegister != cstRegister);

	m_assembler.VpcmpeqdVo(cstRegister, cstRegister, CX86Assembler::MakeXmmRegisterAddress(cstRegister));
	m_assembler.VpxorVo(dstRegister, cstRegister, MakeVariable128SymbolAddress(src1));

	CommitSymbolRegisterMdAvx(dst, dstRegister);
}

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_mdAvxConstMatchers[] = 
{
	{ OP_MD_ADD_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDB> },
	{ OP_MD_ADD_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDH> },
	{ OP_MD_ADD_W, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDW> },

	{ OP_MD_ADDSS_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDSSB> },
	{ OP_MD_ADDSS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDSSH> },

	{ OP_MD_ADDUS_B, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDUSB> },
	{ OP_MD_ADDUS_H, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_ADDUSH> },

	{ OP_MD_AND, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_AND> },
	{ OP_MD_OR,  MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_OR>  },
	{ OP_MD_XOR, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_VARIABLE128, &CCodeGen_x86::Emit_Md_Avx_VarVarVar<MDOP_XOR> },

	{ OP_MD_NOT, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Not_VarVar },

	{ OP_MOV, MATCH_REGISTER128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Mov_RegVar, },
	{ OP_MOV, MATCH_MEMORY128,   MATCH_REGISTER128, MATCH_NIL, &CCodeGen_x86::Emit_Md_Avx_Mov_MemReg, },

	{ OP_MOV, MATCH_NIL,         MATCH_NIL,         MATCH_NIL, nullptr },
};
