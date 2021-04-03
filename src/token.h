#ifndef TOKEN_H
#define TOKEN_H

////////////////////////////////////////////////////////////////////////////////
// Note: This file contains inlined class definitions and class templates.
// All of the classes are inlined where appropriate for consistency.
////////////////////////////////////////////////////////////////////////////////

#include <sstream>
#include <string>

enum TokenType {
	TOK_INVALID = 0, // Invalid token - the default
	TOK_RW_PROG, // program
	TOK_RW_IS, // is
	TOK_RW_BEG, // begin
	TOK_RW_END, // end
	TOK_RW_GLOB, // global
	TOK_RW_PROC, // procedure
	TOK_RW_VAR, // variable
	TOK_RW_INT, // integer
	TOK_RW_FLT, // float
	TOK_RW_STR, // string
	TOK_RW_BOOL, // bool
	TOK_RW_IF, // if
	TOK_RW_THEN, // then
	TOK_RW_ELSE, // else
	TOK_RW_FOR, // for
	TOK_RW_RET, // return
	TOK_RW_NOT, // not
	TOK_RW_TRUE, // true
	TOK_RW_FALSE, // false
	TOK_OP_EXPR, // & |
	TOK_OP_ARITH, // + -
	TOK_OP_RELAT, // < <= > >= == !=
	TOK_OP_ASS, // :=
	TOK_OP_TERM, // * /
	TOK_IDENT, // Identifiers
	TOK_NUM, // Numbers (float and int)
	TOK_STR, // String literals: "[^"]"
	TOK_PERIOD, // .
	TOK_COMMA, // ,
	TOK_SEMICOL, // ;
	TOK_COLON, // :
	TOK_LPAREN, // (
	TOK_RPAREN, // )
	TOK_LBRACK, // [
	TOK_RBRACK, // ]
	TOK_LBRACE, // {
	TOK_RBRACE, // }
	TOK_EOF, // End of file
	NUM_TOK_ENUMS, // Number of token enums (for array size)
};

enum TypeMark {
	TYPE_NONE = 0,
	TYPE_INT,
	TYPE_FLT,
	TYPE_STR,
	TYPE_BOOL,
	TYPE_PROC,
	NUM_TYPE_ENUMS,
};

////////////////////////////////////////////////////////////////////////////////
// Base token class
// Reserve words, invalid, and punctuation
////////////////////////////////////////////////////////////////////////////////
class Token {
public:
	Token() : type(TOK_INVALID) {}
	Token(const TokenType& t) : type(t) {}
	virtual ~Token() {}
	virtual std::string const getStr() {
		std::stringstream ss;
		ss << "{ " << token_names[type] << " }";
		return ss.str();
	}
	TokenType const getType() { return type; }
	static std::string getTokenName(const TokenType& t) {
		return token_names[t];
	}

protected:
	static const std::string token_names[NUM_TOK_ENUMS];
	TokenType type;
};

////////////////////////////////////////////////////////////////////////////////
// Operator token class
////////////////////////////////////////////////////////////////////////////////
class OpToken : public Token {
public:
	OpToken(const TokenType& t, const std::string& v) : val(v) { type = t; }
	std::string getVal() { return val; }
	std::string const getStr() {
		std::stringstream ss;
		ss << "{ " << token_names[type] << ", " << val << " }";
		return ss.str();
	}

private:
	std::string val;
};

////////////////////////////////////////////////////////////////////////////////
// Identifier token class
// Variables and procedures
////////////////////////////////////////////////////////////////////////////////
class IdToken : public Token {
public:
	IdToken(const TokenType& t, const std::string& lex) :
			lexeme(lex),
			type_mark(TYPE_NONE) {
		type = t;
	}
	std::string const getStr() {
		std::stringstream ss;
		ss << "{ " << token_names[type] << ", " << lexeme << ", "
			<< type_mark_names[type_mark] << " }";
		return ss.str();
	}
	TypeMark getTypeMark() { return type_mark; }
	void setTypeMark(const TypeMark& tm) { type_mark = tm; }
	std::string getLexeme() { return lexeme; }

private:
	static const std::string type_mark_names[NUM_TYPE_ENUMS];
	std::string lexeme;
	TypeMark type_mark;
};

////////////////////////////////////////////////////////////////////////////////
// Literal token classes
// Int, float, string, and bool
////////////////////////////////////////////////////////////////////////////////
template <class T>
class LiteralToken : public Token {
public:
	LiteralToken<T>(const TokenType& t, const T& v) : val(v) { type = t; }
	T const getVal() { return val; }
	std::string const getStr() {
		std::stringstream ss;
		ss << "{ " << token_names[type] << ", " << val << " }";
		return ss.str();
	}

private:
	T val;
};

#endif // TOKEN_H
