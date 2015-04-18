#include "Jitter_CodeGen_Arm.h"

using namespace Jitter;

void CCodeGen_Arm::LoadMemory128AddressInRegister(CArmAssembler::REGISTER dstReg, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		LoadRelative128AddressInRegister(dstReg, symbol);
		break;
	case SYM_TEMPORARY128:
		LoadTemporary128AddressInRegister(dstReg, symbol);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_Arm::LoadRelative128AddressInRegister(CArmAssembler::REGISTER dstReg, CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE128);

	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(!TryGetAluImmediateParams(symbol->m_valueLow, immediate, shiftAmount))
	{
		throw std::runtime_error("Failed to build immediate for symbol.");
	}
	m_assembler.Add(dstReg, g_baseRegister, CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
}

void CCodeGen_Arm::LoadTemporary128AddressInRegister(CArmAssembler::REGISTER dstReg, CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY128);

	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(!TryGetAluImmediateParams(symbol->m_stackLocation + m_stackLevel, immediate, shiftAmount))
	{
		throw std::runtime_error("Failed to build immediate for symbol.");
	}
	m_assembler.Add(dstReg, CArmAssembler::rSP, CArmAssembler::MakeImmediateAluOperand(immediate, shiftAmount));
}

void CCodeGen_Arm::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CArmAssembler::r0;
	auto src1AddrReg = CArmAssembler::r1;
	auto tmpReg = CArmAssembler::q0;
	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	m_assembler.Vld1_32x4(tmpReg, src1AddrReg);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

void CCodeGen_Arm::Emit_Md_Not_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CArmAssembler::r0;
	auto src1AddrReg = CArmAssembler::r1;
	auto zeroReg = CArmAssembler::q0;
	auto tmpReg = CArmAssembler::q1;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);

	m_assembler.Vld1_32x4(tmpReg, src1AddrReg);
	m_assembler.Veor(zeroReg, zeroReg, zeroReg);
	m_assembler.Vorn(tmpReg, zeroReg, tmpReg);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

template <typename MDOP>
void CCodeGen_Arm::Emit_Md_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CArmAssembler::r0;
	auto src1AddrReg = CArmAssembler::r1;
	auto dstReg = CArmAssembler::q0;
	auto src1Reg = CArmAssembler::q1;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);

	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

template <typename MDOP>
void CCodeGen_Arm::Emit_Md_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = CArmAssembler::r0;
	auto src1AddrReg = CArmAssembler::r1;
	auto src2AddrReg = CArmAssembler::r2;
	auto dstReg = CArmAssembler::q0;
	auto src1Reg = CArmAssembler::q1;
	auto src2Reg = CArmAssembler::q2;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
	m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

void CCodeGen_Arm::Emit_Md_Expand_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CArmAssembler::r0;
	auto src1AddrReg = CArmAssembler::r1;
	auto tmpReg = CArmAssembler::q0;

	LoadMemory128AddressInRegister(dstAddrReg, dst);

	m_assembler.Vdup(tmpReg, g_registers[src1->m_valueLow]);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

void CCodeGen_Arm::Emit_Md_Expand_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CArmAssembler::r0;
	auto src1Reg = CArmAssembler::r1;
	auto tmpReg = CArmAssembler::q0;

	LoadMemoryInRegister(src1Reg, src1);
	LoadMemory128AddressInRegister(dstAddrReg, dst);

	m_assembler.Vdup(tmpReg, src1Reg);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

void CCodeGen_Arm::Emit_Md_Expand_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CArmAssembler::r0;
	auto src1Reg = CArmAssembler::r1;
	auto tmpReg = CArmAssembler::q0;

	LoadConstantInRegister(src1Reg, src1->m_valueLow);
	LoadMemory128AddressInRegister(dstAddrReg, dst);

	m_assembler.Vdup(tmpReg, src1Reg);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

CCodeGen_Arm::CONSTMATCHER CCodeGen_Arm::g_mdConstMatchers[] = 
{
	{ OP_MD_ADD_W,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<MDOP_ADDW>					},

	{ OP_MD_SUB_B,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<MDOP_SUBB>					},
	{ OP_MD_SUB_W,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<MDOP_SUBW>					},

	{ OP_MD_ADD_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<MDOP_ADDS>					},
	{ OP_MD_SUB_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<MDOP_SUBS>					},
	{ OP_MD_MUL_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<MDOP_MULS>					},

	{ OP_MD_ABS_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_Arm::Emit_Md_MemMem<MDOP_ABSS>					},
	{ OP_MD_MIN_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<FPUMDOP_MIN>				},
	{ OP_MD_MAX_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<FPUMDOP_MAX>				},

	{ OP_MD_AND,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<MDOP_AND>					},
	{ OP_MD_OR,					MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<MDOP_OR>					},
	{ OP_MD_XOR,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_Arm::Emit_Md_MemMemMem<MDOP_XOR>					},

	{ OP_MD_NOT,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_Arm::Emit_Md_Not_MemMem							},

	{ OP_MOV,					MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_Arm::Emit_Md_Mov_MemMem							},

	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_REGISTER,				MATCH_NIL,				&CCodeGen_Arm::Emit_Md_Expand_MemReg						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_MEMORY,				MATCH_NIL,				&CCodeGen_Arm::Emit_Md_Expand_MemMem						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_CONSTANT,				MATCH_NIL,				&CCodeGen_Arm::Emit_Md_Expand_MemCst						},

	{ OP_MOV,					MATCH_NIL,					MATCH_NIL,					MATCH_NIL,				NULL														},
};
