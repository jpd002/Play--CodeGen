#include "Jitter_CodeGen_AArch64.h"
#include <stdexcept>

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

void CCodeGen_AArch64::LoadTemporary256ElementAddressInRegister(CAArch64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_TEMPORARY256);

	uint32 totalOffset = symbol->m_stackLocation + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Add(dstReg, CAArch64Assembler::xSP, totalOffset, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
}

CAArch64Assembler::REGISTERMD CCodeGen_AArch64::PrepareSymbolRegisterDefMd(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(symbol->m_valueLow < MAX_MDREGISTERS);
		return g_registersMd[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		return GetNextTempRegisterMd();
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch64Assembler::REGISTERMD CCodeGen_AArch64::PrepareSymbolRegisterUseMd(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(symbol->m_valueLow < MAX_MDREGISTERS);
		return g_registersMd[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
	{
		auto tempRegister = GetNextTempRegisterMd();
		LoadMemory128InRegister(tempRegister, symbol);
		return tempRegister;
	}
	break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_AArch64::CommitSymbolRegisterMd(CSymbol* symbol, CAArch64Assembler::REGISTERMD usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(usedRegister == g_registersMd[symbol->m_valueLow]);
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		StoreRegisterInMemory128(symbol, usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch64Assembler::REGISTERMD CCodeGen_AArch64::PrepareLiteralRegisterMd(const LITERAL128* literalPtr)
{
	for(int i = 0; i < MAX_TEMP_MD_REGS; i++)
	{
		if(m_tempRegisterMdLiteral[i] == literalPtr)
		{
			return g_tempRegistersMd[i];
		}
	}
	auto result = g_tempRegistersMd[m_nextTempRegisterMd];
	m_tempRegisterMdLiteral[m_nextTempRegisterMd] = literalPtr;
	if(literalPtr == &g_fpClampMask2)
	{
		m_assembler.Mvni_4s(result, 0x80, CAArch64Assembler::MOVI_4S_IMM_SHIFT_LSL16);
	}
	else
	{
		m_assembler.Ldr_Pc(result, *literalPtr);
	}
	m_nextTempRegisterMd++;
	m_nextTempRegisterMd %= MAX_TEMP_MD_REGS;
	return result;
}

void CCodeGen_AArch64::MdBlendRegisters(CAArch64Assembler::REGISTERMD dstReg, CAArch64Assembler::REGISTERMD srcReg, uint8 mask)
{
	//Try some aligned 64-bit inserts first
	for(unsigned int i = 0; i < 3; i += 2)
	{
		uint8 maskBits = (0x03 << i);
		if((mask & maskBits) == maskBits)
		{
			m_assembler.Ins_1d(dstReg, i / 2, srcReg, i / 2);
			mask &= ~maskBits;
		}
	}

	//Do remaining inserts
	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			m_assembler.Ins_1s(dstReg, i, srcReg, i);
		}
	}
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);

	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);
	auto src2Reg = PrepareSymbolRegisterUseMd(src2);

	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_VarVarVarRev(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);
	auto src2Reg = PrepareSymbolRegisterUseMd(src2);

	((m_assembler).*(MDOP::OpReg()))(dstReg, src2Reg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDSHIFTOP>
void CCodeGen_AArch64::Emit_Md_Shift_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);

	((m_assembler).*(MDSHIFTOP::OpReg()))(dstReg, src1Reg, src2->m_valueLow);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_ClampS_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);
	auto cst1Reg = PrepareLiteralRegisterMd(&g_fpClampMask1);
	auto cst2Reg = PrepareLiteralRegisterMd(&g_fpClampMask2);

	m_assembler.Smin_4s(dstReg, src1Reg, cst1Reg);
	m_assembler.Umin_4s(dstReg, dstReg, cst2Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_MakeClip_VarVarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();

	ResetTempRegisterMdState();

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto gtCmpReg = GetNextTempRegisterMd();
	auto ltCmpReg = GetNextTempRegisterMd();
	auto cstReg = GetNextTempRegisterMd();
	auto workReg = GetNextTempRegisterMd();

	const auto prepSymbolUse =
	    [&](CSymbol* symbol, CAArch64Assembler::REGISTERMD preferedReg) {
		    if(symbol->IsRegister())
		    {
			    return g_registersMd[symbol->m_valueLow];
		    }
		    else
		    {
			    LoadMemory128InRegister(preferedReg, symbol);
			    return preferedReg;
		    }
	    };

	{
		auto valueReg = prepSymbolUse(src1, workReg);

		//src1 > src2
		{
			auto boundReg = prepSymbolUse(src2, cstReg);
			m_assembler.Fcmgt_4s(gtCmpReg, valueReg, boundReg);
		}

		//src1 < src3
		{
			auto boundReg = prepSymbolUse(src3, cstReg);
			m_assembler.Fcmgt_4s(ltCmpReg, boundReg, valueReg);
		}
	}

	assert(ltCmpReg == (gtCmpReg + 1));

	static const LITERAL128 lit1(0xFF14FF04FF10FF00UL, 0xFFFFFFFFFF18FF08UL);
	static const LITERAL128 lit2(0x0008000400020001UL, 0x0000000000200010UL);

	m_assembler.Ldr_Pc(cstReg, lit1);
	m_assembler.Tbl(workReg, gtCmpReg, cstReg);
	m_assembler.Ldr_Pc(cstReg, lit2);
	m_assembler.And_16b(workReg, workReg, cstReg);
	m_assembler.Uaddlv_8h(workReg, workReg);
	m_assembler.Umov_1s(dstReg, workReg, 0);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_MakeSz_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	ResetTempRegisterMdState();

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);

	auto signReg = GetNextTempRegisterMd();
	auto zeroReg = GetNextTempRegisterMd();
	auto cstReg = GetNextTempRegisterMd();

	assert(zeroReg == (signReg + 1));

	m_assembler.Cmltz_4s(signReg, src1Reg);
	m_assembler.Fcmeqz_4s(zeroReg, src1Reg);

	LITERAL128 lit1(0xFF00FF04FF08FF0CUL, 0xFF10FF14FF18FF1CUL);
	LITERAL128 lit2(0x0080004000200010UL, 0x0008000400020001UL);

	m_assembler.Ldr_Pc(cstReg, lit1);
	m_assembler.Tbl(signReg, signReg, cstReg);
	m_assembler.Ldr_Pc(cstReg, lit2);
	m_assembler.And_16b(signReg, signReg, cstReg);
	m_assembler.Uaddlv_8h(signReg, signReg);
	m_assembler.Umov_1s(dstReg, signReg, 0);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_Mov_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(!dst->Equals(src1));

	m_assembler.Mov(g_registersMd[dst->m_valueLow], g_registersMd[src1->m_valueLow]);
}

void CCodeGen_AArch64::Emit_Md_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	LoadMemory128InRegister(g_registersMd[dst->m_valueLow], src1);
}

