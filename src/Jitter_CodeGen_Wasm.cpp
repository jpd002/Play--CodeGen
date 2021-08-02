#include "Jitter_CodeGen_Wasm.h"

#include <stdexcept>

#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

// clang-format off
CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_constMatchers[] =
{
	{ OP_MOV,            MATCH_RELATIVE,       MATCH_RELATIVE,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Mov_RelRel                             },

	{ OP_LABEL,          MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::MarkLabel                                   },

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      nullptr                                                     },
};
// clang-format on

CCodeGen_Wasm::CCodeGen_Wasm()
{
	const auto copyMatchers =
	    [this](const CONSTMATCHER* constMatchers) {
		    for(auto* constMatcher = constMatchers; constMatcher->emitter != nullptr; constMatcher++)
		    {
			    MATCHER matcher;
			    matcher.op = constMatcher->op;
			    matcher.dstType = constMatcher->dstType;
			    matcher.src1Type = constMatcher->src1Type;
			    matcher.src2Type = constMatcher->src2Type;
			    matcher.src3Type = constMatcher->src3Type;
			    matcher.emitter = std::bind(constMatcher->emitter, this, std::placeholders::_1);
			    m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
		    }
	    };

	copyMatchers(g_constMatchers);
}

void CCodeGen_Wasm::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	CWasmModuleBuilder moduleBuilder;

	m_functionStream.ResetBuffer();

	for(const auto& statement : statements)
	{
		bool found = false;
		auto begin = m_matchers.lower_bound(statement.op);
		auto end = m_matchers.upper_bound(statement.op);

		for(auto matchIterator(begin); matchIterator != end; matchIterator++)
		{
			const auto& matcher(matchIterator->second);
			if(!SymbolMatches(matcher.dstType, statement.dst)) continue;
			if(!SymbolMatches(matcher.src1Type, statement.src1)) continue;
			if(!SymbolMatches(matcher.src2Type, statement.src2)) continue;
			if(!SymbolMatches(matcher.src3Type, statement.src3)) continue;
			matcher.emitter(statement);
			found = true;
			break;
		}
		assert(found);
		if(!found)
		{
			throw std::runtime_error("No suitable emitter found for statement.");
		}
	}

	m_functionStream.Write8(Wasm::INST_END);

	auto functionCode = CWasmModuleBuilder::FunctionCode(m_functionStream.GetBuffer(), m_functionStream.GetBuffer() + m_functionStream.GetSize());

	moduleBuilder.AddFunction(std::move(functionCode));
	moduleBuilder.WriteModule(*m_stream);
}

void CCodeGen_Wasm::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
}

void CCodeGen_Wasm::RegisterExternalSymbols(CObjectFile*) const
{
}

unsigned int CCodeGen_Wasm::GetAvailableRegisterCount() const
{
	return 0;
}

unsigned int CCodeGen_Wasm::GetAvailableMdRegisterCount() const
{
	return 0;
}

bool CCodeGen_Wasm::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

uint32 CCodeGen_Wasm::GetPointerSize() const
{
	return 4;
}

void CCodeGen_Wasm::MarkLabel(const STATEMENT&)
{
}

void CCodeGen_Wasm::Emit_Mov_RelRel(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_valueLow < 0x80);
	assert(src1->m_valueLow < 0x80);

	//Compute Store Offset
	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	m_functionStream.Write8(0x00);

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	m_functionStream.Write8(dst->m_valueLow);

	m_functionStream.Write8(Wasm::INST_I32_ADD);

	//Compute Load Offset
	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	m_functionStream.Write8(0x00);

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	m_functionStream.Write8(src1->m_valueLow);

	m_functionStream.Write8(Wasm::INST_I32_ADD);

	//Load
	m_functionStream.Write8(Wasm::INST_I32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);

	//Store
	m_functionStream.Write8(Wasm::INST_I32_STORE);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}
