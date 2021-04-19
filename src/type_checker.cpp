#include "type_checker.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "log.h"
#include "token.h"

TypeChecker::TypeChecker() {}

// For 1-operand operations for convenience
bool TypeChecker::checkCompatible(std::shared_ptr<Token> tok,
		const TypeMark& op1) {
	return checkCompatible(tok, op1, op1);
}

bool TypeChecker::checkCompatible(std::shared_ptr<Token> tok, const TypeMark& op1,
		const TypeMark& op2) {
	LOG(DEBUG) << "Checking types for " << Token::getTypeMarkName(op1) << " "
			<< tok->getVal() << " " << Token::getTypeMarkName(op2);
	bool compatible = false;

	// Compatibility is dependent on the operator type
	// Treating `if' `for' and `return' as operators
	// Kind of hacky but eliminates edge cases
	switch (tok->getType()) {

		// `if' and `for' both require <expression> to resolve to `bool'
		// Assuming op1 is the return from the evaluated expression
		case TOK_RW_IF:
		case TOK_RW_FOR:
			compatible = checkCompatible(op1, TYPE_BOOL);
			break;

		// No type restriction on `return', just compatible with the declared type
		case TOK_RW_RET:
			compatible = checkCompatible(op1, op2);
			break;

		// `&' `|' and `not' must be either only `int' or only `bool'
		// TODO: `not' actually takes 1 operand but I'll just pass the same one
		// for both op1 and op2 as a hack for now.
		case TOK_OP_EXPR:
			compatible = (op1 == op2) && ((op1 == TYPE_INT) || (op1 == TYPE_BOOL));
			break;

		// `+' `-' `*' and `/' work for only `int' or `float'
		case TOK_OP_ARITH:
		case TOK_OP_TERM:
			//compatible = (op1 == op2) && ((op1 == TYPE_INT) || (op1 == TYPE_BOOL));
			compatible = checkCompatible(op1, op2);
			compatible &= (op1 != TYPE_BOOL) && (op2 != TYPE_BOOL);
			break;

		// The types just have to be compatible
		case TOK_OP_RELAT:
			compatible = checkCompatible(op1, op2);

			// Strings can just be `==' and `!='
			if ((op1 == TYPE_STR) || (op2 == TYPE_STR)) {
				compatible &= (tok->getVal() == "==") || (tok->getVal() == "!=");
			}
			break;

		// The types just have to be compatible
		case TOK_OP_ASS:
			compatible = checkCompatible(op1, op2);
			break;
		default:
			LOG(ERROR) << "Invalid operator received: " << tok->getVal();
			compatible = false;
			break;
	}
	if (!compatible) {
		LOG(ERROR) << "Type mismatch: " << Token::getTypeMarkName(op1) << " "
				<< tok->getVal() << " " << Token::getTypeMarkName(op2);
	}
	return compatible;
}

bool TypeChecker::checkCompatible(const TypeMark& op1, const TypeMark& op2) {
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
			compatible = false;
			break;
	}

	// Issue an error if types aren't compatible
	if (compatible) {
		LOG(DEBUG) << "Types are compatible";
	} else {
		LOG(ERROR) << "Incompatible types: " << Token::getTypeMarkName(op1)
				<< " and " << Token::getTypeMarkName(op2);
	}
	return compatible;
}

bool TypeChecker::checkArrayIndex(const TypeMark& op1) {
	LOG(DEBUG) << "Checking array index: " << Token::getTypeMarkName(op1);
	bool compatible = op1 == TYPE_INT;
	if (compatible) {
		LOG(DEBUG) << "Array index type correct";
	} else {
		LOG(ERROR) << "Array index type incorrect";
		LOG(ERROR) << "Expected type " << Token::getTypeMarkName(TYPE_INT)
				<< " but got " << Token::getTypeMarkName(op1);
	}
	return compatible;
}

////////////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////////////

// None yet
