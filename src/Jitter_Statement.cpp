#include <iostream>
#include "Jitter_Statement.h"

using namespace Jitter;

std::string Jitter::ConditionToString(CONDITION condition)
{
	switch(condition)
	{
	case CONDITION_LT:
		return "LT";
		break;
	case CONDITION_LE:
		return "LE";
		break;
	case CONDITION_GT:
		return "GT";
		break;
	case CONDITION_EQ:
		return "EQ";
		break;
	case CONDITION_NE:
		return "NE";
		break;
	case CONDITION_BL:
		return "BL";
		break;
	case CONDITION_AB:
		return "AB";
		break;
	default:
		return "??";
		break;
	}
}

void Jitter::DumpStatementList(const StatementList& statements)
{
	for(const auto& statement : statements)
	{
		if(statement.dst)
		{
			std::cout << statement.dst->ToString();
			std::cout << " := ";
		}

		if(statement.src1)
		{
			std::cout << statement.src1->ToString();
		}

		switch(statement.op)
		{
		case OP_ADD:
		case OP_ADD64:
		case OP_ADDREF:
		case OP_FP_ADD:
			std::cout << " + ";
			break;
		case OP_SUB:
		case OP_SUB64:
		case OP_FP_SUB:
			std::cout << " - ";
			break;
		case OP_CMP:
		case OP_CMP64:
		case OP_FP_CMP:
			std::cout << " CMP(" << ConditionToString(statement.jmpCondition) << ") ";
			break;
		case OP_MUL:
		case OP_MULS:
		case OP_FP_MUL:
			std::cout << " * ";
			break;
		case OP_MULSHL:
			std::cout << " *(HL) ";
			break;
		case OP_MULSHH:
			std::cout << " *(HH) ";
			break;
		case OP_DIV:
		case OP_DIVS:
		case OP_FP_DIV:
			std::cout << " / ";
			break;
		case OP_AND:
		case OP_AND64:
		case OP_MD_AND:
			std::cout << " & ";
			break;
		case OP_LZC:
			std::cout << " LZC";
			break;
		case OP_OR:
		case OP_MD_OR:
			std::cout << " | ";
			break;
		case OP_XOR:
		case OP_MD_XOR:
			std::cout << " ^ ";
			break;
		case OP_NOT:
		case OP_MD_NOT:
			std::cout << " ! ";
			break;
		case OP_SRL:
		case OP_SRL64:
			std::cout << " >> ";
			break;
		case OP_SRA:
		case OP_SRA64:
			std::cout << " >>A ";
			break;
		case OP_SLL:
		case OP_SLL64:
			std::cout << " << ";
			break;
		case OP_NOP:
			std::cout << " NOP ";
			break;
		case OP_MOV:
			break;
		case OP_STOREATREF:
			std::cout << " <- ";
			break;
		case OP_LOADFROMREF:
			std::cout << " LOADFROM ";
			break;
		case OP_RELTOREF:
			std::cout << " TOREF ";
			break;
		case OP_PARAM:
		case OP_PARAM_RET:
			std::cout << " PARAM ";
			break;
		case OP_CALL:
			std::cout << " CALL ";
			break;
		case OP_RETVAL:
			std::cout << " RETURNVALUE ";
			break;
		case OP_JMP:
			std::cout << " JMP{" << statement.jmpBlock << "} ";
			break;
		case OP_CONDJMP:
			std::cout << " JMP{" << statement.jmpBlock << "}(" << ConditionToString(statement.jmpCondition) << ") ";
			break;
		case OP_LABEL:
			std::cout << "LABEL_" << statement.jmpBlock << ":";
			break;
		case OP_EXTLOW64:
			std::cout << " EXTLOW64";
			break;
		case OP_EXTHIGH64:
			std::cout << " EXTHIGH64";
			break;
		case OP_MERGETO64:
			std::cout << " MERGETO64 ";
			break;
		case OP_MERGETO256:
			std::cout << " MERGETO256 ";
			break;
		case OP_FP_ABS:
			std::cout << " ABS";
			break;
		case OP_FP_NEG:
			std::cout << " NEG";
			break;
		case OP_FP_MIN:
			std::cout << " MIN ";
			break;
		case OP_FP_MAX:
			std::cout << " MAX ";
			break;
		case OP_FP_SQRT:
			std::cout << " SQRT";
			break;
		case OP_FP_RSQRT:
			std::cout << " RSQRT";
			break;
		case OP_FP_RCPL:
			std::cout << " RCPL";
			break;
		case OP_FP_TOINT_TRUNC:
			std::cout << " INT(TRUNC)";
			break;
		case OP_FP_LDCST:
			std::cout << " LOAD ";
			break;
		case OP_MD_MOV_MASKED:
			std::cout << " MOVMSK ";
			break;
		case OP_MD_PACK_HB:
			std::cout << " PACK_HB ";
			break;
		case OP_MD_PACK_WH:
			std::cout << " PACK_WH ";
			break;
		case OP_MD_UNPACK_LOWER_BH:
			std::cout << " UNPACK_LOWER_BH ";
			break;
		case OP_MD_UNPACK_LOWER_HW:
			std::cout << " UNPACK_LOWER_HW ";
			break;
		case OP_MD_UNPACK_LOWER_WD:
			std::cout << " UNPACK_LOWER_WD ";
			break;
		case OP_MD_UNPACK_UPPER_BH:
			std::cout << " UNPACK_UPPER_BH ";
			break;
		case OP_MD_UNPACK_UPPER_WD:
			std::cout << " UNPACK_UPPER_WD ";
			break;
		case OP_MD_ADD_B:
			std::cout << " +(B) ";
			break;
		case OP_MD_ADD_H:
			std::cout << " +(H) ";
			break;
		case OP_MD_ADD_W:
			std::cout << " +(W) ";
			break;
		case OP_MD_ADDUS_B:
			std::cout << " +(USB) ";
			break;
		case OP_MD_ADDUS_W:
			std::cout << " +(USW) ";
			break;
		case OP_MD_ADDSS_H:
			std::cout << " +(SSH) ";
			break;
		case OP_MD_ADDSS_W:
			std::cout << " +(SSW) ";
			break;
		case OP_MD_SUB_B:
			std::cout << " -(B) ";
			break;
		case OP_MD_SUB_W:
			std::cout << " -(W) ";
			break;
		case OP_MD_SUBSS_H:
			std::cout << " -(SSH) ";
			break;
		case OP_MD_SLLW:
			std::cout << " <<(W) ";
			break;
		case OP_MD_SLLH:
			std::cout << " <<(H) ";
			break;
		case OP_MD_SRLH:
			std::cout << " >>(H) ";
			break;
		case OP_MD_SRLW:
			std::cout << " >>(W) ";
			break;
		case OP_MD_SRAH:
			std::cout << " >>A(H) ";
			break;
		case OP_MD_SRAW:
			std::cout << " >>A(W) ";
			break;
		case OP_MD_SRL256:
			std::cout << " >>(256) ";
			break;
		case OP_MD_CMPEQ_W:
			std::cout << " CMP(EQ,W) ";
			break;
		case OP_MD_CMPGT_H:
			std::cout << " CMP(GT,H) ";
			break;
		case OP_MD_ADD_S:
			std::cout << " +(S) ";
			break;
		case OP_MD_SUB_S:
			std::cout << " -(S) ";
			break;
		case OP_MD_MUL_S:
			std::cout << " *(S) ";
			break;
		case OP_MD_DIV_S:
			std::cout << " /(S) ";
			break;
		case OP_MD_MIN_H:
			std::cout << " MIN(H) ";
			break;
		case OP_MD_MIN_W:
			std::cout << " MIN(W) ";
			break;
		case OP_MD_MIN_S:
			std::cout << " MIN(S) ";
			break;
		case OP_MD_MAX_H:
			std::cout << " MAX(H) ";
			break;
		case OP_MD_MAX_W:
			std::cout << " MAX(W) ";
			break;
		case OP_MD_MAX_S:
			std::cout << " MAX(S) ";
			break;
		case OP_MD_ISNEGATIVE:
			std::cout << " ISNEGATIVE";
			break;
		case OP_MD_ISZERO:
			std::cout << " ISZERO";
			break;
		case OP_MD_EXPAND:
			std::cout << " EXPAND";
			break;
		case OP_MD_TOWORD_TRUNCATE:
			std::cout << " TOWORD_TRUNCATE";
			break;
		default:
			std::cout << " ?? ";
			break;
		}

		if(statement.src2)
		{
			std::cout << statement.src2->ToString();
		}

		std::cout << std::endl;
	}
}
