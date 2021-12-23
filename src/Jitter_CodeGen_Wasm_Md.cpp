#include "Jitter_CodeGen_Wasm.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

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

CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_mdConstMatchers[] =
{
	{ OP_MOV,            MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,           MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_Mov_MemMem                            },

	{ OP_MD_ADD_B,       MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_ADD>       },

	{ OP_MD_ADDSS_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_ADD_SAT_S> },

	{ OP_MD_ADDUS_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_ADD_SAT_U> },

	{ OP_MD_CMPEQ_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_EQ>        },
	{ OP_MD_CMPEQ_H,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_EQ>        },
	{ OP_MD_CMPEQ_W,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I32x4_EQ>        },

	{ OP_MD_CMPGT_B,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I8x16_GT_S>      },
	{ OP_MD_CMPGT_H,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I16x8_GT_S>      },
	{ OP_MD_CMPGT_W,     MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_MEMORY128,     MATCH_NIL,      &CCodeGen_Wasm::Emit_Md_MemMemMem<Wasm::INST_I32x4_GT_S>      },

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

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      nullptr                                                       },
};
