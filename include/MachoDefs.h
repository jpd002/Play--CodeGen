#pragma once

#include "Types.h"

namespace Macho
{
	enum
	{
		MH_MAGIC = 0xFEEDFACE,
		MH_CIGAM = 0xCEFAEDFE,
	};

	enum CPU_TYPE
	{
		CPU_TYPE_I386		= 0x07,
		CPU_TYPE_ARM		= 0x0E,
	};

	enum CPU_SUBTYPE_I386
	{
		CPU_SUBTYPE_I386_ALL = 0x03,
	};

	enum CPU_SUBTYPE_ARM
	{
		CPU_SUBTYPE_ARM_V6	= 0x06,
		CPU_SUBTYPE_ARM_V7	= 0x09,
	};

	enum FILE_TYPE
	{
		MH_OBJECT			= 0x01,
	};

	enum HEADER_FLAGS
	{
		MH_SPLIT_SEGS		= 0x0020,
		MH_NOMULTIDEFS		= 0x2000,
	};

	enum SECTION_FLAGS
	{
		S_ATTR_SOME_INSTRUCTIONS	= 0x00000400,
		S_ATTR_PURE_INSTRUCTIONS	= 0x80000000,
	};

	enum LOAD_COMMAND
	{
		LC_SEGMENT			= 0x0001,
		LC_SYMTAB			= 0x0002
	};

	struct MACH_HEADER
	{
		uint32	magic;
		uint32	cpuType;
		uint32	cpuSubType;
		uint32	fileType;
		uint32	commandCount;
		uint32	sizeofCommands;
		uint32	flags;
	};
	static_assert(sizeof(MACH_HEADER) == 0x1C, "Size of MACH_HEADER structure must be 28 bytes.");

	struct COMMAND
	{
		uint32	cmd;
		uint32	cmdSize;
	};

	struct SEGMENT_COMMAND : public COMMAND
	{
		char	name[0x10];
		uint32	vmAddress;
		uint32	vmSize;
		uint32	fileOffset;
		uint32	fileSize;
		uint32	maxProtection;
		uint32	initProtection;
		uint32	sectionCount;
		uint32	flags;
	};
	static_assert(sizeof(SEGMENT_COMMAND) == 0x38, "Size of SEGMENT_COMMAND structure must be 48 bytes.");

	struct SECTION
	{
		char	sectionName[0x10];
		char	segmentName[0x10];
		uint32	address;
		uint32	size;
		uint32	offset;
		uint32	align;
		uint32	relocationOffset;
		uint32	relocationCount;
		uint32	flags;
		uint32	reserved1;
		uint32	reserved2;
	};
	static_assert(sizeof(SECTION) == 0x44, "Size of SECTION structure must be 68 bytes.");

	struct SYMTAB_COMMAND : public COMMAND
	{
		uint32	symbolsOffset;
		uint32	symbolCount;
		uint32	stringsOffset;
		uint32	stringCount;
	};
	static_assert(sizeof(SYMTAB_COMMAND) == 0x18, "Size of SYMTAB_COMMAND structure must be 24 bytes.");
}
