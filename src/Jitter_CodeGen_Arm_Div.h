#ifndef _JITTER_CODEGEN_ARM_DIV_H_
#define _JITTER_CODEGEN_ARM_DIV_H_

static uint32 div_unsigned(uint32 a, uint32 b)
{
	return a / b;
}

static int32 div_signed(int32 a, int32 b)
{
	return a / b;
}

static uint32 mod_unsigned(uint32 a, uint32 b)
{
	return a % b;
}

static int32 mod_signed(int32 a, int32 b)
{
	return a % b;
}

template <bool isSigned>
void CCodeGen_Arm::Div_GenericTmp64RegReg_Quotient(CSymbol* dst)
{
	assert(dst->m_type == SYM_TEMPORARY64);
	
	if(isSigned)
	{
		LoadConstantInRegister(CArmAssembler::r2, reinterpret_cast<uint32>(&div_signed));
	}
	else
	{
		LoadConstantInRegister(CArmAssembler::r2, reinterpret_cast<uint32>(&div_unsigned));
	}
	m_assembler.Blx(CArmAssembler::r2);
	
	m_assembler.Str(CArmAssembler::r0, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 0));
}

template <bool isSigned>
void CCodeGen_Arm::Div_GenericTmp64RegReg_Remainder(CSymbol* dst)
{
	assert(dst->m_type == SYM_TEMPORARY64);
	
	if(isSigned)
	{
		LoadConstantInRegister(CArmAssembler::r2, reinterpret_cast<uint32>(&mod_signed));
	}
	else
	{
		LoadConstantInRegister(CArmAssembler::r2, reinterpret_cast<uint32>(&mod_unsigned));
	}
	m_assembler.Blx(CArmAssembler::r2);
	
	m_assembler.Str(CArmAssembler::r0, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 4));
}

template <bool isSigned>
void CCodeGen_Arm::Emit_DivTmp64RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_TEMPORARY64);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);
	
	//Quotient
	m_assembler.Mov(CArmAssembler::r0, g_registers[src1->m_valueLow]);
	m_assembler.Mov(CArmAssembler::r1, g_registers[src2->m_valueLow]);
	Div_GenericTmp64RegReg_Quotient<isSigned>(dst);
	
	//Remainder
	m_assembler.Mov(CArmAssembler::r0, g_registers[src1->m_valueLow]);
	m_assembler.Mov(CArmAssembler::r1, g_registers[src2->m_valueLow]);
	Div_GenericTmp64RegReg_Remainder<isSigned>(dst);	
}

template <bool isSigned>
void CCodeGen_Arm::Emit_DivTmp64RegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type  == SYM_TEMPORARY64);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);
	
	//Quotient
	m_assembler.Mov(CArmAssembler::r0, g_registers[src1->m_valueLow]);
	LoadConstantInRegister(CArmAssembler::r1, src2->m_valueLow);
	Div_GenericTmp64RegReg_Quotient<isSigned>(dst);
	
	//Remainder
	m_assembler.Mov(CArmAssembler::r0, g_registers[src1->m_valueLow]);
	LoadConstantInRegister(CArmAssembler::r1, src2->m_valueLow);
	Div_GenericTmp64RegReg_Remainder<isSigned>(dst);	
}

#endif
