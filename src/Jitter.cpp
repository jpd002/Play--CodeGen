#include <assert.h>
#include "Jitter.h"
#include "placeholder_def.h"

using namespace std;
using namespace Jitter;

CJitter::CJitter(CCodeGen* codeGen)
    : m_codeGen(codeGen)
    , m_codeGenSupportsCmpSelect(codeGen->SupportsCmpSelect())
{
}

CJitter::~CJitter()
{
	delete m_codeGen;
}

CCodeGen* CJitter::GetCodeGen()
{
	return m_codeGen;
}

void CJitter::SetStream(Framework::CStream* stream)
{
	m_codeGen->SetStream(stream);
}

void CJitter::Begin()
{
	assert(m_blockStarted == false);
	m_blockStarted = true;
	m_nextTemporary = 1;
	m_nextBlockId = 1;
	m_basicBlocks.clear();

	StartBlock(m_nextBlockId++);
}

void CJitter::End()
{
	assert(m_shadow.GetCount() == 0);
	assert(m_blockStarted == true);
	m_blockStarted = false;

	Compile();
}

bool CJitter::IsStackEmpty() const
{
	return m_shadow.GetCount() == 0;
}

void CJitter::StartBlock(uint32 blockId)
{
	auto blockIterator = m_basicBlocks.emplace(m_basicBlocks.end(), BASIC_BLOCK());
	m_currentBlock = &(*blockIterator);
	m_currentBlock->id = blockId;
}

CJitter::LABEL CJitter::CreateLabel()
{
	return m_nextLabelId++;
}

void CJitter::MarkLabel(LABEL label)
{
	uint32 newBlockId = m_nextBlockId++;
	StartBlock(newBlockId);
	m_labels[label] = newBlockId;
}

void CJitter::Goto(LABEL label)
{
	assert(m_shadow.GetCount() == 0);

	STATEMENT statement;
	statement.op = OP_GOTO;
	statement.jmpBlock = label;
	InsertStatement(statement);
}

void CJitter::BeginIf(CONDITION condition)
{
	uint32 jumpBlockId = m_nextBlockId++;
	m_ifStack.push(jumpBlockId);

	STATEMENT statement;
	statement.op = OP_CONDJMP;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.jmpCondition = NegateCondition(condition);
	statement.jmpBlock = jumpBlockId;
	InsertStatement(statement);

	assert(m_shadow.GetCount() == 0);

	uint32 newBlockId = m_nextBlockId++;
	StartBlock(newBlockId);
}

void CJitter::Else()
{
	assert(!m_ifStack.empty());
	assert(m_shadow.GetCount() == 0);

	uint32 nextBlockId = m_ifStack.top();
	m_ifStack.pop();

	uint32 jumpBlockId = m_nextBlockId++;
	m_ifStack.push(jumpBlockId);

	STATEMENT statement;
	statement.op = OP_JMP;
	statement.jmpBlock = jumpBlockId;
	InsertStatement(statement);

	StartBlock(nextBlockId);
}

void CJitter::EndIf()
{
	assert(!m_ifStack.empty());
	assert(m_shadow.GetCount() == 0);

	uint32 nextBlockId = m_ifStack.top();
	m_ifStack.pop();
	StartBlock(nextBlockId);
}

void CJitter::PushCtx()
{
	m_shadow.Push(MakeSymbol(SYM_CONTEXT, 0));
}

void CJitter::PushCst(uint32 nValue)
{
	m_shadow.Push(MakeSymbol(SYM_CONSTANT, nValue));
}

void CJitter::PushRel(size_t nOffset)
{
	m_shadow.Push(MakeSymbol(SYM_RELATIVE, static_cast<uint32>(nOffset)));
}

void CJitter::PushIdx(unsigned int nIndex)
{
	m_shadow.Push(m_shadow.GetAt(nIndex));
}

void CJitter::PushTop()
{
	PushIdx(0);
}

uint32 CJitter::GetTopCursor() const
{
	uint32 cursor = m_shadow.GetCount();
	assert(cursor != 0);
	return cursor;
}

void CJitter::PushCursor(uint32 cursor)
{
	int32 relativeIndex = static_cast<int32>(m_shadow.GetCount()) - static_cast<int32>(cursor);
	if(relativeIndex < 0)
	{
		throw std::runtime_error("Invalid cursor.");
	}
	PushIdx(relativeIndex);
}

