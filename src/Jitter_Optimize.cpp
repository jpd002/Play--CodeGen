#include <assert.h>
#include <vector>
#include "Jitter.h"

#ifdef _DEBUG
//#define DUMP_STATEMENTS
#endif

#ifdef DUMP_STATEMENTS
#include <iostream>
#endif

using namespace std;
using namespace Jitter;

static bool IsPowerOfTwo(uint32 number)
{
	uint32 complement = number - 1;
	return (number != 0) && ((number & complement) == 0);
}

static uint32 ones32(uint32 x)
{
	/* 32-bit recursive reduction using SWAR...
	   but first step is mapping 2-bit values
	   into sum of 2 1-bit values in sneaky way
	*/
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8);
	x += (x >> 16);
	return (x & 0x0000003f);
}

static uint32 GetPowerOf2(uint32 x)
{
	assert(IsPowerOfTwo(x));

	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);

	return ones32(x >> 1);
}

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
				symbolRef = SymbolRefPtr(new CVersionedSymbolRef(symbolRef->GetSymbol(), currentVersion));
			}
			if(CSymbol* symbol = dynamic_symbolref_cast(SYM_RELATIVE64, symbolRef))
			{
				unsigned int currentVersion = relativeVersions.GetRelativeVersion(symbol->m_valueLow);
				symbolRef = SymbolRefPtr(new CVersionedSymbolRef(symbolRef->GetSymbol(), currentVersion));
			}
		}
	};

	for(StatementList::const_iterator statementIterator(statements.begin());
		statements.end() != statementIterator; statementIterator++)
	{
		STATEMENT newStatement(*statementIterator);

		ReplaceUse()(newStatement.src1, result.relativeVersions);
		ReplaceUse()(newStatement.src2, result.relativeVersions);

		if(CSymbol* dst = dynamic_symbolref_cast(SYM_RELATIVE, newStatement.dst))
		{
			unsigned int nextVersion = result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow);
			newStatement.dst = SymbolRefPtr(new CVersionedSymbolRef(newStatement.dst->GetSymbol(), nextVersion));
		}
		//Increment relative versions to prevent some optimization problems
		else if(CSymbol* dst = dynamic_symbolref_cast(SYM_FP_REL_SINGLE, newStatement.dst))
		{
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow);
		}
		else if(CSymbol* dst = dynamic_symbolref_cast(SYM_FP_REL_INT32, newStatement.dst))
		{
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow);
		}
		else if(CSymbol* dst = dynamic_symbolref_cast(SYM_RELATIVE64, newStatement.dst))
		{
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 0);
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 4);
		}
		else if(CSymbol* dst = dynamic_symbolref_cast(SYM_RELATIVE128, newStatement.dst))
		{
			uint8 mask = 0xF;
			if(newStatement.op == OP_MD_MOV_MASKED)
			{
				mask = static_cast<uint8>(newStatement.src2->GetSymbol()->m_valueLow);
			}

			if(mask & 0x01) result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 0);
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
	for(StatementList::const_iterator statementIterator(statements.statements.begin());
		statementIterator != statements.statements.end(); statementIterator++)
	{
		STATEMENT newStatement(*statementIterator);

		if(VersionedSymbolRefPtr src1 = std::dynamic_pointer_cast<CVersionedSymbolRef>(newStatement.src1))
		{
			newStatement.src1 = SymbolRefPtr(new CSymbolRef(src1->GetSymbol()));
		}

		if(VersionedSymbolRefPtr src2 = std::dynamic_pointer_cast<CVersionedSymbolRef>(newStatement.src2))
		{
			newStatement.src2 = SymbolRefPtr(new CSymbolRef(src2->GetSymbol()));
		}

		if(VersionedSymbolRefPtr dst = std::dynamic_pointer_cast<CVersionedSymbolRef>(newStatement.dst))
		{
			newStatement.dst = SymbolRefPtr(new CSymbolRef(dst->GetSymbol()));
		}

		result.push_back(newStatement);
	}
	return result;
}

