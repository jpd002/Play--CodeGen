#include "MachoObjectFile.h"

using namespace Jitter;

CMachoObjectFile::CMachoObjectFile()
{

}

CMachoObjectFile::~CMachoObjectFile()
{

}

void CMachoObjectFile::Write(Framework::CStream& stream)
{
	auto internalSymbolInfos = InternalSymbolInfoArray(m_internalSymbols.size());
	auto externalSymbolInfos = ExternalSymbolInfoArray(m_externalSymbols.size());

	StringTable stringTable;
	FillStringTable(stringTable, m_internalSymbols, internalSymbolInfos);
	FillStringTable(stringTable, m_externalSymbols, externalSymbolInfos);

	auto textSection = BuildSection(m_internalSymbols, internalSymbolInfos, INTERNAL_SYMBOL_LOCATION_TEXT);
	auto dataSection = BuildSection(m_internalSymbols, internalSymbolInfos, INTERNAL_SYMBOL_LOCATION_DATA);
	
	uint32 sectionCount = 1;
	SectionArray sections;

	{
		Macho::SECTION section;
		memset(&section, 0, sizeof(Macho::SECTION));
		strncpy(section.sectionName, "__text", 0x10);
		strncpy(section.segmentName, "__TEXT", 0x10);
		
		section.sectionName[0x0F] = 0;
		section.segmentName[0x0F] = 0;

		section.address				= 0;
		section.size				= textSection.data.size();
		section.offset				= sizeof(Macho::MACH_HEADER) + sizeof(Macho::SEGMENT_COMMAND) + (sizeof(Macho::SECTION) * sectionCount);
		section.align				= 0x02;
		section.relocationOffset	= 0;
		section.relocationCount		= 0;
		section.flags				= Macho::S_ATTR_SOME_INSTRUCTIONS | Macho::S_ATTR_PURE_INSTRUCTIONS;
		section.reserved1			= 0;
		section.reserved2			= 0;

		sections.push_back(section);
	}

	Macho::SEGMENT_COMMAND segmentCommand;
	memset(&segmentCommand, 0, sizeof(Macho::SEGMENT_COMMAND));
	segmentCommand.cmd				= Macho::LC_SEGMENT;
	segmentCommand.cmdSize			= sizeof(Macho::SEGMENT_COMMAND) + (sizeof(Macho::SECTION) * sections.size());
	segmentCommand.vmAddress		= 0;
	segmentCommand.vmSize			= textSection.data.size();
	segmentCommand.fileOffset		= sizeof(Macho::MACH_HEADER) + sizeof(Macho::SEGMENT_COMMAND);
	segmentCommand.fileSize			= textSection.data.size();
	segmentCommand.maxProtection	= 0x07;
	segmentCommand.initProtection	= 0x07;
	segmentCommand.sectionCount		= sections.size();
	segmentCommand.flags			= 0;

	Macho::SYMTAB_COMMAND symtabCommand;
	memset(&symtabCommand, 0, sizeof(Macho::SYMTAB_COMMAND));
	symtabCommand.cmd				= Macho::LC_SYMTAB;
	symtabCommand.cmdSize			= sizeof(Macho::SYMTAB_COMMAND);

	Macho::MACH_HEADER header = {};
	header.magic			= Macho::MH_MAGIC;
	header.cpuType			= Macho::CPU_TYPE_I386;
	header.cpuSubType		= Macho::CPU_SUBTYPE_I386_ALL;
	header.fileType			= Macho::MH_OBJECT;
	header.commandCount		= 1;
	header.sizeofCommands	= 0;
	header.flags			= Macho::MH_NOMULTIDEFS;

	stream.Write(&header, sizeof(Macho::MACH_HEADER));
	stream.Write(&segmentCommand, sizeof(Macho::SEGMENT_COMMAND));
	for(const auto& section : sections)
	{
		stream.Write(&section, sizeof(Macho::SECTION));
	}
	stream.Write(&symtabCommand, sizeof(Macho::SYMTAB_COMMAND));
	stream.Write(textSection.data.data(), textSection.data.size());
}

void CMachoObjectFile::FillStringTable(StringTable& stringTable, const InternalSymbolArray& internalSymbols, InternalSymbolInfoArray& internalSymbolInfos)
{
	uint32 stringTableSizeIncrement = 0;
	for(const auto& internalSymbol : internalSymbols)
	{
		stringTableSizeIncrement += internalSymbol.name.length() + 1;
	}
	stringTable.reserve(stringTable.size() + stringTableSizeIncrement);
	for(uint32 i = 0; i < internalSymbols.size(); i++)
	{
		const auto& internalSymbol = internalSymbols[i];
		auto& internalSymbolInfo = internalSymbolInfos[i];
		internalSymbolInfo.nameOffset = stringTable.size();
		stringTable.insert(std::end(stringTable), std::begin(internalSymbol.name), std::end(internalSymbol.name));
		stringTable.push_back(0);
	}
}

void CMachoObjectFile::FillStringTable(StringTable& stringTable, const ExternalSymbolArray& externalSymbols, ExternalSymbolInfoArray& externalSymbolInfos)
{
	uint32 stringTableSizeIncrement = 0;
	for(const auto& externalSymbol : externalSymbols)
	{
		stringTableSizeIncrement += externalSymbol.name.length() + 1;
	}
	stringTable.reserve(stringTable.size() + stringTableSizeIncrement);
	for(uint32 i = 0; i < externalSymbols.size(); i++)
	{
		const auto& externalSymbol = externalSymbols[i];
		auto& externalSymbolInfo = externalSymbolInfos[i];
		externalSymbolInfo.nameOffset = stringTable.size();
		stringTable.insert(std::end(stringTable), std::begin(externalSymbol.name), std::end(externalSymbol.name));
		stringTable.push_back(0);
	}
}

CMachoObjectFile::SECTION CMachoObjectFile::BuildSection(const InternalSymbolArray& internalSymbols, InternalSymbolInfoArray& internalSymbolInfos, INTERNAL_SYMBOL_LOCATION location)
{
	SECTION section;
	auto& sectionData(section.data);
	uint32 sectionSize = 0;
	for(const auto& internalSymbol : internalSymbols)
	{
		sectionSize += internalSymbol.data.size();
	}
	sectionData.reserve(sectionSize);
	for(uint32 i = 0; i < internalSymbols.size(); i++)
	{
		const auto& internalSymbol = internalSymbols[i];
		if(internalSymbol.location != location) continue;

		auto& internalSymbolInfo = internalSymbolInfos[i];
		internalSymbolInfo.dataOffset = sectionData.size();
		for(const auto& symbolReference : internalSymbol.symbolReferences)
		{
			SYMBOL_REFERENCE newReference;
			newReference.offset			= symbolReference.offset + internalSymbolInfo.dataOffset;
			newReference.symbolIndex	= symbolReference.symbolIndex;
			newReference.type			= symbolReference.type;
			section.symbolReferences.push_back(newReference);
		}
		sectionData.insert(std::end(sectionData), std::begin(internalSymbol.data), std::end(internalSymbol.data));
	}
	return section;
}
