#include <assert.h>
#include <vector>
#include <algorithm>
#include "Jitter.h"
#include "BitManip.h"

#ifdef _DEBUG
//#define DUMP_STATEMENTS
#endif

#ifdef DUMP_STATEMENTS
#include <iostream>
#endif

using namespace Jitter;

static uint64 MergeConstant64(uint32 lo, uint32 hi)
{
	uint64 result = static_cast<uint64>(lo) | (static_cast<uint64>(hi) >> 32);
	return result;
}

unsigned int CJitter::CRelativeVersionManager::GetRelativeVersion(uint32 relativeId)
{
	RelativeVersionMap::const_iterator versionIterator(m_relativeVersions.find(relativeId));
	if(versionIterator == m_relativeVersions.end()) return 0;
	return versionIterator->second;
}

unsigned int CJitter::CRelativeVersionManager::IncrementRelativeVersion(uint32 relativeId)
{
	unsigned int nextVersion = GetRelativeVersion(relativeId) + 1;
	m_relativeVersions[relativeId] = nextVersion;
	return nextVersion;
}

CJitter::VERSIONED_STATEMENT_LIST CJitter::GenerateVersionedStatementList(const StatementList& statements)
{
	VERSIONED_STATEMENT_LIST result;

	struct ReplaceUse
	{
		void operator() (SymbolRefPtr& symbolRef, CRelativeVersionManager& relativeVersions) const
		{
			if(CSymbol* symbol = dynamic_symbolref_cast(SYM_RELATIVE, symbolRef))
			{
				unsigned int currentVersion = relativeVersions.GetRelativeVersion(symbol->m_valueLow);
				symbolRef = std::make_shared<CSymbolRef>(symbolRef->GetSymbol(), currentVersion);
			}
			else if(CSymbol* symbol = dynamic_symbolref_cast(SYM_REL_REFERENCE, symbolRef))
			{
				unsigned int currentVersion = relativeVersions.GetRelativeVersion(symbol->m_valueLow);
				symbolRef = std::make_shared<CSymbolRef>(symbolRef->GetSymbol(), currentVersion);
			}
			else if(CSymbol* symbol = dynamic_symbolref_cast(SYM_RELATIVE64, symbolRef))
			{
				//Since this symbol can be aliased, use the sum of the versions of all
				//of its parts.
				unsigned int currentVersion =
				    relativeVersions.GetRelativeVersion(symbol->m_valueLow + 0x0) +
				    relativeVersions.GetRelativeVersion(symbol->m_valueLow + 0x4);
				symbolRef = std::make_shared<CSymbolRef>(symbolRef->GetSymbol(), currentVersion);
			}
			else if(CSymbol* symbol = dynamic_symbolref_cast(SYM_FP_RELATIVE32, symbolRef))
			{
				unsigned int currentVersion = relativeVersions.GetRelativeVersion(symbol->m_valueLow);
				symbolRef = std::make_shared<CSymbolRef>(symbolRef->GetSymbol(), currentVersion);
			}
			else if(CSymbol* symbol = dynamic_symbolref_cast(SYM_RELATIVE128, symbolRef))
			{
				//Since this symbol can be aliased, use the sum of the versions of all
				//of its parts.
				unsigned int currentVersion =
				    relativeVersions.GetRelativeVersion(symbol->m_valueLow + 0x0) +
				    relativeVersions.GetRelativeVersion(symbol->m_valueLow + 0x4) +
				    relativeVersions.GetRelativeVersion(symbol->m_valueLow + 0x8) +
				    relativeVersions.GetRelativeVersion(symbol->m_valueLow + 0xC);
				symbolRef = std::make_shared<CSymbolRef>(symbolRef->GetSymbol(), currentVersion);
			}
		}
	};

	for(auto newStatement : statements)
	{
		ReplaceUse()(newStatement.src1, result.relativeVersions);
		ReplaceUse()(newStatement.src2, result.relativeVersions);
		ReplaceUse()(newStatement.src3, result.relativeVersions);

		if(auto dst = dynamic_symbolref_cast(SYM_RELATIVE, newStatement.dst))
		{
			unsigned int nextVersion = result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow);
			newStatement.dst = std::make_shared<CSymbolRef>(newStatement.dst->GetSymbol(), nextVersion);
		}
		//Increment relative versions to prevent some optimization problems
		else if(auto dst = dynamic_symbolref_cast(SYM_REL_REFERENCE, newStatement.dst))
		{
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow);
		}
		else if(auto dst = dynamic_symbolref_cast(SYM_FP_RELATIVE32, newStatement.dst))
		{
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow);
		}
		else if(auto dst = dynamic_symbolref_cast(SYM_RELATIVE64, newStatement.dst))
		{
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 0);
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 4);
		}
		else if(auto dst = dynamic_symbolref_cast(SYM_RELATIVE128, newStatement.dst))
		{
			uint8 mask = 0xF;
			if(newStatement.op == OP_MD_MOV_MASKED)
			{
				mask = static_cast<uint8>(newStatement.jmpCondition);
			}

			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 0);
			if(mask & 0x02) result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 4);
			if(mask & 0x04) result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 8);
			if(mask & 0x08) result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 12);
		}

		result.statements.push_back(newStatement);
	}

	return result;
}

StatementList CJitter::CollapseVersionedStatementList(const VERSIONED_STATEMENT_LIST& statements)
{
	StatementList result;
	for(auto newStatement : statements.statements)
	{
		newStatement.VisitOperands(
			[](SymbolRefPtr& symbolRef, bool)
			{
				if(symbolRef->IsVersioned())
				{
					symbolRef = std::make_shared<CSymbolRef>(symbolRef->GetSymbol());
				}
			}
		);
		
		result.push_back(newStatement);
	}
	return result;
}

