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
	"PROGRAM",
	"IS",
	"BEGIN",
	"END",
	"GLOBAL",
	"PROCEDURE",
	"VARIABLE",
	"TYPE",
	"INTEGER",
	"FLOAT",
	"STRING",
	"BOOL",
	"ENUM",
	"IF",
	"THEN",
	"ELSE",
	"FOR",
	"RETURN",
	"NOT",
	"TRUE",
	"FALSE",
	"EXPRESSION",
	"ARITHMETIC",
	"RELATION",
	"ASSIGNMENT",
	"TERM",
	"IDENTIFIER",
	"NUMBER",
	"LITERAL_STRING",
	"PERIOD",
	"COMMA",
	"SEMICOLON",
	"COLON",
	"L_PARENTHESIS",
	"R_PARENTHESIS",
	"L_BRACKET",
	"R_BRACKET",
	"L_BRACE",
	"R_BRACE",
	"END_OF_FILE"
};