void CCodeGen_AArch64::Emit_Md_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	StoreRegisterInMemory128(dst, g_registersMd[src1->m_valueLow]);
}

void CCodeGen_AArch64::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpReg = GetNextTempRegisterMd();

	LoadMemory128InRegister(tmpReg, src1);
	StoreRegisterInMemory128(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_Md_LoadFromRef_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto dstReg = PrepareSymbolRegisterDefMd(dst);

	m_assembler.Ldr_1q(dstReg, addressReg, 0);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_LoadFromRef_VarVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto dstReg = PrepareSymbolRegisterDefMd(dst);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x10000))
	{
		m_assembler.Ldr_1q(dstReg, addressReg, scaledIndex);
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		m_assembler.Ldr_1q(dstReg, addressReg, static_cast<CAArch64Assembler::REGISTER64>(indexReg), (scale == 0x10));
	}

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_StoreAtRef_VarVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto valueReg = PrepareSymbolRegisterUseMd(src2);

	m_assembler.Str_1q(valueReg, addressReg, 0);
}

void CCodeGen_AArch64::Emit_Md_StoreAtRef_VarAnyVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto valueReg = PrepareSymbolRegisterUseMd(src3);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x10000))
	{
		m_assembler.Str_1q(valueReg, addressReg, scaledIndex);
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		m_assembler.Str_1q(valueReg, addressReg, static_cast<CAArch64Assembler::REGISTER64>(indexReg), (scale == 0x10));
	}
}

