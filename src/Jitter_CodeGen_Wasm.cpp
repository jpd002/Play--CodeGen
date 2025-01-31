#include "Jitter_CodeGen_Wasm.h"

#include <stdexcept>
#include <set>

#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

#include "Jitter_CodeGen_Wasm_LoadStore.h"

template <bool isSigned>
void CCodeGen_Wasm::Emit_Mul_Tmp64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY64);

	uint32 localIdx = GetTemporaryLocation(dst);

	PrepareSymbolUse(src1);
	m_functionStream.Write8(isSigned ? Wasm::INST_I64_EXTEND_I32_S : Wasm::INST_I64_EXTEND_I32_U);

	PrepareSymbolUse(src2);
	m_functionStream.Write8(isSigned ? Wasm::INST_I64_EXTEND_I32_S : Wasm::INST_I64_EXTEND_I32_U);

	m_functionStream.Write8(Wasm::INST_I64_MUL);

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

template <bool isSigned>
void CCodeGen_Wasm::Emit_Div_Tmp64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY64);

	uint32 localIdx = GetTemporaryLocation(dst);

	//Compute dividend
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(isSigned ? Wasm::INST_I32_DIV_S : Wasm::INST_I32_DIV_U);
	m_functionStream.Write8(Wasm::INST_I64_EXTEND_I32_U);

	//Compute remainder
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(isSigned ? Wasm::INST_I32_REM_S : Wasm::INST_I32_REM_U);
	m_functionStream.Write8(Wasm::INST_I64_EXTEND_I32_U);

	m_functionStream.Write8(Wasm::INST_I64_CONST);
	CWasmModuleBuilder::WriteSLeb128(m_functionStream, 32);

	m_functionStream.Write8(Wasm::INST_I64_SHL);

	//Combine
	m_functionStream.Write8(Wasm::INST_I64_OR);

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

