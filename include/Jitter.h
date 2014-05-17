#ifndef _JITTER_H_
#define _JITTER_H_

#include <string>
#include <memory>
#include <list>
#include <map>
#include <unordered_map>
#include <deque>
#include <vector>
#include "ArrayStack.h"
#include "Stream.h"
#include "Jitter_SymbolTable.h"
#include "Jitter_CodeGen.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#include "Jitter_Statement.h"

namespace Jitter
{

	class CJitter
	{
	public:
		enum ROUNDMODE
		{
			ROUND_NEAREST = 0,
			ROUND_PLUSINFINITY = 1,
			ROUND_MINUSINFINITY = 2,
			ROUND_TRUNCATE = 3
		};

		enum RETURN_VALUE_TYPE
		{
			RETURN_VALUE_NONE,
			RETURN_VALUE_32,
			RETURN_VALUE_64,
			RETURN_VALUE_128,
		};

		typedef unsigned int LABEL;

										CJitter(CCodeGen*);
		virtual							~CJitter();

		virtual void					Begin();
		virtual void					End();
		
		bool							IsStackEmpty();

		void							BeginIf(CONDITION);
		void							Else();
		void							EndIf();
		
		LABEL							CreateLabel();
		void							MarkLabel(LABEL);
		void							Goto(LABEL);

		void							PushCtx();
		void							PushCst(uint32);
		virtual void					PushRel(size_t);
		void							PushTop();
		void							PushIdx(unsigned int);

		virtual void					PullRel(size_t);
		void							PullTop();
		void							Swap();

		void							Add();
		void							And();
		void							Call(void*, unsigned int, bool);
		void							Call(void*, unsigned int, RETURN_VALUE_TYPE);
		void							Cmp(CONDITION);
		void							Div();
		void							DivS();
		void							Lookup(uint32*);
		void							Lzc();
		void							Mult();
		void							MultS();
		void							MultSHL();
		void							MultSHH();
		void							Not();
		void							Or();
		void							SignExt();
		void							SignExt8();
		void							SignExt16();
		void							Shl();
		void							Shl(uint8);
		void							Sra();
		void							Sra(uint8);
		void							Srl();
		void							Srl(uint8);
		void							Sub();
		void							Xor();

		//Memory operations
		void							PushRelRef(size_t);
		void							PushRelAddrRef(size_t);

		void							AddRef();
		void							LoadFromRef();
		void							StoreAtRef();

		//64-bits
		virtual void					PushRel64(size_t);
		void							PushCst64(uint64);

		void							PullRel64(size_t);

		void							MergeTo64();
		void							ExtLow64();
		void							ExtHigh64();

		void							Add64();
		void							Sub64();
		void							And64();
		void							Cmp64(CONDITION);
		void							Srl64();
		void							Srl64(uint8);
		void							Sra64();
		void							Sra64(uint8);
		void							Shl64();
		void							Shl64(uint8);

		//FPU
		virtual void					FP_PushWord(size_t);
		virtual void					FP_PushSingle(size_t);
		virtual void					FP_PullWordTruncate(size_t);
		virtual void					FP_PullSingle(size_t);
		virtual void					FP_PushCst(float);

		void							FP_Add();
		void							FP_Abs();
		void							FP_Sub();
		void							FP_Max();
		void							FP_Min();
		void							FP_Mul();
		void							FP_Div();
		void							FP_Cmp(CONDITION);
		void							FP_Neg();
		void							FP_Rcpl();
		void							FP_Sqrt();
		void							FP_Rsqrt();

		//SIMD (128-bits only)
		virtual void					MD_PushRel(size_t);
		virtual void					MD_PushRelExpand(size_t);
		void							MD_PushCstExpand(uint32);
		void							MD_PushCstExpand(float);
		virtual void					MD_PullRel(size_t);
		virtual void					MD_PullRel(size_t, bool, bool, bool, bool);

		void							MD_AbsS();
		void							MD_AddB();
		void							MD_AddBUS();
		void							MD_AddH();
		void							MD_AddW();
		void							MD_AddWSS();
		void							MD_AddWUS();
		void							MD_AddS();
		void							MD_And();
		void							MD_CmpEqB();
		void							MD_CmpEqH();
		void							MD_CmpEqW();
		void							MD_CmpGtH();
		void							MD_DivS();
		void							MD_IsNegative();
		void							MD_IsZero();
		void							MD_MaxH();
		void							MD_MaxW();
		void							MD_MaxS();
		void							MD_MinH();
		void							MD_MinW();
		void							MD_MinS();
		void							MD_MulS();
		void							MD_Not();
		void							MD_Or();
		void							MD_PackHB();
		void							MD_PackWH();
		void							MD_SllH(uint8);
		void							MD_SllW(uint8);
		void							MD_SraH(uint8);
		void							MD_SraW(uint8);
		void							MD_SrlH(uint8);
		void							MD_SrlW(uint8);
		void							MD_Srl256();
		void							MD_SubB();
		void							MD_SubW();
		void							MD_SubS();
		void							MD_ToSingle();
		void							MD_ToWordTruncate();
		void							MD_UnpackLowerBH();
		void							MD_UnpackLowerHW();
		void							MD_UnpackLowerWD();
		void							MD_UnpackUpperBH();
		void							MD_UnpackUpperWD();
		void							MD_Xor();

