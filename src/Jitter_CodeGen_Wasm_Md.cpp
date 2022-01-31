#include "Jitter_CodeGen_Wasm.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

static const uint8 g_packHBShuffle[0x10] = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 };
static const uint8 g_packWHShuffle[0x10] = { 0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29 };

static const uint8 g_unpackLowerBHShuffle[0x10] = { 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23 };
static const uint8 g_unpackLowerHWShuffle[0x10] = { 0, 1, 16, 17, 2, 3, 18, 19, 4, 5, 20, 21, 6, 7, 22, 23 };
static const uint8 g_unpackLowerWDShuffle[0x10] = { 0, 1, 2, 3, 16, 17, 18, 19, 4, 5, 6, 7, 20, 21, 22, 23 };

static const uint8 g_unpackUpperBHShuffle[0x10] = { 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31 };
static const uint8 g_unpackUpperHWShuffle[0x10] = { 8, 9, 24, 25, 10, 11, 26, 27, 12, 13, 28, 29, 14, 15, 30, 31 };
static const uint8 g_unpackUpperWDShuffle[0x10] = { 8, 9, 10, 11, 24, 25, 26, 27, 12, 13, 14, 15, 28, 29, 30, 31 };

template <uint32 OP>
void CCodeGen_Wasm::Emit_Md_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, OP);

	CommitSymbol(dst);
}

template <uint32 OP>
void CCodeGen_Wasm::Emit_Md_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, OP);

	CommitSymbol(dst);
}

template <uint32 OP>
void CCodeGen_Wasm::Emit_Md_Shift_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, OP);

	CommitSymbol(dst);
}

template <const uint8* SHUFFLE_PATTERN>
void CCodeGen_Wasm::Emit_Md_Unpack_MemMemMemRev(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src2);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I8x16_SHUFFLE);
	m_functionStream.Write(SHUFFLE_PATTERN, 0x10);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::PushRelative128(CSymbol* symbol)
{
	PushRelativeAddress(symbol);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_V128_LOAD);
	m_functionStream.Write8(0x04);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PushTemporary128(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY128);

	uint32 localIdx = GetTemporaryLocation(symbol);

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::PullTemporary128(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY128);

	uint32 localIdx = GetTemporaryLocation(symbol);

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx);
}

void CCodeGen_Wasm::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Md_AddSSW_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

//	This is based on code from http://locklessinc.com/articles/sat_arithmetic/ modified to work without cmovns
//	s32b sat_adds32b(s32b x, s32b y)
//	{
//		u32b ux = x;
//		u32b uy = y;
//		u32b res = ux + uy;
//	
//		/* Calculate overflowed result. (Don't change the sign bit of ux) */
//		s32b ovf = (ux >> 31) + INT_MAX;
//	
//		s32b sign = (s32b) ((ovf ^ uy) | ~(uy ^ res))
//		sign >>= 31;		/* Arithmetic shift, either 0 or ~0*/
//		res = (res & sign) | (ovf & ~sign);
//		
//		return res;
//	}

	auto pushRes = [this, src1, src2]()
	{
		PrepareSymbolUse(src1);
		PrepareSymbolUse(src2);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_ADD);
	};

	auto pushOvf = [this, src1]()
	{
		//Push INT_MAX vector
		m_functionStream.Write8(Wasm::INST_I32_CONST);
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, 0x7FFFFFFF);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		m_functionStream.Write8(Wasm::INST_I32x4_SPLAT);
		
		//Compute (x >> 31)
		PrepareSymbolUse(src1);

		m_functionStream.Write8(Wasm::INST_I32_CONST);
		m_functionStream.Write8(31);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_SHR_U);

		//Add
		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_ADD);
	};

	auto pushSign = [this, src2, pushRes, pushOvf]()
	{
		//~(uy ^ res)
		{
			PrepareSymbolUse(src2);
			pushRes();

			m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
			CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_XOR);

			m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
			CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_NOT);
		}

		//(ovf ^ uy)
		{
			PrepareSymbolUse(src2);
			pushOvf();

			m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
			CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_XOR);
		}

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_OR);

		//extract sign
		m_functionStream.Write8(Wasm::INST_I32_CONST);
		m_functionStream.Write8(31);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_SHR_S);
	};

	PrepareSymbolDef(dst);

	//(res & sign)
	{
		pushRes();
		pushSign();

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_AND);
	}

	//(ovf & ~sign)
	{
		pushOvf();
		pushSign();

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_NOT);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_AND);
	}

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_OR);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Md_AddUSW_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