// clang-format off
CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_constMatchers[] =
{
	{ OP_NOP,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Nop                                    },
	{ OP_BREAK,          MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Break                                  },

	{ OP_MOV,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Mov_VarAny                             },

	{ OP_RELTOREF,       MATCH_VAR_REF,        MATCH_CONSTANT,       MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_RelToRef_VarCst                        },

	{ OP_ADDREF,         MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_AddRef_AnyAnyAny                       },

	{ OP_ISREFNULL,      MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_IsRefNull_VarVar                       },

	{ OP_LOADFROMREF,    MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_LoadFromRef_VarVar                     },
	{ OP_LOADFROMREF,    MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_Wasm::Emit_LoadFromRef_VarVarAny                  },

	// Reusing the same generators for refs since they're 32-bit values
	{ OP_LOADFROMREF,    MATCH_VAR_REF,        MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_LoadFromRef_VarVar                     },
	{ OP_LOADFROMREF,    MATCH_VAR_REF,        MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_Wasm::Emit_LoadFromRef_VarVarAny                  },

	{ OP_LOAD8FROMREF,   MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Generic_LoadFromRef_MemVar<Wasm::INST_I32_LOAD8_U, 0>     },
	{ OP_LOAD8FROMREF,   MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_Wasm::Emit_Generic_LoadFromRef_MemVarAny<Wasm::INST_I32_LOAD8_U, 0>  },
	{ OP_LOAD16FROMREF,  MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Generic_LoadFromRef_MemVar<Wasm::INST_I32_LOAD16_U, 1>    },
	{ OP_LOAD16FROMREF,  MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_Wasm::Emit_Generic_LoadFromRef_MemVarAny<Wasm::INST_I32_LOAD16_U, 1> },

	{ OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_Wasm::Emit_StoreAtRef_VarAny                      },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY32,         MATCH_ANY32,    &CCodeGen_Wasm::Emit_StoreAtRef_VarAnyAny                   },

	{ OP_STORE8ATREF,    MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAny<Wasm::INST_I32_STORE8, 0>     },
	{ OP_STORE8ATREF,    MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_ANY32,    &CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAnyAny<Wasm::INST_I32_STORE8, 0>  },
	{ OP_STORE16ATREF,   MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAny<Wasm::INST_I32_STORE16, 1>    },
	{ OP_STORE16ATREF,   MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_ANY32,    &CCodeGen_Wasm::Emit_Generic_StoreAtRef_VarAnyAny<Wasm::INST_I32_STORE16, 1> },

	{ OP_PARAM,          MATCH_NIL,            MATCH_CONTEXT,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Param_Ctx                              },
	{ OP_PARAM,          MATCH_NIL,            MATCH_ANY,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Param_Any                              },

	{ OP_CALL,           MATCH_NIL,            MATCH_CONSTANTPTR,    MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Call                                   },
	{ OP_RETVAL,         MATCH_TEMPORARY,      MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_RetVal_Tmp                             },

	{ OP_EXTERNJMP,      MATCH_NIL,            MATCH_CONSTANTPTR,    MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_ExternJmp                              },

	{ OP_JMP,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Jmp                                    },

	{ OP_CONDJMP,        MATCH_NIL,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_CondJmp_AnyAny                         },

	{ OP_CMP,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Cmp_AnyAnyAny                          },

	{ OP_SELECT,         MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_ANY,           MATCH_ANY,      &CCodeGen_Wasm::Emit_Select_VarVarAnyAny                    },

	{ OP_SLL,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Sll_AnyAnyAny                          },
	{ OP_SRL,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Srl_AnyAnyAny                          },
	{ OP_SRA,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Sra_AnyAnyAny                          },

	{ OP_NOT,            MATCH_ANY,            MATCH_ANY,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Not_AnyAny                             },
	{ OP_LZC,            MATCH_ANY,            MATCH_ANY,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Lzc_AnyAny                             },
	{ OP_AND,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_And_AnyAnyAny                          },
	{ OP_OR,             MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Or_AnyAnyAny                           },
	{ OP_XOR,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Xor_AnyAnyAny                          },

	{ OP_ADD,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Add_AnyAnyAny                          },
	{ OP_SUB,            MATCH_ANY,            MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Sub_AnyAnyAny                          },

	{ OP_DIV,            MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Div_Tmp64AnyAny<false>                 },
	{ OP_DIVS,           MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Div_Tmp64AnyAny<true>                  },

	{ OP_MUL,            MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Mul_Tmp64AnyAny<false>                 },
	{ OP_MULS,           MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Mul_Tmp64AnyAny<true>                  },

	{ OP_EXTLOW64,       MATCH_VARIABLE,       MATCH_MEMORY64,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_ExtLow64VarMem64                       },
	{ OP_EXTHIGH64,      MATCH_VARIABLE,       MATCH_MEMORY64,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_ExtHigh64VarMem64                      },

	{ OP_LABEL,          MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::MarkLabel                                   },

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      nullptr                                                     },
};
// clang-format on

#ifdef __EMSCRIPTEN__