void CJitter::Compile()
{
	while(1)
	{
		for(auto& basicBlock : m_basicBlocks)
		{
			if(!basicBlock.optimized)
			{
				m_currentBlock = &basicBlock;

				//DumpStatementList(m_currentBlock->statements);

				auto versionedStatements = GenerateVersionedStatementList(basicBlock.statements);

				while(1)
				{
					bool dirty = false;
					dirty |= VariableFolding(versionedStatements.statements);
					dirty |= ConstantPropagation(versionedStatements.statements);
					dirty |= ConstantFolding(versionedStatements.statements);
					dirty |= ReorderAdd(versionedStatements.statements);
					dirty |= CopyPropagation(versionedStatements.statements);
					dirty |= DeadcodeElimination(versionedStatements);
					dirty |= CommonExpressionElimination(versionedStatements);

					if(!dirty) break;
				}

				basicBlock.statements = CollapseVersionedStatementList(versionedStatements);
				FixFlowControl(basicBlock.statements);
				basicBlock.optimized = true;
			}
		}

		bool dirty = false;
		dirty |= PruneBlocks();
		dirty |= MergeBlocks();

		if(!dirty) break;
	}

	unsigned int stackSize = 0;

	//Allocate registers
	for(auto& basicBlock : m_basicBlocks)
	{
		m_currentBlock = &basicBlock;

		CoalesceTemporaries(basicBlock);
		RemoveSelfAssignments(basicBlock);
		PruneSymbols(basicBlock);

		AllocateRegisters(basicBlock);
		unsigned int blockStackSize = AllocateStack(basicBlock);
		stackSize = std::max<unsigned int>(stackSize, blockStackSize);

		NormalizeStatements(basicBlock);
	}

	auto result = ConcatBlocks(m_basicBlocks);

#ifdef DUMP_STATEMENTS
	DumpStatementList(result.statements);
	std::cout << std::endl;
#endif

	m_codeGen->GenerateCode(result.statements, stackSize);

	m_labels.clear();
}

void CJitter::InsertStatement(const STATEMENT& statement)
{
	m_currentBlock->statements.push_back(statement);
}

SymbolPtr CJitter::MakeSymbol(SYM_TYPE type, uint32 value)
{
	return MakeSymbol(m_currentBlock, type, value, 0);
}

SymbolPtr CJitter::MakeConstantPtr(uintptr_t value)
{
#if (UINTPTR_MAX == UINT32_MAX)
	uint32 valueLo = static_cast<uint32>(value);
	return MakeSymbol(m_currentBlock, SYM_CONSTANTPTR, valueLo, 0);
#elif (UINTPTR_MAX == UINT64_MAX)
	uint32 valueLo = static_cast<uint32>(value);
	uint32 valueHi = static_cast<uint32>(value >> 32);
	return MakeSymbol(m_currentBlock, SYM_CONSTANTPTR, valueLo, valueHi);
#else
	static_assert(false, "Unsupported pointer size.");
#endif
}

SymbolPtr CJitter::MakeConstant64(uint64 value)
{
	uint32 valueLo = static_cast<uint32>(value);
	uint32 valueHi = static_cast<uint32>(value >> 32);
	return MakeSymbol(m_currentBlock, SYM_CONSTANT64, valueLo, valueHi);
}

SymbolPtr CJitter::MakeSymbol(BASIC_BLOCK* basicBlock, SYM_TYPE type, uint32 valueLo, uint32 valueHi)
{
	CSymbolTable& currentSymbolTable(basicBlock->symbolTable);
	return currentSymbolTable.MakeSymbol(type, valueLo, valueHi);
}

SymbolRefPtr CJitter::MakeSymbolRef(const SymbolPtr& symbol)
{
	return std::make_shared<CSymbolRef>(symbol);
}

int CJitter::GetSymbolSize(const SymbolRefPtr& symbolRef)
{
	return symbolRef->GetSymbol().get()->GetSize();
}

