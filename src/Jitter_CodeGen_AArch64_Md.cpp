#include "Jitter_CodeGen_AArch64.h"

using namespace Jitter;

void CCodeGen_AArch64::LoadMemory128AddressInRegister(CAArch64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		LoadRelative128AddressInRegister(dstReg, symbol, offset);
		break;
	case SYM_TEMPORARY128:
		LoadTemporary128AddressInRegister(dstReg, symbol, offset);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::LoadRelative128AddressInRegister(CAArch64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_RELATIVE128);

	uint32 totalOffset = symbol->m_valueLow + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Add(dstReg, g_baseRegister, totalOffset, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
}

void CCodeGen_AArch64::LoadTemporary128AddressInRegister(CAArch64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_TEMPORARY128);

	uint32 totalOffset = symbol->m_stackLocation + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Add(dstReg, CAArch64Assembler::xSP, totalOffset, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
}

void CCodeGen_AArch64::Emit_Md_AddSSW_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstAddrReg = GetNextTempRegister64();
	auto src1AddrReg = GetNextTempRegister64();
	auto src2AddrReg = GetNextTempRegister64();
	
	auto dstReg = CAArch64Assembler::v0;
	auto src1Reg = CAArch64Assembler::v1;
	auto src2Reg = CAArch64Assembler::v2;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Ld1_4s(src1Reg, src1AddrReg);
	m_assembler.Ld1_4s(src2Reg, src2AddrReg);
	m_assembler.Sqadd_4s(dstReg, src1Reg, src2Reg);
	m_assembler.St1_4s(dstReg, dstAddrReg);
}

void CCodeGen_AArch64::Emit_Md_MovMasked_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
}

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_mdConstMatchers[] =
{
	{ OP_MD_ADDSS_W,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_AddSSW_MemMemMem                 },
	
	{ OP_MD_MOV_MASKED,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MovMasked_MemMemMem              },

	{ OP_MOV,              MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           nullptr                                                     },
};