#include <emscripten.h>
// clang-format off
EM_JS_DEPS(WasmRegisterExternFunction, "$convertJsFunctionToWasm");
EM_JS(int, RegisterExternFunction, (const char* functionName, const char* functionSig), {
	let fctName = UTF8ToString(functionName);
	let fctSig = UTF8ToString(functionSig);
	let fct = Module[fctName];
	if(fct === undefined)
	{
		out(`Warning: Could not find function '${fctName}' (missing export?).`);
	}
	if(Module.codeGenImportTable === undefined) {
		out("Creating import table...");
		Module.codeGenImportTable = new WebAssembly.Table({
			element: 'anyfunc',
			initial: 32
		});
		Module.codeGenImportTableNextIndex = 0;
	}
	//TODO: Check how we can use the export from the WASM module directly
	//instead of having to wrap the wrapper
	let wrappedFct = convertJsFunctionToWasm(fct, fctSig);
	let fctId = Module.codeGenImportTableNextIndex++;
	Module.codeGenImportTable.set(fctId, wrappedFct);
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
	copyMatchers(g_64ConstMatchers);
	copyMatchers(g_fpuConstMatchers);
	copyMatchers(g_mdConstMatchers);
}

void CCodeGen_Wasm::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	CWasmModuleBuilder moduleBuilder;

	assert((stackSize & 0x3) == 0);

	m_functionStream.ResetBuffer();
	m_signatures.clear();
	m_labelFlows.clear();
	m_temporaryLocations.clear();
	m_localI32Count = 0;
	m_localI64Count = 0;
	m_localF32Count = 0;
	m_localV128Count = 0;
	m_isInsideBlock = false;
	m_isInsideLoop = false;
	m_currentBlockDepth = 0;
	m_loopBlock = -1;

	BuildLabelFlows(statements);
	PrepareSignatures(moduleBuilder, statements);
	PrepareLocalVars(statements);

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

	//Terminate current block
	assert(m_isInsideBlock);
	m_functionStream.Write8(Wasm::INST_END);

	//Terminate loop
	if(m_isInsideLoop)
	{
		m_functionStream.Write8(Wasm::INST_END);
	}

	m_functionStream.Write8(Wasm::INST_END);

	CWasmModuleBuilder::FUNCTION function;
	function.code = CWasmModuleBuilder::FunctionCode(m_functionStream.GetBuffer(), m_functionStream.GetBuffer() + m_functionStream.GetSize());
	function.localI32Count = m_localI32Count;
	function.localI64Count = m_localI64Count;
	function.localF32Count = m_localF32Count;
	function.localV128Count = m_localV128Count;

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

bool CCodeGen_Wasm::Has128BitsCallOperands() const
{
	return false;
}

bool CCodeGen_Wasm::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

bool CCodeGen_Wasm::SupportsExternalJumps() const
{
	return false;
}

uint32 CCodeGen_Wasm::GetPointerSize() const
{
	return 4;
}

void CCodeGen_Wasm::BuildLabelFlows(const StatementList& statements)
{
	//Patterns
	//-----------

	//If/EndIf
	//-------
	//OP_CONDJMP y
	//OP_LABEL x
	//...
	//OP_LABEL y
	//...

	//If/Else/EndIf
	//-------------
	//OP_CONDJMP y
	//OP_LABEL x
	//...
	//OP_JMP z
	//OP_LABEL y
	//...
	//OP_LABEL z
	//...

	//Loop
	//----
	//OP_LABEL x
	//...
	//OP_CONDJMP z
	//OP_LABEL y
	//...
	//OP_JMP x (jump up)
	//OP_LABEL z
	//...

	std::set<uint32> visitedLabels;
	for(auto outerStatementIterator = statements.begin();
	    outerStatementIterator != statements.end(); outerStatementIterator++)
	{
		const auto& outerStatement = *outerStatementIterator;
		if(outerStatement.op == OP_LABEL)
		{
			visitedLabels.insert(outerStatement.jmpBlock);
		}
		else if((outerStatement.op == OP_JMP) && visitedLabels.find(outerStatement.jmpBlock) != std::end(visitedLabels))
		{
			//Jumping to a label that we've already seen (jumping up)
			//Note: We only support one loop block
			assert(m_loopBlock == -1);
			m_loopBlock = outerStatement.jmpBlock;
		}
		else if(outerStatement.op == OP_CONDJMP)
		{
			//Find the target label
			auto innerStatementIterator = outerStatementIterator;

			//Next statement should be a label (start of if block)
			innerStatementIterator++;
			assert(innerStatementIterator->op == OP_LABEL);
			{
				FRAMEWORK_MAYBE_UNUSED auto insertResult = m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_IF));
				assert(insertResult.second);
			}

			bool foundEnd = false;
			for(; (innerStatementIterator != statements.end()) && !foundEnd; innerStatementIterator++)
			{
				const auto& innerStatement = *innerStatementIterator;
				if((innerStatement.op == OP_LABEL) &&
				   (innerStatement.jmpBlock == outerStatement.jmpBlock))
				{
					foundEnd = true;
					auto prevStatementIterator = innerStatementIterator;
					prevStatementIterator--;
					const auto& prevStatement = *prevStatementIterator;
					//Check that we're jumping down to some other block
					if((prevStatement.op == OP_JMP) && (visitedLabels.find(prevStatement.jmpBlock) == std::end(visitedLabels)))
					{
						//We have a If/Else/EndIf situation
						{
							FRAMEWORK_MAYBE_UNUSED auto insertResult = m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_ELSE));
							assert(insertResult.second);
						}
						{
							FRAMEWORK_MAYBE_UNUSED auto insertResult = m_labelFlows.insert(std::make_pair(prevStatementIterator->jmpBlock, LABEL_FLOW_ENDIF));
							assert(insertResult.second);
						}
					}
					else
					{
						//We have a If/EndIf situation
						{
							FRAMEWORK_MAYBE_UNUSED auto insertResult = m_labelFlows.insert(std::make_pair(innerStatementIterator->jmpBlock, LABEL_FLOW_ENDIF));
							assert(insertResult.second);
						}
					}
				}
			}
			assert(foundEnd);
		}
	}

	if(m_loopBlock != -1)
	{
		FRAMEWORK_MAYBE_UNUSED auto insertResult = m_labelFlows.insert(std::make_pair(m_loopBlock, LABEL_FLOW_LOOP));
		assert(insertResult.second);
	}

	//Look for unbound labels and build blocks
	for(const auto& statement : statements)
	{
		if(statement.op != OP_LABEL) continue;
		auto labelFlowIterator = m_labelFlows.find(statement.jmpBlock);
		if(labelFlowIterator != std::end(m_labelFlows)) continue;
		m_labelFlows.insert(std::make_pair(statement.jmpBlock, LABEL_FLOW_BLOCK));
	}

