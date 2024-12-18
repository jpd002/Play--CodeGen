#include "Jitter_CodeGen.h"

using namespace Jitter;

void CCodeGen::SetExternalSymbolReferencedHandler(const ExternalSymbolReferencedHandler& externalSymbolReferencedHandler)
{
	m_externalSymbolReferencedHandler = externalSymbolReferencedHandler;
}

bool CCodeGen::SymbolMatches(MATCHTYPE match, const SymbolRefPtr& symbolRef)
{
	if(match == MATCH_ANY) return true;
	if(match == MATCH_NIL)
	{
		if(!symbolRef)
			return true;
		else
			return false;
	}
	if(!symbolRef) return false;
	CSymbol* symbol = symbolRef->GetSymbol().get();
	switch(match)
	{
	case MATCH_RELATIVE:
		return (symbol->m_type == SYM_RELATIVE);
	case MATCH_CONSTANT:
		return (symbol->m_type == SYM_CONSTANT);
	case MATCH_CONSTANTPTR:
		return (symbol->m_type == SYM_CONSTANTPTR);
	case MATCH_REGISTER:
		return (symbol->m_type == SYM_REGISTER);
	case MATCH_TEMPORARY:
		return (symbol->m_type == SYM_TEMPORARY);
	case MATCH_MEMORY:
		return (symbol->m_type == SYM_RELATIVE) || (symbol->m_type == SYM_TEMPORARY);
	case MATCH_VARIABLE:
		return (symbol->m_type == SYM_REGISTER) || (symbol->m_type == SYM_RELATIVE) || (symbol->m_type == SYM_TEMPORARY);
	case MATCH_ANY32:
		return (symbol->m_type == SYM_REGISTER) || (symbol->m_type == SYM_RELATIVE) || (symbol->m_type == SYM_TEMPORARY) || (symbol->m_type == SYM_CONSTANT);

	case MATCH_REL_REF:
		return (symbol->m_type == SYM_REL_REFERENCE);
	case MATCH_REG_REF:
		return (symbol->m_type == SYM_REG_REFERENCE);
	case MATCH_TMP_REF:
		return (symbol->m_type == SYM_TMP_REFERENCE);
	case MATCH_MEM_REF:
		return (symbol->m_type == SYM_REL_REFERENCE) || (symbol->m_type == SYM_TMP_REFERENCE);
	case MATCH_VAR_REF:
		return (symbol->m_type == SYM_REG_REFERENCE) || (symbol->m_type == SYM_REL_REFERENCE) || (symbol->m_type == SYM_TMP_REFERENCE);

	case MATCH_RELATIVE64:
		return (symbol->m_type == SYM_RELATIVE64);
	case MATCH_TEMPORARY64:
		return (symbol->m_type == SYM_TEMPORARY64);
	case MATCH_CONSTANT64:
		return (symbol->m_type == SYM_CONSTANT64);
	case MATCH_MEMORY64:
		return (symbol->m_type == SYM_RELATIVE64) || (symbol->m_type == SYM_TEMPORARY64);

	case MATCH_FP_REGISTER32:
		return (symbol->m_type == SYM_FP_REGISTER32);
	case MATCH_FP_RELATIVE32:
		return (symbol->m_type == SYM_FP_RELATIVE32);
	case MATCH_FP_TEMPORARY32:
		return (symbol->m_type == SYM_FP_TEMPORARY32);
	case MATCH_FP_MEMORY32:
		return (symbol->m_type == SYM_FP_RELATIVE32) || (symbol->m_type == SYM_FP_TEMPORARY32);
	case MATCH_FP_VARIABLE32:
		return (symbol->m_type == SYM_FP_REGISTER32) || (symbol->m_type == SYM_FP_RELATIVE32) || (symbol->m_type == SYM_FP_TEMPORARY32);

	case MATCH_REGISTER128:
		return (symbol->m_type == SYM_REGISTER128);
	case MATCH_RELATIVE128:
		return (symbol->m_type == SYM_RELATIVE128);
	case MATCH_TEMPORARY128:
		return (symbol->m_type == SYM_TEMPORARY128);
	case MATCH_MEMORY128:
		return (symbol->m_type == SYM_RELATIVE128) || (symbol->m_type == SYM_TEMPORARY128);
	case MATCH_VARIABLE128:
		return (symbol->m_type == SYM_REGISTER128) || (symbol->m_type == SYM_RELATIVE128) || (symbol->m_type == SYM_TEMPORARY128);

	case MATCH_MEMORY256:
		return (symbol->m_type == SYM_TEMPORARY256);

	case MATCH_CONTEXT:
		return (symbol->m_type == SYM_CONTEXT);

	default:
		assert(false);
		return false;
	}
}

uint32 CCodeGen::GetRegisterUsage(const StatementList& statements)
{
	uint32 registerUsage = 0;
	for(const auto& statement : statements)
	{
		if(auto dst = dynamic_symbolref_cast(SYM_REGISTER, statement.dst))
		{
			registerUsage |= (1 << dst->m_valueLow);
		}
		else if(auto dst = dynamic_symbolref_cast(SYM_REG_REFERENCE, statement.dst))
		{
			registerUsage |= (1 << dst->m_valueLow);
		}
	}
	return registerUsage;
}