//	This is based on code from http://locklessinc.com/articles/sat_arithmetic/
//	u32b sat_addu32b(u32b x, u32b y)
//	{
//		u32b res = x + y;
//		res |= -(res < x);
//	
//		return res;
//	}

	PrepareSymbolDef(dst);
	
	//-(res < x) -> -((x + y) < x)
	{
		PrepareSymbolUse(src1);
		PrepareSymbolUse(src2);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_ADD);

		PrepareSymbolUse(src1);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_LT_U);
	}

	//(x + y)
	{
		PrepareSymbolUse(src1);
		PrepareSymbolUse(src2);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_ADD);
	}

	//Combine
	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_OR);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Md_MakeSz_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	static const uint8 zeroConst[0x10] = { 0 };

	static const uint8 makeSzShufflePattern[0x10] =
	{
		0x1E, 0x1C, 0x1A, 0x18, 0x16, 0x14, 0x12, 0x10,
		0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x00,
	};

	PrepareSymbolDef(dst);

	//Compute sign
	{
		PrepareSymbolUse(src1);

		m_functionStream.Write8(Wasm::INST_I32_CONST);
		m_functionStream.Write8(31);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I32x4_SHR_S);
	}

	//Compute zero
	{
		PrepareSymbolUse(src1);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		m_functionStream.Write8(Wasm::INST_V128_CONST);
		m_functionStream.Write(zeroConst, 0x10);

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		m_functionStream.Write8(Wasm::INST_F32x4_EQ);
	}

	//Merge sign and zero
	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I8x16_SHUFFLE);
	m_functionStream.Write(makeSzShufflePattern, 0x10);

	//Extract bits
	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I16x8_BITMASK);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Md_LoadFromRef_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_V128_LOAD);
	m_functionStream.Write8(0x04);
	m_functionStream.Write8(0x00);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Md_StoreAtRef_MemMem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_V128_STORE);
	m_functionStream.Write8(0x04);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Md_MovMasked_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto mask = static_cast<uint8>(statement.jmpCondition);

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_I8x16_SHUFFLE);

	for(uint32 i = 0; i < 16; i++)
	{
		uint32 wordIndex = (i / 4);
		uint32 select = mask & (1 << wordIndex);
		uint32 byteIndex = i + (select ? 16 : 0);
		assert(byteIndex < 32);
		m_functionStream.Write8(static_cast<uint8>(byteIndex));
	}

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Md_Expand_MemAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	m_functionStream.Write8(Wasm::INST_I32x4_SPLAT);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Md_Srl256_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto localIdx = GetTemporaryLocation(src1);

	PrepareSymbolDef(dst);

	for(uint32 part = 0; part < 2; part++)
	{
		m_functionStream.Write8(Wasm::INST_LOCAL_GET);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx + part);

		//Generate empty vector
		{
			m_functionStream.Write8(Wasm::INST_I32_CONST);
			CWasmModuleBuilder::WriteSLeb128(m_functionStream, 0);

			m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
			m_functionStream.Write8(Wasm::INST_I32x4_SPLAT);
		}

		//Generate swizzle pattern
		for(uint32 i = 0; i < 0x10; i++)
		{
			PrepareSymbolUse(src2);

			m_functionStream.Write8(Wasm::INST_I32_CONST);
			CWasmModuleBuilder::WriteSLeb128(m_functionStream, 0x7F);

			m_functionStream.Write8(Wasm::INST_I32_AND);

			m_functionStream.Write8(Wasm::INST_I32_CONST);
			CWasmModuleBuilder::WriteSLeb128(m_functionStream, 3);

			m_functionStream.Write8(Wasm::INST_I32_SHR_U);

			m_functionStream.Write8(Wasm::INST_I32_CONST);
			if(part == 0)
			{
				CWasmModuleBuilder::WriteSLeb128(m_functionStream, static_cast<int32>(i));
			}
			else
			{
				CWasmModuleBuilder::WriteSLeb128(m_functionStream, static_cast<int32>(i - 0x10));
			}

			m_functionStream.Write8(Wasm::INST_I32_ADD);

			m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
			m_functionStream.Write8(Wasm::INST_I8x16_REPLACE_LANE);
			m_functionStream.Write8(i);
		}

		m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
		CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I8x16_SWIZZLE);
	}

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_V128_OR);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Md_Srl256_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto localIdx = GetTemporaryLocation(src1);
	auto byteShiftAmount = (src2->m_valueLow & 0x7F) / 8;

	uint8 shufflePattern[0x10] = {};
	for(uint32 i = 0; i < 0x10; i++)
	{
		shufflePattern[i] = i + byteShiftAmount;
	}

	PrepareSymbolDef(dst);

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx + 0);

	m_functionStream.Write8(Wasm::INST_LOCAL_GET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx + 1);

	m_functionStream.Write8(Wasm::INST_PREFIX_SIMD);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, Wasm::INST_I8x16_SHUFFLE);
	m_functionStream.Write(shufflePattern, 0x10);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_MergeTo256_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto localIdx = GetTemporaryLocation(dst);

	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx + 1);

	m_functionStream.Write8(Wasm::INST_LOCAL_SET);
	CWasmModuleBuilder::WriteULeb128(m_functionStream, localIdx + 0);
}

CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_mdConstMatchers[] =
{
	{ OP_MOV,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Mov_MemMem                            },

	{ OP_MD_ADD_B,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_ADD>       },
	{ OP_MD_ADD_H,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_ADD>       },
	{ OP_MD_ADD_W,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I32x4_ADD>       },

	{ OP_MD_ADDSS_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_ADD_SAT_S> },
	{ OP_MD_ADDSS_H,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_ADD_SAT_S> },
	{ OP_MD_ADDSS_W,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_AddSSW_MemMemMem                      },

	{ OP_MD_ADDUS_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_ADD_SAT_U> },
	{ OP_MD_ADDUS_H,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_ADD_SAT_U> },
	{ OP_MD_ADDUS_W,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_AddUSW_MemMemMem                      },

	{ OP_MD_SUB_B,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_SUB>       },
	{ OP_MD_SUB_H,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_SUB>       },
	{ OP_MD_SUB_W,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I32x4_SUB>       },

	{ OP_MD_SUBSS_H,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_SUB_SAT_S> },

	{ OP_MD_SUBUS_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_SUB_SAT_U> },
	{ OP_MD_SUBUS_H,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_SUB_SAT_U> },

	{ OP_MD_CMPEQ_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_EQ>        },
	{ OP_MD_CMPEQ_H,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_EQ>        },
	{ OP_MD_CMPEQ_W,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I32x4_EQ>        },

	{ OP_MD_CMPGT_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_GT_S>      },
	{ OP_MD_CMPGT_H,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_GT_S>      },
	{ OP_MD_CMPGT_W,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I32x4_GT_S>      },

	{ OP_MD_MIN_H,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_MIN_S>     },
	{ OP_MD_MIN_W,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I32x4_MIN_S>     },

	{ OP_MD_MAX_H,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_MAX_S>     },
	{ OP_MD_MAX_W,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I32x4_MAX_S>     },

	{ OP_MD_ADD_S,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_F32x4_ADD>       },
	{ OP_MD_SUB_S,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_F32x4_SUB>       },
	{ OP_MD_MUL_S,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_F32x4_MUL>       },
	{ OP_MD_DIV_S,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_F32x4_DIV>       },

	{ OP_MD_ABS_S,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMem<Wasm::INST_F32x4_ABS>          },
	{ OP_MD_MIN_S,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_F32x4_MIN>       },
	{ OP_MD_MAX_S,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_F32x4_MAX>       },

	{ OP_MD_CMPLT_S,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_F32x4_LT>        },
	{ OP_MD_CMPGT_S,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_F32x4_GT>        },

	{ OP_MD_AND,         MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_V128_AND>        },
	{ OP_MD_OR,          MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_V128_OR>         },
	{ OP_MD_XOR,         MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_V128_XOR>        },
	{ OP_MD_NOT,         MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMem<Wasm::INST_V128_NOT>           },

	{ OP_MD_SLLH,        MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Shift_MemMemCst<Wasm::INST_I16x8_SHL> },
	{ OP_MD_SLLW,        MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Shift_MemMemCst<Wasm::INST_I32x4_SHL> },

	{ OP_MD_SRLH,        MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Shift_MemMemCst<Wasm::INST_I16x8_SHR_U> },
	{ OP_MD_SRLW,        MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Shift_MemMemCst<Wasm::INST_I32x4_SHR_U> },

	{ OP_MD_SRAH,        MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Shift_MemMemCst<Wasm::INST_I16x8_SHR_S> },
	{ OP_MD_SRAW,        MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Shift_MemMemCst<Wasm::INST_I32x4_SHR_S> },

	{ OP_MD_MAKESZ,      MATCH_MEMORY,         MATCH_MEMORY128,      MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MakeSz_MemMem                           },

	{ OP_MD_TOSINGLE,           MATCH_MEMORY128,    MATCH_MEMORY128,    MATCH_NIL,        MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMem<Wasm::INST_F32x4_CONVERT_I32x4_S>   },
	{ OP_MD_TOWORD_TRUNCATE,    MATCH_MEMORY128,    MATCH_MEMORY128,    MATCH_NIL,        MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMem<Wasm::INST_I32x4_TRUNC_SAT_F32x4_S> },

	{ OP_LOADFROMREF,    MATCH_MEMORY128,      MATCH_MEM_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_LoadFromRef_MemMem                    },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_StoreAtRef_MemMem                     },

	{ OP_MD_MOV_MASKED,  MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MovMasked_MemMemMem                   },

	{ OP_MD_EXPAND,      MATCH_MEMORY128,      MATCH_MEMORY,         MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Expand_MemAny                         },
	{ OP_MD_EXPAND,      MATCH_MEMORY128,      MATCH_CONSTANT,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Expand_MemAny                         },

	{ OP_MD_PACK_HB,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Unpack_MemMemMemRev<g_packHBShuffle>  },
	{ OP_MD_PACK_WH,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Unpack_MemMemMemRev<g_packWHShuffle>  },

	{ OP_MD_UNPACK_LOWER_BH, MATCH_MEMORY128,  MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Unpack_MemMemMemRev<g_unpackLowerBHShuffle> },
	{ OP_MD_UNPACK_LOWER_HW, MATCH_MEMORY128,  MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Unpack_MemMemMemRev<g_unpackLowerHWShuffle> },
	{ OP_MD_UNPACK_LOWER_WD, MATCH_MEMORY128,  MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Unpack_MemMemMemRev<g_unpackLowerWDShuffle> },

	{ OP_MD_UNPACK_UPPER_BH, MATCH_MEMORY128,  MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Unpack_MemMemMemRev<g_unpackUpperBHShuffle> },
	{ OP_MD_UNPACK_UPPER_HW, MATCH_MEMORY128,  MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Unpack_MemMemMemRev<g_unpackUpperHWShuffle> },
	{ OP_MD_UNPACK_UPPER_WD, MATCH_MEMORY128,  MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Unpack_MemMemMemRev<g_unpackUpperWDShuffle> },

	{ OP_MD_SRL256,      MATCH_VARIABLE128,    MATCH_MEMORY256,      MATCH_VARIABLE,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Srl256_MemMemVar                      },
	{ OP_MD_SRL256,      MATCH_VARIABLE128,    MATCH_MEMORY256,      MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Srl256_MemMemCst                      },

	{ OP_MERGETO256,     MATCH_MEMORY256,      MATCH_VARIABLE128,    MATCH_VARIABLE128,   MATCH_NIL,      &CCodeGen_Wasm::Emit_MergeTo256_MemMemMem                     },

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      nullptr                                                       },
};