void CJitter::PullRel(size_t nOffset)
{
	STATEMENT statement;
	statement.op = OP_MOV;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(MakeSymbol(SYM_RELATIVE, static_cast<uint32>(nOffset)));
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
}

void CJitter::PullTop()
{
	m_shadow.Pull();
}

void CJitter::Swap()
{
	SymbolPtr symbol1 = m_shadow.Pull();
	SymbolPtr symbol2 = m_shadow.Pull();
	m_shadow.Push(symbol1);
	m_shadow.Push(symbol2);
}

void CJitter::Add()
{
	InsertBinaryStatement(OP_ADD);
}

void CJitter::And()
{
	InsertBinaryStatement(OP_AND);
}

void CJitter::Break()
{
	STATEMENT statement;
	statement.op = OP_BREAK;
	InsertStatement(statement);
}

void CJitter::Call(void* func, unsigned int paramCount, RETURN_VALUE_TYPE returnValue)
{
	for(unsigned int i = 0; i < paramCount; i++)
	{
		STATEMENT paramStatement;
		paramStatement.src1 = MakeSymbolRef(m_shadow.Pull());
		paramStatement.op = OP_PARAM;
		InsertStatement(paramStatement);
	}

	bool hasImplicitReturnValueParam = false;
	SymbolPtr returnValueSym;
	switch(returnValue)
	{
	case RETURN_VALUE_NONE:
		break;
	case RETURN_VALUE_32:
		returnValueSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);
		break;
	case RETURN_VALUE_64:
		returnValueSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);
		break;
	case RETURN_VALUE_128:
		returnValueSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);
		if(!m_codeGen->CanHold128BitsReturnValueInRegisters())
		{
			STATEMENT paramStatement;
			paramStatement.src1 = MakeSymbolRef(returnValueSym);
			paramStatement.op = OP_PARAM_RET;
			InsertStatement(paramStatement);
			paramCount++;
			hasImplicitReturnValueParam = true;
		}
		break;
	default:
		assert(false);
		break;
	}

	STATEMENT callStatement;
	callStatement.src1 = MakeSymbolRef(MakeConstantPtr(reinterpret_cast<uintptr_t>(func)));
	callStatement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, paramCount));
	callStatement.op = OP_CALL;
	InsertStatement(callStatement);

	if(returnValue != RETURN_VALUE_NONE)
	{
		if(!hasImplicitReturnValueParam)
		{
			STATEMENT returnStatement;
			returnStatement.dst = MakeSymbolRef(returnValueSym);
			returnStatement.op = OP_RETVAL;
			InsertStatement(returnStatement);
		}

		m_shadow.Push(returnValueSym);
	}
}

void CJitter::Cmp(CONDITION condition)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_CMP;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.jmpCondition = condition;
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Div()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_DIV;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::DivS()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_DIVS;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::JumpTo(void* func)
{
	STATEMENT statement;
	statement.src1 = MakeSymbolRef(MakeConstantPtr(reinterpret_cast<uintptr_t>(func)));
	statement.op = OP_EXTERNJMP;
	InsertStatement(statement);
}

void CJitter::JumpToDynamic(void* func)
{
	STATEMENT statement;
	statement.src1 = MakeSymbolRef(MakeConstantPtr(reinterpret_cast<uintptr_t>(func)));
	statement.op = OP_EXTERNJMP_DYN;
	InsertStatement(statement);
}

void CJitter::Lookup(uint32* table)
{
	throw std::exception();
}

void CJitter::Lzc()
{
	InsertUnaryStatement(OP_LZC);
}

void CJitter::Mult()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MUL;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MultS()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MULS;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Not()
{
	InsertUnaryStatement(OP_NOT);
}

void CJitter::Or()
{
	InsertBinaryStatement(OP_OR);
}

void CJitter::Select()
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_SELECT;
	statement.src3 = MakeSymbolRef(m_shadow.Pull());
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::SignExt()
{
	Sra(31);
}

void CJitter::SignExt8()
{
	Shl(24);
	Sra(24);
}

void CJitter::SignExt16()
{
	Shl(16);
	Sra(16);
}

void CJitter::Shl()
{
	InsertBinaryStatement(OP_SLL);
}

void CJitter::Shl(uint8 amount)
{
	InsertShiftCstStatement(OP_SLL, amount);
}

void CJitter::Sra()
{
	InsertBinaryStatement(OP_SRA);
}