bool CJitter::FoldConstantOperation(STATEMENT& statement)
{
	CSymbol* src1cst = dynamic_symbolref_cast(SYM_CONSTANT, statement.src1);
	CSymbol* src2cst = dynamic_symbolref_cast(SYM_CONSTANT, statement.src2);

	//Nothing we can do
	if(src1cst == NULL && src2cst == NULL) return false;

	bool changed = false;

	if(statement.op == OP_ADD)
	{
		if(src1cst && src2cst)
		{
			//Adding 2 constants
			uint32 result = src1cst->m_valueLow + src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
		else if(src1cst && src1cst->m_valueLow == 0)
		{
			statement.op = OP_MOV;
			statement.src1 = statement.src2;
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && src2cst->m_valueLow == 0)
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SUB)
	{
		if(src1cst && src2cst)
		{
			//2 constants
			uint32 result = src1cst->m_valueLow - src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && src2cst->m_valueLow == 0)
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_AND)
	{
		if(src1cst && src2cst)
		{
			//2 constants
			uint32 result = src1cst->m_valueLow & src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
		else if(
			(src1cst && src1cst->m_valueLow == 0) ||
			(src2cst && src2cst->m_valueLow == 0)
		)
		{
			//Anding with zero
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, 0));
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && src2cst->m_valueLow == ~0)
		{
			//Anding with ~0
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_OR)
	{
		if(src1cst && src2cst)
		{
			uint32 result = src1cst->m_valueLow | src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
		else if(src1cst && src1cst->m_valueLow == 0)
		{
			//Oring with zero
			statement.op = OP_MOV;
			std::swap(statement.src1, statement.src2);
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && src2cst->m_valueLow == 0)
		{
			//Oring with zero
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_XOR)
	{
		if(src1cst && src2cst)
		{
			uint32 result = src1cst->m_valueLow ^ src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
		else if(src1cst && src1cst->m_valueLow == 0)
		{
			//Xoring with zero
			statement.op = OP_MOV;
			std::swap(statement.src1, statement.src2);
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && src2cst->m_valueLow == 0)
		{
			//Xoring with zero
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_NOT)
	{
		if(src1cst)
		{
			uint32 result = ~src1cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SRA)
	{
		if(src1cst && src2cst)
		{
			uint32 result = static_cast<int32>(src1cst->m_valueLow) >> static_cast<int32>(src2cst->m_valueLow & 0x1F);
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && ((src2cst->m_valueLow & 0x1F) == 0))
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SRL)
	{
		if(src1cst && src2cst)
		{
			uint32 result = src1cst->m_valueLow >> (src2cst->m_valueLow & 0x1F);
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && ((src2cst->m_valueLow & 0x1F) == 0))
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SLL)
	{
		if(src1cst && src2cst)
		{
			uint32 result = src1cst->m_valueLow << (src2cst->m_valueLow & 0x1F);
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && ((src2cst->m_valueLow & 0x1F) == 0))
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_LZC)
	{
		if(src1cst)
		{
			uint32 result = src1cst->m_valueLow;
			if(result & 0x80000000) result = ~result;
			if(result == 0)
			{
				result = 0x1F;
			}
			else
			{
				result = __builtin_clz(result) - 1;
			}
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_MERGETO64)
	{
		if(src1cst && src2cst)
		{
			uint64 result = MergeConstant64(src1cst->m_valueLow, src2cst->m_valueLow);
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_MUL)
	{
		if(src1cst && src2cst)
		{
			uint64 result = static_cast<uint64>(src1cst->m_valueLow) * static_cast<uint64>(src2cst->m_valueLow);
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_MULS)
	{
		if(src1cst && src2cst)
		{
			int64 result = static_cast<int64>(static_cast<int32>(src1cst->m_valueLow)) * static_cast<int64>(static_cast<int32>(src2cst->m_valueLow));
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(static_cast<uint64>(result)));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_DIV)
	{
		if(src1cst && src2cst)
		{
			uint64 result = 0;
			if(src2cst->m_valueLow != 0)
			{
				uint32 quotient = src1cst->m_valueLow / src2cst->m_valueLow;
				uint32 remainder = src1cst->m_valueLow % src2cst->m_valueLow;
				result = static_cast<uint64>(quotient) | (static_cast<uint64>(remainder) << 32);
			}
			else
			{
				result = ~0ULL;
			}
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(result));
			statement.src2.reset();
			changed = true;
		}
		//else if(src2cst && IsPowerOfTwo(src2cst->m_valueLow))
		//{
		//	statement.op = OP_SRL;
		//	src2cst->m_valueLow = GetPowerOf2(src2cst->m_valueLow);
		//}
	}
	else if (statement.op == OP_DIVS)
	{
		if (src1cst && src2cst)
		{
			uint64 result = 0;
			if(src2cst->m_valueLow != 0)
			{
				uint32 quotient = static_cast<int32>(src1cst->m_valueLow) / static_cast<int32>(src2cst->m_valueLow);
				uint32 remainder = static_cast<int32>(src1cst->m_valueLow) % static_cast<int32>(src2cst->m_valueLow);
				result = static_cast<uint64>(quotient) | (static_cast<uint64>(remainder) << 32);
			}
			else
			{
				result = ~0ULL;
			}
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_CMP)
	{
		if(src1cst && src2cst)
		{
			bool result = false;
			switch(statement.jmpCondition)
			{
			case CONDITION_BL:
				result = src1cst->m_valueLow < src2cst->m_valueLow;
				break;
			case CONDITION_LT:
				result = static_cast<int32>(src1cst->m_valueLow) < static_cast<int32>(src2cst->m_valueLow);
				break;
			case CONDITION_LE:
				result = static_cast<int32>(src1cst->m_valueLow) <= static_cast<int32>(src2cst->m_valueLow);
				break;
			case CONDITION_GT:
				result = static_cast<int32>(src1cst->m_valueLow) > static_cast<int32>(src2cst->m_valueLow);
				break;
			case CONDITION_EQ:
				result = static_cast<int32>(src1cst->m_valueLow) == static_cast<int32>(src2cst->m_valueLow);
				break;
			default:
				assert(0);
				break;
			}
			changed = true;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result ? 1 : 0));
			statement.src2.reset();
		}
	}
	else if(statement.op == OP_CONDJMP)
	{
		if(src1cst && src2cst)
		{
			bool result = false;
			switch(statement.jmpCondition)
			{
			case CONDITION_NE:
				result = src1cst->m_valueLow != src2cst->m_valueLow;
				break;
			case CONDITION_EQ:
				result = src1cst->m_valueLow == src2cst->m_valueLow;
				break;
			case CONDITION_BL:
				result = static_cast<uint32>(src1cst->m_valueLow) < static_cast<uint32>(src2cst->m_valueLow);
				break;
			case CONDITION_LE:
				result = static_cast<int32>(src1cst->m_valueLow) <= static_cast<int32>(src2cst->m_valueLow);
				break;
			case CONDITION_GT:
				result = static_cast<int32>(src1cst->m_valueLow) > static_cast<int32>(src2cst->m_valueLow);
				break;
			case CONDITION_GE:
				result = static_cast<int32>(src1cst->m_valueLow) >= static_cast<int32>(src2cst->m_valueLow);
				break;
			default:
				assert(0);
				break;
			}
			changed = true;
			if(result)
			{
				statement.op = OP_JMP;
			}
			else
			{
				statement.op = OP_NOP;
			}
			statement.src1.reset();
			statement.src2.reset();
		}
	}

	return changed;
}

bool CJitter::FoldConstant64Operation(STATEMENT& statement)
{
	CSymbol* src1cst = dynamic_symbolref_cast(SYM_CONSTANT64, statement.src1);
	CSymbol* src2cst = dynamic_symbolref_cast(SYM_CONSTANT64, statement.src2);

	//Nothing we can do
	if(src1cst == NULL && src2cst == NULL) return false;

	bool changed = false;

	if(statement.op == OP_EXTLOW64)
	{
		if(src1cst)
		{
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, src1cst->m_valueLow));
			changed = true;
		}
	}
	else if(statement.op == OP_EXTHIGH64)
	{
		if(src1cst)
		{
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, src1cst->m_valueHigh));
			changed = true;
		}
	}
	else if(statement.op == OP_ADD64)
	{
		if(src1cst && src2cst)
		{
			uint64 cst1 = MergeConstant64(src1cst->m_valueLow, src1cst->m_valueHigh);
			uint64 cst2 = MergeConstant64(src2cst->m_valueLow, src2cst->m_valueHigh);
			uint64 result = cst1 + cst2;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(result));
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && (src2cst->m_valueLow == 0) && (src2cst->m_valueHigh == 0))
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SUB64)
	{
		if(src1cst && src2cst)
		{
			uint64 cst1 = MergeConstant64(src1cst->m_valueLow, src1cst->m_valueHigh);
			uint64 cst2 = MergeConstant64(src2cst->m_valueLow, src2cst->m_valueHigh);
			uint64 result = cst1 - cst2;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_AND64)
	{
		if(src1cst && src2cst)
		{
			uint64 cst1 = MergeConstant64(src1cst->m_valueLow, src1cst->m_valueHigh);
			uint64 cst2 = MergeConstant64(src2cst->m_valueLow, src2cst->m_valueHigh);
			uint64 result = cst1 & cst2;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(result));
			statement.src2.reset();
			changed = true;
		}
		else if(
			(src1cst && (src1cst->m_valueLow == 0) && (src1cst->m_valueHigh == 0)) ||
			(src2cst && (src2cst->m_valueLow == 0) && (src2cst->m_valueHigh == 0))
			)
		{
			//ANDing anything with 0 gives 0
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(0));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_CMP64)
	{
		if(src1cst && src2cst)
		{
			bool result = false;
			switch(statement.jmpCondition)
			{
			case CONDITION_EQ:
				result = src1cst->GetConstant64() == src2cst->GetConstant64();
				break;
			case CONDITION_NE:
				result = src1cst->GetConstant64() != src2cst->GetConstant64();
				break;
			case CONDITION_BL:
				result = src1cst->GetConstant64() < src2cst->GetConstant64();
				break;
			case CONDITION_AB:
				result = src1cst->GetConstant64() > src2cst->GetConstant64();
				break;
			case CONDITION_LT:
				result = static_cast<int64>(src1cst->GetConstant64()) < static_cast<int64>(src2cst->GetConstant64());
				break;
			case CONDITION_LE:
				result = static_cast<int64>(src1cst->GetConstant64()) <= static_cast<int64>(src2cst->GetConstant64());
				break;
			case CONDITION_GT:
				result = static_cast<int64>(src1cst->GetConstant64()) > static_cast<int64>(src2cst->GetConstant64());
				break;
			default:
				assert(0);
				break;
			}
			changed = true;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result ? 1 : 0));
			statement.src2.reset();
		}
	}

	assert(!(src1cst && src2cst && !changed));

	return changed;
}