void CCodeGen_AArch64::Emit_Md_LoadFromRefMasked_VarVarAnyVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	auto mask = static_cast<uint8>(statement.jmpCondition);
	uint8 scale = 1;

	assert(dst->Equals(src3));

	auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto src3Reg = PrepareSymbolRegisterUseMd(src3);
	auto tmpReg = GetNextTempRegisterMd();

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x10000))
	{
		m_assembler.Ldr_1q(tmpReg, addressReg, scaledIndex);
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		m_assembler.Ldr_1q(tmpReg, addressReg, static_cast<CAArch64Assembler::REGISTER64>(indexReg), (scale == 0x10));
	}

	MdBlendRegisters(src3Reg, tmpReg, mask);

	//This is only valid if dst == src3
	CommitSymbolRegisterMd(dst, src3Reg);
}

void CCodeGen_AArch64::Emit_Md_StoreAtRefMasked_VarAnyVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 mask = static_cast<uint8>(statement.jmpCondition);
	uint8 scale = 1;

	auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto valueReg = PrepareSymbolRegisterUseMd(src3);
	auto tmpReg = GetNextTempRegisterMd();
	auto indexReg = GetNextTempRegister();

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x10000))
	{
		m_assembler.Ldr_1q(tmpReg, addressReg, scaledIndex);
	}
	else
	{
		indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		m_assembler.Ldr_1q(tmpReg, addressReg, static_cast<CAArch64Assembler::REGISTER64>(indexReg), (scale == 0x10));
	}

	MdBlendRegisters(tmpReg, valueReg, mask);

	if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x10000))
	{
		m_assembler.Str_1q(tmpReg, addressReg, scaledIndex);
	}
	else
	{
		m_assembler.Str_1q(tmpReg, addressReg, static_cast<CAArch64Assembler::REGISTER64>(indexReg), (scale == 0x10));
	}
}

void CCodeGen_AArch64::Emit_Md_MovMasked_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->Equals(src1));

	auto mask = static_cast<uint8>(statement.jmpCondition);

	auto src1Reg = PrepareSymbolRegisterUseMd(src1);
	auto src2Reg = PrepareSymbolRegisterUseMd(src2);

	MdBlendRegisters(src1Reg, src2Reg, mask);

	//This is only valid if dst == src1
	CommitSymbolRegisterMd(dst, src1Reg);
}