void CJitter::Sra(uint8 amount)
{
	InsertShiftCstStatement(OP_SRA, amount);
}

void CJitter::Srl()
{
	InsertBinaryStatement(OP_SRL);
}

void CJitter::Srl(uint8 amount)
{
	InsertShiftCstStatement(OP_SRL, amount);
}

void CJitter::Sub()
{
	InsertBinaryStatement(OP_SUB);
}

void CJitter::Xor()
{
	InsertBinaryStatement(OP_XOR);
}

//Memory Functions
//------------------------------------------------
void CJitter::PushRelRef(size_t offset)
{
	m_shadow.Push(MakeSymbol(SYM_REL_REFERENCE, static_cast<uint32>(offset)));
}

void CJitter::PushRelAddrRef(size_t offset)
{
	auto tempSym = MakeSymbol(SYM_TMP_REFERENCE, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_RELTOREF;
	statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, static_cast<uint32>(offset)));
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::AddRef()
{
	auto tempSym = MakeSymbol(SYM_TMP_REFERENCE, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_ADDREF;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::IsRefNull()
{
	InsertUnaryStatement(OP_ISREFNULL);
}

void CJitter::LoadFromRef()
{
	InsertUnaryStatement(OP_LOADFROMREF);
}

void CJitter::LoadFromRefIdx(size_t scale)
{
	assert(scale == 1 || scale == 4);
	InsertLoadFromRefIdxStatement(OP_LOADFROMREF, scale);
}

void CJitter::Load8FromRef()
{
	InsertUnaryStatement(OP_LOAD8FROMREF);
}

void CJitter::Load8FromRefIdx(size_t scale)
{
	assert(scale == 1);
	InsertLoadFromRefIdxStatement(OP_LOAD8FROMREF, scale);
}

void CJitter::Load16FromRef()
{
	InsertUnaryStatement(OP_LOAD16FROMREF);
}

void CJitter::Load16FromRefIdx(size_t scale)
{
	assert(scale == 1);
	InsertLoadFromRefIdxStatement(OP_LOAD16FROMREF, scale);
}

void CJitter::Load64FromRef()
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_LOADFROMREF;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Load64FromRefIdx(size_t scale)
{
	assert(scale == 1);

	auto tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_LOADFROMREF;
	statement.jmpCondition = static_cast<CONDITION>(scale);
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::LoadRefFromRef()
{
	auto tempSym = MakeSymbol(SYM_TMP_REFERENCE, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_LOADFROMREF;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::LoadRefFromRefIdx()
{
	size_t scale = m_codeGen->GetPointerSize();

	auto tempSym = MakeSymbol(SYM_TMP_REFERENCE, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_LOADFROMREF;
	statement.jmpCondition = static_cast<CONDITION>(scale);
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::StoreAtRef()
{
	STATEMENT statement;
	statement.op = OP_STOREATREF;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	InsertStatement(statement);
}

void CJitter::StoreAtRefIdx(size_t scale)
{
	assert(scale == 1 || scale == 4);
	InsertStoreAtRefIdxStatement(OP_STOREATREF, scale);
}

void CJitter::Store8AtRef()
{
	STATEMENT statement;
	statement.op = OP_STORE8ATREF;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	InsertStatement(statement);
}

void CJitter::Store8AtRefIdx(size_t scale)
{
	assert(scale == 1);
	InsertStoreAtRefIdxStatement(OP_STORE8ATREF, scale);
}

void CJitter::Store16AtRef()
{
	STATEMENT statement;
	statement.op = OP_STORE16ATREF;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	InsertStatement(statement);
}

void CJitter::Store16AtRefIdx(size_t scale)
{
	assert(scale == 1);
	InsertStoreAtRefIdxStatement(OP_STORE16ATREF, scale);
}

void CJitter::Store64AtRef()
{
	StoreAtRef();
}

void CJitter::Store64AtRefIdx(size_t scale)
{
	assert(scale == 1);
	StoreAtRefIdx(scale);
}

//64-bits
//------------------------------------------------
void CJitter::PushRel64(size_t offset)
{
	m_shadow.Push(MakeSymbol(SYM_RELATIVE64, static_cast<uint32>(offset)));
}

void CJitter::PushCst64(uint64 constant)
{
	m_shadow.Push(MakeConstant64(constant));
}

void CJitter::PullRel64(size_t offset)
{
	STATEMENT statement;
	statement.op = OP_MOV;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(MakeSymbol(SYM_RELATIVE64, static_cast<uint32>(offset)));
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
}

void CJitter::MergeTo64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MERGETO64;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::ExtLow64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_EXTLOW64;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == 8);

	m_shadow.Push(tempSym);
}

void CJitter::ExtHigh64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_EXTHIGH64;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == 8);

	m_shadow.Push(tempSym);
}

void CJitter::Add64()
{
	InsertBinary64Statement(OP_ADD64);
}

void CJitter::And64()
{
	InsertBinary64Statement(OP_AND64);
}

void CJitter::Cmp64(CONDITION condition)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_CMP64;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.jmpCondition = condition;
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Sub64()
{
	InsertBinary64Statement(OP_SUB64);
}

void CJitter::Srl64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_SRL64;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Srl64(uint8 nAmount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_SRL64;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, nAmount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Sra64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_SRA64;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Sra64(uint8 nAmount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_SRA64;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, nAmount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Shl64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_SLL64;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Shl64(uint8 nAmount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_SLL64;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, nAmount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

//Floating-Point
//------------------------------------------------
void CJitter::FP_PushCst32(float constant)
{
	auto tempSym = MakeSymbol(SYM_FP_TEMPORARY32, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_FP_LDCST;
	statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, *reinterpret_cast<uint32*>(&constant)));
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_PushRel32(size_t offset)
{
	m_shadow.Push(MakeSymbol(SYM_FP_RELATIVE32, static_cast<uint32>(offset)));
}

void CJitter::FP_PullRel32(size_t offset)
{
	STATEMENT statement;
	statement.op = OP_MOV;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(MakeSymbol(SYM_FP_RELATIVE32, static_cast<uint32>(offset)));
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
}

void CJitter::FP_AddS()
{
	InsertBinaryFp32Statement(OP_FP_ADD_S);
}

void CJitter::FP_SubS()
{
	InsertBinaryFp32Statement(OP_FP_SUB_S);
}

void CJitter::FP_MulS()
{
	InsertBinaryFp32Statement(OP_FP_MUL_S);
}

void CJitter::FP_DivS()
{
	InsertBinaryFp32Statement(OP_FP_DIV_S);
}

void CJitter::FP_CmpS(Jitter::CONDITION condition)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_FP_CMP_S;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	statement.jmpCondition = condition;
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_SqrtS()
{
	InsertUnaryFp32Statement(OP_FP_SQRT_S);
}

void CJitter::FP_RsqrtS()
{
	InsertUnaryFp32Statement(OP_FP_RSQRT_S);
}

void CJitter::FP_RcplS()
{
	InsertUnaryFp32Statement(OP_FP_RCPL_S);
}

void CJitter::FP_AbsS()
{
	InsertUnaryFp32Statement(OP_FP_ABS_S);
}

void CJitter::FP_NegS()
{
	InsertUnaryFp32Statement(OP_FP_NEG_S);
}

void CJitter::FP_MinS()
{
	InsertBinaryFp32Statement(OP_FP_MIN_S);
}

void CJitter::FP_MaxS()
{
	InsertBinaryFp32Statement(OP_FP_MAX_S);
}

void CJitter::FP_ClampS()
{
	InsertUnaryFp32Statement(OP_FP_CLAMP_S);
}

void CJitter::FP_ToInt32TruncateS()
{
	auto tempSym = MakeSymbol(SYM_FP_TEMPORARY32, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_FP_TOINT32_TRUNC_S;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_ToSingleI32()
{
	auto tempSym = MakeSymbol(SYM_FP_TEMPORARY32, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_FP_TOSINGLE_I32;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_SetRoundingMode(ROUNDINGMODE roundingMode)
{
	STATEMENT statement;
	statement.op = OP_FP_SETROUNDINGMODE;
	statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, roundingMode));
	InsertStatement(statement);
}

//SIMD
//------------------------------------------------
void CJitter::MD_PullRel(size_t offset)
{
	STATEMENT statement;
	statement.op = OP_MOV;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(MakeSymbol(SYM_RELATIVE128, static_cast<uint32>(offset)));
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
}

void CJitter::MD_PullRel(size_t offset, bool save0, bool save1, bool save2, bool save3)
{
	if(save0 && save1 && save2 && save3)
	{
		MD_PullRel(offset);
	}
	else
	{
		uint8 mask =
		    ((save0 ? 1 : 0) << 0) |
		    ((save1 ? 1 : 0) << 1) |
		    ((save2 ? 1 : 0) << 2) |
		    ((save3 ? 1 : 0) << 3);

		assert(mask != 0);

		STATEMENT statement;
		statement.op = OP_MD_MOV_MASKED;
		statement.dst = MakeSymbolRef(MakeSymbol(SYM_RELATIVE128, static_cast<uint32>(offset)));
		statement.src1 = MakeSymbolRef(MakeSymbol(SYM_RELATIVE128, static_cast<uint32>(offset)));
		statement.src2 = MakeSymbolRef(m_shadow.Pull());
		statement.jmpCondition = static_cast<Jitter::CONDITION>(mask);
		InsertStatement(statement);

		assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
	}
}

void CJitter::MD_PushRel(size_t offset)
{
	m_shadow.Push(MakeSymbol(SYM_RELATIVE128, static_cast<uint32>(offset)));
}

void CJitter::MD_PushRelElementExpandW(size_t offset, uint32 elementIdx)
{
	assert(elementIdx < 4);

	auto tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_EXPAND_W;
	statement.src1 = MakeSymbolRef(MakeSymbol(SYM_RELATIVE128, static_cast<uint32>(offset)));
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, elementIdx));
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_PushCstExpandW(uint32 constant)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_EXPAND_W;
	statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, constant));
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_PushCstExpandS(float value)
{
	MD_PushCstExpandW(*reinterpret_cast<uint32*>(&value));
}

void CJitter::MD_LoadFromRef()
{
	InsertUnaryMdStatement(OP_LOADFROMREF);
}

void CJitter::MD_LoadFromRefIdx(size_t scale)
{
	//Natural scale not supported yet (no use for it atm and x86 doesn't support it)
	assert(scale == 1);

	auto tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_LOADFROMREF;
	statement.jmpCondition = static_cast<CONDITION>(scale);
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_LoadFromRefIdxMasked(bool save0, bool save1, bool save2, bool save3)
{
	if(save0 && save1 && save2 && save3)
	{
		//Discard value to blend with (since we're just keeping everything from mem access)
		m_shadow.Pull();
		MD_LoadFromRefIdx(1);
	}
	else
	{
		auto tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

		uint8 mask =
		    ((save0 ? 1 : 0) << 0) |
		    ((save1 ? 1 : 0) << 1) |
		    ((save2 ? 1 : 0) << 2) |
		    ((save3 ? 1 : 0) << 3);

		assert(mask != 0);

		STATEMENT statement;
		statement.op = OP_MD_LOADFROMREF_MASKED;
		statement.src3 = MakeSymbolRef(m_shadow.Pull());
		statement.src2 = MakeSymbolRef(m_shadow.Pull());
		statement.src1 = MakeSymbolRef(m_shadow.Pull());
		statement.dst = MakeSymbolRef(tempSym);
		statement.jmpCondition = static_cast<Jitter::CONDITION>(mask);

		InsertStatement(statement);

		m_shadow.Push(tempSym);
	}
}

void CJitter::MD_StoreAtRefIdxMasked(bool save0, bool save1, bool save2, bool save3)
{
	if(save0 && save1 && save2 && save3)
	{
		MD_LoadFromRefIdx(1);
	}
	else
	{
		uint8 mask =
		    ((save0 ? 1 : 0) << 0) |
		    ((save1 ? 1 : 0) << 1) |
		    ((save2 ? 1 : 0) << 2) |
		    ((save3 ? 1 : 0) << 3);

		assert(mask != 0);

		STATEMENT statement;
		statement.op = OP_MD_STOREATREF_MASKED;
		statement.jmpCondition = static_cast<CONDITION>(mask);
		statement.src3 = MakeSymbolRef(m_shadow.Pull());
		statement.src2 = MakeSymbolRef(m_shadow.Pull());
		statement.src1 = MakeSymbolRef(m_shadow.Pull());
		InsertStatement(statement);
	}
}

void CJitter::MD_StoreAtRef()
{
	StoreAtRef();
}

void CJitter::MD_StoreAtRefIdx(size_t scale)
{
	//Natural scale not supported yet (no use for it atm and x86 doesn't support it)
	assert(scale == 1);
	StoreAtRefIdx(scale);
}

void CJitter::MD_AddB()
{
	InsertBinaryMdStatement(OP_MD_ADD_B);
}

void CJitter::MD_AddBSS()
{
	InsertBinaryMdStatement(OP_MD_ADDSS_B);
}

void CJitter::MD_AddBUS()
{
	InsertBinaryMdStatement(OP_MD_ADDUS_B);
}

void CJitter::MD_AddH()
{
	InsertBinaryMdStatement(OP_MD_ADD_H);
}

void CJitter::MD_AddHSS()
{
	InsertBinaryMdStatement(OP_MD_ADDSS_H);
}

void CJitter::MD_AddHUS()
{
	InsertBinaryMdStatement(OP_MD_ADDUS_H);
}

void CJitter::MD_AddW()
{
	InsertBinaryMdStatement(OP_MD_ADD_W);
}

void CJitter::MD_AddWSS()
{
	InsertBinaryMdStatement(OP_MD_ADDSS_W);
}

void CJitter::MD_AddWUS()
{
	InsertBinaryMdStatement(OP_MD_ADDUS_W);
}

void CJitter::MD_SubB()
{
	InsertBinaryMdStatement(OP_MD_SUB_B);
}

void CJitter::MD_SubBUS()
{
	InsertBinaryMdStatement(OP_MD_SUBUS_B);
}

void CJitter::MD_SubHSS()
{
	InsertBinaryMdStatement(OP_MD_SUBSS_H);
}

void CJitter::MD_SubHUS()
{
	InsertBinaryMdStatement(OP_MD_SUBUS_H);
}

void CJitter::MD_SubH()
{
	InsertBinaryMdStatement(OP_MD_SUB_H);
}

void CJitter::MD_SubW()
{
	InsertBinaryMdStatement(OP_MD_SUB_W);
}

void CJitter::MD_SubWSS()
{
	InsertBinaryMdStatement(OP_MD_SUBSS_W);
}

void CJitter::MD_SubWUS()
{
	InsertBinaryMdStatement(OP_MD_SUBUS_W);
}

void CJitter::MD_And()
{
	InsertBinaryMdStatement(OP_MD_AND);
}

void CJitter::MD_Or()
{
	InsertBinaryMdStatement(OP_MD_OR);
}

void CJitter::MD_Xor()
{
	InsertBinaryMdStatement(OP_MD_XOR);
}

void CJitter::MD_Not()
{
	InsertUnaryMdStatement(OP_MD_NOT);
}

void CJitter::MD_SllH(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_SLLH;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SllW(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_SLLW;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SrlH(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_SRLH;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SrlW(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_SRLW;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SraH(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_SRAH;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SraW(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_SRAW;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_Srl256()
{
	SymbolPtr shiftAmount = m_shadow.Pull();
	SymbolPtr src2 = m_shadow.Pull();
	SymbolPtr src1 = m_shadow.Pull();

	{
		//Operand order is reversed here because what's shifted is actually the concatenation of (src1 || src2)
		SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY256, m_nextTemporary++);

		STATEMENT statement;
		statement.op = OP_MERGETO256;
		statement.src2 = MakeSymbolRef(src1);
		statement.src1 = MakeSymbolRef(src2);
		statement.dst = MakeSymbolRef(tempSym);
		InsertStatement(statement);

		m_shadow.Push(tempSym);
	}

	{
		SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

		STATEMENT statement;
		statement.op = OP_MD_SRL256;
		statement.src2 = MakeSymbolRef(shiftAmount);
		statement.src1 = MakeSymbolRef(m_shadow.Pull());
		statement.dst = MakeSymbolRef(tempSym);
		InsertStatement(statement);

		m_shadow.Push(tempSym);
	}
}

void CJitter::MD_MinH()
{
	InsertBinaryMdStatement(OP_MD_MIN_H);
}

void CJitter::MD_MinW()
{
	InsertBinaryMdStatement(OP_MD_MIN_W);
}

void CJitter::MD_MaxH()
{
	InsertBinaryMdStatement(OP_MD_MAX_H);
}

void CJitter::MD_MaxW()
{
	InsertBinaryMdStatement(OP_MD_MAX_W);
}

void CJitter::MD_ClampS()
{
	InsertUnaryMdStatement(OP_MD_CLAMP_S);
}

void CJitter::MD_CmpEqB()
{
	InsertBinaryMdStatement(OP_MD_CMPEQ_B);
}

void CJitter::MD_CmpEqH()
{
	InsertBinaryMdStatement(OP_MD_CMPEQ_H);
}

void CJitter::MD_CmpEqW()
{
	InsertBinaryMdStatement(OP_MD_CMPEQ_W);
}

void CJitter::MD_CmpGtB()
{
	InsertBinaryMdStatement(OP_MD_CMPGT_B);
}

void CJitter::MD_CmpGtH()
{
	InsertBinaryMdStatement(OP_MD_CMPGT_H);
}

void CJitter::MD_CmpGtW()
{
	InsertBinaryMdStatement(OP_MD_CMPGT_W);
}

void CJitter::MD_CmpLtS()
{
	InsertBinaryMdStatement(OP_MD_CMPLT_S);
}

void CJitter::MD_CmpGtS()
{
	InsertBinaryMdStatement(OP_MD_CMPGT_S);
}

void CJitter::MD_UnpackLowerBH()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_LOWER_BH);
}

void CJitter::MD_UnpackLowerHW()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_LOWER_HW);
}

void CJitter::MD_UnpackLowerWD()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_LOWER_WD);
}

void CJitter::MD_UnpackUpperBH()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_UPPER_BH);
}

void CJitter::MD_UnpackUpperHW()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_UPPER_HW);
}

void CJitter::MD_UnpackUpperWD()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_UPPER_WD);
}

