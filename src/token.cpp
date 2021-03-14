#include "token.h"
#include <sstream>

////////////////////////////////////////////////////////////////////////////////
// Token
////////////////////////////////////////////////////////////////////////////////

Token::Token() : type(TOK_INVALID) {}

Token::Token(const TokenType &t) : type(t) {}

TokenType const Token::getType() {
	return type;
}

std::string const Token::getStr() {
	std::stringstream ss;
	ss << "< " << type_names[type] << " >";
	return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
// StrToken
////////////////////////////////////////////////////////////////////////////////

StrToken::StrToken(const TokenType &t, const std::string &v) : val(v) {
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

////////////////////////////////////////////////////////////////////////////////
// IntToken
////////////////////////////////////////////////////////////////////////////////

IntToken::IntToken(const TokenType &t, const std::string &v) {
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

////////////////////////////////////////////////////////////////////////////////
// FltToken
////////////////////////////////////////////////////////////////////////////////

FltToken::FltToken(const TokenType &t, const std::string &v) {
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

////////////////////////////////////////////////////////////////////////////////
// type_names
////////////////////////////////////////////////////////////////////////////////

const std::string Token::type_names[NUM_TOK_ENUMS] = {
	"INVALID",
	"PROGRAM",
	"IS",
	"BEGIN",
	"END",
	"GLOBAL",
	"PROCEDURE",
	"VARIABLE",
	"INTEGER",
	"FLOAT",
	"STRING",
	"BOOL",
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