bool CJitter::FoldConstant6432Operation(STATEMENT& statement)
{
	CSymbol* src1cst = dynamic_symbolref_cast(SYM_CONSTANT64, statement.src1);
	CSymbol* src2cst = dynamic_symbolref_cast(SYM_CONSTANT, statement.src2);

	//Nothing we can do
	if(src1cst == NULL && src2cst == NULL) return false;

	bool changed = false;

	if(
		statement.op == OP_SLL64 ||
		statement.op == OP_SRL64 ||
		statement.op == OP_SRA64)
	{
		if(src2cst && ((src2cst->m_valueLow & 0x3F) == 0))
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
		else if(src1cst && (src1cst->m_valueLow == 0) && (src1cst->m_valueHigh == 0))
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}

	return changed;
}

bool CJitter::FoldConstant12832Operation(STATEMENT& statement)
{
	auto src2cst = dynamic_symbolref_cast(SYM_CONSTANT, statement.src2);

	//Nothing we can do
	if(src2cst == nullptr) return false;

	bool changed = false;

	if(
		statement.op == OP_MD_SLLH ||
		statement.op == OP_MD_SRLH ||
		statement.op == OP_MD_SRAH)
	{
		if(src2cst && ((src2cst->m_valueLow & 0xF) == 0))
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}
	else if(
		statement.op == OP_MD_SLLW ||
		statement.op == OP_MD_SRLW ||
		statement.op == OP_MD_SRAW)
	{
		if(src2cst && ((src2cst->m_valueLow & 0x1F) == 0))
		{
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
	}

	return changed;
}

bool CJitter::VariableFolding(StatementList& statements)
{
	auto changed = false;
	for(auto& statement : statements)
	{
		if(!statement.src2) continue;

		bool src1cst = statement.src1.get()->GetSymbol().get()->IsConstant();
		bool src2cst = statement.src2.get()->GetSymbol().get()->IsConstant();

		if(src1cst || src2cst || !statement.src1.get()->Equals(statement.src2.get())) continue;

		switch(statement.op)
		{
			case OP_AND64:
			case OP_MD_OR:
			{
				statement.op = OP_MOV;
				statement.src2.reset();
				changed = true;
			}
			break;
			case OP_XOR:
			{
				statement.op = OP_MOV;
				statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, 0));
				statement.src2.reset();
				changed = true;
			}
			break;
			case OP_FP_SUB:
			{
				float constant = 0;
				statement.op = OP_FP_LDCST;
				statement.src1	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, *reinterpret_cast<uint32*>(&constant)));
				statement.src2.reset();
				changed = true;
			}
			break;
			case OP_CMP64:
			case OP_FP_CMP:
			{
				SymbolPtr result;
				switch(statement.jmpCondition)
				{
					case CONDITION_EQ:
					case CONDITION_LE:
					case CONDITION_GE:
						result = MakeSymbol(SYM_CONSTANT, 1);
					break;
					default:
						result = MakeSymbol(SYM_CONSTANT, 0);
					break;

				}
				statement.op = OP_MOV;
				statement.src1 = MakeSymbolRef(result);
				statement.src2.reset();
				changed = true;
			}
			break;
			case OP_FP_DIV:
			{
				float constant = 1;
				statement.op = OP_FP_LDCST;
				statement.src1	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, *reinterpret_cast<uint32*>(&constant)));
				statement.src2.reset();
				changed = true;
			};
			break;
			default:
			break;
		}
	}
	return changed;
}

bool CJitter::ConstantFolding(StatementList& statements)
{
	bool changed = false;
	for(auto& statement : statements)
	{
		changed |= FoldConstantOperation(statement);
		changed |= FoldConstant64Operation(statement);
		changed |= FoldConstant6432Operation(statement);
		changed |= FoldConstant12832Operation(statement);
	}
	return changed;
}

void CJitter::FixFlowControl(StatementList& statements)
{
	//Resolve GOTO instructions
	for(auto& statement : statements)
	{
		if(statement.op == OP_GOTO)
		{
			LabelMapType::const_iterator labelIterator = m_labels.find(statement.jmpBlock);
			assert(labelIterator != m_labels.end());
			if(labelIterator == m_labels.end()) continue;

			statement.op		= OP_JMP;
			statement.jmpBlock	= labelIterator->second;
		}
	}

	//Remove any excess flow control instructions
	for(StatementList::iterator statementIterator(statements.begin());
		statementIterator != statements.end(); ++statementIterator)
	{
		const STATEMENT& statement(*statementIterator);

		if(statement.op == OP_JMP || statement.op == OP_CONDJMP)
		{
			++statementIterator;
			statements.erase(statementIterator, statements.end());
			break;
		}
	}
}

void CJitter::MergeBasicBlocks(BASIC_BLOCK& dstBlock, const BASIC_BLOCK& srcBlock)
{
	auto& dstSymbolTable = dstBlock.symbolTable;

	for(auto statement : srcBlock.statements)
	{
		statement.VisitOperands(
			[&dstSymbolTable](SymbolRefPtr& symbolRef, bool)
			{
				auto symbol = symbolRef->GetSymbol();
				symbolRef = std::make_shared<CSymbolRef>(dstSymbolTable.MakeSymbol(symbol));
			}
		);
		dstBlock.statements.push_back(statement);
	}

	dstBlock.optimized = false;
}

