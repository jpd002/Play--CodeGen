#pragma once

#include "ObjectFile.h"
#include "MachoDefs.h"

namespace Jitter
{
	class CMachoObjectFile : public CObjectFile
	{
	public:
						CMachoObjectFile(CPU_ARCH);
		virtual			~CMachoObjectFile();

		virtual void	Write(Framework::CStream&) override;

	private:
		typedef std::vector<char> StringTable;
		typedef std::vector<uint8> SectionData;
		typedef std::vector<Macho::SECTION> SectionArray;
		typedef std::vector<Macho::NLIST> SymbolArray;
		typedef std::vector<Macho::RELOCATION_INFO> RelocationArray;

		struct INTERNAL_SYMBOL_INFO
		{
			INTERNAL_SYMBOL_INFO()
			{
				nameOffset = 0;
				dataOffset = 0;
				symbolIndex = 0;
			}

			uint32					nameOffset;
			uint32					dataOffset;
			uint32					symbolIndex;
		};
		typedef std::vector<INTERNAL_SYMBOL_INFO> InternalSymbolInfoArray;

		struct EXTERNAL_SYMBOL_INFO
		{
			EXTERNAL_SYMBOL_INFO()
			{
				nameOffset = 0;
				symbolIndex = 0;
			}

			uint32		nameOffset;
			uint32		symbolIndex;
		};
		typedef std::vector<EXTERNAL_SYMBOL_INFO> ExternalSymbolInfoArray;

		struct SECTION
		{
			SectionData				data;
			SymbolReferenceArray	symbolReferences;
		};

		static void				FillStringTable(StringTable&, const InternalSymbolArray&, InternalSymbolInfoArray&);
		static void				FillStringTable(StringTable&, const ExternalSymbolArray&, ExternalSymbolInfoArray&);
		static SECTION			BuildSection(const InternalSymbolArray&, InternalSymbolInfoArray&, INTERNAL_SYMBOL_LOCATION);
		static SymbolArray		BuildSymbols(const InternalSymbolArray&, InternalSymbolInfoArray&, const ExternalSymbolArray&, ExternalSymbolInfoArray&, uint32, uint32);
		static RelocationArray	BuildRelocations(SECTION&, const InternalSymbolInfoArray&, const ExternalSymbolInfoArray&);
	};
}