void CCodeGen_AArch64::Emit_Md_ExpandW_VarReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);

	m_assembler.Dup_4s(dstReg, g_registers[src1->m_valueLow]);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_ExpandW_VarMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = GetNextTempRegister();

	LoadMemoryInRegister(src1Reg, src1);

	m_assembler.Dup_4s(dstReg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_ExpandW_VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = GetNextTempRegister();

	switch(src1->m_valueLow)
	{
	case 0:
		m_assembler.Movi_4s(dstReg, 0, CAArch64Assembler::MOVI_4S_IMM_SHIFT_LSL0);
		break;
	case 0x3F800000:
		//1.0f
		m_assembler.Fmov_4s(dstReg, 0x70);
		break;
	case 0x41800000:
		//16.0f
		m_assembler.Fmov_4s(dstReg, 0x30);
		break;
	case 0xBF800000:
		//-1.0f;
		m_assembler.Fmov_4s(dstReg, 0xF0);
		break;
	default:
		if((src1->m_valueLow & 0x000000FF) == src1->m_valueLow)
		{
			uint8 imm = static_cast<uint8>(src1->m_valueLow);
			m_assembler.Movi_4s(dstReg, imm, CAArch64Assembler::MOVI_4S_IMM_SHIFT_LSL0);
		}
		else if((src1->m_valueLow & 0xFF000000) == src1->m_valueLow)
		{
			uint8 imm = static_cast<uint8>(src1->m_valueLow >> 24);
			m_assembler.Movi_4s(dstReg, imm, CAArch64Assembler::MOVI_4S_IMM_SHIFT_LSL24);
		}
		else
		{
			LoadConstantInRegister(src1Reg, src1->m_valueLow);
			m_assembler.Dup_4s(dstReg, src1Reg);
		}
		break;
	}

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_ExpandW_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);
	assert(src2->m_valueLow < 4);

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);

	m_assembler.Dup_4s(dstReg, src1Reg, src2->m_valueLow);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_PackHB_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);
	auto src2Reg = PrepareSymbolRegisterUseMd(src2);

	if(dstReg == src1Reg)
	{
		auto tmpReg = GetNextTempRegisterMd();
		m_assembler.Xtn1_8b(tmpReg, src2Reg);
		m_assembler.Xtn2_16b(tmpReg, src1Reg);
		m_assembler.Mov(dstReg, tmpReg);
	}
	else
	{
		m_assembler.Xtn1_8b(dstReg, src2Reg);
		m_assembler.Xtn2_16b(dstReg, src1Reg);
	}

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_PackWH_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);
	auto src2Reg = PrepareSymbolRegisterUseMd(src2);

	if(dstReg == src1Reg)
	{
		auto tmpReg = GetNextTempRegisterMd();
		m_assembler.Xtn1_4h(tmpReg, src2Reg);
		m_assembler.Xtn2_8h(tmpReg, src1Reg);
		m_assembler.Mov(dstReg, tmpReg);
	}
	else
	{
		m_assembler.Xtn1_4h(dstReg, src2Reg);
		m_assembler.Xtn2_8h(dstReg, src1Reg);
	}

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_MergeTo256_MemVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY256);

	auto dstLoAddrReg = GetNextTempRegister64();
	auto dstHiAddrReg = GetNextTempRegister64();
	auto src1Reg = PrepareSymbolRegisterUseMd(src1);
	auto src2Reg = PrepareSymbolRegisterUseMd(src2);

	LoadTemporary256ElementAddressInRegister(dstLoAddrReg, dst, 0x00);
	LoadTemporary256ElementAddressInRegister(dstHiAddrReg, dst, 0x10);

	m_assembler.St1_4s(src1Reg, dstLoAddrReg);
	m_assembler.St1_4s(src2Reg, dstHiAddrReg);
}

void CCodeGen_AArch64::Emit_Md_Srl256_VarMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY256);
	assert(src2->m_type == SYM_CONSTANT);

	auto src1AddrReg = GetNextTempRegister64();
	auto dstReg = PrepareSymbolRegisterDefMd(dst);

	uint32 offset = (src2->m_valueLow & 0x7F) / 8;
	LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, offset);

	m_assembler.Ld1_4s(dstReg, src1AddrReg);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_Srl256_VarMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY256);

	auto offsetRegister = GetNextTempRegister();
	auto src1AddrReg = GetNextTempRegister64();
	auto src2Register = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

	auto dstReg = PrepareSymbolRegisterDefMd(dst);

	LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, 0);

	//Compute offset and modify address
	LOGICAL_IMM_PARAMS logicalImmParams;
	FRAMEWORK_MAYBE_UNUSED bool result = TryGetLogicalImmParams(0x7F, logicalImmParams);
	assert(result);
	m_assembler.And(offsetRegister, src2Register, logicalImmParams.n, logicalImmParams.immr, logicalImmParams.imms);
	m_assembler.Lsr(offsetRegister, offsetRegister, 3);
	m_assembler.Add(src1AddrReg, src1AddrReg, static_cast<CAArch64Assembler::REGISTER64>(offsetRegister));

	m_assembler.Ld1_4s(dstReg, src1AddrReg);

	CommitSymbolRegisterMd(dst, dstReg);
}

