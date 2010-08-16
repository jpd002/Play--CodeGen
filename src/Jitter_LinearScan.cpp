#include "Jitter.h"
#include <algorithm>

using namespace Jitter;

typedef std::list<SymbolPtr> SortedSymbolList;

static bool LiveRangeSymbolComparator(const SymbolPtr& symbol1, const SymbolPtr& symbol2)
{
	if(symbol1->m_rangeBegin == symbol2->m_rangeBegin)
	{
		return symbol1->m_rangeEnd < symbol2->m_rangeEnd;
	}
	else
	{
		return symbol1->m_rangeBegin < symbol2->m_rangeBegin;
	}
}

void CJitter::AllocateRegisters_ReplaceOperand(CSymbolTable& symbolTable, SymbolRefPtr& symbolRef, unsigned int statementIdx)
{
	REGALLOC_STATE& state(m_regAllocState);
	if(symbolRef)
	{
		CSymbol* symbol = symbolRef->GetSymbol().get();
		if(symbol->m_regAlloc_register != -1)
		{
			if((symbol->m_regAlloc_notAllocatedAfterIdx != -1) && (statementIdx > symbol->m_regAlloc_notAllocatedAfterIdx))
			{
				return;
			}
			else
			{
				//assert((symbol->m_regAlloc_loadBeforeIdx == -1) || (statementIdx >= symbol->m_regAlloc_loadBeforeIdx));
				//assert((symbol->m_regAlloc_saveAfterIdx == -1)  || (statementIdx <= symbol->m_regAlloc_saveAfterIdx));
				symbolRef = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, symbol->m_regAlloc_register));
			}
		}
	}
}

void CJitter::AllocateRegisters_SpillSymbol(ActiveSymbolList::iterator& symbolIterator, unsigned int saveIndex)
{
	REGALLOC_STATE& state(m_regAllocState);

	CSymbol* spillSymbol = symbolIterator->second;
	if(spillSymbol->m_regAlloc_register != -1)
	{
		spillSymbol->m_regAlloc_notAllocatedAfterIdx = (saveIndex == -1) ? spillSymbol->m_rangeEnd : saveIndex;
		bool mustSave = false;
		if((spillSymbol->m_type == SYM_RELATIVE) && (spillSymbol->m_firstDef != -1)) mustSave = true;
		if((spillSymbol->m_type == SYM_TEMPORARY) && (saveIndex != -1)) mustSave = true;
		if(mustSave)
		{
			spillSymbol->m_regAlloc_saveAfterIdx = (saveIndex == -1) ? spillSymbol->m_lastDef : saveIndex;
		}
		state.availableRegs.push_back(spillSymbol->m_regAlloc_register);
	}

	state.activeSymbols.erase(symbolIterator);
}

