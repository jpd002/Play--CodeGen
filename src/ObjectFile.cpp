#include "ObjectFile.h"
#include <algorithm>
#include <assert.h>
#include <stdexcept>

using namespace Jitter;

CObjectFile::CObjectFile(CPU_ARCH cpuArch)
: m_cpuArch(cpuArch)
{

}

CObjectFile::~CObjectFile()
{

}

unsigned int CObjectFile::AddInternalSymbol(const INTERNAL_SYMBOL& internalSymbol)
{
	{
		auto symbolIterator = std::find_if(std::begin(m_internalSymbols), std::end(m_internalSymbols), 
			[&] (const INTERNAL_SYMBOL& symbol) 
			{
				return symbol.name == internalSymbol.name;
			}
		);
		if(symbolIterator != std::end(m_internalSymbols))
		{
			throw std::runtime_error("Symbol already exists.");
		}
	}
	m_internalSymbols.push_back(internalSymbol);
	return m_internalSymbols.size() - 1;
}

unsigned int CObjectFile::AddExternalSymbol(const EXTERNAL_SYMBOL& externalSymbol)
{
	assert(std::find_if(std::begin(m_externalSymbols), std::end(m_externalSymbols), [&] (const EXTERNAL_SYMBOL& symbol) { return symbol.name == externalSymbol.name; }) == std::end(m_externalSymbols));
	m_externalSymbols.push_back(externalSymbol);
	return m_externalSymbols.size() - 1;
}

unsigned int CObjectFile::AddExternalSymbol(const std::string& name, void* value)
{
	EXTERNAL_SYMBOL symbol = { name, value };
	return AddExternalSymbol(symbol);
}

unsigned int CObjectFile::GetExternalSymbolIndexByValue(void* value) const
{
	auto externalSymbolIterator = std::find_if(std::begin(m_externalSymbols), std::end(m_externalSymbols),
		[&] (const EXTERNAL_SYMBOL& externalSymbol)
		{
			//This is kinda bad, but we have no choice if we want to be able to cross compile...
			auto srcValue = reinterpret_cast<intptr_t>(externalSymbol.value);
			auto dstValue = reinterpret_cast<intptr_t>(value);
			return srcValue == dstValue;
		}
	);
	if(externalSymbolIterator == std::end(m_externalSymbols))
	{
		throw std::runtime_error("Symbol not found.");
	}
	return externalSymbolIterator - std::begin(m_externalSymbols);
}
