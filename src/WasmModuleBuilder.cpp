#include "WasmModuleBuilder.h"
#include <cassert>
#include "WasmDefs.h"

static void WriteName(Framework::CStream& stream, const char* str)
{
	auto length = strlen(str);
	assert(length < 0x80);
	stream.Write8(length);
	stream.Write(str, length);
}

void CWasmModuleBuilder::WriteModule(Framework::CStream& stream)
{
	stream.Write32(Wasm::BINARY_MAGIC);
	stream.Write32(Wasm::BINARY_VERSION);

	//Section "Type"
	{
		//Header
		Wasm::SECTION_HEADER typeSection;
		typeSection.code = Wasm::SECTION_ID_TYPE;
		typeSection.size = 5;
		stream.Write(&typeSection, sizeof(Wasm::SECTION_HEADER));

		stream.Write8(1); //Type vector size

		//Type 0
		stream.Write8(0x60); //Func
		stream.Write8(1); //Num params
		stream.Write8(Wasm::TYPE_I32);
		stream.Write8(0); //Num results
	}

	//Section "Import"
	{
		//Header
		Wasm::SECTION_HEADER importSection;
		importSection.code = Wasm::SECTION_ID_IMPORT;
		importSection.size = 0x0F;
		stream.Write(&importSection, sizeof(Wasm::SECTION_HEADER));

		stream.Write8(1); //Import vector size

		//Import 0
		WriteName(stream, "env"); //Import module name
		WriteName(stream, "memory"); //Import field name
		stream.Write8(Wasm::IMPORT_EXPORT_TYPE_MEMORY);
		stream.Write8(0x00); //Limit type 0
		stream.Write8(0x01); //Min
	}

	//Section "Function"
	{
		Wasm::SECTION_HEADER functionSection;
		functionSection.code = Wasm::SECTION_ID_FUNCTION;
		functionSection.size = 0x02;
		stream.Write(&functionSection, sizeof(Wasm::SECTION_HEADER));

		stream.Write8(1); //Function vector size

		//Function 0
		stream.Write8(0); //Signature index
	}

	//Section "Export"
	{
		//Header
		Wasm::SECTION_HEADER exportSection;
		exportSection.code = Wasm::SECTION_ID_EXPORT;
		exportSection.size = 0x0A;
		stream.Write(&exportSection, sizeof(Wasm::SECTION_HEADER));

		stream.Write8(1); //Export vector size

		//Export 0
		WriteName(stream, "addTwo");
		stream.Write8(Wasm::IMPORT_EXPORT_TYPE_FUNCTION);
		stream.Write8(0); //Function index
	}

	//Section "Code"
	{
		//Header
		Wasm::SECTION_HEADER codeSection;
		codeSection.code = Wasm::SECTION_ID_CODE;
		codeSection.size = 0x0E;
		stream.Write(&codeSection, sizeof(Wasm::SECTION_HEADER));

		stream.Write8(1); //Function vector size

		//Function 0
		stream.Write8(0x0C); //Function body size
		stream.Write8(0); //Local declaration count

		stream.Write8(0x20); //local.get
		stream.Write8(0x00);

		stream.Write8(0x41); //i32.const
		stream.Write8(0x04);

		stream.Write8(Wasm::INST_I32_ADD);

		stream.Write8(0x41); //i32.const
		stream.Write8(0x7F);

		stream.Write8(0x36); //i32.store
		stream.Write8(0x02);
		stream.Write8(0x00);

		stream.Write8(Wasm::INST_END);
	}
}
