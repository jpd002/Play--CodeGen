#include "Jitter_CodeGen_AArch64.h"

using namespace Jitter;

void CCodeGen_AArch64::LoadMemory128InRegister(CAArch64Assembler::REGISTERMD dstReg, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		m_assembler.Ldr_1q(dstReg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_TEMPORARY128:
		m_assembler.Ldr_1q(dstReg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInMemory128(CSymbol* symbol, CAArch64Assembler::REGISTERMD srcReg)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		m_assembler.Str_1q(srcReg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_TEMPORARY128:
		m_assembler.Str_1q(srcReg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(0);
		break;
	}
}

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
void CCodeGen_AArch64::Emit_Md_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();

	LoadMemory128InRegister(src1Reg, src1);
	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
	StoreRegisterInMemory128(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto src2Reg = GetNextTempRegisterMd();

	LoadMemory128InRegister(src1Reg, src1);
	LoadMemory128InRegister(src2Reg, src2);
	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);
	StoreRegisterInMemory128(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_MemMemMemRev(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto src2Reg = GetNextTempRegisterMd();

	LoadMemory128InRegister(src1Reg, src1);
	LoadMemory128InRegister(src2Reg, src2);
	((m_assembler).*(MDOP::OpReg()))(dstReg, src2Reg, src1Reg);
	StoreRegisterInMemory128(dst, dstReg);
}

template <typename MDSHIFTOP>
void CCodeGen_AArch64::Emit_Md_Shift_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();

	LoadMemory128InRegister(src1Reg, src1);
	((m_assembler).*(MDSHIFTOP::OpReg()))(dstReg, src1Reg, src2->m_valueLow);
	StoreRegisterInMemory128(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_Test_VarMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1Reg = GetNextTempRegisterMd();
	auto tmpValueReg = GetNextTempRegister();
	auto tmpCmpReg = GetNextTempRegisterMd();
	
	LoadMemory128InRegister(src1Reg, src1);

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());

	((m_assembler).*(MDOP::OpReg()))(tmpCmpReg, src1Reg);
	
	m_assembler.Eor(dstReg, dstReg, dstReg);
	for(unsigned int i = 0; i < 4; i++)
	{
		LOGICAL_IMM_PARAMS logicalImmParams;
		bool result = TryGetLogicalImmParams((1 << i), logicalImmParams);
		assert(result);

		m_assembler.Umov_1s(tmpValueReg, tmpCmpReg, 3 - i);
		m_assembler.And(tmpValueReg, tmpValueReg,
			logicalImmParams.n, logicalImmParams.immr, logicalImmParams.imms);
		m_assembler.Orr(dstReg, dstReg, tmpValueReg);
	}

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpReg = GetNextTempRegisterMd();
	
	LoadMemory128InRegister(tmpReg, src1);
	StoreRegisterInMemory128(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_Md_Not_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpReg = GetNextTempRegisterMd();
	auto zeroReg = GetNextTempRegisterMd();
	
	LoadMemory128InRegister(tmpReg, src1);
	m_assembler.Eor_16b(zeroReg, zeroReg, zeroReg);
	m_assembler.Orn_16b(tmpReg, zeroReg, tmpReg);
	StoreRegisterInMemory128(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_Md_LoadFromRef_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1AddrReg = GetNextTempRegister64();
	auto dstReg = GetNextTempRegisterMd();

	LoadMemoryReferenceInRegister(src1AddrReg, src1);

	m_assembler.Ldr_1q(dstReg, src1AddrReg, 0);
	StoreRegisterInMemory128(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_StoreAtRef_MemMem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto src1AddrReg = GetNextTempRegister64();
	auto src2Reg = GetNextTempRegisterMd();

	LoadMemoryReferenceInRegister(src1AddrReg, src1);
	LoadMemory128InRegister(src2Reg, src2);
	
	m_assembler.Str_1q(src2Reg, src1AddrReg, 0);
}

void CCodeGen_AArch64::Emit_Md_MovMasked_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->Equals(src1));

	auto mask = static_cast<uint8>(statement.jmpCondition);

	auto dstReg = GetNextTempRegisterMd();
	auto src2Reg = GetNextTempRegisterMd();

	LoadMemory128InRegister(dstReg, dst);
	LoadMemory128InRegister(src2Reg, src2);
	
	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			m_assembler.Ins_1s(dstReg, i, src2Reg, i);
		}
	}

	StoreRegisterInMemory128(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_Expand_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpReg = GetNextTempRegisterMd();

	m_assembler.Dup_4s(tmpReg, g_registers[src1->m_valueLow]);
	StoreRegisterInMemory128(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_Md_Expand_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1Reg = GetNextTempRegister();
	auto tmpReg = GetNextTempRegisterMd();

	LoadMemoryInRegister(src1Reg, src1);

	m_assembler.Dup_4s(tmpReg, src1Reg);
	StoreRegisterInMemory128(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_Md_Expand_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1Reg = GetNextTempRegister();
	auto tmpReg = GetNextTempRegisterMd();

	LoadConstantInRegister(src1Reg, src1->m_valueLow);

	m_assembler.Dup_4s(tmpReg, src1Reg);
	StoreRegisterInMemory128(dst, tmpReg);
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

	{ OP_MD_CMPEQ_W,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_CMPEQW>                },

	{ OP_MD_CMPGT_H,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_CMPGTH>                },

	{ OP_MD_ADD_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_ADDS>                  },
	{ OP_MD_SUB_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_SUBS>                  },
	{ OP_MD_MUL_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_MULS>                  },
	{ OP_MD_DIV_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_DIVS>                  },

	{ OP_MD_ABS_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_MemMem<MDOP_ABSS>                     },
	{ OP_MD_MIN_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_MINS>                  },
	{ OP_MD_MAX_S,              MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_MAXS>                  },

	{ OP_MD_AND,                MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_AND>                   },
	{ OP_MD_OR,                 MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_OR>                    },
	{ OP_MD_XOR,                MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMem<MDOP_XOR>                   },
	
	{ OP_MD_NOT,                MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_Not_MemMem                            },
	
	{ OP_MD_SLLH,               MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Md_Shift_MemMemCst<MDOP_SLLH>            },
	{ OP_MD_SLLW,               MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Md_Shift_MemMemCst<MDOP_SLLW>            },

	{ OP_MD_SRLH,               MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Md_Shift_MemMemCst<MDOP_SRLH>            },
	{ OP_MD_SRLW,               MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Md_Shift_MemMemCst<MDOP_SRLW>            },

	{ OP_MD_SRAH,               MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Md_Shift_MemMemCst<MDOP_SRAH>            },
	{ OP_MD_SRAW,               MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Md_Shift_MemMemCst<MDOP_SRAW>            },
	
	{ OP_MD_ISNEGATIVE,         MATCH_VARIABLE,       MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_Test_VarMem<MDOP_CMPLTZS>             },
	{ OP_MD_ISZERO,             MATCH_VARIABLE,       MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_Test_VarMem<MDOP_CMPEQZS>             },
	
	{ OP_MD_TOSINGLE,           MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_MemMem<MDOP_TOSINGLE>                 },
	{ OP_MD_TOWORD_TRUNCATE,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_MemMem<MDOP_TOWORD>                   },

	{ OP_LOADFROMREF,           MATCH_MEMORY128,      MATCH_MEM_REF,        MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_LoadFromRef_MemMem                    },
	{ OP_STOREATREF,            MATCH_NIL,            MATCH_MEM_REF,        MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_StoreAtRef_MemMem                     },

	{ OP_MD_MOV_MASKED,         MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MovMasked_MemMemMem                   },

	{ OP_MD_EXPAND,             MATCH_MEMORY128,      MATCH_REGISTER,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_Expand_MemReg                         },
	{ OP_MD_EXPAND,             MATCH_MEMORY128,      MATCH_MEMORY,         MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_Expand_MemMem                         },
	{ OP_MD_EXPAND,             MATCH_MEMORY128,      MATCH_CONSTANT,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_Expand_MemCst                         },

	{ OP_MD_UNPACK_LOWER_BH,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_BH>    },
	{ OP_MD_UNPACK_LOWER_HW,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_HW>    },
	{ OP_MD_UNPACK_LOWER_WD,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_LOWER_WD>    },
	
	{ OP_MD_UNPACK_UPPER_BH,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_UPPER_BH>    },
	{ OP_MD_UNPACK_UPPER_HW,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_UPPER_HW>    },
	{ OP_MD_UNPACK_UPPER_WD,    MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     &CCodeGen_AArch64::Emit_Md_MemMemMemRev<MDOP_UNPACK_UPPER_WD>    },

	{ OP_MOV,                   MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Md_Mov_MemMem                            },
	
	{ OP_MOV,                   MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           nullptr                                                          },
};
