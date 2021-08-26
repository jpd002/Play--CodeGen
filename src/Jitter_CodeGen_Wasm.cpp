#include "Jitter_CodeGen_Wasm.h"

#include <stdexcept>

#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

// clang-format off
CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_constMatchers[] =
{
	{ OP_MOV,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Mov_VarAny                             },

	{ OP_RELTOREF,       MATCH_VAR_REF,        MATCH_CONSTANT,       MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_RelToRef_VarCst                        },

	{ OP_ADDREF,         MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_AddRef_AnyAnyAny                       },

	{ OP_ISREFNULL,      MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_IsRefNull_VarVar                       },

	{ OP_LOADFROMREF,    MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_LoadFromRef_VarVar                     },
	{ OP_LOADFROMREF,    MATCH_VAR_REF,        MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_LoadFromRef_Ref_VarVar                 },

	{ OP_LOAD8FROMREF,   MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Load8FromRef_MemVar                    },
	{ OP_LOAD16FROMREF,  MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Load16FromRef_MemVar                  },

	//Cannot use MATCH_ANY here because it will match non 32-bits symbols
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_VARIABLE,      MATCH_NIL,      &CCodeGen_Wasm::Emit_StoreAtRef_VarAny                      },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_StoreAtRef_VarAny                      },

	{ OP_STORE8ATREF,    MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Store8AtRef_VarAny                     },
	{ OP_STORE16ATREF,   MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Store16AtRef_VarAny                    },

	{ OP_PARAM,          MATCH_NIL,            MATCH_CONTEXT,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Param_Ctx                              },
	{ OP_PARAM,          MATCH_NIL,            MATCH_ANY,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Param_Any                              },

	{ OP_CALL,           MATCH_NIL,            MATCH_CONSTANTPTR,    MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Call                                   },
	{ OP_RETVAL,         MATCH_TEMPORARY,      MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_RetVal_Tmp                             },

	{ OP_EXTERNJMP,      MATCH_NIL,            MATCH_CONSTANTPTR,    MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_ExternJmp                              },

	{ OP_JMP,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Jmp                                    },

	{ OP_CONDJMP,        MATCH_NIL,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_CondJmp_AnyAny                         },

	{ OP_CMP,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Cmp_AnyAnyAny                          },

	{ OP_SLL,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Sll_AnyAnyAny                          },
	{ OP_SRL,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Srl_AnyAnyAny                          },
	{ OP_SRA,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Sra_AnyAnyAny                          },

	{ OP_AND,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_And_AnyAnyAny                          },
	{ OP_OR,             MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Or_AnyAnyAny                           },
	{ OP_XOR,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Xor_AnyAnyAny                          },

	{ OP_ADD,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Add_AnyAnyAny                          },
	{ OP_SUB,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Sub_AnyAnyAny                          },

	{ OP_LABEL,          MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::MarkLabel                                   },

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      nullptr                                                     },
};
// clang-format on

#ifdef __EMSCRIPTEN__

#include <emscripten.h>
// clang-format off
EM_JS(int, RegisterExternFunction, (const char* functionName, const char* functionSig), {
	let fctName = UTF8ToString(functionName);
	let fctSig = UTF8ToString(functionSig);
	let fct = Module[fctName];
	if(fct === undefined)
	{
		out(`Warning: Could not find function '${fctName}' (missing export?).`);
	}
	let fctId = addFunction(fct, fctSig);
	out(`Registered function '${fctName}(${fctSig})' = > id = ${fctId}.`);
	return fctId;
});
// clang-format on

std::map<uintptr_t, CWasmFunctionRegistry::WASM_FUNCTION_INFO> CWasmFunctionRegistry::m_functions;

void CWasmFunctionRegistry::RegisterFunction(uintptr_t functionPtr, const char* functionName, const char* functionSig)
{
	{
		auto fctIterator = m_functions.find(functionPtr);
		assert(fctIterator == m_functions.end());
	}
	WASM_FUNCTION_INFO functionInfo;
	functionInfo.id = RegisterExternFunction(functionName, functionSig);
	functionInfo.signature = functionSig;
	m_functions.insert(std::make_pair(functionPtr, functionInfo));
}

