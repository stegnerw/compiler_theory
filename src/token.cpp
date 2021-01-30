#include "token.h"
#include <sstream>

/*
 * Token base class
 */

Token::Token() : type(TOK_INVALID) {}

Token::Token(const token_type &t) : type(t) {}

token_type const Token::getType() {
	return type;
}

std::string const Token::getStr() {
	std::stringstream ss;
	ss << "< " << type_names[type] << " >";
	return ss.str();
}

/*
 * Token string class
 */

StrToken::StrToken(const token_type &t, const std::string &v) : val(v) {
	type = t;
}

std::string const StrToken::getVal() {
	return val;
}

std::string const StrToken::getStr() {
	std::stringstream ss;
	ss << "< " << type_names[type] << ",\t" << val << " >";
	return ss.str();
}

/*
 * Token int class
 */

IntToken::IntToken(const token_type &t, const std::string &v) {
	type = t;
	val = std::stoi(v);
}

int const IntToken::getVal() {
	return val;
}

std::string const IntToken::getStr() {
	std::stringstream ss;
	ss << "< " << type_names[type] << ",\t" << val << " >";
	return ss.str();
}

/*
 * Token float class
 */

FltToken::FltToken(const token_type &t, const std::string &v) {
	type = t;
	val = std::stod(v);
}

double const FltToken::getVal() {
	return val;
}

std::string const FltToken::getStr() {
	std::stringstream ss;
	ss << "< " << type_names[type] << ",\t" << val << " >";
	return ss.str();
}

/*
 * Type names list
 */

const std::string Token::type_names[41] = {
	"INVALID",
	"RW_PROG",
	"RW_IS",
	"RW_BEG",
	"RW_END",
	"RW_GLOB",
	"RW_PROC",
	"RW_VAR",
	"RW_TYPE",
	"RW_INT",
	"RW_FLT",
	"RW_STR",
	"RW_BOOL",
	"RW_ENUM",
	"RW_IF",
	"RW_THEN",
	"RW_ELSE",
	"RW_FOR",
	"RW_RET",
	"RW_NOT",
	"RW_TRUE",
	"RW_FALSE",
	"OP_EXPR",
	"OP_ARITH",
	"OP_RELAT",
	"OP_ASS",
	"OP_TERM",
	"IDENT",
	"NUM",
	"STR",
	"PERIOD",
	"COMMA",
	"SEMICOL",
	"COLON",
	"LPAREN",
	"RPAREN",
	"LBRACK",
	"RBRACK",
	"LBRACE",
	"RBRACE",
	"EOF"
};

