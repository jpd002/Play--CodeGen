#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <memory>
#include "Stream.h"

namespace Jitter
{
	class CObjectFile
	{
	public:
		enum SYMBOL_TYPE
		{
			SYMBOL_TYPE_INTERNAL,
			SYMBOL_TYPE_EXTERNAL
		};

		struct SYMBOL_REFERENCE
		{
			SYMBOL_TYPE		type;
			unsigned int	symbolIndex;
			unsigned int	offset;
		};
		typedef std::vector<SYMBOL_REFERENCE> SymbolReferenceArray;

		enum INTERNAL_SYMBOL_LOCATION
		{
			INTERNAL_SYMBOL_LOCATION_TEXT,
			INTERNAL_SYMBOL_LOCATION_DATA
		};

		struct INTERNAL_SYMBOL
		{
			std::string						name;
			INTERNAL_SYMBOL_LOCATION		location;
			std::vector<uint8>				data;
			SymbolReferenceArray			symbolReferences;
		};

		struct EXTERNAL_SYMBOL
		{
			std::string						name;
			void*							value;
		};

								CObjectFile();
		virtual					~CObjectFile();

		unsigned int			AddInternalSymbol(const INTERNAL_SYMBOL&);

		unsigned int			AddExternalSymbol(const EXTERNAL_SYMBOL&);
		unsigned int			AddExternalSymbol(const std::string&, void*);

		unsigned int			GetExternalSymbolIndexByValue(void*) const;

		virtual void			Write(Framework::CStream&) = 0;

	protected:
		typedef std::vector<INTERNAL_SYMBOL> InternalSymbolArray;
		typedef std::vector<EXTERNAL_SYMBOL> ExternalSymbolArray;

		InternalSymbolArray		m_internalSymbols;
		ExternalSymbolArray		m_externalSymbols;
	};
}
