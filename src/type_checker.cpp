#include "type_checker.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "log.h"
#include "token.h"

TypeChecker::TypeChecker() {}

// For 1-operand operations for convenience
bool TypeChecker::areCompatible(std::shared_ptr<Token> tok,
		const TypeMark& op1) {
	return areCompatible(tok, op1, TYPE_NONE);
}

bool TypeChecker::areCompatible(std::shared_ptr<Token> tok, const TypeMark& op1,
		const TypeMark& op2) {
	bool compatible = false;

	// Compatibility is dependent on the operator type
	// Treating `if' `for' and `return' as operators
	// Kind of hacky but eliminates edge cases
	switch (tok->getType()) {

		// `if' and `for' both require <expression> to resolve to `bool'
		case TOK_RW_IF:
		case TOK_RW_FOR:
			compatible = areCompatible(op1, TYPE_BOOL);
			break;

		// No type restriction on `return', just that it matches the declared type
		case TOK_RW_RET:
			compatible = areCompatible(op1, op2);
			break;

		// `&' `|' and `not' must be either only `int' or only `bool'
		case TOK_OP_EXPR:
			break;

		// `+' `-' `*' and `/' work for only `int' or only `bool'
		case TOK_OP_ARITH:
		case TOK_OP_TERM:
			break;

		// 
		case TOK_OP_RELAT: // String special case `==' and `!=' only
			break;
		case TOK_OP_ASS:
			break;
		default:
			compatible = false;
			break;
	}
	return compatible;
}

bool TypeChecker::areCompatible(const TypeMark& op1, const TypeMark& op2) {
	bool compatible = false;
	LOG(DEBUG) << "Comparing types " << Token::getTypeMarkName(op1) << " and "
			<< Token::getTypeMarkName(op2);
	switch (op1) {

		// Int is compatible with int, float, and bool
		case TYPE_INT:
			compatible = (op2 == TYPE_INT) || (op2 == TYPE_FLT) || (op2 == TYPE_BOOL);
			break;

		// Float is compatible with float and int
		case TYPE_FLT:
			compatible = (op2 == TYPE_FLT) || (op2 == TYPE_INT);
			break;

		// Bool is compatible with bool and int
		case TYPE_BOOL:
			compatible = (op2 == TYPE_BOOL) || (op2 == TYPE_INT);
			break;

		// String is compatible with string only
		case TYPE_STR:
			compatible = op2 == TYPE_STR;
			break;

		// This should never happen
		case TYPE_NONE:
		case NUM_TYPE_ENUMS:  // Compiler warns if this isn't there
			LOG(WARN) << "Encontered invalid type mark";
			compatible = false;
			break;
	}

	// Only warn if types are incompatible
	// Let the parser decide if it's an error (it probably is)
	if (compatible) {
		LOG(DEBUG) << "Types are compatible";
	} else {
		LOG(WARN) << "Incompatible types: " << Token::getTypeMarkName(op1)
				<< " and " << Token::getTypeMarkName(op2);
	}
	return compatible;
}

////////////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////////////

// None yet
