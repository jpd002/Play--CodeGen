#pragma once

extern "C" uint32 CodeGen_Arm_div_unsigned(uint32 a, uint32 b)
{
	return a / b;
}

extern "C" int32 CodeGen_Arm_div_signed(int32 a, int32 b)
{
	return a / b;
}

extern "C" uint32 CodeGen_Arm_mod_unsigned(uint32 a, uint32 b)
{
	return a % b;
}

extern "C" int32 CodeGen_Arm_mod_signed(int32 a, int32 b)
{
	return a % b;
}

template <bool isSigned>
void CCodeGen_Arm::Div_GenericTmp64AnyAnySoft(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto divFct = isSigned ? 
		reinterpret_cast<uintptr_t>(&CodeGen_Arm_div_signed) : reinterpret_cast<uintptr_t>(&CodeGen_Arm_div_unsigned);
	auto modFct = isSigned ? 
		reinterpret_cast<uintptr_t>(&CodeGen_Arm_mod_signed) : reinterpret_cast<uintptr_t>(&CodeGen_Arm_mod_unsigned);

	assert(dst->m_type == SYM_TEMPORARY64);

	//Quotient
	{
		auto src1Reg = PrepareSymbolRegisterUse(src1, CArmAssembler::r0);
		auto src2Reg = PrepareSymbolRegisterUse(src2, CArmAssembler::r1);

		if(src1Reg != CArmAssembler::r0)
		{
			m_assembler.Mov(CArmAssembler::r0, src1Reg);
		}

		if(src2Reg != CArmAssembler::r1)
		{
			m_assembler.Mov(CArmAssembler::r1, src2Reg);
		}

		LoadConstantPtrInRegister(CArmAssembler::r2, divFct);
		m_assembler.Blx(CArmAssembler::r2);

		m_assembler.Str(CArmAssembler::r0, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 0));
	}

	//Remainder
	{
		auto src1Reg = PrepareSymbolRegisterUse(src1, CArmAssembler::r0);
		auto src2Reg = PrepareSymbolRegisterUse(src2, CArmAssembler::r1);

		if(src1Reg != CArmAssembler::r0)
		{
			m_assembler.Mov(CArmAssembler::r0, src1Reg);
		}

		if(src2Reg != CArmAssembler::r1)
		{
			m_assembler.Mov(CArmAssembler::r1, src2Reg);
		}
	
		LoadConstantPtrInRegister(CArmAssembler::r2, modFct);
		m_assembler.Blx(CArmAssembler::r2);
	
		m_assembler.Str(CArmAssembler::r0, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 4));
	}
}

template <bool isSigned>
void CCodeGen_Arm::Div_GenericTmp64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY64);

	auto src1Reg = PrepareSymbolRegisterUse(src1, CArmAssembler::r0);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CArmAssembler::r1);
	auto modReg0 = CArmAssembler::r1;	//Potentially overlaps src2Reg because it won't be needed after mul
	auto modReg1 = CArmAssembler::r2;
	auto resReg = CArmAssembler::r3;

	if(isSigned)
	{
		m_assembler.Sdiv(resReg, src1Reg, src2Reg);
		m_assembler.Smull(modReg0, modReg1, resReg, src2Reg);
	}
	else
	{
		m_assembler.Udiv(resReg, src1Reg, src2Reg);
		m_assembler.Umull(modReg0, modReg1, resReg, src2Reg);
	}

	m_assembler.Sub(modReg0, src1Reg, modReg0);

	m_assembler.Str(resReg, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 0));
	m_assembler.Str(modReg0, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 4));
}

template <bool isSigned>
void CCodeGen_Arm::Emit_DivTmp64AnyAny(const STATEMENT& statement)
{
	if(m_hasIntegerDiv)
	{
		Div_GenericTmp64AnyAny<isSigned>(statement);
	}
	else
	{
		Div_GenericTmp64AnyAnySoft<isSigned>(statement);
	}
}