CJitter::BASIC_BLOCK CJitter::ConcatBlocks(const BasicBlockList& blocks)
{
	BASIC_BLOCK result;
	for(const auto& basicBlock : blocks)
	{
		//First, add a mark label statement
		STATEMENT labelStatement;
		labelStatement.op		= OP_LABEL;
		labelStatement.jmpBlock	= basicBlock.id;
		result.statements.push_back(labelStatement);

		MergeBasicBlocks(result, basicBlock);
	}
	return result;
}

bool CJitter::PruneBlocks()
{
	bool changed = true;
	int deletedBlocks = 0;

	while(changed)
	{
		changed = false;

		auto toDeleteIterator = m_basicBlocks.cend();
		for(auto outerBlockIterator(m_basicBlocks.cbegin());
			outerBlockIterator != m_basicBlocks.cend(); outerBlockIterator++)
		{
			//First block is always referenced
			if(outerBlockIterator == m_basicBlocks.begin()) continue;

			auto candidateBlockIterator = outerBlockIterator;
			bool referenced = false;

			//Check if there's a reference to this block in here
			for(auto innerBlockIterator(m_basicBlocks.cbegin());
				innerBlockIterator != m_basicBlocks.cend(); innerBlockIterator++)
			{
				const auto& block(*innerBlockIterator);
				bool referencesNext = false;

				if(block.statements.empty())
				{
					//Empty blocks references next one
					referencesNext = true;
				}
				else
				{
					//Check if this block references the next one or if it jumps to another one
					auto lastInstruction(block.statements.end());
					lastInstruction--;
					const auto& statement(*lastInstruction);

					//It jumps to a block, so check if it references the one we're looking for
					if(statement.op == OP_JMP || statement.op == OP_CONDJMP)
					{
						if(statement.jmpBlock == candidateBlockIterator->id)
						{
							referenced = true;
							break;
						}
					}

					//Otherwise, it references the next one if it's not a jump
					if(statement.op != OP_JMP)
					{
						referencesNext = true;
					}
				}

				if(referencesNext)
				{
					auto nextBlockIterator(innerBlockIterator);
					nextBlockIterator++;
					if(nextBlockIterator != m_basicBlocks.end())
					{
						if(nextBlockIterator->id == candidateBlockIterator->id)
						{
							referenced = true;
							break;
						}
					}
				}
			}

			if(!referenced)
			{
				toDeleteIterator = candidateBlockIterator;
				break;
			}
		}
		if(toDeleteIterator == m_basicBlocks.end()) continue;

		m_basicBlocks.erase(toDeleteIterator);
		deletedBlocks++;
		changed = true;
	}

	HarmonizeBlocks();
	return deletedBlocks != 0;
}

void CJitter::HarmonizeBlocks()
{
	//Remove any jumps that jump to the next block
	for(BasicBlockList::iterator blockIterator(m_basicBlocks.begin());
		blockIterator != m_basicBlocks.end(); ++blockIterator)
	{
		BasicBlockList::iterator nextBlockIterator(blockIterator);
		++nextBlockIterator;
		if(nextBlockIterator == m_basicBlocks.end()) continue;

		auto& basicBlock(*blockIterator);
		if(basicBlock.statements.size() == 0) continue;

		StatementList::iterator lastStatementIterator(basicBlock.statements.end());
		--lastStatementIterator;
		const STATEMENT& statement(*lastStatementIterator);
		if(statement.op != OP_JMP) continue;
		if(statement.jmpBlock != nextBlockIterator->id) continue;

		//Remove the jump
		basicBlock.statements.erase(lastStatementIterator);
	}

	//Flag any block that have a reference from a jump
	for(BasicBlockList::iterator outerBlockIterator(m_basicBlocks.begin());
		outerBlockIterator != m_basicBlocks.end(); ++outerBlockIterator)
	{
		auto& outerBlock(*outerBlockIterator);
		outerBlock.hasJumpRef = false;

		for(const auto& block : m_basicBlocks)
		{
			if(block.statements.size() == 0) continue;

			//Check if this block references the next one or if it jumps to another one
			StatementList::const_iterator lastInstruction(block.statements.end());
			--lastInstruction;
			const STATEMENT& statement(*lastInstruction);

			//It jumps to a block, so check if it references the one we're looking for
			if(statement.op == OP_JMP || statement.op == OP_CONDJMP)
			{
				if(statement.jmpBlock == outerBlockIterator->id)
				{
					outerBlock.hasJumpRef = true;
					break;
				}
			}
		}
	}
}

bool CJitter::MergeBlocks()
{
	int deletedBlocks = 0;
	bool changed = true;
	while(changed)
	{
		changed = false;
		for(BasicBlockList::iterator blockIterator(m_basicBlocks.begin());
			m_basicBlocks.end() != blockIterator; ++blockIterator)
		{
			BasicBlockList::iterator nextBlockIterator(blockIterator);
			++nextBlockIterator;
			if(nextBlockIterator == m_basicBlocks.end()) continue;

			auto& basicBlock(*blockIterator);
			auto& nextBlock(*nextBlockIterator);

			if(nextBlock.hasJumpRef) continue;

			//Check if the last statement is a jump
			if(!basicBlock.statements.empty())
			{
				auto lastStatementIterator(basicBlock.statements.end());
				--lastStatementIterator;
				const auto& statement(*lastStatementIterator);
				if(statement.op == OP_CONDJMP) continue;
				if(statement.op == OP_JMP) continue;
			}

			//Blocks can be merged
			MergeBasicBlocks(basicBlock, nextBlock);

			m_basicBlocks.erase(nextBlockIterator);

			++deletedBlocks;
			changed = true;
			break;
		}
	}
	return deletedBlocks != 0;
}

bool CJitter::ConstantPropagation(StatementList& statements)
{
	bool changed = false;

	for(StatementList::iterator outerStatementIterator(statements.begin());
		statements.end() != outerStatementIterator; ++outerStatementIterator)
	{
		STATEMENT& outerStatement(*outerStatementIterator);

		if(outerStatement.op != OP_MOV) continue;

		CSymbol* constant = dynamic_symbolref_cast(SYM_CONSTANT, outerStatement.src1);
		if(constant == NULL)
		{
			constant = dynamic_symbolref_cast(SYM_CONSTANT64, outerStatement.src1);
		}
		if(!constant) continue;

		//Find anything that uses this operand and replace it with the constant
		for(StatementList::iterator innerStatementIterator(outerStatementIterator);
			statements.end() != innerStatementIterator; ++innerStatementIterator)
		{
			if(outerStatementIterator == innerStatementIterator) continue;

			auto& innerStatement(*innerStatementIterator);
			
			innerStatement.VisitSources(
				[&] (SymbolRefPtr& symbol, bool)
				{
					if(symbol->Equals(outerStatement.dst.get()))
					{
						symbol = outerStatement.src1;
						changed = true;
					}
				}
			);
		}
	}
	return changed;
}

