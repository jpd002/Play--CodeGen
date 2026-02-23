#pragma once

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);

	if(m_cpuFeatures.hasBmi2)
	{
		auto src2Register = PrepareSymbolRegisterUse(src2, CX86Assembler::rCX);

		((m_assembler).*(SHIFTOP::OpVarBmi()))(
		    m_registers[dst->m_valueLow],
		    CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), src2Register);
	}
	else
	{
		m_assembler.MovEd(CX86Assembler::rCX, MakeVariableSymbolAddress(src2));

		if(!dst->Equals(src1))
		{
			m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
		}

		((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
	}
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), static_cast<uint8>(src2->m_valueLow));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);

	if(m_cpuFeatures.hasBmi2)
	{
		auto src2Register = PrepareSymbolRegisterUse(src2, CX86Assembler::rCX);

		((m_assembler).*(SHIFTOP::OpVarBmi()))(
		    m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1), src2Register);
	}
	else
	{
		m_assembler.MovEd(CX86Assembler::rCX, MakeVariableSymbolAddress(src2));
		m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
		((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
	}
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), static_cast<uint8>(src2->m_valueLow));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemRegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	auto dstRegister = CX86Assembler::rAX;

	if(m_cpuFeatures.hasBmi2)
	{
		auto src2Register = PrepareSymbolRegisterUse(src2, CX86Assembler::rCX);

		((m_assembler).*(SHIFTOP::OpVarBmi()))(
		    dstRegister, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), src2Register);
	}
	else
	{
		m_assembler.MovEd(dstRegister, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
		m_assembler.MovEd(CX86Assembler::rCX, MakeVariableSymbolAddress(src2));
		((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(dstRegister));
	}

	m_assembler.MovGd(MakeMemorySymbolAddress(dst), dstRegister);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemRegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rCX, MakeVariableSymbolAddress(src2));

	if(dst->Equals(src1))
	{
		((m_assembler).*(SHIFTOP::OpVar()))(MakeMemorySymbolAddress(dst));
	}
	else
	{
		m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
		((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
		m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
	}
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	if(dst->Equals(src1))
	{
		((m_assembler).*(SHIFTOP::OpCst()))(MakeMemorySymbolAddress(dst), static_cast<uint8>(src2->m_valueLow));
	}
	else
	{
		m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
		((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
		m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
	}
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_VarCstVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	auto dstRegister = PrepareSymbolRegisterDef(dst, CX86Assembler::rAX);

	if(m_cpuFeatures.hasBmi2)
	{
		auto src1Register = PrepareSymbolRegisterUse(src1, CX86Assembler::rDX);
		auto src2Register = PrepareSymbolRegisterUse(src2, CX86Assembler::rCX);

		((m_assembler).*(SHIFTOP::OpVarBmi()))(
		    dstRegister, CX86Assembler::MakeRegisterAddress(src1Register), src2Register);
	}
	else
	{
		m_assembler.MovEd(CX86Assembler::rCX, MakeVariableSymbolAddress(src2));
		//This needs to happen here since dst could be equal to src2
		m_assembler.MovId(dstRegister, src1->m_valueLow);

		((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(dstRegister));
	}

	CommitSymbolRegister(dst, dstRegister);
}

// clang-format off
#define SHIFT_CONST_MATCHERS(SHIFTOP_CST, SHIFTOP) \
	{ SHIFTOP_CST, MATCH_REGISTER, MATCH_REGISTER, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_Shift_RegRegVar<SHIFTOP> }, \
	{ SHIFTOP_CST, MATCH_REGISTER, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Shift_RegRegCst<SHIFTOP> }, \
	{ SHIFTOP_CST, MATCH_REGISTER, MATCH_MEMORY,   MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_Shift_RegMemVar<SHIFTOP> }, \
	{ SHIFTOP_CST, MATCH_REGISTER, MATCH_MEMORY,   MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Shift_RegMemCst<SHIFTOP> }, \
\
	{ SHIFTOP_CST, MATCH_MEMORY, MATCH_REGISTER, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_Shift_MemRegVar<SHIFTOP> }, \
	{ SHIFTOP_CST, MATCH_MEMORY, MATCH_REGISTER, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Shift_MemRegCst<SHIFTOP> }, \
	{ SHIFTOP_CST, MATCH_MEMORY, MATCH_MEMORY,   MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_Shift_MemMemVar<SHIFTOP> }, \
	{ SHIFTOP_CST, MATCH_MEMORY, MATCH_MEMORY,   MATCH_CONSTANT, MATCH_NIL, &CCodeGen_x86::Emit_Shift_MemMemCst<SHIFTOP> }, \
\
	{ SHIFTOP_CST, MATCH_VARIABLE, MATCH_CONSTANT, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_x86::Emit_Shift_VarCstVar<SHIFTOP> },
// clang-format on