void CJitter::MD_PackHB()
{
	InsertBinaryMdStatement(OP_MD_PACK_HB);
}

void CJitter::MD_PackWH()
{
	InsertBinaryMdStatement(OP_MD_PACK_WH);
}

void CJitter::MD_AddS()
{
	InsertBinaryMdStatement(OP_MD_ADD_S);
}

void CJitter::MD_SubS()
{
	InsertBinaryMdStatement(OP_MD_SUB_S);
}

void CJitter::MD_MulS()
{
	InsertBinaryMdStatement(OP_MD_MUL_S);
}

void CJitter::MD_DivS()
{
	InsertBinaryMdStatement(OP_MD_DIV_S);
}

void CJitter::MD_ExpandW()
{
	InsertUnaryMdStatement(OP_MD_EXPAND_W);
}

void CJitter::MD_AbsS()
{
	InsertUnaryMdStatement(OP_MD_ABS_S);
}

void CJitter::MD_NegS()
{
	InsertUnaryMdStatement(OP_MD_NEG_S);
}

void CJitter::MD_MinS()
{
	InsertBinaryMdStatement(OP_MD_MIN_S);
}

void CJitter::MD_MaxS()
{
	InsertBinaryMdStatement(OP_MD_MAX_S);
}

void CJitter::MD_MakeClip()
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_MAKECLIP;
	statement.src3 = MakeSymbolRef(m_shadow.Pull());
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_MakeSignZero()
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = OP_MD_MAKESZ;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_ToInt32TruncateS()
{
	InsertUnaryMdStatement(OP_MD_TOINT32_TRUNC_S);
}