		CCodeGen*						GetCodeGen();

		void							SetStream(Framework::CStream*);

	protected:
		CArrayStack<SymbolPtr>			m_Shadow;
		CArrayStack<uint32>				m_IfStack;

		void							PushTmp64(unsigned int);

	private:
		typedef size_t LABELREF;
		typedef std::map<LABEL, unsigned int> LabelMapType;

		enum MAX_STACK
		{
			MAX_STACK = 0x100,
		};

		enum IFBLOCKS
		{
			IFBLOCK,
			IFELSEBLOCK,
		};

		class CRelativeVersionManager
		{
		public:

			unsigned int				GetRelativeVersion(uint32);
			unsigned int				IncrementRelativeVersion(uint32);

		private:
			typedef std::unordered_map<uint32, unsigned int> RelativeVersionMap;

			RelativeVersionMap			m_relativeVersions;
		};

		struct BASIC_BLOCK
		{
			BASIC_BLOCK() : optimized(false), hasJumpRef(false) { }

			StatementList				statements;
			CSymbolTable				symbolTable;
			bool						optimized;
			bool						hasJumpRef;
		};
		typedef std::map<unsigned int, BASIC_BLOCK> BasicBlockList;

		struct VERSIONED_STATEMENT_LIST
		{
			StatementList				statements;
			CRelativeVersionManager		relativeVersions;
		};

		struct INSERT_COMMAND
		{
			StatementList::iterator insertionPoint;
			STATEMENT statement;
		};
		typedef std::vector<INSERT_COMMAND> InsertCommandList;

		typedef std::deque<unsigned int> AvailableRegsSet;
		typedef std::multimap<unsigned int, CSymbol*> ActiveSymbolList;
		typedef std::map<unsigned int, unsigned int, std::greater<unsigned int> > CallRangeMap;

		struct REGALLOC_STATE
		{
			InsertCommandList	insertCommands;
			ActiveSymbolList	activeSymbols;
			AvailableRegsSet	availableRegs;
			CallRangeMap		callRanges;
		};

		void							Compile();

		bool							ConstantFolding(StatementList&);
		bool							ConstantPropagation(StatementList&);
		bool							CopyPropagation(StatementList&);
		bool							DeadcodeElimination(VERSIONED_STATEMENT_LIST&);

		void							FixFlowControl(StatementList&);

		bool							FoldConstantOperation(STATEMENT&);
		bool							FoldConstant64Operation(STATEMENT&);
		bool							FoldConstant6432Operation(STATEMENT&);

		BASIC_BLOCK						ConcatBlocks(const BasicBlockList&);
		bool							MergeBlocks();
		bool							PruneBlocks();
		void							HarmonizeBlocks();
		void							MergeBasicBlocks(BASIC_BLOCK&, const BASIC_BLOCK&);

		uint32							CreateBlock();
		BASIC_BLOCK*					GetBlock(uint32);

		void							InsertStatement(const STATEMENT&);

		SymbolPtr						MakeSymbol(SYM_TYPE, uint32);
		SymbolPtr						MakeSymbol(BASIC_BLOCK*, SYM_TYPE, uint32, uint32);
		SymbolPtr						MakeConstant64(uint64);

		SymbolRefPtr					MakeSymbolRef(const SymbolPtr&);
		int								GetSymbolSize(const SymbolRefPtr&);

		static CONDITION				GetReverseCondition(CONDITION);

		void							DumpStatementList(const StatementList&);
		std::string						ConditionToString(CONDITION);
		VERSIONED_STATEMENT_LIST		GenerateVersionedStatementList(const StatementList&);
		StatementList					CollapseVersionedStatementList(const VERSIONED_STATEMENT_LIST&);
		void							CoalesceTemporaries(BASIC_BLOCK&);
		void							RemoveSelfAssignments(BASIC_BLOCK&);
		void							ComputeLivenessAndPruneSymbols(BASIC_BLOCK&);
		void							AllocateRegisters(BASIC_BLOCK&);
//		void							AllocateRegisters_ReplaceOperand(CSymbolTable&, SymbolRefPtr&, unsigned int);
//		void							AllocateRegisters_SpillSymbol(ActiveSymbolList::iterator&, unsigned int = -1);
//		void							AllocateRegisters_ComputeCallRanges(const BASIC_BLOCK&);
#ifdef _DEBUG
//		void							AllocateRegisters_VerifyProperCallSequence(const BASIC_BLOCK&);
#endif
		void							NormalizeStatements(BASIC_BLOCK&);
		unsigned int					AllocateStack(BASIC_BLOCK&);

		bool							m_nBlockStarted;

		unsigned int					m_nextTemporary;
		unsigned int					m_nextBlockId;

		BASIC_BLOCK*					m_currentBlock;
		BasicBlockList					m_basicBlocks;
		CCodeGen*						m_codeGen;

		unsigned int					m_nextLabelId;
		LabelMapType					m_labels;

		REGALLOC_STATE					m_regAllocState;
	};

}

#endif
