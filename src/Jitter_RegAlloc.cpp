#include "Jitter.h"
#include <iostream>

#ifdef _DEBUG
//#define DUMP_STATEMENTS
#endif

#ifdef DUMP_STATEMENTS
#include <iostream>
#endif

using namespace Jitter;

void CJitter::AllocateRegisters(BASIC_BLOCK& basicBlock)
{
	auto& symbolTable = basicBlock.symbolTable;

	{
		unsigned int regCount = m_codeGen->GetAvailableRegisterCount();
		for(unsigned int i = 0; i < regCount; i++)
		{
			symbolTable.MakeSymbol(SYM_REGISTER, i);
		}
	}

	{
		unsigned int regCount = m_codeGen->GetAvailableMdRegisterCount();
		for(unsigned int i = 0; i < regCount; i++)
		{
			symbolTable.MakeSymbol(SYM_REGISTER128, i);
		}
	}

	std::multimap<unsigned int, STATEMENT> loadStatements;
	std::multimap<unsigned int, STATEMENT> spillStatements;
#ifdef DUMP_STATEMENTS
	DumpStatementList(basicBlock.statements);
	std::cout << std::endl;
#endif

	auto allocRanges = ComputeAllocationRanges(basicBlock);
	for(const auto& allocRange : allocRanges)
	{
		SymbolRegAllocInfo symbolRegAllocs;
		ComputeLivenessForRange(basicBlock, allocRange, symbolRegAllocs);

		MarkAliasedSymbols(basicBlock, allocRange, symbolRegAllocs);

		auto regAllocs = AllocateRegisters(symbolRegAllocs);
//		auto regAllocs = AllocateRegistersMd(symbolRegAllocs);

		for(const auto& statementInfo : IndexedStatementList(basicBlock.statements))
		{
			auto& statement(statementInfo.statement);
			const auto& statementIdx(statementInfo.index);
			if(statementIdx < allocRange.first) continue;
			if(statementIdx > allocRange.second) break;

			statement.VisitOperands(
				[&] (SymbolRefPtr& symbolRef, bool)
				{
					auto symbol = symbolRef->GetSymbol();
					auto regAllocIterator = regAllocs.find(symbol);
					if(regAllocIterator != std::end(regAllocs))
					{
						symbolRef = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, regAllocIterator->second));
//						symbolRef = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER128, regAllocIterator->second));
					}
				}
			);
		}

		//Prepare load and spills
		for(const auto& regAlloc : regAllocs)
		{
			const auto& symbol = regAlloc.first;
			const auto& symbolRegAlloc = symbolRegAllocs[symbol];
			//firstUse == -1 means it is written to but never used afterwards in this block

			//Do we need to load register at the beginning?
			//If symbol is read and we use this symbol before we define it, so we need to load it first
			if((symbolRegAlloc.firstUse != -1) && (symbolRegAlloc.firstUse <= symbolRegAlloc.firstDef))
			{
				STATEMENT statement;
				statement.op	= OP_MOV;
				statement.dst	= std::make_shared<CSymbolRef>(symbolTable.MakeSymbol(SYM_REGISTER, regAlloc.second));
//				statement.dst	= std::make_shared<CSymbolRef>(symbolTable.MakeSymbol(SYM_REGISTER128, regAlloc.second));
				statement.src1	= std::make_shared<CSymbolRef>(symbol);

				loadStatements.insert(std::make_pair(allocRange.first, statement));
			}

			//If symbol is defined, we need to save it at the end
			if(symbolRegAlloc.firstDef != -1)
			{
				STATEMENT statement;
				statement.op	= OP_MOV;
				statement.dst	= std::make_shared<CSymbolRef>(symbol);
				statement.src1	= std::make_shared<CSymbolRef>(symbolTable.MakeSymbol(SYM_REGISTER, regAlloc.second));
//				statement.src1	= std::make_shared<CSymbolRef>(symbolTable.MakeSymbol(SYM_REGISTER128, regAlloc.second));

				spillStatements.insert(std::make_pair(allocRange.second, statement));
			}
		}
	}

#ifdef DUMP_STATEMENTS
	DumpStatementList(basicBlock.statements);
	std::cout << std::endl;
