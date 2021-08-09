#include "Jitter_CodeGen_Wasm.h"

#include <stdexcept>

#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

// clang-format off
CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_constMatchers[] =
{
	{ OP_MOV,            MATCH_RELATIVE,       MATCH_ANY,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Mov_RelAny                             },

	{ OP_PARAM,          MATCH_NIL,            MATCH_CONTEXT,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Param_Ctx                              },

	{ OP_CALL,           MATCH_NIL,            MATCH_CONSTANTPTR,    MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Call                                   },
	{ OP_RETVAL,         MATCH_TEMPORARY,      MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_RetVal_Tmp                             },

	{ OP_JMP,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Jmp                                    },

	{ OP_CONDJMP,        MATCH_NIL,            MATCH_RELATIVE,       MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_CondJmp_RelCst                         },

	{ OP_SLL,            MATCH_RELATIVE,       MATCH_RELATIVE,       MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Sll_RelRelCst                          },

	{ OP_LABEL,          MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::MarkLabel                                   },

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      nullptr                                                     },
};
// clang-format on

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

EM_JS(int, RegisterExternFunction, (const char* functionName, const char* functionSig), {
	let fctName = UTF8ToString(functionName);
	let fctSig = UTF8ToString(functionSig);
	let fct = Module[fctName];
	let fctId = addFunction(fct, fctSig);
	out('Registered function ' + fctName + '(' + fctSig + ') => id = ' + fctId + '.');
	return fctId;
});

std::map<uintptr_t, int32> CWasmFunctionRegistry::m_functions;

void CWasmFunctionRegistry::RegisterFunction(uintptr_t functionPtr, const char* functionName, const char* functionSig)
{
	assert(!strcmp(functionSig, "vi"));
	uint32 functionId = RegisterExternFunction(functionName, functionSig);
	auto fctIterator = m_functions.find(functionPtr);
	assert(fctIterator == m_functions.end());
	m_functions.insert(std::make_pair(functionPtr, functionId));
}

int32 CWasmFunctionRegistry::FindFunction(uintptr_t functionPtr)
{
	auto fctIterator = m_functions.find(functionPtr);
	assert(false);
	assert(fctIterator != m_functions.end());
	return fctIterator->second;
}

#else

void CWasmFunctionRegistry::RegisterFunction(uintptr_t functionPtr, const char* functionName, const char* functionSig)
{
}

int32 CWasmFunctionRegistry::FindFunction(uintptr_t functionPtr)
{
	return 0;
}

#endif

static void WriteLeb128(Framework::CStream& stream, int32 value)
{
	bool more = true;
	while(more)
	{
		uint8 byte = (value & 0x7F);
		value >>= 7;
		if(
		    (value == 0 && ((byte & 0x40) == 0)) ||
		    (value == -1 && ((byte & 0x40) != 0)))
		{
			more = false;
		}
		else
		{
			byte |= 0x80;
		}
		stream.Write8(byte);
	}
}

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

	assert((stackSize & 0x3) == 0);

	m_functionStream.ResetBuffer();
	m_labelFlows.clear();

	BuildLabelFlows(statements);

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

	CWasmModuleBuilder::FUNCTION function;
	function.code = CWasmModuleBuilder::FunctionCode(m_functionStream.GetBuffer(), m_functionStream.GetBuffer() + m_functionStream.GetSize());
	function.localI32Count = stackSize / 4;

	moduleBuilder.AddFunction(std::move(function));
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

void CCodeGen_Wasm::BuildLabelFlows(const StatementList& statements)
{
	for(auto outerStatementIterator = statements.begin();
	    outerStatementIterator != statements.end(); outerStatementIterator++)
	{
		const auto& outerStatement = *outerStatementIterator;
		if(outerStatement.op == OP_CONDJMP)
		{
			//Check if we find the target label or a uncond jmp first
			//If we find the target label, we have a If block
			//If we find an uncond jmp, we have a If/Else block

			auto innerStatementIterator = outerStatementIterator;

			//Next statement should be a label (start of if block)
			innerStatementIterator++;
			assert(innerStatementIterator->op == OP_LABEL);
			m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_IF));

			innerStatementIterator++;
			for(; innerStatementIterator != statements.end(); innerStatementIterator++)
			{
				const auto& innerStatement = *innerStatementIterator;
				if(innerStatement.op == OP_JMP)
				{
					//Take note of the final label
					uint32 finalLabel = innerStatement.jmpBlock;

					//Next statement should be the label the outer statement is referencing
					innerStatementIterator++;
					assert(innerStatementIterator != statements.end());
					assert(innerStatementIterator->op == OP_LABEL);
					assert(innerStatementIterator->jmpBlock == outerStatement.jmpBlock);

					m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_ELSE));

					//Skip OP_LABEL statement
					innerStatementIterator++;

					for(; innerStatementIterator != statements.end(); innerStatementIterator++)
					{
						const auto& innerStatement = *innerStatementIterator;
						if(innerStatement.op == OP_LABEL)
						{
							assert(innerStatement.jmpBlock == finalLabel);
							m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_END));
							break;
						}
					}

					break;
				}
				else if(innerStatement.op == OP_LABEL)
				{
					//If block
					//TODO
					//Label is end
					assert(innerStatement.jmpBlock == outerStatement.jmpBlock);
				}
			}
		}
	}
}

