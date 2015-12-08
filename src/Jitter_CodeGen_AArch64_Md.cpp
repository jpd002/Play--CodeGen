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

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstAddrReg = GetNextTempRegister64();
	auto src1AddrReg = GetNextTempRegister64();
	auto src2AddrReg = GetNextTempRegister64();
	
	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto src2Reg = GetNextTempRegisterMd();

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Ld1_4s(src1Reg, src1AddrReg);
	m_assembler.Ld1_4s(src2Reg, src2AddrReg);
	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);
	m_assembler.St1_4s(dstReg, dstAddrReg);
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_MemMemMemRev(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = GetNextTempRegister64();
	auto src1AddrReg = GetNextTempRegister64();
	auto src2AddrReg = GetNextTempRegister64();
	
	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto src2Reg = GetNextTempRegisterMd();

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Ld1_4s(src1Reg, src1AddrReg);
	m_assembler.Ld1_4s(src2Reg, src2AddrReg);
	((m_assembler).*(MDOP::OpReg()))(dstReg, src2Reg, src1Reg);
	m_assembler.St1_4s(dstReg, dstAddrReg);
}

void CCodeGen_AArch64::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = GetNextTempRegister64();
	auto src1AddrReg = GetNextTempRegister64();

	auto tmpReg = GetNextTempRegisterMd();
	
	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	
	m_assembler.Ld1_4s(tmpReg, src1AddrReg);
	m_assembler.St1_4s(tmpReg, dstAddrReg);
}

void CCodeGen_AArch64::Emit_Md_Not_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = GetNextTempRegister64();
	auto src1AddrReg = GetNextTempRegister64();

	auto tmpReg = GetNextTempRegisterMd();
	auto zeroReg = GetNextTempRegisterMd();
	
	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	
	m_assembler.Ld1_4s(tmpReg, src1AddrReg);
	m_assembler.Eor_16b(zeroReg, zeroReg, zeroReg);
	m_assembler.Orn_16b(tmpReg, zeroReg, tmpReg);
	m_assembler.St1_4s(tmpReg, dstAddrReg);
}

void CCodeGen_AArch64::Emit_Md_MovMasked_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
}

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_mdConstMatchers[] =
{
	{ OP_MD_ADD_B,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_ADDB>                  },
	{ OP_MD_ADD_H,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_ADDH>                  },
	{ OP_MD_ADD_W,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_ADDW>                  },

	{ OP_MD_ADDUS_B,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_ADDBUS>                },
	{ OP_MD_ADDUS_W,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_ADDWUS>                },

	{ OP_MD_ADDSS_H,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_ADDHSS>                },
	{ OP_MD_ADDSS_W,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_ADDWSS>                },
	
	{ OP_MD_SUB_B,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_SUBB>                  },
	{ OP_MD_SUB_H,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_SUBH>                  },
	{ OP_MD_SUB_W,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_SUBW>                  },
	
	{ OP_MD_SUBUS_B,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_SUBBUS>                },
	{ OP_MD_SUBUS_H,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_SUBHUS>                },

	{ OP_MD_SUBSS_H,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_SUBHSS>                },
	{ OP_MD_SUBSS_W,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_SUBWSS>                },

	{ OP_MD_ADD_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_ADDS>                  },
	{ OP_MD_SUB_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_SUBS>                  },
	{ OP_MD_MUL_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_MULS>                  },
	{ OP_MD_DIV_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_DIVS>                  },

	{ OP_MD_AND,                MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_AND>                   },
	{ OP_MD_OR,                 MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_OR>                    },
	{ OP_MD_XOR,                MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_XOR>                   },
	
	{ OP_MD_NOT,                MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_Not_MemMem                            },
	
	{ OP_MD_UNPACK_LOWER_BH,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_BH>    },
	{ OP_MD_UNPACK_LOWER_HW,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_HW>    },
	{ OP_MD_UNPACK_LOWER_WD,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_WD>    },
	
	{ OP_MD_UNPACK_UPPER_BH,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_UPPER_BH>    },
	{ OP_MD_UNPACK_UPPER_HW,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_UPPER_HW>    },
	{ OP_MD_UNPACK_UPPER_WD,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_UPPER_WD>    },

	{ OP_MOV,                   MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_Mov_MemMem                            },
	
	{ OP_MOV,                   MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           nullptr                                                          },
};