#endif

	std::map<unsigned int, StatementList::const_iterator> loadPoints;
	std::map<unsigned int, StatementList::const_iterator> spillPoints;

	//Load
	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		const auto& statementIdx(statementInfo.index);
		if(loadStatements.find(statementIdx) != std::end(loadStatements))
		{
			loadPoints.insert(std::make_pair(statementIdx, statementInfo.iterator));
		}
	}

	//Spill
	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		const auto& statementIdx(statementInfo.index);
		if(spillStatements.find(statementIdx) != std::end(spillStatements))
		{
			const auto& statement = statementInfo.statement;
			auto statementIterator = statementInfo.iterator;
			if((statement.op != OP_CONDJMP) && (statement.op != OP_JMP))
			{
				statementIterator++;
			}
			spillPoints.insert(std::make_pair(statementIdx, statementIterator));
		}
	}

	//Loads
	for(const auto& loadPoint : loadPoints)
	{
		unsigned int statementIndex = loadPoint.first;
		for(auto statementIterator = loadStatements.lower_bound(statementIndex);
			statementIterator != loadStatements.upper_bound(statementIndex);
			statementIterator++)
		{
			const auto& statement(statementIterator->second);
			basicBlock.statements.insert(loadPoint.second, statement);
		}
	}

	//Spills
	for(const auto& spillPoint : spillPoints)
	{
		unsigned int statementIndex = spillPoint.first;
		for(auto statementIterator = spillStatements.lower_bound(statementIndex);
			statementIterator != spillStatements.upper_bound(statementIndex);
			statementIterator++)
		{
			const auto& statement(statementIterator->second);
			basicBlock.statements.insert(spillPoint.second, statement);
		}
	}

#ifdef DUMP_STATEMENTS
	DumpStatementList(basicBlock.statements);
	std::cout << std::endl;
#endif
}

CJitter::SymbolAllocRegMap CJitter::AllocateRegisters(SymbolRegAllocInfo& symbolRegAllocs) const
{
	auto isRegisterAllocatable =
		[] (SYM_TYPE symbolType)
		{
			return (symbolType == SYM_RELATIVE) || (symbolType == SYM_TEMPORARY);
		};

	//Sort symbols by usage count
	std::list<SymbolPtr> sortedSymbols;
	for(const auto& symbolRegAlloc : symbolRegAllocs)
	{
		const auto& symbol(symbolRegAlloc.first);
		if(!isRegisterAllocatable(symbol->m_type)) continue;
		if(symbolRegAlloc.second.aliased) continue;
		sortedSymbols.push_back(symbolRegAlloc.first);
	}
	sortedSymbols.sort(
		[&] (const SymbolPtr& symbol1, const SymbolPtr& symbol2)
		{
			auto symbol1RegAllocIterator = symbolRegAllocs.find(symbol1);
			auto symbol2RegAllocIterator = symbolRegAllocs.find(symbol2);
			assert(symbol1RegAllocIterator != std::end(symbolRegAllocs));
			assert(symbol2RegAllocIterator != std::end(symbolRegAllocs));
			unsigned int symbol1UseCount = symbol1RegAllocIterator->second.useCount;
			unsigned int symbol2UseCount = symbol2RegAllocIterator->second.useCount;
			return symbol1UseCount > symbol2UseCount;
		}
	);

	SymbolAllocRegMap symbolAllocReg;
	unsigned int regCount = m_codeGen->GetAvailableRegisterCount();
	unsigned int currentRegister = 0;

	for(const auto& symbol : sortedSymbols)
	{
		if(currentRegister == regCount)
		{
			//We're done
			break;
		}

		symbolAllocReg[symbol] = currentRegister;
		currentRegister++;
	}

	return symbolAllocReg;
}

