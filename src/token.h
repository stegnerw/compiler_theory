#ifndef TOKEN_H
#define TOKEN_H

////////////////////////////////////////////////////////////////////////////////
// Note: This file contains inlined class definitions and class templates.
// All of the classes are inlined where appropriate for consistency.
////////////////////////////////////////////////////////////////////////////////

#include <sstream>
#include <string>

#include "log.h"

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
	TOK_EOF, // End of file
	NUM_TOK_ENUMS, // Number of token enums (for array size)
};

enum TypeMark {
	TYPE_NONE = 0,
	TYPE_INT,
	TYPE_FLT,
	TYPE_STR,
	TYPE_BOOL,
	NUM_TYPE_ENUMS,
};

////////////////////////////////////////////////////////////////////////////////
// Base token class
// Reserve words, invalid, and punctuation
////////////////////////////////////////////////////////////////////////////////
class Token {
public:
	Token() : type(TOK_INVALID), type_mark(TYPE_NONE), val("") {}
	Token(const TokenType& t, const std::string& v) : type(t), val(v) {}
	virtual ~Token() {}
	virtual std::string const getStr() {
		std::stringstream ss;
		ss << "{ " << tok_names[type] << ", " << val << " }";
		return ss.str();
	}
	TokenType const getType() { return type; }
	void setType(const TokenType& t) { type = t; }
	TypeMark getTypeMark() { return type_mark; }
	void setTypeMark(const TypeMark& tm) { type_mark = tm; }
	std::string const getVal() { return val; }
	static std::string getTokenName(const TokenType& t) {
		if (t < NUM_TOK_ENUMS) {
			return tok_names[t];
		} else {
			return "NUM_TOK_ENUMS";
		}
	}
	static std::string getTypeMarkName(const TypeMark& tm) {
		if (tm < NUM_TYPE_ENUMS) {
			return type_mark_names[tm];
		} else {
			return "NUM_TYPE_ENUMS";
		}
	}
	bool isValid() { return type != TOK_INVALID; }

protected:
	static const std::string tok_names[NUM_TOK_ENUMS];
	static const std::string type_mark_names[NUM_TYPE_ENUMS];
	TokenType type;
	TypeMark type_mark;
	std::string val;
};

////////////////////////////////////////////////////////////////////////////////
// Identifier token class
// Variables and procedures
////////////////////////////////////////////////////////////////////////////////
class IdToken : public Token {
public:
	IdToken(const TokenType& t, const std::string& v) :
			num_elements(0),
			procedure(false) {
		type = t;
		type_mark = TYPE_NONE;
		val = v;
	}
	std::string const getStr() {
		std::stringstream ss;
		ss << "{ " << tok_names[type] << ", " << val << ", "
			<< type_mark_names[type_mark] /*<< ", " << num_elements << " }"*/ ;
		if (num_elements > 0) {
			ss << ", " << num_elements << " elems";
		}
		if (procedure) {
			ss << ", PROCEDURE";
		}
		ss << " }";
		return ss.str();
	}
	bool setNumElements(const int& n) {
		if (n >= 1) {
			num_elements = n;
			return true;
		} else {
			LOG(ERROR) << "Could not set number of elements to: " << n;
			return false;
		}
	}
	int getNumElements() { return num_elements; }
	void setProcedure(bool b) { procedure = b; }
	bool getProcedure() { return procedure; }

private:
	int num_elements;
	bool procedure;
};

////////////////////////////////////////////////////////////////////////////////
// Literal token classes
// Int, float, string, and bool
////////////////////////////////////////////////////////////////////////////////
template <class T>
class LiteralToken : public Token {
public:
	LiteralToken<T>(const TokenType& t, const T& v, const TypeMark& tm) :
		val(v) { type = t; type_mark = tm; }
	T const getVal() { return val; }
	std::string const getStr() {
		std::stringstream ss;
		ss << "{ " << tok_names[type] << ", " << val << ", "
			<< type_mark_names[type_mark] << " }";
		return ss.str();
	}

private:
	T val;
};

#endif // TOKEN_H