void CJitter::MD_ToSingleI32()
{
	InsertUnaryMdStatement(OP_MD_TOSINGLE_I32);
}

//Generic Statement Inserters
//------------------------------------------------

void CJitter::InsertUnaryStatement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = operation;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertBinaryStatement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = operation;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertShiftCstStatement(Jitter::OPERATION operation, uint8 amount)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = operation;
	statement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertLoadFromRefIdxStatement(Jitter::OPERATION operation, size_t scale)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op = operation;
	statement.jmpCondition = static_cast<CONDITION>(scale);
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertStoreAtRefIdxStatement(Jitter::OPERATION operation, size_t scale)
{
	STATEMENT statement;
	statement.op = operation;
	statement.jmpCondition = static_cast<CONDITION>(scale);
	statement.src3 = MakeSymbolRef(m_shadow.Pull());
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	InsertStatement(statement);
}

void CJitter::InsertBinary64Statement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op = operation;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertUnaryFp32Statement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_FP_TEMPORARY32, m_nextTemporary++);

	STATEMENT statement;
	statement.op = operation;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertBinaryFp32Statement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_FP_TEMPORARY32, m_nextTemporary++);

	STATEMENT statement;
	statement.op = operation;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertUnaryMdStatement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = operation;
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertBinaryMdStatement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op = operation;
	statement.src2 = MakeSymbolRef(m_shadow.Pull());
	statement.src1 = MakeSymbolRef(m_shadow.Pull());
	statement.dst = MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}
