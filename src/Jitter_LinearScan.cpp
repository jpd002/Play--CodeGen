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

void CJitter::AllocateRegisters_ComputeCallRanges(const BASIC_BLOCK& basicBlock)
{
	REGALLOC_STATE& state(m_regAllocState);

	state.callRanges.clear();

	unsigned int statementIdx = 0;
	unsigned int rangeBegin = -1;
	unsigned int rangeEnd = -1;
	
	for(StatementList::const_iterator statementIterator(basicBlock.statements.begin());
		statementIterator != basicBlock.statements.end(); statementIterator++, statementIdx++)
	{
		const STATEMENT& statement(*statementIterator);
		if(statement.op == OP_PARAM)
		{
			if(rangeBegin != -1 && rangeEnd != -1)
			{
				state.callRanges[rangeBegin] = rangeEnd;
				rangeBegin = -1;
				rangeEnd = -1;
			}
			if(rangeBegin == -1)
			{
				rangeBegin = statementIdx;
			}
		}
		else if(statement.op == OP_CALL)
		{
			if(rangeBegin != -1 && rangeEnd != -1)
			{
				state.callRanges[rangeBegin] = rangeEnd;
				rangeBegin = -1;
				rangeEnd = -1;
			}
			if(rangeBegin == -1)
			{
				rangeBegin = statementIdx;
			}
			rangeEnd = statementIdx;
		}
		else if(statement.op == OP_RETVAL)
		{
			assert(rangeBegin != -1);
			rangeEnd = statementIdx;
			state.callRanges[rangeBegin] = rangeEnd;
			rangeBegin = -1;
			rangeEnd = -1;
		}
	}

	if(rangeBegin != -1)
	{
		assert(rangeEnd != -1);
		state.callRanges[rangeBegin] = rangeEnd;
		rangeBegin = -1;
		rangeEnd = -1;
	}
}

#ifdef _DEBUG

void CJitter::AllocateRegisters_VerifyProperCallSequence(const BASIC_BLOCK& basicBlock)
{
	enum BLOCK_STATE
	{
		BLOCK_STATE_IDLE,
		BLOCK_STATE_PARAM,
		BLOCK_STATE_CHECK_RETVAL,
	};

	BLOCK_STATE blockState = BLOCK_STATE_IDLE;
	unsigned int retValOpCount = 0;
	//Sanity check... Make sure that nothing is present between the param...call...retval pattern
	for(StatementList::const_iterator statementIterator(basicBlock.statements.begin());
		statementIterator != basicBlock.statements.end(); statementIterator++)
	{
		const STATEMENT& statement(*statementIterator);
		if(blockState == BLOCK_STATE_IDLE || blockState == BLOCK_STATE_CHECK_RETVAL)
		{
			if(statement.op == OP_RETVAL)
			{
				assert(retValOpCount == 0);
				blockState = BLOCK_STATE_IDLE;
			}
			else if(statement.op == OP_PARAM)
			{
				blockState = BLOCK_STATE_PARAM;
			}
			else if(statement.op == OP_CALL)
			{
				blockState = BLOCK_STATE_CHECK_RETVAL;
				retValOpCount = 0;
			}
			else
			{
				retValOpCount++;
			}
		}
		else if(blockState == BLOCK_STATE_PARAM)
		{
			if(statement.op == OP_CALL)
			{
				blockState = BLOCK_STATE_CHECK_RETVAL;
				retValOpCount = 0;
			}
			else if(statement.op == OP_PARAM)
			{
				blockState = BLOCK_STATE_PARAM;
			}
			else
			{
				assert(0);
			}
		}
	}
}

#endif

void CJitter::AllocateRegisters(BASIC_BLOCK& basicBlock)
{
	if(basicBlock.statements.size() == 0) return;

	REGALLOC_STATE& state(m_regAllocState);

	assert(state.activeSymbols.size() == 0);

#ifdef _DEBUG
//	AllocateRegisters_VerifyProperCallSequence(basicBlock);
#endif

//	AllocateRegisters_ComputeCallRanges(basicBlock);

	unsigned int regCount = m_codeGen->GetAvailableRegisterCount();
	CSymbolTable& symbolTable(basicBlock.symbolTable);
	StatementList& statements(basicBlock.statements);

	//for(CallRangeMap::const_iterator callRangeIterator(state.callRanges.begin());
	//	callRangeIterator != state.callRanges.end(); callRangeIterator++)
	//{
	//	printf("Range (%d, %d).\r\n", callRangeIterator->first, callRangeIterator->second);
	//}

	SortedSymbolList sortedSymbols;
	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		////Extend or shorten ranges so that register spilling doesn't occur in
		////the call ranges
		//SymbolPtr& symbol(*symbolIterator);
		//printf("Symbol (%d, %d).", symbol->m_rangeBegin, symbol->m_rangeEnd);
		//if(symbol->m_rangeBegin != -1)
		//{
		//	CallRangeMap::const_iterator callRangeIterator = state.callRanges.lower_bound(symbol->m_rangeBegin);
		//	if(callRangeIterator != state.callRanges.end() && (symbol->m_rangeBegin < callRangeIterator->second))
		//	{
		//		printf("Replacing begin with %d.", callRangeIterator->first);
		//		symbol->m_rangeBegin = callRangeIterator->first;
		//	}
		//}
		//if(symbol->m_rangeEnd != -1)
		//{
		//	CallRangeMap::const_iterator callRangeIterator = state.callRanges.lower_bound(symbol->m_rangeEnd);
		//	if(callRangeIterator != state.callRanges.end() && (symbol->m_rangeEnd < callRangeIterator->second))
		//	{
		//		printf("Replacing end with %d.", callRangeIterator->first);
		//		symbol->m_rangeEnd = callRangeIterator->first;
		//	}
		//}
		//printf("\r\n");
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
		ActiveSymbolList::iterator symbolIterator = state.activeSymbols.begin();
		AllocateRegisters_SpillSymbol(symbolIterator);
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

#ifdef _DEBUG
//	AllocateRegisters_VerifyProperCallSequence(basicBlock);
#endif

}