void CCodeGen_Wasm::PushContext()
{
	//Context is the first param
	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PushRelativeAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE);
	assert(symbol->m_valueLow < 0x80);

	PushContext();

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	m_functionStream.Write8(symbol->m_valueLow);

	m_functionStream.Write8(Wasm::INST_I32_ADD);
}

void CCodeGen_Wasm::PushRelative(CSymbol* symbol)
{
	PushRelativeAddress(symbol);

	m_functionStream.Write8(Wasm::INST_I32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PushTemporary(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY);

	uint32 localIdx = (symbol->m_stackLocation / 4) + 1;

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	WriteLeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PullTemporary(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY);

	uint32 localIdx = (symbol->m_stackLocation / 4) + 1;

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	WriteLeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PushSymbol(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE:
		PushRelative(symbol);
		break;
	case SYM_TEMPORARY:
		PushTemporary(symbol);
		break;
	case SYM_CONSTANT:
		m_functionStream.Write8(Wasm::INST_I32_CONST);
		WriteLeb128(m_functionStream, symbol->m_valueLow);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Wasm::MarkLabel(const STATEMENT& statement)
{
	auto labelFlowIterator = m_labelFlows.find(statement.jmpBlock);
	if(labelFlowIterator != std::end(m_labelFlows))
	{
		switch(labelFlowIterator->second)
		{
		case LABEL_FLOW_IF:
			m_functionStream.Write8(Wasm::INST_IF);
			m_functionStream.Write8(Wasm::INST_BLOCKTYPE_VOID);
			break;
		case LABEL_FLOW_ELSE:
			m_functionStream.Write8(Wasm::INST_ELSE);
			break;
		case LABEL_FLOW_END:
			m_functionStream.Write8(Wasm::INST_END);
			break;
		default:
			assert(false);
			break;
		}
	}
}

void CCodeGen_Wasm::Emit_Mov_RelAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PushRelativeAddress(dst);
	PushSymbol(src1);

	m_functionStream.Write8(Wasm::INST_I32_STORE);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Param_Ctx(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	assert(src1->m_type == SYM_CONTEXT);

	PushContext();
}

void CCodeGen_Wasm::Emit_Call(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANTPTR);
	assert(src2->m_type == SYM_CONSTANT);

	unsigned int paramCount = src2->m_valueLow;

	//TODO: Register signature in our module
	int32 tableEntryIdx = CWasmFunctionRegistry::FindFunction(src1->m_valueLow);

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	WriteLeb128(m_functionStream, tableEntryIdx);

	m_functionStream.Write8(Wasm::INST_CALL_INDIRECT);
	m_functionStream.Write8(0x01); //Signature index
	m_functionStream.Write8(0x00); //Table index
}

void CCodeGen_Wasm::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	PullTemporary(dst);
}

void CCodeGen_Wasm::Emit_Jmp(const STATEMENT&)
{
}

void CCodeGen_Wasm::Emit_CondJmp_RelCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PushSymbol(src1);
	PushSymbol(src2);

	assert(statement.jmpCondition == CONDITION_NE);
	m_functionStream.Write8(Wasm::INST_I32_EQ);
}

void CCodeGen_Wasm::Emit_Sll_RelRelCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_valueLow < 0x80);

	PushRelativeAddress(dst);
	PushRelativeAddress(src1);

	//Load
	m_functionStream.Write8(Wasm::INST_I32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);

	//Shift
	m_functionStream.Write8(Wasm::INST_I32_CONST);
	m_functionStream.Write8(src2->m_valueLow);

	m_functionStream.Write8(Wasm::INST_I32_SHL);

	//Store
	m_functionStream.Write8(Wasm::INST_I32_STORE);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}