CJitter::SymbolAllocRegMap CJitter::AllocateRegistersMd(SymbolRegAllocInfo& symbolRegAllocs) const
{
	auto isRegisterAllocatable =
		[] (SYM_TYPE symbolType)
		{
			return (symbolType == SYM_RELATIVE128) || (symbolType == SYM_TEMPORARY128);
		};

	//Sort symbols by usage count
	std::list<SymbolPtr> sortedSymbols;
	for(const auto& symbolRegAlloc : symbolRegAllocs)
	{
		const auto& symbol(symbolRegAlloc.first);
		if(!isRegisterAllocatable(symbol->m_type)) continue;
		if(symbolRegAlloc.second.aliased) continue;
		sortedSymbols.push_back(symbolRegAlloc.first);
	}
	sortedSymbols.sort(
		[&] (const SymbolPtr& symbol1, const SymbolPtr& symbol2)
		{
			auto symbol1RegAllocIterator = symbolRegAllocs.find(symbol1);
			auto symbol2RegAllocIterator = symbolRegAllocs.find(symbol2);
			assert(symbol1RegAllocIterator != std::end(symbolRegAllocs));
			assert(symbol2RegAllocIterator != std::end(symbolRegAllocs));
			unsigned int symbol1UseCount = symbol1RegAllocIterator->second.useCount;
			unsigned int symbol2UseCount = symbol2RegAllocIterator->second.useCount;
			return symbol1UseCount > symbol2UseCount;
		}
	);

	SymbolAllocRegMap symbolAllocReg;
	unsigned int regCount = m_codeGen->GetAvailableMdRegisterCount();
	unsigned int currentRegister = 0;

	for(const auto& symbol : sortedSymbols)
	{
		if(currentRegister == regCount)
		{
			//We're done
			break;
		}

		symbolAllocReg[symbol] = currentRegister;
		currentRegister++;
	}

	return symbolAllocReg;
}

CJitter::AllocationRangeArray CJitter::ComputeAllocationRanges(const BASIC_BLOCK& basicBlock)
{
	AllocationRangeArray result;
	unsigned int currentStart = 0;
	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		const auto& statement(statementInfo.statement);
		const auto& statementIdx(statementInfo.index);
		if(statement.op == OP_CALL)
		{
			//Gotta split here
			result.push_back(std::make_pair(currentStart, statementIdx));
			currentStart = statementIdx + 1;
		}
	}
	result.push_back(std::make_pair(currentStart, basicBlock.statements.size() - 1));
	return result;
}

void CJitter::ComputeLivenessForRange(const BASIC_BLOCK& basicBlock, const AllocationRange& allocRange, SymbolRegAllocInfo& symbolRegAllocs) const
{
	auto& symbolTable(basicBlock.symbolTable);
	const auto& statements(basicBlock.statements);

	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		const auto& statement(statementInfo.statement);
		unsigned int statementIdx(statementInfo.index);
		if(statementIdx < allocRange.first) continue;
		if(statementIdx > allocRange.second) continue;

		statement.VisitDestination(
			[&] (const SymbolRefPtr& symbolRef, bool)
			{
				auto symbol(symbolRef->GetSymbol());
				auto& symbolRegAlloc = symbolRegAllocs[symbol];
				symbolRegAlloc.useCount++;
				if(symbolRegAlloc.firstDef == -1)
				{
					symbolRegAlloc.firstDef = statementIdx;
				}
				if((symbolRegAlloc.lastDef == -1) || (statementIdx > symbolRegAlloc.lastDef))
				{
					symbolRegAlloc.lastDef = statementIdx;
				}
			}
		);

		statement.VisitSources(
			[&] (const SymbolRefPtr& symbolRef, bool)
			{
				auto symbol(symbolRef->GetSymbol());
				auto& symbolRegAlloc = symbolRegAllocs[symbol];
				symbolRegAlloc.useCount++;
				if(symbolRegAlloc.firstUse == -1)
				{
					symbolRegAlloc.firstUse = statementIdx;
				}
			}
		);
	}
}

void CJitter::MarkAliasedSymbols(const BASIC_BLOCK& basicBlock, const AllocationRange& allocRange, SymbolRegAllocInfo& symbolRegAllocs) const
{
	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		auto& statement(statementInfo.statement);
		const auto& statementIdx(statementInfo.index);
		if(statementIdx < allocRange.first) continue;
		if(statementIdx > allocRange.second) break;
		if(statement.op == OP_PARAM_RET)
		{
			auto& symbolRegAlloc = symbolRegAllocs[statement.src1->GetSymbol()];
			symbolRegAlloc.aliased = true;
		}
		for(auto& symbolRegAlloc : symbolRegAllocs)
		{
			if(symbolRegAlloc.second.aliased) continue;
			auto testedSymbol = symbolRegAlloc.first;
			statement.VisitOperands(
				[&](const SymbolRefPtr& symbolRef, bool)
				{
					auto symbol = symbolRef->GetSymbol();
					if(symbol->Equals(testedSymbol.get())) return;
					if(symbol->Aliases(testedSymbol.get()))
					{
						symbolRegAlloc.second.aliased = true;
					}
				}
			);
		}
	}
}