void CJitter::AllocateRegisters(BASIC_BLOCK& basicBlock)
{
	if(basicBlock.statements.size() == 0) return;

	REGALLOC_STATE& state(m_regAllocState);

	assert(state.activeSymbols.size() == 0);

	unsigned int regCount = m_codeGen->GetAvailableRegisterCount();
	CSymbolTable& symbolTable(basicBlock.symbolTable);
	StatementList& statements(basicBlock.statements);

	SortedSymbolList sortedSymbols;
	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		sortedSymbols.push_back(*symbolIterator);
	}
	sortedSymbols.sort(LiveRangeSymbolComparator);

	state.availableRegs.clear();
	for(unsigned int i = 0; i < regCount; i++)
	{
		symbolTable.MakeSymbol(SYM_REGISTER, i);
		state.availableRegs.push_back(i);
	}

	for(SortedSymbolList::iterator symbolIterator(sortedSymbols.begin());
		symbolIterator != sortedSymbols.end(); symbolIterator++)
	{
		SymbolPtr symbol(*symbolIterator);

		bool allocatable = false;

		if((symbol->m_type == SYM_RELATIVE) && (!symbol->m_aliased)) allocatable = true;
		if((symbol->m_type == SYM_TEMPORARY)) allocatable = true;

		if(!allocatable) continue;

		assert(state.activeSymbols.size() <= regCount);

		//Expire ranges that are over
		while(1)
		{
			if(state.activeSymbols.size() == 0) break;
			ActiveSymbolList::iterator symbolIterator = state.activeSymbols.begin();
			if(symbol->m_rangeBegin > symbolIterator->first)
			{
				//We have to spill this register
				AllocateRegisters_SpillSymbol(symbolIterator);
			}
			else
			{
				break;
			}
		}

		if(state.availableRegs.size() == 0)
		{
			//gotta do something about that
			state.activeSymbols.insert(ActiveSymbolList::value_type(symbol->m_rangeEnd, symbol.get()));
			ActiveSymbolList::iterator lastActiveSymbol = state.activeSymbols.end();
			lastActiveSymbol--;

			//If the one we added is the one that needs to be spilled...
			if(lastActiveSymbol->second->Equals(symbol.get()))
			{
				//Just remove it from the list
				state.activeSymbols.erase(lastActiveSymbol);
			}
			else
			{
				//We need to transfer the register that's allocated to the lastActiveSymbol to the new symbol
				assert(symbol->m_rangeBegin != 0);
				AllocateRegisters_SpillSymbol(lastActiveSymbol, symbol->m_rangeBegin - 1);

				unsigned int registerId = state.availableRegs.front();
				state.availableRegs.pop_front();
				symbol->m_regAlloc_register = registerId;
				if((symbol->m_firstUse != -1) && (symbol->m_rangeBegin == symbol->m_firstUse))
				{
					symbol->m_regAlloc_loadBeforeIdx = symbol->m_rangeBegin;
				}
			}
		}
		else
		{
			unsigned int registerId = state.availableRegs.front();
			state.availableRegs.pop_front();
			symbol->m_regAlloc_register = registerId;
			if((symbol->m_firstUse != -1) && (symbol->m_rangeBegin == symbol->m_firstUse))
			{
				symbol->m_regAlloc_loadBeforeIdx = symbol->m_rangeBegin;
			}
			state.activeSymbols.insert(ActiveSymbolList::value_type(symbol->m_rangeEnd, symbol.get()));
		}
	}

	//Expire all remaining ranges
	while(state.activeSymbols.size() != 0)
	{
		AllocateRegisters_SpillSymbol(state.activeSymbols.begin());
	}

	//Issue insert commands
	for(SortedSymbolList::iterator symbolIterator(sortedSymbols.begin());
		symbolIterator != sortedSymbols.end(); symbolIterator++)
	{
		SymbolPtr symbol(*symbolIterator);
		if(symbol->m_regAlloc_loadBeforeIdx != -1)
		{
			STATEMENT newStatement;
			newStatement.op		= OP_MOV;
			newStatement.dst	= MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, symbol->m_regAlloc_register));
			newStatement.src1	= MakeSymbolRef(symbol);

			INSERT_COMMAND command;
			command.insertionPoint	= statements.begin();
			command.statement		= newStatement;
			std::advance(command.insertionPoint, symbol->m_regAlloc_loadBeforeIdx);

			state.insertCommands.push_back(command);
		}
		if(symbol->m_regAlloc_saveAfterIdx != -1)
		{
			STATEMENT newStatement;
			newStatement.op		= OP_MOV;
			newStatement.dst	= MakeSymbolRef(symbol);
			newStatement.src1	= MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, symbol->m_regAlloc_register));

			INSERT_COMMAND command;
			command.insertionPoint	= statements.begin();
			command.statement		= newStatement;
			std::advance(command.insertionPoint, symbol->m_regAlloc_saveAfterIdx + 1);

			state.insertCommands.push_back(command);
		}
	}

	unsigned int statementIdx = 0;
	for(StatementList::iterator statementIterator(basicBlock.statements.begin());
		statementIterator != basicBlock.statements.end(); statementIterator++, statementIdx++)
	{
		STATEMENT& statement(*statementIterator);

		AllocateRegisters_ReplaceOperand(symbolTable, statement.src2, statementIdx);
		AllocateRegisters_ReplaceOperand(symbolTable, statement.src1, statementIdx);
		AllocateRegisters_ReplaceOperand(symbolTable, statement.dst,  statementIdx);
	}

	for(InsertCommandList::const_iterator insertCommandIterator(state.insertCommands.begin());
		insertCommandIterator != state.insertCommands.end(); insertCommandIterator++)
	{
		const INSERT_COMMAND& insertCommand(*insertCommandIterator);
		basicBlock.statements.insert(insertCommand.insertionPoint, insertCommand.statement); 
	}

	state.insertCommands.clear();
}
