#pragma once

#include "Jitter_Symbol.h"
#include "maybe_unused.h"

namespace Jitter
{
	class CSymbolRef final
	{
	public:
		static constexpr int UNVERSIONED = -1;

		CSymbolRef(const SymbolPtr& symbol, int version = UNVERSIONED)
		    : m_symbol(symbol)
		    , m_version(version)
		{
			
		}

		SymbolPtr GetSymbol() const
		{
			return m_symbol.lock();
		}

		std::string ToString() const
		{
			return GetSymbol()->ToString();
		}

		bool Equals(CSymbolRef* symbolRef) const
		{
			if(!symbolRef) return false;
			return (m_version == symbolRef->m_version) && GetSymbol()->Equals(symbolRef->GetSymbol().get());
		}

		int GetVersion() const
		{
			return m_version;
		}

		bool IsVersioned() const
		{
			return m_version != UNVERSIONED;
		}

	private:
		WeakSymbolPtr m_symbol;
		int m_version = UNVERSIONED;
	};

	typedef std::shared_ptr<CSymbolRef> SymbolRefPtr;

	FRAMEWORK_MAYBE_UNUSED
	static CSymbol* dynamic_symbolref_cast(SYM_TYPE type, const SymbolRefPtr& symbolRef)
	{
		if(!symbolRef) return nullptr;
		auto result = symbolRef->GetSymbol().get();
		if(result->m_type != type) return nullptr;
		return result;
	}
}
