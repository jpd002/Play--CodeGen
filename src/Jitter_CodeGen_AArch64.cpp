#include "Jitter_CodeGen_AArch64.h"

using namespace Jitter;

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_constMatchers[] = 
{
	{ OP_LABEL,    MATCH_NIL,    MATCH_NIL,    MATCH_NIL,    &CCodeGen_AArch64::MarkLabel    },
};

CCodeGen_AArch64::CCodeGen_AArch64()
{
	for(const auto& constMatcher : g_constMatchers)
	{
		MATCHER matcher;
		matcher.op       = constMatcher.op;
		matcher.dstType  = constMatcher.dstType;
		matcher.src1Type = constMatcher.src1Type;
		matcher.src2Type = constMatcher.src2Type;
		matcher.emitter  = std::bind(constMatcher.emitter, this, std::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}
}

CCodeGen_AArch64::~CCodeGen_AArch64()
{

}

unsigned int CCodeGen_AArch64::GetAvailableRegisterCount() const
{
	return 0;
}

unsigned int CCodeGen_AArch64::GetAvailableMdRegisterCount() const
{
	return 0;
}

bool CCodeGen_AArch64::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

void CCodeGen_AArch64::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
//	m_assembler.SetStream(stream);
}

void CCodeGen_AArch64::RegisterExternalSymbols(CObjectFile* objectFile) const
{

}

void CCodeGen_AArch64::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	//Align stack size (must be aligned on 16 bytes boundary)
	stackSize = (stackSize + 0xF) & ~0xF;

	for(const auto& statement : statements)
	{
		bool found = false;
		auto begin = m_matchers.lower_bound(statement.op);
		auto end = m_matchers.upper_bound(statement.op);

		for(auto matchIterator(begin); matchIterator != end; matchIterator++)
		{
			const MATCHER& matcher(matchIterator->second);
			if(!SymbolMatches(matcher.dstType, statement.dst)) continue;
			if(!SymbolMatches(matcher.src1Type, statement.src1)) continue;
			if(!SymbolMatches(matcher.src2Type, statement.src2)) continue;
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
}

void CCodeGen_AArch64::MarkLabel(const STATEMENT& statement)
{
//	CAArch32Assembler::LABEL label = GetLabel(statement.jmpBlock);
//	m_assembler.MarkLabel(label);
}