const CWasmFunctionRegistry::WASM_FUNCTION_INFO* CWasmFunctionRegistry::FindFunction(uintptr_t functionPtr)
{
	auto fctIterator = m_functions.find(functionPtr);
	if(fctIterator == std::end(m_functions)) return nullptr;
	return &fctIterator->second;
}

#else

void CWasmFunctionRegistry::RegisterFunction(uintptr_t functionPtr, const char* functionName, const char* functionSig)
{
}

const CWasmFunctionRegistry::WASM_FUNCTION_INFO* CWasmFunctionRegistry::FindFunction(uintptr_t functionPtr)
{
	static const WASM_FUNCTION_INFO fctInfo = {0, "v"};
	return &fctInfo;
}

#endif

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
	m_signatures.clear();
	m_labelFlows.clear();

	BuildLabelFlows(statements);
	PrepareSignatures(moduleBuilder, statements);

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

	assert(m_params.empty());
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

			bool foundEnd = false;
			auto innerStatementIterator = outerStatementIterator;

			//Next statement should be a label (start of if block)
			innerStatementIterator++;
			assert(innerStatementIterator->op == OP_LABEL);
			{
				auto insertResult = m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_IF));
				assert(insertResult.second);
			}

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

					{
						auto insertResult = m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_ELSE));
						assert(insertResult.second);
					}

					//Skip OP_LABEL statement
					innerStatementIterator++;

					for(; innerStatementIterator != statements.end(); innerStatementIterator++)
					{
						const auto& innerStatement = *innerStatementIterator;
						if(innerStatement.op == OP_LABEL)
						{
							if(innerStatement.jmpBlock == finalLabel)
							{
								auto insertResult = m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_END));
								assert(insertResult.second);
								foundEnd = true;
								break;
							}
						}
					}

					break;
				}
				else if(innerStatement.op == OP_LABEL)
				{
					//If block
					//Check if it's our end label
					if(innerStatement.jmpBlock == outerStatement.jmpBlock)
					{
						auto insertResult = m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_END));
						assert(insertResult.second == true);
						foundEnd = true;
						break;
					}
				}
			}
			assert(foundEnd);
		}
	}
}

void CCodeGen_Wasm::PrepareSignatures(CWasmModuleBuilder& moduleBuilder, const StatementList& statements)
{
	//Register this function's signature
	RegisterSignature(moduleBuilder, "vi");

	for(const auto& statement : statements)
	{
		if(statement.op != OP_CALL) continue;

		auto src1 = statement.src1->GetSymbol().get();
		assert(src1->m_type == SYM_CONSTANTPTR);

		auto fctInfo = CWasmFunctionRegistry::FindFunction(src1->m_valueLow);
		assert(fctInfo);

		assert(!fctInfo->signature.empty());

		auto signatureIterator = m_signatures.find(fctInfo->signature);
		if(signatureIterator == std::end(m_signatures))
		{
			RegisterSignature(moduleBuilder, fctInfo->signature);
		}
	}
}

void CCodeGen_Wasm::RegisterSignature(CWasmModuleBuilder& moduleBuilder, std::string signature)
{
	CWasmModuleBuilder::FUNCTION_TYPE functionType;

	switch(signature[0])
	{
	case 'v':
		break;
	case 'i':
		functionType.results.push_back(Wasm::TYPE_I32);
		break;
	default:
		assert(false);
		break;
	}

	for(uint32 i = 1; i < signature.size(); i++)
	{
		switch(signature[i])
		{
		case 'i':
			functionType.params.push_back(Wasm::TYPE_I32);
			break;
		default:
			assert(false);
			break;
		}
	}

	moduleBuilder.AddFunctionType(std::move(functionType));

	m_signatures.insert(std::make_pair(signature, static_cast<uint32>(m_signatures.size())));
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

	PushContext();

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	CWasmModuleBuilder::WriteSLeb128(m_functionStream, symbol->m_valueLow);

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
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PullTemporary(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY);

	uint32 localIdx = (symbol->m_stackLocation / 4) + 1;

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PushRelativeRefAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_REL_REFERENCE);

	PushContext();

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	CWasmModuleBuilder::WriteSLeb128(m_functionStream, symbol->m_valueLow);

	m_functionStream.Write8(Wasm::INST_I32_ADD);
}