#ifdef _DEBUG
	//Validate jmps to other blocks: we can only jump to the next block
	for(auto outerStatementIterator = statements.begin();
	    outerStatementIterator != statements.end(); outerStatementIterator++)
	{
		const auto& outerStatement = *outerStatementIterator;
		if(outerStatement.op != OP_JMP) continue;

		//Check that this jmp targets a block (not part of a If structure)
		{
			auto labelFlowIterator = m_labelFlows.find(outerStatement.jmpBlock);
			assert(labelFlowIterator != std::end(m_labelFlows));
			if(labelFlowIterator->second != LABEL_FLOW_BLOCK) continue;
		}

		//Check that the next label we encounter is our target label
		for(auto innerStatementIterator = outerStatementIterator;
		    innerStatementIterator != statements.end(); innerStatementIterator++)
		{
			const auto& innerStatement = *innerStatementIterator;
			if(innerStatement.op != OP_LABEL) continue;

			//Check that label is a new block
			auto labelFlowIterator = m_labelFlows.find(innerStatement.jmpBlock);
			assert(labelFlowIterator != std::end(m_labelFlows));
			if(labelFlowIterator->second != LABEL_FLOW_BLOCK) continue;

			//This is the next block, check that our jmp targets this.
			assert(innerStatement.jmpBlock == outerStatement.jmpBlock);
			break;
		}
	}
#endif
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
	case 'j':
		functionType.results.push_back(Wasm::TYPE_I64);
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
		case 'j':
			functionType.params.push_back(Wasm::TYPE_I64);
			break;
		default:
			assert(false);
			break;
		}
	}

	moduleBuilder.AddFunctionType(std::move(functionType));

	m_signatures.insert(std::make_pair(signature, static_cast<uint32>(m_signatures.size())));
}