bool CJitter::ReorderAdd(StatementList& statements)
{
	bool changed = false;

	for(auto statementIterator(statements.begin());
		statements.end() != statementIterator; ++statementIterator)
	{
		auto& statement(*statementIterator);

		//We're only interested by additions
		if(statement.op != OP_ADD) continue;

		//Do some more checks
		auto addDst = statement.dst.get();
		assert(addDst);

		auto addSrc2Cst = dynamic_symbolref_cast(SYM_CONSTANT, statement.src2);

		//Don't mess with relatives
		if(addDst->GetSymbol()->IsRelative() || !addSrc2Cst)
		{
			continue;
		}

		//Check for OP_SLL that uses the result of this operation and propagate the shift
		auto nextStatementIterator = std::next(statementIterator);
		auto& nextStatement(*nextStatementIterator);
		if(nextStatement.op == OP_SLL && nextStatement.src1->Equals(addDst))
		{
			auto shiftSrc2Cst = dynamic_symbolref_cast(SYM_CONSTANT, nextStatement.src2);
			if(shiftSrc2Cst)
			{
				uint32 result = addSrc2Cst->m_valueLow << shiftSrc2Cst->m_valueLow;

				std::swap(statement, nextStatement);
				std::swap(statement.src1, nextStatement.src1);
				std::swap(statement.dst, nextStatement.dst);
				nextStatement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
				changed = true;
			}
		}
	}
	return changed;
}

bool CJitter::CopyPropagation(StatementList& statements)
{
	bool changed = false;

	struct USAGEINFO
	{
		uint32 count = 0;
		StatementList::iterator lastUse;
	};
	std::map<SymbolPtr, USAGEINFO> usageInfos;

	for(auto statementIterator(statements.begin());
	    statements.end() != statementIterator; ++statementIterator)
	{
		auto& statement(*statementIterator);

		statement.VisitSources(
		    [&](const SymbolRefPtr& symbolRef, bool) {
			    auto symbol = symbolRef->GetSymbol();
			    if(symbol->IsRelative()) return;
			    if(symbol->IsConstant()) return;
			    auto& usageInfo = usageInfos[symbol];
			    usageInfo.count++;
			    usageInfo.lastUse = statementIterator;
		    });
	}

	for(auto outerStatementIterator(statements.begin());
	    statements.end() != outerStatementIterator; ++outerStatementIterator)
	{
		auto& outerStatement(*outerStatementIterator);

		//Some operations we can't propagate
		if(outerStatement.op == OP_RETVAL) continue;

		CSymbolRef* outerDstSymbol = outerStatement.dst.get();
		if(outerDstSymbol == NULL) continue;

		//Don't mess with relatives
		if(outerDstSymbol->GetSymbol()->IsRelative())
		{
			continue;
		}

		auto usageInfoIterator = usageInfos.find(outerDstSymbol->GetSymbol());
		if(usageInfoIterator == std::end(usageInfos))
		{
			continue;
		}

		const auto& usageInfo = usageInfoIterator->second;
		if(usageInfo.count != 1)
		{
			continue;
		}

		auto& innerStatement(*usageInfo.lastUse);
		if(!innerStatement.src1->Equals(outerDstSymbol))
		{
			//Possibly excluding interesting optimization possibilities
			continue;
		}

		//Substitute a OP_MOV statement that use outerDstSymbol with its definition (outerStatement)
		//Example:
		//outerDstSymbol -> t0
		//Before:
		// - t0 = r0 + r1   //outerStatement
		// - t1 = t0        //innerStatement
		//After:
		// - t0 = r0 + r1   //outerStatement
		// - t1 = r0 + r1   //innerStatement
		//After substitution, t0 will not be used anymore making outerStatement eligible for removal
		if(innerStatement.op == OP_MOV)
		{
			innerStatement.op = outerStatement.op;
			innerStatement.src1 = outerStatement.src1;
			innerStatement.src2 = outerStatement.src2;
			innerStatement.src3 = outerStatement.src3;
			innerStatement.jmpCondition = outerStatement.jmpCondition;
			changed = true;
		}
		//Find all the add/sub constant and add them together
		//Example
		//outerDstSymbol -> t0
		//Before:
		// - t0 = r0 + 10   //outerStatement
		// - t1 = t0 + 20   //innerStatement
		//After:
		// - t0 = r0 + 10   //outerStatement
		// - t1 = r0 + 30   //innerStatement
		//After substitution, t0 will not be used anymore making outerStatement eligible for removal
		else if(
			(outerStatement.op == innerStatement.op) && 
			((innerStatement.op == OP_ADD) || (innerStatement.op == OP_ADDREF))
			)
		{
			auto innerSrc2cst = dynamic_symbolref_cast(SYM_CONSTANT, innerStatement.src2);
			auto outerSrc2cst = dynamic_symbolref_cast(SYM_CONSTANT, outerStatement.src2);
			if(innerSrc2cst && outerSrc2cst)
			{
				uint32 result = innerSrc2cst->m_valueLow + outerSrc2cst->m_valueLow;
				innerStatement.src1 = outerStatement.src1;
				innerStatement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
				changed = true;
			}
		}
	}

	return changed;
}

