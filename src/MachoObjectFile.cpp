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
	SectionArray sections;

	{
		SECTION section;
		memset(&section, 0, sizeof(SECTION));
		strncpy(section.sectionName, "__text", 0x10);
		strncpy(section.segmentName, "__TEXT", 0x10);
		
		section.sectionName[0x0F] = 0;
		section.segmentName[0x0F] = 0;

		section.address				= 0;
		section.size				= 0x250;
		section.offset				= 0x6D8;
		section.align				= 0x02;
		section.relativeOffset		= 0x3230;
		section.relocationCount		= 0x0D;
		section.flags				= S_ATTR_SOME_INSTRUCTIONS | S_ATTR_PURE_INSTRUCTIONS;
		section.reserved1			= 0;
		section.reserved2			= 0;

		sections.push_back(section);
	}

	SegmentCommandArray segmentCommands;

	{
		SEGMENT_COMMAND segmentCommand;
		memset(&segmentCommand, 0, sizeof(SEGMENT_COMMAND));
		segmentCommand.cmd				= LC_SEGMENT;
		segmentCommand.cmdSize			= 0x654;
		segmentCommand.vmAddress		= 0;
		segmentCommand.vmSize			= 0x2B63;
		segmentCommand.fileOffset		= 0x6D8;
		segmentCommand.fileSize			= 0x2B58;
		segmentCommand.maxProtection	= 0x07;
		segmentCommand.initProtection	= 0x07;
		segmentCommand.sectorCount		= 0x17;
		segmentCommand.flags			= 0;

		segmentCommands.push_back(segmentCommand);
	}

	MACH_HEADER header = {};
	header.magic			= MH_MAGIC;
	header.cpuType			= CPU_TYPE_I386;
	header.cpuSubType		= CPU_SUBTYPE_I386_ALL;
	header.fileType			= MH_OBJECT;
	header.commandCount		= 1;
	header.sizeofCommands	= 0;
	header.flags			= MH_NOMULTIDEFS;

	stream.Write(&header, sizeof(MACH_HEADER));

	for(const auto& segmentCommand : segmentCommands)
	{
		stream.Write(&segmentCommand, sizeof(SEGMENT_COMMAND));
	}

	for(const auto& section : sections)
	{
		stream.Write(&section, sizeof(SECTION));
	}
}