void CCodeGen_Wasm::PushRelativeRef(CSymbol* symbol)
{
	PushRelativeRefAddress(symbol);

	m_functionStream.Write8(Wasm::INST_I32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PushTemporaryRef(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TMP_REFERENCE);

	uint32 localIdx = (symbol->m_stackLocation / 4) + 1;

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PullTemporaryRef(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TMP_REFERENCE);

	uint32 localIdx = (symbol->m_stackLocation / 4) + 1;

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PrepareSymbolUse(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE:
		PushRelative(symbol);
		break;
	case SYM_REL_REFERENCE:
		PushRelativeRef(symbol);
		break;
	case SYM_TEMPORARY:
		PushTemporary(symbol);
		break;
	case SYM_TMP_REFERENCE:
		PushTemporaryRef(symbol);
		break;
	case SYM_CONSTANT:
		m_functionStream.Write8(Wasm::INST_I32_CONST);
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, symbol->m_valueLow);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Wasm::PrepareSymbolDef(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE:
		PushRelativeAddress(symbol);
		break;
	case SYM_TEMPORARY:
	case SYM_TMP_REFERENCE:
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Wasm::CommitSymbol(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE:
		m_functionStream.Write8(Wasm::INST_I32_STORE);
		m_functionStream.Write8(0x02);
		m_functionStream.Write8(0x00);
		break;
	case SYM_TEMPORARY:
		PullTemporary(symbol);
		break;
	case SYM_TMP_REFERENCE:
		PullTemporaryRef(symbol);
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

void CCodeGen_Wasm::Emit_Mov_VarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_RelToRef_VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PushContext();

	m_functionStream.Write8(Wasm::INST_I32_ADD);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_AddRef_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_ADD);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_IsRefNull_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I32_EQZ);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_LoadFromRef_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_LoadFromRef_Ref_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Load8FromRef_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I32_LOAD8_U);
	m_functionStream.Write8(0x00); //Align
	m_functionStream.Write8(0x00);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Load16FromRef_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I32_LOAD16_U);
	m_functionStream.Write8(0x01); //Align
	m_functionStream.Write8(0x00);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_StoreAtRef_VarAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_STORE);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Store8AtRef_VarAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_STORE8);
	m_functionStream.Write8(0x00); //Align
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Store16AtRef_VarAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_STORE16);
	m_functionStream.Write8(0x01); //Align
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Param_Ctx(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	assert(src1->m_type == SYM_CONTEXT);

	m_params.push([this]() { PushContext(); });
}

void CCodeGen_Wasm::Emit_Param_Any(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	m_params.push([this, src1]() { PrepareSymbolUse(src1); });
}

void CCodeGen_Wasm::Emit_Call(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANTPTR);
	assert(src2->m_type == SYM_CONSTANT);

	unsigned int paramCount = src2->m_valueLow;
	for(unsigned int i = 0; i < paramCount; i++)
	{
		auto paramFct = m_params.top();
		paramFct();
		m_params.pop();
	}

	auto fctInfo = CWasmFunctionRegistry::FindFunction(src1->m_valueLow);
	auto sigIdxIterator = m_signatures.find(fctInfo->signature);
	assert(sigIdxIterator != std::end(m_signatures));
	auto sigIdx = sigIdxIterator->second;

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, fctInfo->id);

	m_functionStream.Write8(Wasm::INST_CALL_INDIRECT);
	m_functionStream.Write8(sigIdx); //Signature index
	m_functionStream.Write8(0x00);   //Table index
}