bool CJitter::CommonExpressionElimination(VERSIONED_STATEMENT_LIST& versionedStatementList)
{
	bool changed = false;
	std::vector<StatementList::const_iterator> tempDefs;
	std::unordered_map<SymbolPtr, SymbolPtr> tempReplaceMap;
	tempDefs.reserve(versionedStatementList.statements.size());

	for(auto statementIterator(versionedStatementList.statements.begin());
	    versionedStatementList.statements.end() != statementIterator; ++statementIterator)
	{
		auto& statement(*statementIterator);

		//If this is a statement defining a temporary
		if(
		    (statement.op != OP_RETVAL) &&
		    (statement.dst) &&
		    (statement.dst->GetSymbol()->IsTemporary()))
		{
			const auto& newTemp = statement.dst->GetSymbol();

			bool found = false;
			//Check if our temporary already has a similar definition
			for(const auto& tempDef : tempDefs)
			{
				const auto& tempDefStatement = *tempDef;
				assert(tempDefStatement.dst);
				const auto& temp = tempDefStatement.dst;
				if(statement.op != tempDefStatement.op) continue;
				if(statement.jmpCondition != tempDefStatement.jmpCondition) continue;
				if(statement.src1 && !statement.src1->Equals(tempDefStatement.src1.get())) continue;
				if(statement.src2 && !statement.src2->Equals(tempDefStatement.src2.get())) continue;
				if(statement.src3 && !statement.src3->Equals(tempDefStatement.src3.get())) continue;
				auto [_, inserted] = tempReplaceMap.insert(std::make_pair(newTemp, temp->GetSymbol()));
				assert(inserted);
				found = true;
				//We assume the first replacement we find is gonna be the best
				break;
			}

			//We haven't found a replacement for our definition, assume it's new
			if(!found)
			{
				tempDefs.push_back(statementIterator);
			}
		}

		if(tempReplaceMap.empty())
		{
			continue;
		}

		statement.VisitSources(
		    [&](SymbolRefPtr& innerSymbolRef, bool) {
			    if(!innerSymbolRef->GetSymbol()->IsTemporary()) return;
			    if(auto tempReplaceIterator = tempReplaceMap.find(innerSymbolRef->GetSymbol()); tempReplaceIterator != std::end(tempReplaceMap))
			    {
				    innerSymbolRef = MakeSymbolRef(tempReplaceIterator->second);
				    changed = true;
			    }
		    });
	}

	return changed;
}

bool CJitter::DeadcodeElimination(VERSIONED_STATEMENT_LIST& versionedStatementList)
{
	bool changed = false;

	typedef std::list<StatementList::iterator> ToDeleteList;
	ToDeleteList toDelete;

	for(auto outerStatementIterator(versionedStatementList.statements.begin());
		versionedStatementList.statements.end() != outerStatementIterator; ++outerStatementIterator)
	{
		auto& outerStatement(*outerStatementIterator);
		const auto& symbolRef(outerStatement.dst);

		CSymbol* candidate = nullptr;
		if(symbolRef && symbolRef->GetSymbol()->IsTemporary())
		{
			candidate = symbolRef->GetSymbol().get();
		}
		else if(auto relativeSymbol = dynamic_symbolref_cast(SYM_RELATIVE, symbolRef))
		{
			assert(symbolRef->IsVersioned());
			if(symbolRef->GetVersion() != versionedStatementList.relativeVersions.GetRelativeVersion(relativeSymbol->m_valueLow))
			{
				candidate = relativeSymbol;
			}
		}

		if(!candidate) continue;

		//Look for any possible use of this symbol
		bool used = false;
		for(auto innerStatementIterator(outerStatementIterator);
			versionedStatementList.statements.end() != innerStatementIterator; ++innerStatementIterator)
		{
			if(outerStatementIterator == innerStatementIterator) continue;

			const auto& innerStatement(*innerStatementIterator);
			
			innerStatement.VisitSources(
				[&](const SymbolRefPtr& innerSymbolRef, bool)
				{
					if(innerSymbolRef->Equals(symbolRef.get()))
					{
						used = true;
						return;
					}

					auto symbol(innerSymbolRef->GetSymbol());
					if(!symbol->Equals(candidate) && symbol->Aliases(candidate))
					{
						used = true;
						return;
					}
				}
			);
			
			if(used) break;
		}

		if(!used)
		{
			//Kill it!
			toDelete.push_back(outerStatementIterator);
		}
	}

	for(ToDeleteList::const_iterator deleteIterator(toDelete.begin());
		toDelete.end() != deleteIterator; ++deleteIterator)
	{
		versionedStatementList.statements.erase(*deleteIterator);
		changed = true;
	}

	return changed;
}

void CJitter::CoalesceTemporaries(BASIC_BLOCK& basicBlock)
{
	typedef std::vector<CSymbol*> EncounteredTempList;
	EncounteredTempList encounteredTemps;

	for(auto outerStatementIterator(basicBlock.statements.begin());
		basicBlock.statements.end() != outerStatementIterator; ++outerStatementIterator)
	{
		auto& outerStatement(*outerStatementIterator);

		if(!outerStatement.dst) continue;
		if(!outerStatement.dst->GetSymbol()->IsTemporary()) continue;

		auto tempSymbol = outerStatement.dst->GetSymbol().get();
		CSymbol* candidate = nullptr;

		//Check for a possible replacement
		for(auto* encounteredTemp : encounteredTemps)
		{
			if(encounteredTemp->m_type != tempSymbol->m_type) continue;

			//Look for any possible use of this symbol
			bool used = false;

			for(auto innerStatementIterator(outerStatementIterator);
				basicBlock.statements.end() != innerStatementIterator; ++innerStatementIterator)
			{
				if(outerStatementIterator == innerStatementIterator) continue;

				const auto& innerStatement(*innerStatementIterator);
				innerStatement.VisitOperands(
					[&](const SymbolRefPtr& symbolRef, bool)
					{
						auto symbol = symbolRef->GetSymbol();
						if(symbol->Equals(encounteredTemp))
						{
							used = true;
						}
					}
				);
				
				if(used) break;
			}

			if(!used)
			{
				candidate = encounteredTemp;
				break;
			}
		}

		if(candidate == nullptr)
		{
			encounteredTemps.push_back(tempSymbol);
		}
		else
		{
			auto candidatePtr = MakeSymbol(candidate->m_type, candidate->m_valueLow);

			outerStatement.dst = MakeSymbolRef(candidatePtr);

			//Replace all occurences of this temp with the candidate
			for(auto innerStatementIterator(outerStatementIterator);
				basicBlock.statements.end() != innerStatementIterator; ++innerStatementIterator)
			{
				if(outerStatementIterator == innerStatementIterator) continue;

				auto& innerStatement(*innerStatementIterator);

				if(innerStatement.dst)
				{
					auto symbol(innerStatement.dst->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.dst = MakeSymbolRef(candidatePtr);
					}
				}
				if(innerStatement.src1)
				{
					auto symbol(innerStatement.src1->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.src1 = MakeSymbolRef(candidatePtr);
					}
				}
				if(innerStatement.src2)
				{
					auto symbol(innerStatement.src2->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.src2 = MakeSymbolRef(candidatePtr);
					}
				}
				if(innerStatement.src3)
				{
					auto symbol(innerStatement.src3->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.src3 = MakeSymbolRef(candidatePtr);
					}
				}
			}
		}
	}
}

