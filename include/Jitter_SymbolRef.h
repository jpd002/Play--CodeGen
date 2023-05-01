#ifndef _JITTER_SYMBOLREF_H_
#define _JITTER_SYMBOLREF_H_

#include "Jitter_Symbol.h"
#include "maybe_unused.h"

namespace Jitter
{
	class CSymbolRef
	{
	public:
		CSymbolRef(const SymbolPtr& symbol) :
		  symbol(symbol)
		{
			
		}

		virtual ~CSymbolRef() = default;

		SymbolPtr GetSymbol() const
		{
			return symbol.lock();
		}

		virtual std::string ToString() const
		{
			return GetSymbol()->ToString();
		}

		virtual bool Equals(CSymbolRef* symbolRef) const
		{
			if(symbolRef == NULL) return false;
			return GetSymbol()->Equals(symbolRef->GetSymbol().get());
		}

		WeakSymbolPtr symbol;
	};

	typedef std::shared_ptr<CSymbolRef> SymbolRefPtr;

	class CVersionedSymbolRef : public CSymbolRef
	{
	public:
		CVersionedSymbolRef(const SymbolPtr& symbol, int version) :
		  CSymbolRef(symbol),
		  version(version)
		{

		}

		bool Equals(CSymbolRef* symbolRef) const override
		{
			if(symbolRef == NULL) return false;
			if(!GetSymbol()->Equals(symbolRef->GetSymbol().get())) return false;
			CVersionedSymbolRef* versionedSymbolRef = dynamic_cast<CVersionedSymbolRef*>(symbolRef);
			if(versionedSymbolRef == NULL) return false;
			return versionedSymbolRef->version == version;
		}

		std::string ToString() const override
		{
			return CSymbolRef::ToString() + "{" +  std::to_string(version) + "}";
		}

		int version;
	};

	typedef std::shared_ptr<CVersionedSymbolRef> VersionedSymbolRefPtr;

	FRAMEWORK_MAYBE_UNUSED
	static CSymbol* dynamic_symbolref_cast(SYM_TYPE type, const SymbolRefPtr& symbolRef)
	{
		if(!symbolRef) return nullptr;
		CSymbol* result = symbolRef->GetSymbol().get();
		if(result->m_type != type) return nullptr;
		return result;
	}
}

#endif
