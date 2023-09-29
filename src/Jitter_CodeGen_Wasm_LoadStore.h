#pragma once

template <uint8 inst, uint8 align>
void CCodeGen_Wasm::Emit_Generic_LoadFromRef_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(inst);
	m_functionStream.Write8(align);
	m_functionStream.Write8(0x00);

	CommitSymbol(dst);
}

template <uint8 inst, uint8 align>
void CCodeGen_Wasm::Emit_Generic_LoadFromRef_MemVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	FRAMEWORK_MAYBE_UNUSED uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_ADD);

	m_functionStream.Write8(inst);
	m_functionStream.Write8(align);
	m_functionStream.Write8(0x00);

	CommitSymbol(dst);
}

template <uint8 inst, uint8 align>
void CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(inst);
	m_functionStream.Write8(align);
	m_functionStream.Write8(0x00);
}

template <uint8 inst, uint8 align>
void CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAnyAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	FRAMEWORK_MAYBE_UNUSED uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_ADD);

	PrepareSymbolUse(src3);

	m_functionStream.Write8(inst);
	m_functionStream.Write8(align);
	m_functionStream.Write8(0x00);
}