void CJitter::RemoveSelfAssignments(BASIC_BLOCK& basicBlock)
{
	for(StatementList::iterator outerStatementIterator(basicBlock.statements.begin());
		basicBlock.statements.end() != outerStatementIterator; )
	{
		STATEMENT& outerStatement(*outerStatementIterator);

		if(outerStatement.op == OP_MOV && outerStatement.dst->Equals(outerStatement.src1.get()))
		{
			outerStatementIterator = basicBlock.statements.erase(outerStatementIterator);
		}
		else
		{
			++outerStatementIterator;
		}
	}
}

void CJitter::PruneSymbols(BASIC_BLOCK& basicBlock) const
{
	auto& symbolTable(basicBlock.symbolTable);

	SymbolUseCountMap symbolUseCount;
	for(const auto& statement : basicBlock.statements)
	{
		statement.VisitOperands(
			[&] (const SymbolRefPtr& symbolRef, bool)
			{
				const auto& symbol = symbolRef->GetSymbol();
				symbolUseCount[symbol.get()]++;
			}
		);
	}

	for(auto symbolIterator(std::begin(symbolTable.GetSymbols()));
		symbolIterator != std::end(symbolTable.GetSymbols());)
	{
		const auto& symbol(*symbolIterator);
		if(symbolUseCount.find(symbol.get()) == std::end(symbolUseCount))
		{
			symbolIterator = symbolTable.RemoveSymbol(symbolIterator);
		}
		else
		{
			symbolIterator++;
		}
	}
}

unsigned int CJitter::AllocateStack(BASIC_BLOCK& basicBlock)
{
	unsigned int stackAlloc = 0;
	for(const auto& symbol : basicBlock.symbolTable.GetSymbols())
	{
		if((symbol->m_type == SYM_TEMPORARY) || (symbol->m_type == SYM_FP_TEMPORARY32))
		{
			symbol->m_stackLocation = stackAlloc;
			stackAlloc += 4;
		}
		else if(symbol->m_type == SYM_TMP_REFERENCE)
		{
			size_t symbolSize = sizeof(void*);
			size_t symbolMask = symbolSize - 1;
			if((stackAlloc & symbolMask) != 0)
			{
				stackAlloc += (symbolSize - (stackAlloc & symbolMask));
			}
			assert((stackAlloc & symbolMask) == 0);
			symbol->m_stackLocation = stackAlloc;
			stackAlloc += symbolSize;
		}
		else if(symbol->m_type == SYM_TEMPORARY64)
		{
			if((stackAlloc & 7) != 0)
			{
				stackAlloc += (8 - (stackAlloc & 7));
			}
			assert((stackAlloc & 7) == 0);
			symbol->m_stackLocation = stackAlloc;
			stackAlloc += 8;
		}
		else if(symbol->m_type == SYM_TEMPORARY128)
		{
			if((stackAlloc & 15) != 0)
			{
				stackAlloc += (16 - (stackAlloc & 15));
			}
			assert((stackAlloc & 15) == 0);
			symbol->m_stackLocation = stackAlloc;
			stackAlloc += 16;
		}
		else if(symbol->m_type == SYM_TEMPORARY256)
		{
			if((stackAlloc & 31) != 0)
			{
				stackAlloc += (32 - (stackAlloc & 31));
			}
			assert((stackAlloc & 31) == 0);
			symbol->m_stackLocation = stackAlloc;
			stackAlloc += 32;
		}
	}
	return stackAlloc;
}

void CJitter::NormalizeStatements(BASIC_BLOCK& basicBlock)
{
	//Reorganize the commutative statements 
	//1. Always have registers as the first operand
	//2. Always have constants as the last operand

	for(auto& statement : basicBlock.statements)
	{
		bool isCommutative = false;
		bool conditionSwapRequired = false;

		switch(statement.op)
		{
			case OP_ADD:
			case OP_ADD64:
			case OP_AND:
			case OP_AND64:
			case OP_OR:
			case OP_XOR:
			case OP_MUL:
			case OP_MULS:
			case OP_MD_AND:
			case OP_MD_OR:
			case OP_MD_XOR:
			case OP_MD_ADD_B:
			case OP_MD_ADD_H:
			case OP_MD_ADD_W:
			case OP_MD_ADDSS_H:
			case OP_MD_ADDSS_W:
			case OP_MD_ADDUS_B:
			case OP_MD_ADDUS_W:
			case OP_MD_CMPEQ_B:
			case OP_MD_CMPEQ_H:
			case OP_MD_CMPEQ_W:
			case OP_MD_MIN_H:
			case OP_MD_MIN_W:
			case OP_MD_MAX_H:
			case OP_MD_MAX_W:
			case OP_MD_ADD_S:
			case OP_MD_MUL_S:
			case OP_MD_MIN_S:
			case OP_MD_MAX_S:
				isCommutative = true;
				break;
			case OP_CMP:
			case OP_CMP64:
			case OP_CONDJMP:
				isCommutative = true;
				conditionSwapRequired = true;
				break;
			default:
				isCommutative = false;
				break;
		}

		if(!isCommutative) continue;

		bool swapped = false;

		//Check if constant operand is at the beginning and swap if it is the case
		{
			bool src1cst = statement.src1->GetSymbol()->IsConstant();
			bool src2cst = statement.src2->GetSymbol()->IsConstant();

			if(src1cst && !src2cst)
			{
				std::swap(statement.src1, statement.src2);
				swapped = true;
			}
		}

		//Check if register operand is at the end and swap if it is the case
		{
			bool dstreg  = statement.dst && statement.dst->GetSymbol()->IsRegister();
			bool src1reg = statement.src1->GetSymbol()->IsRegister();
			bool src2reg = statement.src2->GetSymbol()->IsRegister();

			if(!src1reg && src2reg)
			{
				std::swap(statement.src1, statement.src2);
				swapped = true;
			}
			else if(dstreg && src1reg && src2reg && 
				statement.dst->GetSymbol()->Equals(statement.src2->GetSymbol().get()))
			{
				//If all operands are registers and dst is equal to src2, swap to make dst and src2 side by side
				std::swap(statement.src1, statement.src2);
				swapped = true;
			}
		}

		if(swapped && conditionSwapRequired)
		{
			switch(statement.jmpCondition)
			{
			case CONDITION_EQ:
			case CONDITION_NE:
				break;
			case CONDITION_BL:
				statement.jmpCondition = CONDITION_AB;
				break;
			case CONDITION_AB:
				statement.jmpCondition = CONDITION_BL;
				break;
			case CONDITION_LT:
				statement.jmpCondition = CONDITION_GT;
				break;
			case CONDITION_GT:
				statement.jmpCondition = CONDITION_LT;
				break;
			default:
				assert(0);
				break;
			}
		}
	}
}