void CCodeGen_Wasm::PrepareLocalVars(const StatementList& statements)
{
	for(const auto& statement : statements)
	{
		statement.VisitOperands(
		    [this](const SymbolRefPtr& symbolRef, bool isDef) {
			    auto symbol = symbolRef->GetSymbol();
			    if(!symbol->IsTemporary()) return;
			    auto temporaryInstance = std::make_pair(symbol->m_type, symbol->m_stackLocation);
			    if(m_temporaryLocations.find(temporaryInstance) != std::end(m_temporaryLocations)) return;
			    switch(symbol->m_type)
			    {
			    case SYM_TEMPORARY:
			    case SYM_TMP_REFERENCE:
				    m_temporaryLocations[temporaryInstance] = m_localI32Count;
				    m_localI32Count++;
				    break;
			    case SYM_TEMPORARY64:
				    m_temporaryLocations[temporaryInstance] = m_localI64Count;
				    m_localI64Count++;
				    break;
			    case SYM_FP_TEMPORARY32:
				    m_temporaryLocations[temporaryInstance] = m_localF32Count;
				    m_localF32Count++;
				    break;
			    case SYM_TEMPORARY128:
				    m_temporaryLocations[temporaryInstance] = m_localV128Count;
				    m_localV128Count++;
				    break;
			    case SYM_TEMPORARY256:
				    //Allocate 2 v128 for a 256-bit temp
				    m_temporaryLocations[temporaryInstance] = m_localV128Count;
				    m_localV128Count += 2;
				    break;
			    default:
				    assert(false);
				    break;
			    }
		    });
	}
}

uint32 CCodeGen_Wasm::GetTemporaryLocation(CSymbol* symbol) const
{
	assert(symbol->IsTemporary());
	auto temporaryLocationIterator = m_temporaryLocations.find(std::make_pair(symbol->m_type, symbol->m_stackLocation));
	assert(temporaryLocationIterator != std::end(m_temporaryLocations));
	uint32 temporaryLocation = temporaryLocationIterator->second;

	//First local is the function's parameter
	uint32 localIdx = 0;

	switch(symbol->m_type)
	{
	case SYM_TEMPORARY:
	case SYM_TMP_REFERENCE:
		localIdx = temporaryLocation + 1;
		break;
	case SYM_TEMPORARY64:
		localIdx = temporaryLocation + m_localI32Count + 1;
		break;
	case SYM_FP_TEMPORARY32:
		localIdx = temporaryLocation + m_localI32Count + m_localI64Count + 1;
		break;
	case SYM_TEMPORARY128:
	case SYM_TEMPORARY256:
		localIdx = temporaryLocation + m_localI32Count + m_localI64Count + m_localF32Count + 1;
		break;
	default:
		assert(false);
		break;
	}

	return localIdx;
}

