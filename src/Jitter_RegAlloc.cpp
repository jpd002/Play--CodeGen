#include "Jitter.h"

using namespace Jitter;

static bool UseCountSymbolComparator(const CSymbol* symbol1, const CSymbol* symbol2)
{
	return symbol1->m_useCount > symbol2->m_useCount;
}

void CJitter::AllocateRegisters(BASIC_BLOCK& basicBlock)
{
	if(basicBlock.statements.size() == 0) return;

	unsigned int regCount = m_codeGen->GetAvailableRegisterCount();
	unsigned int currentRegister = 0;
	CSymbolTable& symbolTable(basicBlock.symbolTable);
	StatementList& statements(basicBlock.statements);

	typedef std::list<CSymbol*> UseCountSymbolSortedList;

	UseCountSymbolSortedList sortedSymbols;
	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		sortedSymbols.push_back(symbolIterator->get());
	}
	sortedSymbols.sort(UseCountSymbolComparator);

	for(unsigned int i = 0; i < regCount; i++)
	{
		symbolTable.MakeSymbol(SYM_REGISTER, i);
	}

	UseCountSymbolSortedList::iterator symbolIterator = sortedSymbols.begin();
	while(1)
	{
		if(symbolIterator == sortedSymbols.end())
		{
			//We're done
			break;
		}

		if(currentRegister == regCount)
		{
			//We're done
			break;
		}

		CSymbol* symbol(*symbolIterator);
		if(
			((symbol->m_type == SYM_RELATIVE) && (symbol->m_useCount != 0) && (!symbol->m_aliased)) ||
			((symbol->m_type == SYM_TEMPORARY) && (symbol->m_useCount != 0))
			)
		{
			symbol->m_regAlloc_register = currentRegister;

			//Replace all uses of this symbol with register
			for(StatementList::iterator statementIterator(statements.begin());
				statementIterator != statements.end(); statementIterator++)
			{
				STATEMENT& statement(*statementIterator);
				if(statement.dst && statement.dst->GetSymbol()->Equals(symbol))
				{
					statement.dst = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, currentRegister));
				}

				if(statement.src1 && statement.src1->GetSymbol()->Equals(symbol))
				{
					statement.src1 = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, currentRegister));
				}

				if(statement.src2 && statement.src2->GetSymbol()->Equals(symbol))
				{
					statement.src2 = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, currentRegister));
				}
			}

			currentRegister++;
		}

		symbolIterator++;
	}

	//Find the final instruction where to dump registers to
	StatementList::iterator endInsertionPoint(statements.end());
	{
		StatementList::const_iterator endInstructionIterator(statements.end());
		endInstructionIterator--;
		const STATEMENT& statement(*endInstructionIterator);
		if(statement.op == OP_CONDJMP || statement.op == OP_JMP)
		{
			endInsertionPoint--;
		}
	}

	//Emit copies to registers
	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		const SymbolPtr& symbol(*symbolIterator);
		if(symbol->m_regAlloc_register == -1) continue;
		if(symbol->m_type == SYM_TEMPORARY) continue;

		//We use this symbol before we define it, so we need to load it first
		if(symbol->m_firstUse != -1 && symbol->m_firstUse <= symbol->m_firstDef)
		{
			STATEMENT statement;
			statement.op	= OP_MOV;
			statement.dst	= SymbolRefPtr(new CSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, symbol->m_regAlloc_register)));
			statement.src1	= SymbolRefPtr(new CSymbolRef(symbol));

			statements.push_front(statement);
		}

		//If symbol is defined, we need to save it at the end
		if(symbol->m_firstDef != -1)
		{
			STATEMENT statement;
			statement.op	= OP_MOV;
			statement.dst	= SymbolRefPtr(new CSymbolRef(symbol));
			statement.src1	= SymbolRefPtr(new CSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, symbol->m_regAlloc_register)));

			statements.insert(endInsertionPoint, statement);
		}
	}
}