// clang-format off
CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_mdConstMatchers[] =
{
	{ OP_MD_ADD_B,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDB>                  },
	{ OP_MD_ADD_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDH>                  },
	{ OP_MD_ADD_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDW>                  },

	{ OP_MD_ADDUS_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDBUS>                },
	{ OP_MD_ADDUS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDHUS>                },
	{ OP_MD_ADDUS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDWUS>                },

	{ OP_MD_ADDSS_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDBSS>                },
	{ OP_MD_ADDSS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDHSS>                },
	{ OP_MD_ADDSS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDWSS>                },
	
	{ OP_MD_SUB_B,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBB>                  },
	{ OP_MD_SUB_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBH>                  },
	{ OP_MD_SUB_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBW>                  },
	
	{ OP_MD_SUBUS_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBBUS>                },
	{ OP_MD_SUBUS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBHUS>                },
	{ OP_MD_SUBUS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBWUS>                },

	{ OP_MD_SUBSS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBHSS>                },
	{ OP_MD_SUBSS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBWSS>                },

	{ OP_MD_CLAMP_S,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_ClampS_VarVar                         },

	{ OP_MD_CMPEQ_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_CMPEQB>                },
	{ OP_MD_CMPEQ_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_CMPEQH>                },
	{ OP_MD_CMPEQ_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_CMPEQW>                },

	{ OP_MD_CMPGT_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_CMPGTB>                },
	{ OP_MD_CMPGT_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_CMPGTH>                },
	{ OP_MD_CMPGT_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_CMPGTW>                },

	{ OP_MD_MIN_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MINH>                  },
	{ OP_MD_MIN_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MINW>                  },

	{ OP_MD_MAX_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MAXH>                  },
	{ OP_MD_MAX_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MAXW>                  },

	{ OP_MD_ADD_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDS>                  },
	{ OP_MD_SUB_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBS>                  },
	{ OP_MD_MUL_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MULS>                  },
	{ OP_MD_DIV_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_DIVS>                  },

	{ OP_MD_ABS_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVar<MDOP_ABSS>                     },
	{ OP_MD_NEG_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVar<MDOP_NEGS>                     },
	{ OP_MD_MIN_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MINS>                  },
	{ OP_MD_MAX_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MAXS>                  },

	{ OP_MD_CMPLT_S,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_CMPGTS>             },
	{ OP_MD_CMPGT_S,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_CMPGTS>                },
	
	{ OP_MD_AND,                MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_AND>                   },
	{ OP_MD_OR,                 MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_OR>                    },
	{ OP_MD_XOR,                MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_XOR>                   },
	{ OP_MD_NOT,                MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVar<MDOP_NOT>                      },
	
	{ OP_MD_SLLH,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SLLH>            },
	{ OP_MD_SLLW,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SLLW>            },

	{ OP_MD_SRLH,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SRLH>            },
	{ OP_MD_SRLW,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SRLW>            },

	{ OP_MD_SRAH,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SRAH>            },
	{ OP_MD_SRAW,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SRAW>            },
	
	{ OP_MD_SRL256,             MATCH_VARIABLE128,    MATCH_MEMORY256,      MATCH_VARIABLE,         MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Srl256_VarMemVar                      },
	{ OP_MD_SRL256,             MATCH_VARIABLE128,    MATCH_MEMORY256,      MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Srl256_VarMemCst                      },

	{ OP_MD_MAKECLIP,           MATCH_VARIABLE,       MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_VARIABLE128, &CCodeGen_AArch64::Emit_Md_MakeClip_VarVarVarVar         },
	{ OP_MD_MAKESZ,             MATCH_VARIABLE,       MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL,         &CCodeGen_AArch64::Emit_Md_MakeSz_VarVar                 },
	
	{ OP_MD_TOSINGLE,           MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVar<MDOP_TOSINGLE>                 },
	{ OP_MD_TOWORD_TRUNCATE,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVar<MDOP_TOWORD>                   },

	{ OP_LOADFROMREF,           MATCH_VARIABLE128,    MATCH_VAR_REF,        MATCH_NIL,              MATCH_NIL,         &CCodeGen_AArch64::Emit_Md_LoadFromRef_VarVar            },
	{ OP_LOADFROMREF,           MATCH_VARIABLE128,    MATCH_VAR_REF,        MATCH_ANY32,            MATCH_NIL,         &CCodeGen_AArch64::Emit_Md_LoadFromRef_VarVarAny         },
	{ OP_STOREATREF,            MATCH_NIL,            MATCH_VAR_REF,        MATCH_VARIABLE128,      MATCH_NIL,         &CCodeGen_AArch64::Emit_Md_StoreAtRef_VarVar             },
	{ OP_STOREATREF,            MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY32,            MATCH_VARIABLE128, &CCodeGen_AArch64::Emit_Md_StoreAtRef_VarAnyVar          },

	{ OP_MD_LOADFROMREF_MASKED, MATCH_VARIABLE128,    MATCH_VAR_REF,        MATCH_ANY32,            MATCH_VARIABLE128, &CCodeGen_AArch64::Emit_Md_LoadFromRefMasked_VarVarAnyVar },
	{ OP_MD_STOREATREF_MASKED,  MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY32,            MATCH_VARIABLE128, &CCodeGen_AArch64::Emit_Md_StoreAtRefMasked_VarAnyVar     },
	
	{ OP_MD_MOV_MASKED,         MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_MovMasked_VarVarVar                   },

	{ OP_MD_EXPAND_W,           MATCH_VARIABLE128,    MATCH_REGISTER,       MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_ExpandW_VarReg                        },
	{ OP_MD_EXPAND_W,           MATCH_VARIABLE128,    MATCH_MEMORY,         MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_ExpandW_VarMem                        },
	{ OP_MD_EXPAND_W,           MATCH_VARIABLE128,    MATCH_CONSTANT,       MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_ExpandW_VarCst                        },
	{ OP_MD_EXPAND_W,           MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_AArch64::Emit_Md_ExpandW_VarVarCst                     },

	{ OP_MD_PACK_HB,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_PackHB_VarVarVar                      },
	{ OP_MD_PACK_WH,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_PackWH_VarVarVar                      },

	{ OP_MD_UNPACK_LOWER_BH,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_LOWER_BH>    },
	{ OP_MD_UNPACK_LOWER_HW,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_LOWER_HW>    },
	{ OP_MD_UNPACK_LOWER_WD,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_LOWER_WD>    },
	
	{ OP_MD_UNPACK_UPPER_BH,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_UPPER_BH>    },
	{ OP_MD_UNPACK_UPPER_HW,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_UPPER_HW>    },
	{ OP_MD_UNPACK_UPPER_WD,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_UPPER_WD>    },

	{ OP_MOV,                   MATCH_REGISTER128,    MATCH_REGISTER128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Mov_RegReg                            },
	{ OP_MOV,                   MATCH_REGISTER128,    MATCH_MEMORY128,      MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Mov_RegMem                            },
	{ OP_MOV,                   MATCH_MEMORY128,      MATCH_REGISTER128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Mov_MemReg                            },
	{ OP_MOV,                   MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,              MATCH_NIL, &CCodeGen_AArch64::Emit_Md_Mov_MemMem                            },
	
	{ OP_MERGETO256,            MATCH_MEMORY256,      MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_AArch64::Emit_MergeTo256_MemVarVar                     },
	
	{ OP_MOV,                   MATCH_NIL,            MATCH_NIL,            MATCH_NIL,              MATCH_NIL, nullptr                                                          },
};
// clang-format on