void CCodeGen_Wasm::PushContext()
{
	//Context is the first param
	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PushRelativeAddress(CSymbol* symbol)
{
	assert(
	    (symbol->m_type == SYM_RELATIVE) ||
	    (symbol->m_type == SYM_RELATIVE64) ||
	    (symbol->m_type == SYM_FP_RELATIVE32) ||
	    (symbol->m_type == SYM_RELATIVE128));

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
	uint32 localIdx = GetTemporaryLocation(symbol);

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PullTemporary(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY);

	uint32 localIdx = GetTemporaryLocation(symbol);

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

	uint32 localIdx = GetTemporaryLocation(symbol);

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PullTemporaryRef(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TMP_REFERENCE);

	uint32 localIdx = GetTemporaryLocation(symbol);

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
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, static_cast<int32>(symbol->m_valueLow));
		break;
	case SYM_RELATIVE64:
		PushRelative64(symbol);
		break;
	case SYM_TEMPORARY64:
		PushTemporary64(symbol);
		break;
	case SYM_CONSTANT64:
		m_functionStream.Write8(Wasm::INST_I64_CONST);
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, symbol->GetConstant64());
		break;
	case SYM_FP_RELATIVE32:
		PushRelativeFp32(symbol);
		break;
	case SYM_FP_TEMPORARY32:
		PushTemporaryFp32(symbol);
		break;
	case SYM_RELATIVE128:
		PushRelative128(symbol);
		break;
	case SYM_TEMPORARY128:
		PushTemporary128(symbol);
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
	case SYM_RELATIVE64:
	case SYM_RELATIVE128:
	case SYM_FP_RELATIVE32:
		PushRelativeAddress(symbol);
		break;
	case SYM_TEMPORARY:
	case SYM_TEMPORARY64:
	case SYM_TEMPORARY128:
	case SYM_TMP_REFERENCE:
	case SYM_FP_TEMPORARY32:
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
	case SYM_RELATIVE64:
		m_functionStream.Write8(Wasm::INST_I64_STORE);
		m_functionStream.Write8(0x03);
		m_functionStream.Write8(0x00);
		break;
	case SYM_TEMPORARY64:
		PullTemporary64(symbol);
		break;
	case SYM_FP_RELATIVE32:
		m_functionStream.Write8(Wasm::INST_F32_STORE);
		m_functionStream.Write8(0x02);
		m_functionStream.Write8(0x00);
		break;
	case SYM_FP_TEMPORARY32:
		PullTemporaryFp32(symbol);
		break;
	case SYM_RELATIVE128:
		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		m_functionStream.Write8(Wasm::INST_V128_STORE);
		m_functionStream.Write8(0x04);
		m_functionStream.Write8(0x00);
		break;
	case SYM_TEMPORARY128:
		PullTemporary128(symbol);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Wasm::MarkLabel(const STATEMENT& statement)
{
	auto labelFlowIterator = m_labelFlows.find(statement.jmpBlock);
	assert(labelFlowIterator != std::end(m_labelFlows));
	switch(labelFlowIterator->second)
	{
	case LABEL_FLOW_BLOCK:
		assert(m_currentBlockDepth == 0);
		if(m_isInsideBlock)
		{
			m_functionStream.Write8(Wasm::INST_END);
		}
		m_isInsideBlock = true;
		m_functionStream.Write8(Wasm::INST_BLOCK);
		m_functionStream.Write8(Wasm::BLOCK_TYPE_VOID);
		break;
	case LABEL_FLOW_IF:
		m_functionStream.Write8(Wasm::INST_IF);
		m_functionStream.Write8(Wasm::BLOCK_TYPE_VOID);
		m_currentBlockDepth++;
		break;
	case LABEL_FLOW_ELSE:
		m_functionStream.Write8(Wasm::INST_ELSE);
		break;
	case LABEL_FLOW_ENDIF:
		m_functionStream.Write8(Wasm::INST_END);
		assert(m_currentBlockDepth != 0);
		m_currentBlockDepth--;
		break;
	case LABEL_FLOW_LOOP:
		assert(m_currentBlockDepth == 0);
		if(m_isInsideBlock)
		{
			m_functionStream.Write8(Wasm::INST_END);
		}
		m_isInsideBlock = true;
		m_isInsideLoop = true;
		m_functionStream.Write8(Wasm::INST_LOOP);
		m_functionStream.Write8(Wasm::BLOCK_TYPE_VOID);
		m_functionStream.Write8(Wasm::INST_BLOCK);
		m_functionStream.Write8(Wasm::BLOCK_TYPE_VOID);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Wasm::Emit_Nop(const STATEMENT& statement)
{
}

void CCodeGen_Wasm::Emit_Break(const STATEMENT& statement)
{
	m_functionStream.Write8(Wasm::INST_UNREACHABLE);
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

void CCodeGen_Wasm::Emit_LoadFromRef_VarVarAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert((scale == 1) || (scale == 4));

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	switch(scale)
	{
	case 1:
		//Nothing to do
		break;
	case 4:
		m_functionStream.Write8(Wasm::INST_I32_CONST);
		m_functionStream.Write8(2);
		m_functionStream.Write8(Wasm::INST_I32_SHL);
		break;
	default:
		assert(false);
		break;
	}

	m_functionStream.Write8(Wasm::INST_I32_ADD);

	m_functionStream.Write8(Wasm::INST_I32_LOAD);
	m_functionStream.Write8(0x02);
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

void CCodeGen_Wasm::Emit_StoreAtRef_VarAnyAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert((scale == 1) || (scale == 4));

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	switch(scale)
	{
	case 1:
		//Nothing to do
		break;
	case 4:
		m_functionStream.Write8(Wasm::INST_I32_CONST);
		m_functionStream.Write8(2);
		m_functionStream.Write8(Wasm::INST_I32_SHL);
		break;
	default:
		assert(false);
		break;
	}

	m_functionStream.Write8(Wasm::INST_I32_ADD);

	PrepareSymbolUse(src3);

	m_functionStream.Write8(Wasm::INST_I32_STORE);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Param_Ctx(const STATEMENT& statement)
{
	FRAMEWORK_MAYBE_UNUSED auto src1 = statement.src1->GetSymbol().get();
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

void CCodeGen_Wasm::Emit_Jmp(const STATEMENT& statement)
{
	auto labelFlowIterator = m_labelFlows.find(statement.jmpBlock);
	assert(labelFlowIterator != std::end(m_labelFlows));
	if(labelFlowIterator->second == LABEL_FLOW_BLOCK)
	{
		//This can only be used to exit the current block
		//Validated in BuildLabelFlows
		assert(m_isInsideBlock);
		m_functionStream.Write8(Wasm::INST_BR);
		m_functionStream.Write8(m_currentBlockDepth);
	}
	else if(labelFlowIterator->second == LABEL_FLOW_LOOP)
	{
		assert(m_isInsideBlock);
		assert(m_isInsideLoop);
		m_functionStream.Write8(Wasm::INST_BR);
		m_functionStream.Write8(m_currentBlockDepth + 1);
	}
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

void CCodeGen_Wasm::Emit_Select_VarVarAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src2);
	PrepareSymbolUse(src3);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_SELECT);

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

void CCodeGen_Wasm::Emit_Not_AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	CWasmModuleBuilder::WriteSLeb128(m_functionStream, -1);

	m_functionStream.Write8(Wasm::INST_I32_XOR);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Lzc_AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);

	//Check if MSB is 0 or 1
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I32_CONST);
	CWasmModuleBuilder::WriteSLeb128(m_functionStream, 0);

	m_functionStream.Write8(Wasm::INST_I32_LT_S);

	m_functionStream.Write8(Wasm::INST_IF);
	m_functionStream.Write8(Wasm::TYPE_I32);
	{
		//First bit is 1
		PrepareSymbolUse(src1);
		m_functionStream.Write8(Wasm::INST_I32_CONST);
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, -1);

		m_functionStream.Write8(Wasm::INST_I32_XOR);

		m_functionStream.Write8(Wasm::INST_I32_CLZ);

		m_functionStream.Write8(Wasm::INST_I32_CONST);
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, 1);

		m_functionStream.Write8(Wasm::INST_I32_SUB);
	}
	m_functionStream.Write8(Wasm::INST_ELSE);
	{
		//First bit is 0
		PrepareSymbolUse(src1);
		m_functionStream.Write8(Wasm::INST_I32_CLZ);

		m_functionStream.Write8(Wasm::INST_I32_CONST);
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, 1);

		m_functionStream.Write8(Wasm::INST_I32_SUB);
	}
	m_functionStream.Write8(Wasm::INST_END);

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

void CCodeGen_Wasm::Emit_ExtLow64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);

	uint32 localIdx = GetTemporaryLocation(src1);

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);

	m_functionStream.Write8(Wasm::INST_I32_WRAP_I64);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_ExtHigh64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);

	uint32 localIdx = GetTemporaryLocation(src1);

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);

	m_functionStream.Write8(Wasm::INST_I64_CONST);
	CWasmModuleBuilder::WriteSLeb128(m_functionStream, 32);

	m_functionStream.Write8(Wasm::INST_I64_SHR_U);

	m_functionStream.Write8(Wasm::INST_I32_WRAP_I64);

	CommitSymbol(dst);
}