void CCodeGen_Wasm::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	PullTemporary(dst);
}

void CCodeGen_Wasm::Emit_ExternJmp(const STATEMENT& statement)
{
	//Not really supported, but can be used with some caveats
	//Implemented as a simple indirect call for now (which works fine if the caller returns immediately).
	//This could be implemented using tail calls which doesn't seem to be widely supported.
	//Maybe we could emit a return after the call and be done with it? (stack overflow problems maybe)

	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANTPTR);

	PushContext();

	auto fctInfo = CWasmFunctionRegistry::FindFunction(src1->m_valueLow);
	auto sigIdxIterator = m_signatures.find(fctInfo->signature);
	assert(sigIdxIterator != std::end(m_signatures));
	auto sigIdx = sigIdxIterator->second;

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, fctInfo->id);

	m_functionStream.Write8(Wasm::INST_CALL_INDIRECT);
	m_functionStream.Write8(sigIdx); //Signature index
	m_functionStream.Write8(0x00);   //Table index
}

void CCodeGen_Wasm::Emit_Jmp(const STATEMENT&)
{
}

void CCodeGen_Wasm::Emit_CondJmp_AnyAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	//We use the reverse condition because jmpCondition is the condition to skip
	switch(statement.jmpCondition)
	{
	case CONDITION_EQ:
		m_functionStream.Write8(Wasm::INST_I32_NE);
		break;
	case CONDITION_NE:
		m_functionStream.Write8(Wasm::INST_I32_EQ);
		break;
	case CONDITION_BL:
		m_functionStream.Write8(Wasm::INST_I32_GE_U);
		break;
	case CONDITION_BE:
		m_functionStream.Write8(Wasm::INST_I32_GT_U);
		break;
	case CONDITION_AB:
		m_functionStream.Write8(Wasm::INST_I32_LE_U);
		break;
	case CONDITION_AE:
		m_functionStream.Write8(Wasm::INST_I32_LT_U);
		break;
	case CONDITION_LT:
		m_functionStream.Write8(Wasm::INST_I32_GE_S);
		break;
	case CONDITION_LE:
		m_functionStream.Write8(Wasm::INST_I32_GT_S);
		break;
	case CONDITION_GT:
		m_functionStream.Write8(Wasm::INST_I32_LE_S);
		break;
	case CONDITION_GE:
		m_functionStream.Write8(Wasm::INST_I32_LT_S);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Wasm::Emit_Cmp_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	switch(statement.jmpCondition)
	{
	case CONDITION_EQ:
		m_functionStream.Write8(Wasm::INST_I32_EQ);
		break;
	case CONDITION_NE:
		m_functionStream.Write8(Wasm::INST_I32_NE);
		break;
	case CONDITION_BL:
		m_functionStream.Write8(Wasm::INST_I32_LT_U);
		break;
	case CONDITION_BE:
		m_functionStream.Write8(Wasm::INST_I32_LE_U);
		break;
	case CONDITION_AB:
		m_functionStream.Write8(Wasm::INST_I32_GT_U);
		break;
	case CONDITION_AE:
		m_functionStream.Write8(Wasm::INST_I32_GE_U);
		break;
	case CONDITION_LT:
		m_functionStream.Write8(Wasm::INST_I32_LT_S);
		break;
	case CONDITION_LE:
		m_functionStream.Write8(Wasm::INST_I32_LE_S);
		break;
	case CONDITION_GT:
		m_functionStream.Write8(Wasm::INST_I32_GT_S);
		break;
	case CONDITION_GE:
		m_functionStream.Write8(Wasm::INST_I32_GE_S);
		break;
	default:
		assert(false);
		break;
	}

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Sll_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_SHL);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Srl_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_SHR_U);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Sra_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_SHR_S);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_And_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_AND);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Or_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_OR);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Xor_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_XOR);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Add_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_ADD);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Sub_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_I32_SUB);

	CommitSymbol(dst);
}