void CJitter::Compile()
{
	while(1)
	{
		for(BasicBlockList::iterator blockIterator(m_basicBlocks.begin());
			m_basicBlocks.end() != blockIterator; blockIterator++)
		{
			BASIC_BLOCK& basicBlock(blockIterator->second);
			if(!basicBlock.optimized)
			{
				m_currentBlock = &basicBlock;

				//DumpStatementList(m_currentBlock->statements);

				VERSIONED_STATEMENT_LIST versionedStatements = GenerateVersionedStatementList(basicBlock.statements);

				while(1)
				{
					bool dirty = false;
					dirty |= ConstantPropagation(versionedStatements.statements);
					dirty |= ConstantFolding(versionedStatements.statements);
					dirty |= CopyPropagation(versionedStatements.statements);
					dirty |= DeadcodeElimination(versionedStatements);

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

	//Allocate registers
	for(BasicBlockList::iterator blockIterator(m_basicBlocks.begin());
		m_basicBlocks.end() != blockIterator; blockIterator++)
	{
		BASIC_BLOCK& basicBlock(blockIterator->second);
		m_currentBlock = &basicBlock;

		CoalesceTemporaries(basicBlock);
		RemoveSelfAssignments(basicBlock);
		ComputeLivenessAndPruneSymbols(basicBlock);
		AllocateRegisters(basicBlock);
		NormalizeStatements(basicBlock);
	}

	BASIC_BLOCK result = ConcatBlocks(m_basicBlocks);

#ifdef DUMP_STATEMENTS
	DumpStatementList(result.statements);
	cout << endl;
#endif

	unsigned int stackSize = AllocateStack(result);
	m_codeGen->GenerateCode(result.statements, stackSize);

	m_labels.clear();
}

std::string CJitter::ConditionToString(CONDITION condition)
{
	switch(condition)
	{
	case CONDITION_LT:
		return "LT";
		break;
	case CONDITION_LE:
		return "LE";
		break;
	case CONDITION_GT:
		return "GT";
		break;
	case CONDITION_EQ:
		return "EQ";
		break;
	case CONDITION_NE:
		return "NE";
		break;
	case CONDITION_BL:
		return "BL";
		break;
	case CONDITION_AB:
		return "AB";
		break;
	default:
		return "??";
		break;
	}
}

void CJitter::DumpStatementList(const StatementList& statements)
{
#ifdef DUMP_STATEMENTS
	for(StatementList::const_iterator statementIterator(statements.begin());
		statements.end() != statementIterator; statementIterator++)
	{
		const STATEMENT& statement(*statementIterator);

		if(statement.dst)
		{
			cout << statement.dst->ToString();
			cout << " := ";
		}

		if(statement.src1)
		{
			cout << statement.src1->ToString();
		}

		switch(statement.op)
		{
		case OP_ADD:
		case OP_ADD64:
		case OP_ADDREF:
		case OP_FP_ADD:
			cout << " + ";
			break;
		case OP_SUB:
		case OP_SUB64:
		case OP_FP_SUB:
			cout << " - ";
			break;
		case OP_CMP:
		case OP_CMP64:
		case OP_FP_CMP:
			cout << " CMP(" << ConditionToString(statement.jmpCondition) << ") ";
			break;
		case OP_MUL:
		case OP_MULS:
		case OP_FP_MUL:
			cout << " * ";
			break;
		case OP_MULSHL:
			cout << " *(HL) ";
			break;
		case OP_MULSHH:
			cout << " *(HH) ";
			break;
		case OP_DIV:
		case OP_DIVS:
		case OP_FP_DIV:
			cout << " / ";
			break;
		case OP_AND:
		case OP_AND64:
		case OP_MD_AND:
			cout << " & ";
			break;
		case OP_LZC:
			cout << " LZC";
			break;
		case OP_OR:
		case OP_MD_OR:
			cout << " | ";
			break;
		case OP_XOR:
		case OP_MD_XOR:
			cout << " ^ ";
			break;
		case OP_NOT:
		case OP_MD_NOT:
			cout << " ! ";
			break;
		case OP_SRL:
		case OP_SRL64:
			cout << " >> ";
			break;
		case OP_SRA:
		case OP_SRA64:
			cout << " >>A ";
			break;
		case OP_SLL:
		case OP_SLL64:
			cout << " << ";
			break;
		case OP_NOP:
			cout << " NOP ";
			break;
		case OP_MOV:
			break;
		case OP_STOREATREF:
			cout << " <- ";
			break;
		case OP_LOADFROMREF:
			cout << " LOADFROM ";
			break;
		case OP_RELTOREF:
			cout << " TOREF ";
			break;
		case OP_PARAM:
		case OP_PARAM_RET:
			cout << " PARAM ";
			break;
		case OP_CALL:
			cout << " CALL ";
			break;
		case OP_RETVAL:
			cout << " RETURNVALUE ";
			break;
		case OP_JMP:
			cout << " JMP{" << statement.jmpBlock << "} ";
			break;
		case OP_CONDJMP:
			cout << " JMP{" << statement.jmpBlock << "}(" << ConditionToString(statement.jmpCondition) << ") ";
			break;
		case OP_LABEL:
			cout << "LABEL_" << statement.jmpBlock << ":";
			break;
		case OP_EXTLOW64:
			cout << " EXTLOW64";
			break;
		case OP_EXTHIGH64:
			cout << " EXTHIGH64";
			break;
		case OP_MERGETO64:
			cout << " MERGETO64 ";
			break;
		case OP_MERGETO256:
			cout << " MERGETO256 ";
			break;
		case OP_FP_ABS:
			cout << " ABS";
			break;
		case OP_FP_NEG:
			cout << " NEG";
			break;
		case OP_FP_MIN:
			cout << " MIN ";
			break;
		case OP_FP_MAX:
			cout << " MAX ";
			break;
		case OP_FP_SQRT:
			cout << " SQRT";
			break;
		case OP_FP_RSQRT:
			cout << " RSQRT";
			break;
		case OP_FP_RCPL:
			cout << " RCPL";
			break;
		case OP_FP_TOINT_TRUNC:
			cout << " INT(TRUNC)";
			break;
		case OP_FP_LDCST:
			cout << " LOAD ";
			break;
		case OP_MD_MOV_MASKED:
			cout << " MOVMSK ";
			break;
		case OP_MD_PACK_HB:
			cout << " PACK_HB ";
			break;
		case OP_MD_PACK_WH:
			cout << " PACK_WH ";
			break;
		case OP_MD_UNPACK_LOWER_BH:
			cout << " UNPACK_LOWER_BH ";
			break;
		case OP_MD_UNPACK_LOWER_HW:
			cout << " UNPACK_LOWER_HW ";
			break;
		case OP_MD_UNPACK_LOWER_WD:
			cout << " UNPACK_LOWER_WD ";
			break;
		case OP_MD_UNPACK_UPPER_WD:
			cout << " UNPACK_UPPER_WD ";
			break;
		case OP_MD_ADD_H:
			cout << " +(H) ";
			break;
		case OP_MD_ADD_W:
			cout << " +(W) ";
			break;
		case OP_MD_ADDUS_W:
			cout << " +(USW) ";
			break;
		case OP_MD_ADDSS_W:
			cout << " +(SSW) ";
			break;
		case OP_MD_SUB_B:
			cout << " -(B) ";
			break;
		case OP_MD_SUB_W:
			cout << " -(W) ";
			break;
		case OP_MD_SLLW:
			cout << " <<(W) ";
			break;
		case OP_MD_SRLW:
			cout << " >>(W) ";
			break;
		case OP_MD_SRL256:
			cout << " >>(256) ";
			break;
		case OP_MD_CMPEQ_W:
			cout << " CMP(EQ,W) ";
			break;
		case OP_MD_ADD_S:
			cout << " +(S) ";
			break;
		case OP_MD_SUB_S:
			cout << " -(S) ";
			break;
		case OP_MD_MUL_S:
			cout << " *(S) ";
			break;
		case OP_MD_DIV_S:
			cout << " /(S) ";
			break;
		case OP_MD_MIN_S:
			cout << " MIN(S) ";
			break;
		case OP_MD_MAX_S:
			cout << " MAX(S) ";
			break;
		case OP_MD_ISNEGATIVE:
			cout << " ISNEGATIVE";
			break;
		case OP_MD_ISZERO:
			cout << " ISZERO";
			break;
		case OP_MD_EXPAND:
			cout << " EXPAND";
			break;
		case OP_MD_TOWORD_TRUNCATE:
			cout << " TOWORD_TRUNCATE";
			break;
		default:
			cout << " ?? ";
			break;
		}

		if(statement.src2)
		{
			cout << statement.src2->ToString();
		}

		cout << endl;
	}
#endif
}

uint32 CJitter::CreateBlock()
{
	uint32 newId = m_nextBlockId++;
	m_basicBlocks[newId] = BASIC_BLOCK();
	return newId;
}

CJitter::BASIC_BLOCK* CJitter::GetBlock(uint32 blockId)
{
	BasicBlockList::iterator blockIterator(m_basicBlocks.find(blockId));
	if(blockIterator == m_basicBlocks.end()) return NULL;
	return &blockIterator->second;
}

void CJitter::InsertStatement(const STATEMENT& statement)
{
	m_currentBlock->statements.push_back(statement);
}

SymbolPtr CJitter::MakeSymbol(SYM_TYPE type, uint32 value)
{
	return MakeSymbol(m_currentBlock, type, value, 0);
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

int CJitter::GetSymbolSize(const SymbolRefPtr& symbolRef)
{
	return symbolRef->GetSymbol().get()->GetSize();
}

SymbolRefPtr CJitter::MakeSymbolRef(const SymbolPtr& symbol)
{
	return SymbolRefPtr(new CSymbolRef(symbol));
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
			uint32 result = static_cast<int32>(src1cst->m_valueLow) >> static_cast<int32>(src2cst->m_valueLow);
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SRL)
	{
		if(src1cst && src2cst)
		{
			uint32 result = src1cst->m_valueLow >> src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SLL)
	{
		if(src1cst && src2cst)
		{
			uint32 result = src1cst->m_valueLow << src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
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
			uint32 quotient = src1cst->m_valueLow / src2cst->m_valueLow;
			uint32 remainder = src1cst->m_valueLow % src2cst->m_valueLow;
			uint64 result = static_cast<uint64>(quotient) | (static_cast<uint64>(remainder) << 32);
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
			uint32 quotient = static_cast<int32>(src1cst->m_valueLow) / static_cast<int32>(src2cst->m_valueLow);
			uint32 remainder = static_cast<int32>(src1cst->m_valueLow) % static_cast<int32>(src2cst->m_valueLow);
			uint64 result = static_cast<uint64>(quotient) | (static_cast<uint64>(remainder) << 32);
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
		if(src2cst && (src2cst->m_valueLow == 0) && (src2cst->m_valueHigh == 0))
		{
			statement.op = OP_MOV;
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
	}
	else if(statement.op == OP_CMP64)
	{
		if(src1cst && src2cst)
		{
			bool result = false;
			switch(statement.jmpCondition)
			{
			case CONDITION_NE:
				result = !(
							(src1cst->m_valueLow  == src2cst->m_valueLow) && 
							(src1cst->m_valueHigh == src2cst->m_valueHigh)
							);
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
		if(src2cst && (src2cst->m_valueLow == 0))
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

bool CJitter::ConstantFolding(StatementList& statements)
{
	bool changed = false;

	for(auto statementIterator(std::begin(statements));
		std::end(statements) != statementIterator; statementIterator++)
	{
		STATEMENT& statement(*statementIterator);
		changed |= FoldConstantOperation(statement);
		changed |= FoldConstant64Operation(statement);
		changed |= FoldConstant6432Operation(statement);
	}
	return changed;
}

void CJitter::FixFlowControl(StatementList& statements)
{
	//Resolve GOTO instructions
	for(StatementList::iterator statementIterator(statements.begin());
		statementIterator != statements.end(); statementIterator++)
	{
		STATEMENT& statement(*statementIterator);

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
		statementIterator != statements.end(); statementIterator++)
	{
		const STATEMENT& statement(*statementIterator);

		if(statement.op == OP_JMP || statement.op == OP_CONDJMP)
		{
			statementIterator++;
			statements.erase(statementIterator, statements.end());
			break;
		}
	}
}

void CJitter::MergeBasicBlocks(BASIC_BLOCK& dstBlock, const BASIC_BLOCK& srcBlock)
{
	const StatementList& srcStatements(srcBlock.statements);
	CSymbolTable& dstSymbolTable(dstBlock.symbolTable);

	for(StatementList::const_iterator statementIterator(srcStatements.begin());
		statementIterator != srcStatements.end(); statementIterator++)
	{
		STATEMENT statement(*statementIterator);

		if(statement.dst)
		{
			SymbolPtr symbol(statement.dst->GetSymbol());
			statement.dst = SymbolRefPtr(new CSymbolRef(dstSymbolTable.MakeSymbol(symbol)));
		}

		if(statement.src1)
		{
			SymbolPtr symbol(statement.src1->GetSymbol());
			statement.src1 = SymbolRefPtr(new CSymbolRef(dstSymbolTable.MakeSymbol(symbol)));
		}

		if(statement.src2)
		{
			SymbolPtr symbol(statement.src2->GetSymbol());
			statement.src2 = SymbolRefPtr(new CSymbolRef(dstSymbolTable.MakeSymbol(symbol)));
		}

		dstBlock.statements.push_back(statement);
	}

	dstBlock.optimized = false;
}

CJitter::BASIC_BLOCK CJitter::ConcatBlocks(const BasicBlockList& blocks)
{
	BASIC_BLOCK result;
	for(BasicBlockList::const_iterator blockIterator(blocks.begin());
		blockIterator != blocks.end(); blockIterator++)
	{
		const BASIC_BLOCK& basicBlock(blockIterator->second);
		//const StatementList& statements(basicBlock.statements);

		//First, add a mark label statement
		STATEMENT labelStatement;
		labelStatement.op		= OP_LABEL;
		labelStatement.jmpBlock	= blockIterator->first;
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

		int toDelete = -1;
		for(BasicBlockList::const_iterator outerBlockIterator(m_basicBlocks.begin());
			outerBlockIterator != m_basicBlocks.end(); outerBlockIterator++)
		{
			//First block is always referenced
			if(outerBlockIterator == m_basicBlocks.begin()) continue;

			unsigned int blockId = outerBlockIterator->first;
			bool referenced = false;

			//Check if there's a reference to this block in here
			for(BasicBlockList::const_iterator innerBlockIterator(m_basicBlocks.begin());
				innerBlockIterator != m_basicBlocks.end(); innerBlockIterator++)
			{
				const BASIC_BLOCK& block(innerBlockIterator->second);
				if(block.statements.size() == 0) continue;

				//Check if this block references the next one or if it jumps to another one
				StatementList::const_iterator lastInstruction(block.statements.end());
				lastInstruction--;
				const STATEMENT& statement(*lastInstruction);

				//It jumps to a block, so check if it references the one we're looking for
				if(statement.op == OP_JMP || statement.op == OP_CONDJMP)
				{
					if(statement.jmpBlock == blockId)
					{
						referenced = true;
						break;
					}
				}

				//Otherwise, it references the next one if it's not a jump
				if(statement.op != OP_JMP)
				{
					BasicBlockList::const_iterator nextBlockIterator(innerBlockIterator);
					nextBlockIterator++;
					if(nextBlockIterator != m_basicBlocks.end())
					{
						if(nextBlockIterator->first == blockId)
						{
							referenced = true;
							break;
						}
					}
				}
			}

			if(!referenced)
			{
				toDelete = blockId;
				break;
			}
		}
		if(toDelete == -1) continue;

		m_basicBlocks.erase(toDelete);
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
		blockIterator != m_basicBlocks.end(); blockIterator++)
	{
		BasicBlockList::iterator nextBlockIterator(blockIterator);
		nextBlockIterator++;
		if(nextBlockIterator == m_basicBlocks.end()) continue;

		BASIC_BLOCK& basicBlock(blockIterator->second);
//		BASIC_BLOCK& nextBlock(nextBlockIterator->second);

		if(basicBlock.statements.size() == 0) continue;

		StatementList::iterator lastStatementIterator(basicBlock.statements.end());
		lastStatementIterator--;
		const STATEMENT& statement(*lastStatementIterator);
		if(statement.op != OP_JMP) continue;
		if(statement.jmpBlock != nextBlockIterator->first) continue;

		//Remove the jump
		basicBlock.statements.erase(lastStatementIterator);
	}

	//Flag any block that have a reference from a jump
	for(BasicBlockList::iterator outerBlockIterator(m_basicBlocks.begin());
		outerBlockIterator != m_basicBlocks.end(); outerBlockIterator++)
	{
		BASIC_BLOCK& outerBlock(outerBlockIterator->second);
		outerBlock.hasJumpRef = false;

		for(BasicBlockList::const_iterator innerBlockIterator(m_basicBlocks.begin());
			innerBlockIterator != m_basicBlocks.end(); innerBlockIterator++)
		{
			const BASIC_BLOCK& block(innerBlockIterator->second);
			if(block.statements.size() == 0) continue;

			//Check if this block references the next one or if it jumps to another one
			StatementList::const_iterator lastInstruction(block.statements.end());
			lastInstruction--;
			const STATEMENT& statement(*lastInstruction);

			//It jumps to a block, so check if it references the one we're looking for
			if(statement.op == OP_JMP || statement.op == OP_CONDJMP)
			{
				if(statement.jmpBlock == outerBlockIterator->first)
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
			m_basicBlocks.end() != blockIterator; blockIterator++)
		{
			BasicBlockList::iterator nextBlockIterator(blockIterator);
			nextBlockIterator++;
			if(nextBlockIterator == m_basicBlocks.end()) continue;

			BASIC_BLOCK& basicBlock(blockIterator->second);
			BASIC_BLOCK& nextBlock(nextBlockIterator->second);

			if(nextBlock.hasJumpRef) continue;

			//Check if the last statement is a jump
			StatementList::const_iterator lastStatementIterator(basicBlock.statements.end());
			lastStatementIterator--;
			const STATEMENT& statement(*lastStatementIterator);
			if(statement.op == OP_CONDJMP) continue;
			if(statement.op == OP_JMP) continue;

			//Blocks can be merged
			MergeBasicBlocks(basicBlock, nextBlock);

			m_basicBlocks.erase(nextBlockIterator);

			deletedBlocks++;
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
		statements.end() != outerStatementIterator; outerStatementIterator++)
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
			statements.end() != innerStatementIterator; innerStatementIterator++)
		{
			if(outerStatementIterator == innerStatementIterator) continue;

			STATEMENT& innerStatement(*innerStatementIterator);

			if(innerStatement.src1 && innerStatement.src1->Equals(outerStatement.dst.get()))
			{
				innerStatement.src1 = outerStatement.src1;
				changed = true;
				continue;
			}

			if(innerStatement.src2 && innerStatement.src2->Equals(outerStatement.dst.get()))
			{
				innerStatement.src2 = outerStatement.src1;
				changed = true;
				continue;
			}
		}
	}
	return changed;
}

bool CJitter::CopyPropagation(StatementList& statements)
{
	bool changed = false;

	for(StatementList::iterator outerStatementIterator(statements.begin());
		statements.end() != outerStatementIterator; outerStatementIterator++)
	{
		STATEMENT& outerStatement(*outerStatementIterator);

		//Some operations we can't propagate
		if(outerStatement.op == OP_RETVAL) continue;

		CSymbolRef* outerDstSymbol = outerStatement.dst.get();
		if(outerDstSymbol == NULL) continue;

		//Don't mess with relatives
		if(outerDstSymbol->GetSymbol()->IsRelative()) 
		{
			continue;
		}

		//Count number of uses of this symbol
		unsigned int useCount = 0;
		for(StatementList::iterator innerStatementIterator(statements.begin());
			statements.end() != innerStatementIterator; innerStatementIterator++)
		{
			if(outerStatementIterator == innerStatementIterator) continue;

			STATEMENT& innerStatement(*innerStatementIterator);

			if(
				(innerStatement.src1 && innerStatement.src1->Equals(outerDstSymbol)) || 
				(innerStatement.src2 && innerStatement.src2->Equals(outerDstSymbol))
				)
			{
				useCount++;
			}
		}

		if(useCount == 1)
		{
			//Check for all OP_MOVs that uses the result of this operation and propagate
			for(StatementList::iterator innerStatementIterator(statements.begin());
				statements.end() != innerStatementIterator; innerStatementIterator++)
			{
				if(outerStatementIterator == innerStatementIterator) continue;

				STATEMENT& innerStatement(*innerStatementIterator);

				if(innerStatement.op == OP_MOV && innerStatement.src1->Equals(outerDstSymbol))
				{
					innerStatement.op = outerStatement.op;
					innerStatement.src1 = outerStatement.src1;
					innerStatement.src2 = outerStatement.src2;
					innerStatement.jmpCondition = outerStatement.jmpCondition;
					changed = true;
				}
			}
		}
	}
	return changed;
}

bool CJitter::DeadcodeElimination(VERSIONED_STATEMENT_LIST& versionedStatementList)
{
	bool changed = false;

	typedef std::list<StatementList::iterator> ToDeleteList;
	ToDeleteList toDelete;

	for(StatementList::iterator outerStatementIterator(versionedStatementList.statements.begin());
		versionedStatementList.statements.end() != outerStatementIterator; outerStatementIterator++)
	{
		STATEMENT& outerStatement(*outerStatementIterator);

		CSymbol* candidate = NULL;
		if(outerStatement.dst && outerStatement.dst->GetSymbol()->IsTemporary())
		{
			candidate = outerStatement.dst->GetSymbol().get();
		}
		else if(CSymbol* relativeSymbol = dynamic_symbolref_cast(SYM_RELATIVE, outerStatement.dst))
		{
			VersionedSymbolRefPtr versionedSymbolRef = std::dynamic_pointer_cast<CVersionedSymbolRef>(outerStatement.dst);
			assert(versionedSymbolRef);
			if(versionedSymbolRef->version != versionedStatementList.relativeVersions.GetRelativeVersion(relativeSymbol->m_valueLow))
			{
				candidate = relativeSymbol;
			}
		}

		if(candidate == NULL) continue;
		const SymbolRefPtr& symbolRef(outerStatement.dst);

		//Look for any possible use of this symbol
		bool used = false;
		for(StatementList::iterator innerStatementIterator(outerStatementIterator);
			versionedStatementList.statements.end() != innerStatementIterator; innerStatementIterator++)
		{
			if(outerStatementIterator == innerStatementIterator) continue;

			STATEMENT& innerStatement(*innerStatementIterator);

			if(innerStatement.src1)
			{
				if(innerStatement.src1->Equals(symbolRef.get()))
				{
					used = true;
					break;
				}

				SymbolPtr symbol(innerStatement.src1->GetSymbol());
				if(!symbol->Equals(candidate) && symbol->Aliases(candidate))
				{
					used = true;
					break;
				}
			}
			if(innerStatement.src2)
			{
				if(innerStatement.src2->Equals(symbolRef.get()))
				{
					used = true;
					break;
				}
				SymbolPtr symbol(innerStatement.src2->GetSymbol());
				if(!symbol->Equals(candidate) && symbol->Aliases(candidate))
				{
					used = true;
					break;
				}
			}
		}

		if(!used)
		{
			//Kill it!
			toDelete.push_back(outerStatementIterator);
		}
	}

	for(ToDeleteList::const_iterator deleteIterator(toDelete.begin());
		toDelete.end() != deleteIterator; deleteIterator++)
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

	for(StatementList::iterator outerStatementIterator(basicBlock.statements.begin());
		basicBlock.statements.end() != outerStatementIterator; outerStatementIterator++)
	{
		STATEMENT& outerStatement(*outerStatementIterator);

		CSymbol* tempSymbol = dynamic_symbolref_cast(SYM_TEMPORARY, outerStatement.dst);
		if(tempSymbol == NULL) continue;

		CSymbol* candidate = NULL;

		//Check for a possible replacement
		for(EncounteredTempList::const_iterator tempIterator(encounteredTemps.begin());
			tempIterator != encounteredTemps.end(); tempIterator++)
		{
			//Look for any possible use of this symbol
			CSymbol* encounteredTemp = *tempIterator;
			bool used = false;

			for(StatementList::iterator innerStatementIterator(outerStatementIterator);
				basicBlock.statements.end() != innerStatementIterator; innerStatementIterator++)
			{
				if(outerStatementIterator == innerStatementIterator) continue;

				STATEMENT& innerStatement(*innerStatementIterator);

				if(innerStatement.dst)
				{
					SymbolPtr symbol(innerStatement.dst->GetSymbol());
					if(symbol->Equals(encounteredTemp))
					{
						used = true;
						break;
					}
				}
				if(innerStatement.src1)
				{
					SymbolPtr symbol(innerStatement.src1->GetSymbol());
					if(symbol->Equals(encounteredTemp))
					{
						used = true;
						break;
					}
				}
				if(innerStatement.src2)
				{
					SymbolPtr symbol(innerStatement.src2->GetSymbol());
					if(symbol->Equals(encounteredTemp))
					{
						used = true;
						break;
					}
				}
			}

			if(!used)
			{
				candidate = encounteredTemp;
				break;
			}
		}

		if(candidate == NULL)
		{
			encounteredTemps.push_back(tempSymbol);
		}
		else
		{
			SymbolPtr candidatePtr = MakeSymbol(candidate->m_type, candidate->m_valueLow);

			outerStatement.dst = MakeSymbolRef(candidatePtr);

			//Replace all occurences of this temp with the candidate
			for(StatementList::iterator innerStatementIterator(outerStatementIterator);
				basicBlock.statements.end() != innerStatementIterator; innerStatementIterator++)
			{
				if(outerStatementIterator == innerStatementIterator) continue;

				STATEMENT& innerStatement(*innerStatementIterator);

				if(innerStatement.dst)
				{
					SymbolPtr symbol(innerStatement.dst->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.dst = MakeSymbolRef(candidatePtr);
					}
				}
				if(innerStatement.src1)
				{
					SymbolPtr symbol(innerStatement.src1->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.src1 = MakeSymbolRef(candidatePtr);
					}
				}
				if(innerStatement.src2)
				{
					SymbolPtr symbol(innerStatement.src2->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.src2 = MakeSymbolRef(candidatePtr);
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
			outerStatementIterator++;
		}
	}
}

void CJitter::ComputeLivenessAndPruneSymbols(BASIC_BLOCK& basicBlock)
{
	struct ModifyLiveRangeInfo
	{
		void operator()(SymbolPtr& symbol, unsigned int statementIdx) const
		{
			if(symbol->m_rangeBegin == -1)
			{
				symbol->m_rangeBegin = statementIdx;
			}
			symbol->m_rangeEnd = statementIdx;
		}
	};

	CSymbolTable& symbolTable(basicBlock.symbolTable);
	const StatementList& statements(basicBlock.statements);

	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		SymbolPtr& symbol(const_cast<SymbolPtr&>(*symbolIterator));
		symbol->m_useCount = 0;

		unsigned int statementIdx = 0;
		for(StatementList::const_iterator statementIterator(statements.begin());
			statementIterator != statements.end(); statementIterator++, statementIdx++)
		{
			const STATEMENT& statement(*statementIterator);
			if(statement.dst)
			{
				SymbolPtr dstSymbol(statement.dst->GetSymbol());
				if(dstSymbol->Equals(symbol.get()))
				{
					symbol->m_useCount++;
					if(symbol->m_firstDef == -1)
					{
						symbol->m_firstDef = statementIdx;
					}
					if((symbol->m_lastDef == -1) || (statementIdx > symbol->m_lastDef))
					{
						symbol->m_lastDef = statementIdx;
					}

					ModifyLiveRangeInfo()(symbol, statementIdx);
				}
				else if(!symbol->m_aliased && dstSymbol->Aliases(symbol.get()))
				{
					symbol->m_aliased = true;
				}
			}

			if(statement.src1)
			{
				SymbolPtr src1Symbol(statement.src1->GetSymbol());
				if(src1Symbol->Equals(symbol.get()))
				{
					symbol->m_useCount++;
					if(symbol->m_firstUse == -1)
					{
						symbol->m_firstUse = statementIdx;
					}

					ModifyLiveRangeInfo()(symbol, statementIdx);
				}
				else if(!symbol->m_aliased && src1Symbol->Aliases(symbol.get()))
				{
					symbol->m_aliased = true;
				}
			}

			if(statement.src2)
			{
				SymbolPtr src2Symbol(statement.src2->GetSymbol());
				if(src2Symbol->Equals(symbol.get()))
				{
					symbol->m_useCount++;
					if(symbol->m_firstUse == -1)
					{
						symbol->m_firstUse = statementIdx;
					}

					ModifyLiveRangeInfo()(symbol, statementIdx);
				}
				else if(!symbol->m_aliased && src2Symbol->Aliases(symbol.get()))
				{
					symbol->m_aliased = true;
				}
			}
		}
	}

	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd();)
	{
		const SymbolPtr& symbol(*symbolIterator);
		if(symbol->m_useCount == 0)
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
	CSymbolTable& symbolTable(basicBlock.symbolTable);
	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		const SymbolPtr& symbol(*symbolIterator);

		if(symbol->m_regAlloc_register != -1 && (symbol->m_regAlloc_notAllocatedAfterIdx == symbol->m_rangeEnd)) continue;

		if(symbol->m_type == SYM_TEMPORARY || symbol->m_type == SYM_FP_TMP_SINGLE)
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

	for(StatementList::iterator statementIterator(basicBlock.statements.begin());
		basicBlock.statements.end() != statementIterator; statementIterator++)
	{
		STATEMENT& statement(*statementIterator);

		bool isCommutative = false;
		bool conditionSwapRequired = false;

		switch(statement.op)
		{
			case OP_ADD:
			case OP_AND:
			case OP_OR:
			case OP_XOR:
			case OP_MUL:
			case OP_MULS:
			case OP_MULSHL:
			case OP_MULSHH:
				isCommutative = true;
				break;
			case OP_CMP:
			case OP_CMP64:
			case OP_CONDJMP:
				isCommutative = true;
				conditionSwapRequired = true;
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
			CSymbol* dstreg  = dynamic_symbolref_cast(SYM_REGISTER, statement.dst);
			CSymbol* src1reg = dynamic_symbolref_cast(SYM_REGISTER, statement.src1);
			CSymbol* src2reg = dynamic_symbolref_cast(SYM_REGISTER, statement.src2);

			if(!src1reg && src2reg)
			{
				std::swap(statement.src1, statement.src2);
				swapped = true;
			}
			else if(dstreg && src1reg && src2reg && dstreg->Equals(src2reg))
			{
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
