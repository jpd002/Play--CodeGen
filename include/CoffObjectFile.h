#pragma once

#include "ObjectFile.h"
#include "Stream.h"
#include <vector>
#include <unordered_map>

namespace Jitter
{
	class CCoffObjectFile : public CObjectFile
	{
	public:
		enum MACHINE_TYPE
		{
			MACHINE_TYPE_UNKNOWN	= 0x0000,
			MACHINE_TYPE_I386		= 0x014C,
			MACHINE_TYPE_AMD64		= 0x8664,
		};

		struct HEADER
		{
			uint16		machine;
			uint16		numberOfSections;
			uint32		timeDateStamp;
			uint32		pointerToSymbolTable;
			uint32		numberOfSymbols;
			uint16		sizeOfOptionalHeader;
			uint16		characteristics;
		};
		static_assert(sizeof(HEADER) == 0x14, "Size of HEADER must be 0x14 bytes.");

		struct SECTION_HEADER
		{
			char		name[8];
			uint32		virtualSize;
			uint32		virtualAddress;
			uint32		sizeOfRawData;
			uint32		pointerToRawData;
			uint32		pointerToRelocations;
			uint32		pointerToLineNumbers;
			uint16		numberOfRelocations;
			uint16		numberOfLineNumbers;
			uint32		characteristics;
		};
		static_assert(sizeof(SECTION_HEADER) == 0x28, "Size of SECTION_HEADER must be 0x28 bytes.");
		typedef std::vector<SECTION_HEADER> SectionHeaderArray;

	#pragma pack(push, 1)
		struct RELOCATION
		{
			uint32	rva;
			uint32	symbolIndex;
			uint16	type;
		};
	#pragma pack(pop)
		static_assert(sizeof(RELOCATION) == 0x0A, "Size of RELOCATION must be 0x0A bytes.");
		typedef std::vector<RELOCATION> RelocationArray;

	#pragma pack(push, 1)
		struct SYMBOL
		{
			union
			{
				char shortName[8];
				struct
				{
					uint32 zeroes;
					uint32 offset;
				};
			} name;
			uint32	value;
			uint16	sectionNumber;
			uint16	type;
			uint8	storageClass;
			uint8	numberOfAuxSymbols;
		};
	#pragma pack(pop)
		static_assert(sizeof(SYMBOL) == 0x12, "Size of SYMBOL must be 0x12 bytes.");
		typedef std::vector<SYMBOL> SymbolArray;

		typedef std::vector<char> StringTable;
		typedef std::vector<uint8> SectionData;

		struct SECTION
		{
			SectionData				data;
			SymbolReferenceArray	symbolReferences;
		};

									CCoffObjectFile();
		virtual						~CCoffObjectFile();

		void						Write(Framework::CStream&);

	private:
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

		static void					FillStringTable(StringTable&, const InternalSymbolArray&, InternalSymbolInfoArray&);
		static void					FillStringTable(StringTable&, const ExternalSymbolArray&, ExternalSymbolInfoArray&);
		static SECTION				BuildSection(const InternalSymbolArray&, InternalSymbolInfoArray&, INTERNAL_SYMBOL_LOCATION);
		static SymbolArray			BuildSymbols(const InternalSymbolArray&, InternalSymbolInfoArray&, const ExternalSymbolArray&, ExternalSymbolInfoArray&, uint32, uint32);
		static RelocationArray		BuildRelocations(SECTION&, const InternalSymbolInfoArray&, const ExternalSymbolInfoArray&);
	};
}
