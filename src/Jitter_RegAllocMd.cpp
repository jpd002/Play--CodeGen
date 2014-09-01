#include "Jitter.h"

using namespace Jitter;

static bool UseCountSymbolComparator(const CSymbol* symbol1, const CSymbol* symbol2)
{
	return symbol1->m_useCount > symbol2->m_useCount;
}

void CJitter::AllocateRegistersMd(BASIC_BLOCK& basicBlock)
{
	if(basicBlock.statements.empty()) return;

	unsigned int regCount = m_codeGen->GetAvailableMdRegisterCount();
	unsigned int currentRegister = 0;
	auto& symbolTable(basicBlock.symbolTable);
	auto& statements(basicBlock.statements);

	auto isRegisterAllocatable =
		[] (SYM_TYPE symbolType)
		{
			return (symbolType == SYM_RELATIVE128) || (symbolType == SYM_TEMPORARY128);
		};

	std::list<CSymbol*> sortedSymbols;
	for(const auto& symbol : symbolTable.GetSymbols())
	{
		if(!isRegisterAllocatable(symbol->m_type)) continue;
		sortedSymbols.push_back(symbol.get());
	}
	sortedSymbols.sort(UseCountSymbolComparator);

	for(unsigned int i = 0; i < regCount; i++)
	{
		symbolTable.MakeSymbol(SYM_REGISTER128, i);
	}

	auto symbolIterator = sortedSymbols.begin();
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

		auto symbol(*symbolIterator);
		if(
			((symbol->m_type == SYM_RELATIVE128) && (symbol->m_useCount != 0) && (!symbol->m_aliased)) ||
			((symbol->m_type == SYM_TEMPORARY128) && (symbol->m_useCount != 0))
			)
		{
			symbol->m_regAlloc_register = currentRegister;

			//Replace all uses of this symbol with register
			for(auto& statement : statements)
			{
				if(statement.dst && statement.dst->GetSymbol()->Equals(symbol))
				{
					statement.dst = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER128, currentRegister));
				}

				if(statement.src1 && statement.src1->GetSymbol()->Equals(symbol))
				{
					statement.src1 = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER128, currentRegister));
				}

				if(statement.src2 && statement.src2->GetSymbol()->Equals(symbol))
				{
					statement.src2 = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER128, currentRegister));
				}
			}

			currentRegister++;
		}

		symbolIterator++;
	}

	//Find the final instruction where to dump registers to
	auto endInsertionPoint(statements.end());
	{
		auto endInstructionIterator(statements.end());
		endInstructionIterator--;
		const auto& statement(*endInstructionIterator);
		if(statement.op == OP_CONDJMP || statement.op == OP_JMP)
		{
			endInsertionPoint--;
		}
	}

	//Emit copies to registers
	for(const auto& symbol : symbolTable.GetSymbols())
	{
		if(!isRegisterAllocatable(symbol->m_type)) continue;
		if(symbol->m_regAlloc_register == -1) continue;
		if(symbol->m_type == SYM_TEMPORARY128) continue;

		//We use this symbol before we define it, so we need to load it first
		if(symbol->m_firstUse != -1 && symbol->m_firstUse <= symbol->m_firstDef)
		{
			STATEMENT statement;
			statement.op	= OP_MOV;
			statement.dst	= std::make_shared<CSymbolRef>(symbolTable.MakeSymbol(SYM_REGISTER128, symbol->m_regAlloc_register));
			statement.src1	= std::make_shared<CSymbolRef>(symbol);

			statements.push_front(statement);
		}

		//If symbol is defined, we need to save it at the end
		if(symbol->m_firstDef != -1)
		{
			STATEMENT statement;
			statement.op	= OP_MOV;
			statement.dst	= std::make_shared<CSymbolRef>(symbol);
			statement.src1	= std::make_shared<CSymbolRef>(symbolTable.MakeSymbol(SYM_REGISTER128, symbol->m_regAlloc_register));

			statements.insert(endInsertionPoint, statement);
		}
	}
}
